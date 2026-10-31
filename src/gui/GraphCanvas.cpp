#include "GraphCanvas.hpp"
#include "CurveSampler.hpp"
#include "GraphGridLayout.hpp"
#include "GraphVariableDependencies.hpp"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QFont>
#include <QFontMetrics>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QTimer>
#include <QTransform>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace rationalstudio {
namespace {
QPointF mousePoint(QMouseEvent* e) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return e->position();
#else
    return e->localPos();
#endif
}
QPointF wheelPoint(QWheelEvent* e) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return e->position();
#else
    return e->posF();
#endif
}
void drawCurvePath(QPainter& painter, const QPainterPath& path) {
    // Stroke each disconnected branch separately. Sending thousands of
    // overlapping asymptote branches through one Qt path stroker builds a
    // large overlapping outline and makes dense zoomed-out views expensive.
    for (const auto& branch : path.toSubpathPolygons()) {
        painter.drawPolyline(branch);
    }
}
bool rasterMatches(const QImage& image, const QSize& size, double deviceRatio) {
    return !image.isNull() && image.devicePixelRatioF() == deviceRatio &&
        image.width() == qCeil(size.width() * deviceRatio) &&
        image.height() == qCeil(size.height() * deviceRatio);
}
QImage rasterizeCurve(const QPainterPath& path, const QColor& color,
                      const QSize& size, double deviceRatio,
                      const QTransform& transform = {}, bool dashed = false) {
    QImage image(QSize(qCeil(size.width() * deviceRatio), qCeil(size.height() * deviceRatio)),
                 QImage::Format_ARGB32_Premultiplied);
    image.setDevicePixelRatio(deviceRatio);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setTransform(transform);
    QPen pen(color, 2.5, dashed ? Qt::DashLine : Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    pen.setCosmetic(true);
    painter.setPen(pen);
    drawCurvePath(painter, path);
    return image;
}
QString numberLabel(double value) {
    if (std::abs(value) < 1e-12) value = 0;
    return QString::number(value, 'g', 6);
}
} // namespace

GraphCanvas::GraphCanvas(QWidget* parent) : QWidget(parent) {
    setMouseTracking(true);
    setAccessibleName("Interactive graph plot");
    setAccessibleDescription("Graph curves are visual. Use keyboard expression editing and the "
                             "Intersections and Features commands for textual analysis; "
                             "toolbar Zoom and Home buttons control the viewport.");
    setMinimumSize(350, 300);
    setCursor(Qt::CrossCursor);
    setAttribute(Qt::WA_OpaquePaintEvent);
    (void)viewport_.setDimensions(std::max(1, width()), std::max(1, height()));
    geometryViewport_ = viewport_;

    navigationTimer_.setSingleShot(true);
    navigationTimer_.setInterval(GraphRepaintScheduler::kNavigationIntervalMs);
    connect(&navigationTimer_, &QTimer::timeout, this, [this] {
        if (!repaintScheduler_.takeNavigationRefresh()) return;
        // Mark refresh due, but keep the previous ready paths available.
        // A newer navigation event may arrive before Qt delivers this paint.
        // Invalidating here would leave that event's preview blank.
        refreshQueued_ = true;
        update();
    });

    deferredTimer_.setSingleShot(true);
    deferredTimer_.setInterval(GraphRepaintScheduler::kDeferredIntervalMs);
    connect(&deferredTimer_, &QTimer::timeout, this, [this] {
        if (repaintScheduler_.takeDeferredRefresh()) update();
    });
}

void GraphCanvas::invalidateGeometry(bool retainPreview) {
    pendingGeometry_.resize(curves_.size());
    for (std::size_t i = 0; i < curves_.size(); ++i) {
        auto& pending = pendingGeometry_[i];
        pending.job.reset();
        pending.parametricJob.reset();
        pending.implicitJob.reset();
        pending.raster = {};
        if (retainPreview && geometryCache_.isReady(i)) {
            pending.preview = geometryCache_.readyGeometry(i);
            pending.previewViewport = geometryViewport_;
            pending.hasPreview = true;
        } else if (!retainPreview) {
            pending.preview = {};
            pending.hasPreview = false;
        }
    }
    geometryCache_.invalidateAll();
    for (auto& vertices : traceVertices_) vertices.clear();
    for (auto& shade : inequalityRasters_) shade = {};
}

void GraphCanvas::cancelScheduledWork() {
    navigationTimer_.stop();
    deferredTimer_.stop();
    repaintScheduler_.invalidate();
    refreshQueued_ = false;
}

void GraphCanvas::requestNavigationRefresh() {
    deferredTimer_.stop();
    // If a timer fired but its paint is still queued, the new input makes
    // that pending rebuild obsolete. Preserve ready paths for its preview.
    refreshQueued_ = false;
    for (auto& pending : pendingGeometry_) pending.raster = {};
    if (repaintScheduler_.navigationChanged()) navigationTimer_.start();
    // Preserve old geometry while the input interval is coalescing. It is
    // transformed to match the current viewport during preview paints.
    update();
}

