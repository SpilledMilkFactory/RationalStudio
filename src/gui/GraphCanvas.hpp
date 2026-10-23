#pragma once

#include "Expression.hpp"
#include "GraphViewport.hpp"
#include "CurveGeometryCache.hpp"

#include <QColor>
#include <QPointF>
#include <QPainterPath>
#include <cstddef>
#include <QWidget>
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

struct GraphCurve {
    QColor color;
    std::shared_ptr<const Expression> function;
    bool enabled = true;
    QString label;
};

class GraphCanvas final : public QWidget {
public:
    explicit GraphCanvas(QWidget* parent = nullptr);
    void setCurves(std::vector<GraphCurve> curves);
    void setAngleMode(AngleMode mode);
    void setLastAnswer(double ans);
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
    std::vector<GraphCurve> curves_;
    AngleMode angleMode_ = AngleMode::Radians;
    double lastAnswer_ = 0;
    GraphViewport viewport_;
    CurveGeometryCache<QPainterPath> geometryCache_;
    std::size_t geometryBuildCount_ = 0;
    bool showGrid_ = true;
    bool showAxes_ = true;
    bool dragging_ = false;
    bool pointerInside_ = false;
    QPointF lastDrag_;
    QPointF pointer_;
};

} // namespace rationalstudio
