#include "GraphCanvas.hpp"
#include "CurveSampler.hpp"
#include "GraphGridLayout.hpp"

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
QString numberLabel(double value) {
    if (std::abs(value) < 1e-12) value = 0;
    return QString::number(value, 'g', 6);
}
} // namespace

GraphCanvas::GraphCanvas(QWidget* parent) : QWidget(parent) {
    setMouseTracking(true);
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
    if (repaintScheduler_.navigationChanged()) navigationTimer_.start();
    // Preserve old geometry while the input interval is coalescing. It is
    // transformed to match the current viewport during preview paints.
    update();
}

void GraphCanvas::setCurves(std::vector<GraphCurve> curves) {
    // The mathematical identity, not the curve index or display label,
    // determines whether the existing geometry remains valid.
    std::vector<CurveGeometryCache<QPainterPath>::Key> keys;
    keys.reserve(curves.size());
    for (const auto& curve : curves) {
        keys.push_back(curve.function);
    }
    deferredTimer_.stop();
    repaintScheduler_.cancelDeferred();
    geometryCache_.synchronize(keys);
    curves_ = std::move(curves);
    update();
}
void GraphCanvas::setAngleMode(AngleMode mode) {
    if (angleMode_ == mode) return;
    angleMode_ = mode;
    cancelScheduledWork();
    geometryCache_.invalidateAll();
    update();
}
void GraphCanvas::setLastAnswer(double answer) {
    if (lastAnswer_ == answer || (std::isnan(lastAnswer_) && std::isnan(answer))) return;
    lastAnswer_ = answer;
    cancelScheduledWork();
    geometryCache_.invalidateAll();
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
    geometryCache_.invalidateAll();
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
    geometryCache_.invalidateAll();
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
    p.fillRect(rect(), QColor("#101722"));
    p.setRenderHint(QPainter::Antialiasing, true);
    // Qt may transiently report an empty widget or a resize that could not
    // be committed to GraphViewport. Never paint with stale dimensions.
    const bool drawableView = width() > 0 && height() > 0 && viewport_.isValid() &&
        viewport_.width() == width() && viewport_.height() == height();
    if (!drawableView) return;
    if (showGrid_ || showAxes_) drawGrid(p);
    drawCurves(p);
    if (pointerInside_ && !dragging_) drawTrace(p);
    p.setPen(QColor("#8395aa"));
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
        p.setPen(QPen(QColor("#253243"), 1));
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
        p.setPen(QPen(QColor("#8396ad"), 1.4));
        if (std::isfinite(originY) && originY >= -2.0 &&
            originY <= height() + 2.0)
            p.drawLine(QPointF(0, originY), QPointF(width(), originY));
        if (std::isfinite(originX) && originX >= -2.0 &&
            originX <= width() + 2.0)
            p.drawLine(QPointF(originX, 0), QPointF(originX, height()));
    }

    if (!showGrid_ || width() < 90 || height() < 30) return;
    p.setPen(QColor("#a1b3c7"));
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
                        !geometryCache_.isReady(i)) continue;
                    p.save();
                    p.setTransform(preview, true);
                    // The preview is temporary; final QPainterPath geometry
                    // will be resampled at the new viewport on timer expiry.
                    p.setPen(QPen(curve.color, 2.5, Qt::SolidLine,
                                  Qt::RoundCap, Qt::RoundJoin));
                    p.drawPath(geometryCache_.readyGeometry(i));
                    p.restore();
                }
        }
        samplingDiagnostics_.lastPaintEvaluations = 0;
        samplingDiagnostics_.lastPaintDeferredCurves = 0;
        p.restore();
        return;
    }

    // Commit invalidation only when actually entering a full refresh paint.
    // This prevents a timer-expiry/input interleaving from briefly drawing
    // a preview with no ready geometry.
    if (refreshQueued_) {
        refreshQueued_ = false;
        geometryCache_.invalidateAll();
    }

    // Count only drawable, currently dirty curves. Cached/hidden entries
    // consume no geometry budget. Reserve work for several dirty curves
    // rather than letting the first pathological one consume the frame.
    std::size_t dirty = 0;
    for (std::size_t i = 0; i < curves_.size(); ++i) {
        const auto& curve = curves_[i];
        if (curve.enabled && curve.function && curve.function->valid() &&
            !geometryCache_.isReady(i)) ++dirty;
    }

    // All ready paths in this generation use the same viewport. A change
    // outside interactive preview invalidates every path before rebuilding.
    if (dirty != 0) geometryViewport_ = viewport_;

    CurveEvaluationBudget paintBudget(samplingLimits_.maxEvaluationsPerPaint);
    std::size_t deferred = 0;

    for (std::size_t index = 0; index < curves_.size(); ++index) {
        const auto& curve = curves_[index];
        if (!curve.enabled || !curve.function || !curve.function->valid()) continue;

        if (!geometryCache_.isReady(index)) {
            // When there is insufficient budget for even four callback
            // evaluations, leave the entry unbuilt. A queued next paint
            // receives a fresh budget and will attempt it again.
            if (paintBudget.remaining() < 4) {
                ++deferred;
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

            CurveSamplingOptions options;
            options.maxEvaluations = quota;
            (void)geometryCache_.getOrBuild(index, [&] {
                const CurveSamples samples = sampleExplicitCurve(
                    viewport_,
                    [&](double x) {
                        return curve.function->evaluate(x, lastAnswer_, angleMode_);
                    },
                    options,
                    &paintBudget
                );

                if (samples.truncated) {
                    ++samplingDiagnostics_.truncatedCurveBuilds;
                }

                // A budget-truncated path can contain incomplete geometry.
                // It is cached deliberately to avoid indefinite re-sampling
                // on every mouse move. Changing limits or mathematical
                // inputs invalidates and rebuilds the path.
                QPainterPath result;
                for (const CurveVertex& point : samples.vertices) {
                    if (point.moveTo) result.moveTo(point.screenX, point.screenY);
                    else result.lineTo(point.screenX, point.screenY);
                }
                ++geometryBuildCount_;
                return result;
            });
            --dirty;
        }

        p.setPen(QPen(curve.color, 2.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawPath(geometryCache_.readyGeometry(index));
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

void GraphCanvas::drawTrace(QPainter& p) const {
    const QPointF world = screenToWorld(pointer_);
    if (!std::isfinite(world.x()) || !std::isfinite(world.y())) return;
    const GraphCurve* nearest = nullptr;
    double nearestY = 0;
    double distance = 24;
    for (const auto& curve : curves_) {
        if (!curve.enabled || !curve.function || !curve.function->valid()) continue;
        const double y = curve.function->evaluate(world.x(), lastAnswer_, angleMode_);
        if (!std::isfinite(y)) continue;
        const double screenY = worldToScreen({world.x(), y}).y();
        const double dy = std::abs(screenY - pointer_.y());
        if (dy < distance) { distance = dy; nearest = &curve; nearestY = y; }
    }
    if (!nearest) return;
    const QPointF point = worldToScreen({world.x(), nearestY});
    p.setPen(QPen(nearest->color, 2));
    p.setBrush(QColor("#101722"));
    p.drawEllipse(point, 5, 5);
    const QString tip = QString("(%1, %2)").arg(numberLabel(world.x()), numberLabel(nearestY));
    const QFontMetrics fm(p.font());
    const int boxWidth = fm.horizontalAdvance(tip) + 20;
    const double left = std::clamp(point.x() + 12, 2.0, double(std::max(2, width() - boxWidth - 2)));
    const double top = std::clamp(point.y() - 34, 2.0, double(std::max(2, height() - 30)));
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#263649"));
    p.drawRoundedRect(QRectF(left, top, boxWidth, 27), 5, 5);
    p.setPen(QColor("#f4f7fb"));
    p.drawText(QRectF(left, top, boxWidth, 27), Qt::AlignCenter, tip);
}

void GraphCanvas::updatePointerStatus() const {
    if (!onPointerInfo) return;
    const QPointF world = screenToWorld(pointer_);
    if (!std::isfinite(world.x()) || !std::isfinite(world.y())) {
        onPointerInfo("Outside representable viewport range");
        return;
    }
    const GraphCurve* nearest = nullptr;
    double nearestY = 0;
    double bestDistance = 24;
    for (const auto& curve : curves_) {
        if (!curve.enabled || !curve.function || !curve.function->valid()) continue;
        const double y = curve.function->evaluate(world.x(), lastAnswer_, angleMode_);
        if (!std::isfinite(y)) continue;
        const double dy = std::abs(worldToScreen({world.x(), y}).y() - pointer_.y());
        if (dy < bestDistance) { bestDistance = dy; nearest = &curve; nearestY = y; }
    }
    if (nearest)
        onPointerInfo(nearest->label + QString("  (%1, %2)")
            .arg(numberLabel(world.x()), numberLabel(nearestY)));
    else
        onPointerInfo(QString("x = %1,  y = %2")
            .arg(numberLabel(world.x()), numberLabel(world.y())));
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
    geometryCache_.invalidateAll();
    (void)viewport_.setDimensions(nextWidth, nextHeight);
    update();
}
} // namespace rationalstudio