bool GraphCanvas::visibleGeometryReady() const noexcept {
    if (refreshQueued_ || repaintScheduler_.navigationPending() ||
        geometryCache_.size() != curves_.size()) return false;
    for (std::size_t index = 0; index < curves_.size(); ++index) {
        const auto& curve = curves_[index];
        if (!curve.enabled || !curve.function || !curve.function->valid() ||
            (curve.parametric() && !curve.yCoordinate->valid())) continue;
        if (!geometryCache_.isReady(index)) return false;
    }
    return true;
}

bool GraphCanvas::visibleInequalityReady() const noexcept {
    if (refreshQueued_ || repaintScheduler_.navigationPending() ||
        inequalityRasters_.size() != curves_.size()) return false;
    for (std::size_t i = 0; i < curves_.size(); ++i) {
        const auto& curve = curves_[i];
        if (curve.inequalityCurve && curve.enabled && curve.function &&
            curve.function->valid() && inequalityRasters_[i].finished.isNull()) return false;
    }
    return true;
}

void GraphCanvas::setCurves(std::vector<GraphCurve> curves) {
    // The mathematical identity, not the curve index or display label,
    // determines whether the existing geometry remains valid.
    std::vector<CurveGeometryCache<QPainterPath>::Key> keys;
    keys.reserve(curves.size());
    for (auto& curve : curves) {
        if (curve.parametric()) {
            // An XY curve is identified by BOTH compiled coordinates and
            // its parameter interval. A new Y expression or interval must
            // invalidate its path even when X's AST pointer is unchanged.
            for (const auto& previous : curves_) {
                if (!previous.parametric() || previous.polar() != curve.polar()) continue;
                if (curve.function == previous.function &&
                    curve.yCoordinate == previous.yCoordinate &&
                    curve.tMin == previous.tMin && curve.tMax == previous.tMax) {
                    curve.cacheToken = previous.cacheToken;
                    break;
                }
            }
            if (!curve.cacheToken)
                curve.cacheToken = std::make_shared<Expression>("0");
            keys.push_back(curve.cacheToken);
        } else if (curve.implicit()) {
            // Even if the compiled Expression pointer is shared, an implicit
            // zero contour must not reuse Cartesian y=f(x) path geometry.
            for (const auto& previous : curves_) {
                if (previous.implicit() && previous.function == curve.function) {
                    curve.cacheToken = previous.cacheToken;
                    break;
                }
            }
            if (!curve.cacheToken)
                curve.cacheToken = std::make_shared<Expression>("0");
            keys.push_back(curve.cacheToken);
        } else {
            keys.push_back(curve.function);
        }
    }
    deferredTimer_.stop();
    repaintScheduler_.cancelDeferred();
    geometryCache_.synchronize(keys);
    // Preserve samples across presentation-only edits and reorderings. Match
    // expression identity and parameter bounds, never just row index.
    auto previousTraces = std::move(traceVertices_);
    std::vector<std::vector<CurveVertex>> nextTraces(curves.size());
    std::vector<bool> traceClaimed(curves_.size(), false);
    for (std::size_t i = 0; i < curves.size(); ++i) {
        const auto& next = curves[i];
        for (std::size_t j = 0; j < curves_.size(); ++j) {
            const auto& old = curves_[j];
            if (traceClaimed[j] || old.function != next.function ||
                old.yCoordinate != next.yCoordinate ||
                old.implicitCurve != next.implicitCurve ||
                old.polarCurve != next.polarCurve ||
                old.inequalityCurve != next.inequalityCurve ||
                old.tMin != next.tMin || old.tMax != next.tMax) continue;
            if (j < previousTraces.size())
                nextTraces[i] = std::move(previousTraces[j]);
            traceClaimed[j] = true;
            break;
        }
    }
    traceVertices_ = std::move(nextTraces);
    // Retain completed shade images only for identical mathematical inputs,
    // styling and row visibility. Reordering rows is safe.
    auto previousRasters = std::move(inequalityRasters_);
    std::vector<RegionRaster> newRasters(curves.size());
    std::vector<bool> claimed(curves_.size(), false);
    for (std::size_t i = 0; i < curves.size(); ++i) {
        const auto& next = curves[i];
        if (!next.inequalityCurve) continue;
        for (std::size_t j = 0; j < curves_.size(); ++j) {
            const auto& old = curves_[j];
            if (!claimed[j] && old.inequalityCurve &&
                old.function == next.function && old.color == next.color &&
                old.enabled == next.enabled && old.inequalityAbove == next.inequalityAbove &&
                old.inequalityInclusive == next.inequalityInclusive) {
                if (j < previousRasters.size()) newRasters[i] = std::move(previousRasters[j]);
                claimed[j] = true;
                break;
            }
        }
    }
    inequalityRasters_ = std::move(newRasters);
    curves_ = std::move(curves);
    pendingGeometry_.clear();
    pendingGeometry_.resize(curves_.size());
    update();
}
void GraphCanvas::setVariableBindings(VariableBindings bindings,
                                      const std::vector<std::string>& changed) {
    // A completed CurveSamplingJob captured an immutable bindings snapshot.
    // Discard its work only when one of its dependencies changed.
    bindings_ = std::move(bindings);
    if (changed.empty()) return;
    bool any = false;
    for (std::size_t i = 0; i < curves_.size(); ++i) {
        const auto& expression = curves_[i].function;
        if (!expression ||
            (!graphCurveDependsOnAny(*expression, changed) &&
             (!curves_[i].yCoordinate ||
              !graphCurveDependsOnAny(*curves_[i].yCoordinate, changed)))) continue;
        geometryCache_.invalidate(i);
        if (i < traceVertices_.size()) traceVertices_[i].clear();
        if (i < inequalityRasters_.size()) inequalityRasters_[i] = {};
        auto& pending = pendingGeometry_[i];
        pending.job.reset();
        pending.parametricJob.reset();
        pending.implicitJob.reset();
        pending.preview = {}; // old mathematical values must never be previewed
        pending.raster = {};
        pending.hasPreview = false;
        any = true;
    }
    if (any) {
        // Retain unrelated ready paths/jobs. Existing deferred-paint pacing
        // and navigation coalescing remain authoritative.
        update();
    }
}

