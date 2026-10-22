#pragma once

#include "Expression.hpp"
#include "GraphViewport.hpp"

#include <QColor>
#include <QPointF>
#include <QString>
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

        void setView(
            double centerX,
            double centerY,
            double pixelsPerUnit
        );

        double centerX() const noexcept {
            return viewport_.centerX();
        }

        double centerY() const noexcept {
            return viewport_.centerY();
        }

        double pixelsPerUnit() const noexcept {
            return viewport_.pixelsPerUnit();
        }

        void setGridVisible(bool on) {
            showGrid_ = on;
            update();
        }

        void setAxesVisible(bool on) {
            showAxes_ = on;
            update();
        }

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

        void drawCurves(QPainter& painter) const;

        void drawTrace(QPainter& painter) const;

        void updatePointerStatus() const;

        std::vector<GraphCurve> curves_;

        AngleMode angleMode_ = AngleMode::Radians;

        double lastAnswer_ = 0;

        // Single authoritative source for viewport mathematics.
        GraphViewport viewport_;

        bool showGrid_ = true;
        bool showAxes_ = true;

        bool dragging_ = false;
        bool pointerInside_ = false;

        QPointF lastDrag_;
        QPointF pointer_;
    };

} // namespace rationalstudio
