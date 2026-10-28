#pragma once

#include "Expression.hpp"
#include "CurveSampler.hpp"
#include "ParametricSampler.hpp"
#include "GraphViewport.hpp"
#include "CurveGeometryCache.hpp"
#include "GraphRepaintScheduler.hpp"

#include <QColor>
#include <QImage>
#include <QPointF>
#include <QPainterPath>
#include <cstddef>
#include <QWidget>
#include <QTimer>
#include <functional>
#include <memory>
#include <vector>

class QMouseEvent;
class QWheelEvent;
class QPaintEvent;
class QResizeEvent;
class QEvent;
class QPainter;

namespace rationalstudio {

// Deterministic CPU evaluation limits. They bound callback counts per
// curve per paint and per paint, not callback wall-clock time.
// Unfinished builds resume on later paints until their geometry is complete.
struct GraphSamplingLimits {
    std::size_t maxEvaluationsPerCurve = 8192;
    std::size_t maxEvaluationsPerPaint = 32768;
};

struct GraphSamplingDiagnostics {
    std::size_t lastPaintEvaluations = 0;
    std::size_t totalEvaluations = 0;
    // Historical name: now counts budget pauses; geometry resumes to completion.
    std::size_t truncatedCurveBuilds = 0;
    std::size_t lastPaintDeferredCurves = 0;
};

struct GraphSchedulingDiagnostics {
    std::size_t navigationEvents = 0;
    std::size_t coalescedNavigationEvents = 0;
    std::size_t completedNavigationRefreshes = 0;
    std::size_t deferredRefreshes = 0;
    std::size_t previewPaints = 0;
};

struct GraphCurve {
    QColor color;
    std::shared_ptr<const Expression> function;
    bool enabled = true;
    QString label;
    // Non-null yCoordinate means parametric X(t),Y(t). function is X(t).
    // The existing path geometry cache and raster pipeline remain shared.
    std::shared_ptr<const Expression> yCoordinate{};
    double tMin = 0;
    double tMax = 0;
    // Stable cache identity for this coordinate pair and interval. Created
    // by GraphCanvas; never used as a mathematical coordinate expression.
    std::shared_ptr<const Expression> cacheToken{};
    [[nodiscard]] bool parametric() const noexcept { return bool(yCoordinate); }
};

class GraphCanvas final : public QWidget {
public:
    explicit GraphCanvas(QWidget* parent = nullptr);
    void setCurves(std::vector<GraphCurve> curves);
    // Set a complete binding snapshot. `changed` names are the only
    // definitions whose numeric/undefined values changed; their dependent
    // curves alone lose completed geometry and in-flight sampling jobs.
    void setVariableBindings(VariableBindings bindings,
                             const std::vector<std::string>& changed);
    void setAngleMode(AngleMode mode);
    void setLastAnswer(double ans);
    // Updating limits invalidates existing paths, including previously
    // unfinished builds. Reject invalid settings without mutating state.
    [[nodiscard]] bool setSamplingLimits(GraphSamplingLimits limits);
    [[nodiscard]] GraphSamplingLimits samplingLimits() const noexcept {
        return samplingLimits_;
    }
    [[nodiscard]] GraphSamplingDiagnostics samplingDiagnostics() const noexcept {
        return samplingDiagnostics_;
    }

    [[nodiscard]] GraphSchedulingDiagnostics schedulingDiagnostics() const noexcept {
        return {
            static_cast<std::size_t>(repaintScheduler_.navigationEvents()),
            static_cast<std::size_t>(repaintScheduler_.coalescedEvents()),
            static_cast<std::size_t>(repaintScheduler_.navigationRefreshes()),
            static_cast<std::size_t>(repaintScheduler_.deferredRefreshes()),
            previewPaints_
        };
    }

    void resetView();
    void setView(double centerX, double centerY, double pixelsPerUnit);
    double centerX() const noexcept { return viewport_.centerX(); }
    double centerY() const noexcept { return viewport_.centerY(); }
    double pixelsPerUnit() const noexcept { return viewport_.pixelsPerUnit(); }

    // Read-only diagnostic, useful for regression tests and future profiling.
    // Counts constructed painter paths, not individual mathematical probes.
    [[nodiscard]] std::size_t geometryBuildCount() const noexcept {
        return geometryBuildCount_;
    }
    void setGridVisible(bool on) { showGrid_ = on; update(); }
    void setAxesVisible(bool on) { showAxes_ = on; update(); }
    std::function<void(const QString&)> onPointerInfo;

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    QPointF worldToScreen(const QPointF& p) const;
    QPointF screenToWorld(const QPointF& p) const;
    void drawGrid(QPainter& painter) const;
    void drawCurves(QPainter& painter);
    void drawTrace(QPainter& painter) const;
    void updatePointerStatus() const;
    void requestNavigationRefresh();
    void cancelScheduledWork();
    void invalidateGeometry(bool retainPreview = false);
    struct PendingGeometry {
        std::unique_ptr<CurveSamplingJob> job;
        std::unique_ptr<ParametricSamplingJob> parametricJob;
        QPointF samplingOffset;
        QPainterPath preview;
        QImage raster;
        GraphViewport previewViewport;
        bool hasPreview = false;
    };
    std::vector<PendingGeometry> pendingGeometry_;
    std::vector<GraphCurve> curves_;
    AngleMode angleMode_ = AngleMode::Radians;
    double lastAnswer_ = 0;
    VariableBindings bindings_;
    GraphViewport viewport_;
    CurveGeometryCache<QPainterPath> geometryCache_;
    std::size_t geometryBuildCount_ = 0;
    GraphSamplingLimits samplingLimits_;
    GraphSamplingDiagnostics samplingDiagnostics_;
    GraphRepaintScheduler repaintScheduler_;
    QTimer navigationTimer_;
    QTimer deferredTimer_;
    // A timer expiry requests regeneration, but must not invalidate ready
    // preview paths until a non-preview paint actually begins.
    bool refreshQueued_ = false;
    GraphViewport geometryViewport_;
    std::size_t previewPaints_ = 0;
    bool showGrid_ = true;
    bool showAxes_ = true;
    bool dragging_ = false;
    bool pointerInside_ = false;
    QPointF lastDrag_;
    QPointF pointer_;
};

} // namespace rationalstudio