void GraphCanvas::setAngleMode(AngleMode mode) {
    if (angleMode_ == mode) return;
    angleMode_ = mode;
    cancelScheduledWork();
    invalidateGeometry();
    update();
}
void GraphCanvas::setLastAnswer(double answer) {
    if (lastAnswer_ == answer || (std::isnan(lastAnswer_) && std::isnan(answer))) return;
    lastAnswer_ = answer;
    cancelScheduledWork();
    invalidateGeometry();
    update();
}
bool GraphCanvas::setSamplingLimits(GraphSamplingLimits limits) {
    // Four evaluations is the smallest budget accepted by CurveSampler.
    constexpr std::size_t kMaxConfiguredEvaluations = 10000000;
    if (limits.maxEvaluationsPerCurve < 4 ||
        limits.maxEvaluationsPerPaint < 4 ||
        limits.maxEvaluationsPerCurve > kMaxConfiguredEvaluations ||
        limits.maxEvaluationsPerPaint > kMaxConfiguredEvaluations) {
        return false;
    }
    if (samplingLimits_.maxEvaluationsPerCurve == limits.maxEvaluationsPerCurve &&
        samplingLimits_.maxEvaluationsPerPaint == limits.maxEvaluationsPerPaint) {
        return true;
    }
    samplingLimits_ = limits;
    cancelScheduledWork();
    invalidateGeometry(true);
    update();
    return true;
}

void GraphCanvas::resetView() { setView(0, 0, 58); }
void GraphCanvas::setView(double x, double y, double pixelsPerUnit) {
    const double oldX = viewport_.centerX();
    const double oldY = viewport_.centerY();
    const double oldScale = viewport_.pixelsPerUnit();
    if (!viewport_.setView(x, y, pixelsPerUnit)) return;
    if (oldX == viewport_.centerX() &&
        oldY == viewport_.centerY() &&
        oldScale == viewport_.pixelsPerUnit()) return;
    cancelScheduledWork();
    invalidateGeometry(true);
    if (onViewportChanged) onViewportChanged();
    update();
}
QPointF GraphCanvas::worldToScreen(const QPointF& p) const {
    return {viewport_.worldToScreenX(p.x()), viewport_.worldToScreenY(p.y())};
}
QPointF GraphCanvas::screenToWorld(const QPointF& p) const {
    return {viewport_.screenToWorldX(p.x()), viewport_.screenToWorldY(p.y())};
}

void GraphCanvas::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), QColor(lightAppearance_ ? "#fbfdff" : "#101722"));
    p.setRenderHint(QPainter::Antialiasing, true);
    // Qt may transiently report an empty widget or a resize that could not
    // be committed to GraphViewport. Never paint with stale dimensions.
    const bool drawableView = width() > 0 && height() > 0 && viewport_.isValid() &&
        viewport_.width() == width() && viewport_.height() == height();
    if (!drawableView) return;
    // Commit queued navigation invalidation once, before BOTH shade and
    // boundary render passes. Otherwise the second pass would discard a
    // partially built region created by the first.
    if (refreshQueued_ && !repaintScheduler_.navigationPending()) {
        refreshQueued_ = false;
        invalidateGeometry(true);
    }
    drawInequalityRegions(p);
    if (showGrid_ || showAxes_) drawGrid(p);
    drawCurves(p);
    if (shadeRefreshPending_) {
        shadeRefreshPending_ = false;
        if (!deferredTimer_.isActive()) {
            if (repaintScheduler_.scheduleDeferred()) deferredTimer_.start();
        }
    }
    drawSignificantPoints(p);
    if (pointerInside_ && !dragging_) drawTrace(p);
    p.setPen(QColor(lightAppearance_ ? "#516478" : "#8395aa"));
    p.setFont(QFont("Sans Serif", 9));
    p.drawText(QRect(10, height() - 28, width() - 20, 22),
               Qt::AlignLeft | Qt::AlignVCenter,
               QString::fromUtf8("Drag to pan  •  Scroll to zoom  •  Hover to inspect"));
}

void GraphCanvas::drawGrid(QPainter& p) const {
    // Compute grid positions in bounded screen space. Casting projected
    // coordinates to int is unsafe at the extreme finite centers accepted
    // by GraphViewport; the previous implementation also repeated lines
    // when a world-space step became smaller than one representable ULP.
    const auto xAxis = graphGridAxis(viewport_.centerX(), width(),
                                     viewport_.pixelsPerUnit());
    const auto yAxis = graphGridAxis(viewport_.centerY(), height(),
                                     viewport_.pixelsPerUnit());
    p.setFont(QFont("Sans Serif", 9));
    if (showGrid_) {
        p.setPen(QPen(QColor(lightAppearance_ ? "#dae4ef" : "#253243"), 1));
        if (xAxis) {
            for (int i = 0; i < 250; ++i) {
                const double x = std::fma(double(i), xAxis->spacingPixels,
                                          xAxis->firstPixel);
                if (x > width()) break;
                if (x >= 0.0 && std::isfinite(x))
                    p.drawLine(QPointF(x, 0), QPointF(x, height()));
            }
        }
        if (yAxis) {
            for (int i = 0; i < 250; ++i) {
                const double bottomPixels = std::fma(double(i), yAxis->spacingPixels,
                                                      yAxis->firstPixel);
                if (bottomPixels > height()) break;
                const double y = height() - bottomPixels;
                if (y >= 0.0 && std::isfinite(y))
                    p.drawLine(QPointF(0, y), QPointF(width(), y));
            }
        }
    }

    // The world origin may project to an enormous or non-finite position.
    // Never forward those coordinates to QPainter, even if the viewport's
    // finite center and scale are otherwise legally configured.
    const double originX = viewport_.worldToScreenX(0.0);
    const double originY = viewport_.worldToScreenY(0.0);
    if (showAxes_) {
        p.setPen(QPen(QColor(lightAppearance_ ? "#697f93" : "#8396ad"), 1.4));
        if (std::isfinite(originY) && originY >= -2.0 &&
            originY <= height() + 2.0)
            p.drawLine(QPointF(0, originY), QPointF(width(), originY));
        if (std::isfinite(originX) && originX >= -2.0 &&
            originX <= width() + 2.0)
            p.drawLine(QPointF(originX, 0), QPointF(originX, height()));
    }

    if (!showGrid_ || width() < 90 || height() < 30) return;
    p.setPen(QColor(lightAppearance_ ? "#50657a" : "#a1b3c7"));
    const double labelY = std::clamp(
        std::isfinite(originY) ? originY + 6.0 : 6.0,
        4.0, double(height() - 20));
    const double labelX = std::clamp(
        std::isfinite(originX) ? originX + 8.0 : 8.0,
        4.0, double(width() - 78));

    if (xAxis) {
        for (int i = 0; i < 250; ++i) {
            const double x = std::fma(double(i), xAxis->spacingPixels,
                                      xAxis->firstPixel);
            if (x > width()) break;
            const double world = std::fma(double(i), xAxis->spacingWorld,
                                          xAxis->firstWorld);
            if (x < 0.0 || !std::isfinite(x) || !std::isfinite(world) ||
                std::abs(world) < xAxis->spacingWorld * 0.01) continue;
            p.drawText(QRectF(x - 32.0, labelY, 64.0, 16.0),
                       Qt::AlignHCenter, numberLabel(world));
        }
    }
    if (yAxis) {
        for (int i = 0; i < 250; ++i) {
            const double bottomPixels = std::fma(double(i), yAxis->spacingPixels,
                                                  yAxis->firstPixel);
            if (bottomPixels > height()) break;
            const double y = height() - bottomPixels;
            const double world = std::fma(double(i), yAxis->spacingWorld,
                                          yAxis->firstWorld);
            if (y < 0.0 || !std::isfinite(y) || !std::isfinite(world) ||
                std::abs(world) < yAxis->spacingWorld * 0.01) continue;
            p.drawText(QRectF(labelX, y - 9.0, 72.0, 18.0),
                       Qt::AlignLeft, numberLabel(world));
        }
    }
}

void GraphCanvas::drawInequalityRegions(QPainter& p) {
    // The exact boundary path is still built by the existing Cartesian
    // CurveSamplingJob and cached independently. The region raster has a
    // bounded 2048 evaluations per paint across ALL inequality rows.
    // An incomplete raster is never published as a finished result.
    if (inequalityRasters_.size() != curves_.size())
        inequalityRasters_.resize(curves_.size());
    const auto preview = repaintScheduler_.navigationPending()
        ? graphScreenPreview(geometryViewport_, viewport_) : std::nullopt;
    if (repaintScheduler_.navigationPending()) {
        if (!preview) return;
        p.save();
        p.setClipRect(rect());
        p.setTransform(QTransform(preview->ratio, 0, 0, preview->ratio,
                                  preview->tx, preview->ty), true);
        for (std::size_t i = 0; i < curves_.size(); ++i) {
            if (curves_[i].inequalityCurve && curves_[i].enabled &&
                i < inequalityRasters_.size() && !inequalityRasters_[i].finished.isNull())
                p.drawImage(QPointF(0, 0), inequalityRasters_[i].finished);
        }
        p.restore();
        return;
    }
    constexpr std::size_t maxCallbacksPerPaint = 2048;
    std::size_t budget = maxCallbacksPerPaint;
    bool incomplete = false;
    const double minY = viewport_.screenToWorldY(double(height()));
    const double maxY = viewport_.screenToWorldY(0.);
    if (!std::isfinite(minY) || !std::isfinite(maxY) || !(maxY > minY)) return;
    for (std::size_t i = 0; i < curves_.size(); ++i) {
        const auto& curve = curves_[i];
        if (!curve.inequalityCurve || !curve.enabled || !curve.function ||
            !curve.function->valid()) continue;
        auto& item = inequalityRasters_[i];
        if (item.finished.size() != size()) item.finished = {};
        if (!item.finished.isNull()) {
            p.drawImage(QPointF(0, 0), item.finished);
            continue;
        }
        if (!item.active || item.building.size() != size()) {
            item.building = QImage(size(), QImage::Format_ARGB32_Premultiplied);
            if (item.building.isNull()) continue;
            item.building.fill(Qt::transparent);
            item.nextColumn = 0;
            item.active = true;
        }
        QColor shade = curve.color;
        shade.setAlpha(52);
        QPainter rasterPainter(&item.building);
        rasterPainter.setPen(Qt::NoPen);
        rasterPainter.setBrush(shade);
        constexpr int columnWidth = 2;
        while (item.nextColumn < width() && budget > 0) {
            const int col = item.nextColumn;
            item.nextColumn += columnWidth;
            --budget;
            ++inequalityEvaluations_;
            const double x = viewport_.screenToWorldX(
                double(col) + 0.5 * std::min(columnWidth, width() - col));
            if (!std::isfinite(x)) continue;
            const double boundary = curve.function->evaluate(x, lastAnswer_, angleMode_, bindings_);
            const auto span = inequalityVerticalSpan(boundary, minY, maxY,
                                                      double(height()), curve.inequalityAbove);
            if (span) rasterPainter.drawRect(QRectF(double(col), span->top,
                double(std::min(columnWidth, width()-col)), span->bottom-span->top));
        }
        rasterPainter.end();
        if (item.nextColumn >= width()) {
            item.finished = std::move(item.building);
            item.active = false;
            p.drawImage(QPointF(0, 0), item.finished);
        } else incomplete = true;
    }
    if (incomplete) {
        // drawCurves may cancel its deferred timer when geometry is ready.
        // Schedule an independent follow-up paint after drawCurves completes.
        shadeRefreshPending_ = true;
    }
}

void GraphCanvas::drawCurves(QPainter& p) {
    p.save();
    p.setClipRect(rect());

    // During rapid navigation, the existing cached paths still represent
    // the same world-space curves. An affine screen-space preview makes them
    // follow the latest viewport without evaluating any mathematical curves.
    // The 16-ms timer invalidates and rebuilds at most once per interval.
    if (repaintScheduler_.navigationPending()) {
        ++previewPaints_;
        if (const auto temporary = graphScreenPreview(geometryViewport_, viewport_)) {
                const QTransform preview(temporary->ratio, 0.0, 0.0,
                                         temporary->ratio, temporary->tx,
                                         temporary->ty);
                for (std::size_t i = 0; i < curves_.size(); ++i) {
                    const auto& curve = curves_[i];
                    if (!curve.enabled || !curve.function || !curve.function->valid() ||
                        (curve.parametric() && !curve.yCoordinate->valid())) continue;
                    const QPainterPath* path = nullptr;
                    auto transform = preview;
                    if (geometryCache_.isReady(i)) {
                        path = &geometryCache_.readyGeometry(i);
                    } else if (pendingGeometry_[i].hasPreview) {
                        const auto saved = graphScreenPreview(pendingGeometry_[i].previewViewport, viewport_);
                        if (!saved) continue;
                        transform = QTransform(saved->ratio, 0, 0, saved->ratio, saved->tx, saved->ty);
                        path = &pendingGeometry_[i].preview;
                    }
                    if (!path) continue;
                    p.save();
                    p.setTransform(transform, true);
                    // The preview is temporary; final QPainterPath geometry
                    // will be resampled at the new viewport on timer expiry.
                    QPen pen(curve.color, 2.5, (curve.inequalityCurve && !curve.inequalityInclusive) ? Qt::DashLine : Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
                    pen.setCosmetic(true);
                    p.setPen(pen);
                    drawCurvePath(p, *path);
                    p.restore();
                }
        }
        samplingDiagnostics_.lastPaintEvaluations = 0;
        samplingDiagnostics_.lastPaintDeferredCurves = 0;
        p.restore();
        return;
    }

    // Count only drawable, currently dirty curves. Cached/hidden entries
    // consume no geometry budget. Reserve work for several dirty curves
    // rather than letting the first pathological one consume the frame.
    std::size_t dirty = 0;
    for (std::size_t i = 0; i < curves_.size(); ++i) {
        const auto& curve = curves_[i];
        if (curve.enabled && curve.function && curve.function->valid() &&
            (!curve.parametric() || curve.yCoordinate->valid()) &&
            !geometryCache_.isReady(i)) ++dirty;
    }

    // All ready paths in this generation use the same viewport. A change
    // outside interactive preview invalidates every path before rebuilding.
    if (dirty != 0) geometryViewport_ = viewport_;

    CurveEvaluationBudget paintBudget(samplingLimits_.maxEvaluationsPerPaint);
    std::size_t deferred = 0;

    const double deviceRatio = p.device()->devicePixelRatioF();
    const auto drawPendingPreview = [&](std::size_t index) {
        auto& pending = pendingGeometry_[index];
        if (!pending.hasPreview) return;
        if (!rasterMatches(pending.raster, size(), deviceRatio)) {
            const auto transform = graphScreenPreview(pending.previewViewport, viewport_);
            if (!transform) return;
            pending.raster = rasterizeCurve(pending.preview, curves_[index].color, size(), deviceRatio,
                QTransform(transform->ratio, 0, 0, transform->ratio, transform->tx, transform->ty),
                curves_[index].inequalityCurve && !curves_[index].inequalityInclusive);
        }
        p.drawImage(QPointF(0, 0), pending.raster);
    };

    for (std::size_t index = 0; index < curves_.size(); ++index) {
        const auto& curve = curves_[index];
        if (!curve.enabled || !curve.function || !curve.function->valid() ||
                        (curve.parametric() && !curve.yCoordinate->valid())) continue;

        if (!geometryCache_.isReady(index)) {
            // When there is insufficient budget for even four callback
            // evaluations, leave the entry unbuilt. A queued next paint
            // receives a fresh budget and will attempt it again.
            if (paintBudget.remaining() < 4) {
                ++deferred;
                drawPendingPreview(index);
                continue;
            }

            // The 1024-callback reservation helps avoid producing heavily
            // truncated paths for many visible curves. It is a soft
            // allocation floor, not an addition to the shared hard cap.
            const std::size_t allocationFloor =
                std::min<std::size_t>(1024, samplingLimits_.maxEvaluationsPerCurve);
            const std::size_t fairShare = dirty
                ? paintBudget.remaining() / dirty
                : paintBudget.remaining();
            const std::size_t quota = std::min({
                samplingLimits_.maxEvaluationsPerCurve,
                paintBudget.remaining(),
                std::max(allocationFloor, fairShare)
            });

            auto& pending = pendingGeometry_[index];
            if (!pending.job && !pending.parametricJob && !pending.implicitJob) {
                const auto function = curve.function;
                const auto answer = lastAnswer_;
                const auto mode = angleMode_;
                const auto bindings = bindings_; // immutable per-job snapshot
                // Keep a surrounding viewport of geometry for panning and
                // zoom previews, including tails outside the current window.
                GraphViewport samplingView = viewport_;
                (void)samplingView.setDimensions(width() * 2, height() * 2);
                pending.samplingOffset = QPointF(
                    (samplingView.width() - width()) * 0.5,
                    (samplingView.height() - height()) * 0.5);
                if (curve.implicit()) {
                    pending.implicitJob = std::make_unique<ImplicitContourJob>(
                        samplingView, [function, answer, mode, bindings](double x, double y) mutable {
                            auto values = bindings;
                            (void)values.set("y", y);
                            return function->evaluate(x, answer, mode, values);
                        });
                } else if (curve.parametric()) {
                    const auto yCoordinate = curve.yCoordinate;
                    pending.parametricJob = std::make_unique<ParametricSamplingJob>(
                        samplingView,
                        [function, yCoordinate, answer, mode, bindings](double t) mutable {
                            auto values = bindings;
                            // The parameter is lexically local to this curve, not
                            // a mutation of the session's scalar definition owner.
                            (void)values.set("t", t);
                            return ParametricPoint{
                                function->evaluate(0, answer, mode, values),
                                yCoordinate->evaluate(0, answer, mode, values)};
                        }, curve.tMin, curve.tMax);
                } else {
                    pending.job = std::make_unique<CurveSamplingJob>(samplingView,
                        [function, answer, mode, bindings](double x) {
                            return function->evaluate(x, answer, mode, bindings);
                        });
                }
            }
            const bool complete = pending.implicitJob
                ? pending.implicitJob->advance(quota, paintBudget)
                : (pending.parametricJob
                    ? pending.parametricJob->advance(quota, paintBudget)
                    : pending.job->advance(quota, paintBudget));
            if (!complete) {
                ++samplingDiagnostics_.truncatedCurveBuilds;
                ++deferred;
                --dirty;
                drawPendingPreview(index);
                continue;
            }
            (void)geometryCache_.getOrBuild(index, [&] {
                QPainterPath result;
                const auto& samples = pending.implicitJob
                    ? pending.implicitJob->samples()
                    : (pending.parametricJob ? pending.parametricJob->samples()
                                             : pending.job->samples());
                std::vector<CurveVertex> trace;
                trace.reserve(samples.vertices.size());
                for (const CurveVertex& point : samples.vertices) {
                    CurveVertex adjusted = point;
                    adjusted.screenX -= pending.samplingOffset.x();
                    adjusted.screenY -= pending.samplingOffset.y();
                    trace.push_back(adjusted);
                    const QPointF position(adjusted.screenX, adjusted.screenY);
                    if (point.moveTo) result.moveTo(position);
                    else result.lineTo(position);
                }
                if (index < traceVertices_.size())
                    traceVertices_[index] = std::move(trace);
                ++geometryBuildCount_;
                return result;
            });
            pending.job.reset();
            pending.parametricJob.reset();
            pending.implicitJob.reset();
            pending.preview = {};
            pending.raster = {};
            pending.hasPreview = false;
            --dirty;
        }

        auto& raster = pendingGeometry_[index].raster;
        if (!rasterMatches(raster, size(), deviceRatio)) {
            raster = rasterizeCurve(geometryCache_.readyGeometry(index), curve.color, size(), deviceRatio, {},
                                    curve.inequalityCurve && !curve.inequalityInclusive);
        }
        p.drawImage(QPointF(0, 0), raster);
    }

    samplingDiagnostics_.lastPaintEvaluations = paintBudget.consumed();
    samplingDiagnostics_.totalEvaluations += paintBudget.consumed();
    samplingDiagnostics_.lastPaintDeferredCurves = deferred;
    p.restore();

    if (deferred != 0) {
        // Defer a later paint instead of scheduling a zero-delay chain of
        // paints that could crowd out pointer and window events.
        if (repaintScheduler_.scheduleDeferred()) deferredTimer_.start();
    } else {
        deferredTimer_.stop();
        repaintScheduler_.cancelDeferred();
    }
}

void GraphCanvas::setSignificantPoints(std::vector<GraphSignificantPoint> points) {
    // The owning MainWindow explicitly invalidates these when mathematical
    // state changes; never infer a certified root/extremum from the pixels.
    significantPoints_ = std::move(points);
    update();
}

std::optional<GraphCanvas::Inspection> GraphCanvas::nearestInspection() const {
    // During navigation preview, cached vertices belong to another viewport.
    // Trace only settled, actually completed geometry for the current view.
    if (refreshQueued_ || repaintScheduler_.navigationPending() ||
        !viewport_.isValid()) return std::nullopt;
    std::optional<Inspection> nearest;
    double best = 20.0;
    for (const auto& feature : significantPoints_) {
        const QPointF point = worldToScreen({feature.x, feature.y});
        if (!std::isfinite(point.x()) || !std::isfinite(point.y())) continue;
        const double dist = std::hypot(point.x() - pointer_.x(),
                                       point.y() - pointer_.y());
        if (dist < std::min(best, 12.0)) {
            best = dist;
            nearest = Inspection{point, feature.color,
                feature.description + QString("  (%1, %2) · approximate")
                    .arg(numberLabel(feature.x), numberLabel(feature.y)), dist};
        }
    }
    for (std::size_t i = 0; i < curves_.size(); ++i) {
        const auto& curve = curves_[i];
        if (!curve.enabled || !curve.function || !curve.function->valid() ||
            (curve.parametric() && (!curve.yCoordinate || !curve.yCoordinate->valid())) ||
            !geometryCache_.isReady(i) || i >= traceVertices_.size()) continue;
        const auto hit = nearestGraphTrace(traceVertices_[i], pointer_.x(), pointer_.y(), best);
        if (!hit) continue;
        const QPointF screen(hit->screenX, hit->screenY);
        const QPointF world = screenToWorld(screen);
        if (!std::isfinite(world.x()) || !std::isfinite(world.y())) continue;
        QString type, details;
        if (curve.parametric()) {
            if (!hit->hasParameter) continue;
            const double t = hit->parameter;
            auto bindings = bindings_;
            (void)bindings.set("t", t);
            const double x = curve.function->evaluate(0, lastAnswer_, angleMode_, bindings);
            const double y = curve.yCoordinate->evaluate(0, lastAnswer_, angleMode_, bindings);
            if (!std::isfinite(x) || !std::isfinite(y)) continue;
            if (curve.polar()) {
                const double radians = angleMode_ == AngleMode::Degrees
                    ? t * (3.1415926535897932384626433832795 / 180.0) : t;
                const double r = x * std::cos(radians) + y * std::sin(radians);
                if (!std::isfinite(r)) continue;
                type = "Polar";
                details = QString("θ=%1 %2, r=%3, (x,y)=(%4,%5)")
                    .arg(numberLabel(t), angleMode_ == AngleMode::Degrees ? "deg" : "rad")
                    .arg(numberLabel(r)).arg(numberLabel(x), numberLabel(y));
            } else {
                type = "Parametric";
                details = QString("t=%1, (x,y)=(%2,%3)")
                    .arg(numberLabel(t)).arg(numberLabel(x), numberLabel(y));
            }
        } else if (curve.implicit()) {
            auto bindings = bindings_;
            (void)bindings.set("y", world.y());
            const double residual = curve.function->evaluate(world.x(), lastAnswer_, angleMode_, bindings);
            if (!std::isfinite(residual)) continue;
            type = "Implicit";
            details = QString("(x,y)≈(%1,%2), |F-G|≈%3")
                .arg(numberLabel(world.x()), numberLabel(world.y()), numberLabel(std::abs(residual)));
        } else {
            type = curve.inequalityCurve ? "Inequality boundary" : "Cartesian";
            // Preserve the projected location (not an arbitrary cursor Y).
            const double y = curve.function->evaluate(world.x(), lastAnswer_, angleMode_, bindings_);
            if (!std::isfinite(y)) continue;
            details = QString("(x,y)≈(%1,%2)").arg(numberLabel(world.x()), numberLabel(y));
            if (curve.inequalityCurve)
                details += curve.inequalityInclusive ? " · inclusive" : " · strict (excluded)";
        }
        nearest = Inspection{screen, curve.color, type + " · " + details,
                             hit->distancePixels};
        best = hit->distancePixels;
    }
    return nearest;
}

void GraphCanvas::drawSignificantPoints(QPainter& p) const {
    // Only results explicitly requested through Features are shown. The
    // MainWindow discards them when an expression or viewport becomes stale.
    for (const auto& feature : significantPoints_) {
        const QPointF point = worldToScreen({feature.x, feature.y});
        if (!std::isfinite(point.x()) || !std::isfinite(point.y()) ||
            point.x() < 0 || point.y() < 0 ||
            point.x() >= width() || point.y() >= height()) continue;
        p.setPen(QPen(feature.color, 2));
        p.setBrush(QColor(lightAppearance_ ? "#fbfdff" : "#101722"));
        p.drawEllipse(point, 4, 4);
    }
}

void GraphCanvas::drawTrace(QPainter& p) const {
    const auto selected = nearestInspection();
    if (!selected) return;
    p.setPen(QPen(selected->color, 2));
    p.setBrush(QColor(lightAppearance_ ? "#fbfdff" : "#101722"));
    p.drawEllipse(selected->point, 5, 5);
    const QFontMetrics fm(p.font());
    // Never expand a tooltip beyond the actual widget, including narrow views.
    const int boxWidth = std::min(width() - 4, fm.horizontalAdvance(selected->text) + 20);
    if (boxWidth <= 8) return;
    const QString tip = fm.elidedText(selected->text, Qt::ElideRight, boxWidth - 14);
    const double left = std::clamp(selected->point.x() + 12, 2.0,
        double(std::max(2, width() - boxWidth - 2)));
    const double top = std::clamp(selected->point.y() - 34, 2.0,
        double(std::max(2, height() - 30)));
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(lightAppearance_ ? "#e1eaf4" : "#263649"));
    p.drawRoundedRect(QRectF(left, top, boxWidth, 27), 5, 5);
    p.setPen(QColor(lightAppearance_ ? "#152d42" : "#f4f7fb"));
    p.drawText(QRectF(left, top, boxWidth, 27), Qt::AlignCenter, tip);
}

void GraphCanvas::updatePointerStatus() const {
    if (!onPointerInfo) return;
    if (const auto selected = nearestInspection()) {
        onPointerInfo(selected->text);
        return;
    }
    const QPointF world = screenToWorld(pointer_);
    if (!std::isfinite(world.x()) || !std::isfinite(world.y())) {
        onPointerInfo("Outside representable viewport range");
    } else {
        onPointerInfo(QString("x = %1,  y = %2")
            .arg(numberLabel(world.x()), numberLabel(world.y())));
    }
}

void GraphCanvas::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        dragging_ = true;
        lastDrag_ = mousePoint(e);
        setCursor(Qt::ClosedHandCursor);
    }
}
void GraphCanvas::mouseMoveEvent(QMouseEvent* e) {
    pointer_ = mousePoint(e);
    pointerInside_ = true;
    if (dragging_) {
        const QPointF delta = pointer_ - lastDrag_;
        const double oldX = viewport_.centerX();
        const double oldY = viewport_.centerY();
        if (viewport_.panPixels(delta.x(), delta.y()) &&
            (oldX != viewport_.centerX() || oldY != viewport_.centerY())) {
            requestNavigationRefresh();
            if (onViewportChanged) onViewportChanged();
        }
        lastDrag_ = pointer_;
    }
    // Re-evaluating every curve for a tooltip on each drag event can be more
    // expensive than the coalesced geometry work. Show world coordinates
    // while dragging; normal nearest-curve inspection resumes afterward.
    if (dragging_) {
        if (onPointerInfo) {
            const QPointF world = screenToWorld(pointer_);
            onPointerInfo(QString("x = %1,  y = %2")
                .arg(numberLabel(world.x()), numberLabel(world.y())));
        }
    } else {
        updatePointerStatus();
    }
    update();
}
void GraphCanvas::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        dragging_ = false;
        setCursor(Qt::CrossCursor);
    }
    update();
}
void GraphCanvas::leaveEvent(QEvent* e) {
    pointerInside_ = false;
    if (onPointerInfo) onPointerInfo("Ready");
    update();
    QWidget::leaveEvent(e);
}
void GraphCanvas::wheelEvent(QWheelEvent* e) {
    const QPointF at = wheelPoint(e);
    const double factor = std::pow(1.12, e->angleDelta().y() / 120.0);
    const double oldX = viewport_.centerX();
    const double oldY = viewport_.centerY();
    const double oldScale = viewport_.pixelsPerUnit();
    if (viewport_.zoomAt(factor, at.x(), at.y()) &&
        (oldX != viewport_.centerX() || oldY != viewport_.centerY() ||
         oldScale != viewport_.pixelsPerUnit())) {
        requestNavigationRefresh();
        if (onViewportChanged) onViewportChanged();
    }
    e->accept();
}

void GraphCanvas::resizeEvent(QResizeEvent* e) {
    QWidget::resizeEvent(e);
    const int nextWidth = e->size().width();
    const int nextHeight = e->size().height();
    if (nextWidth == viewport_.width() && nextHeight == viewport_.height())
        return; // no changes, and no redundant sampling
    // A zero-sized transition is not a valid GraphViewport. Cancel work
    // from the previous dimensions and retain the last valid view; the
    // paint guard suppresses stale geometry until positive dimensions return.
    cancelScheduledWork();
    invalidateGeometry(true);
    (void)viewport_.setDimensions(nextWidth, nextHeight);
    if (onViewportChanged) onViewportChanged();
    update();
}
} // namespace rationalstudio
