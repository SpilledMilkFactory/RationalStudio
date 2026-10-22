#pragma once

#include "Expression.hpp"

#include <QColor>
#include <QPointF>
#include <QWidget>

#include <functional>
#include <memory>
#include <vector>

class QMouseEvent;
class QWheelEvent;
class QPaintEvent;
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
            return centerX_;
        }

        double centerY() const noexcept {
            return centerY_;
        }

        double pixelsPerUnit() const noexcept {
            return pixelsPerUnit_;
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

        double centerX_ = 0;
        double centerY_ = 0;

        double pixelsPerUnit_ = 58;

        bool showGrid_ = true;
        bool showAxes_ = true;

        bool dragging_ = false;
        bool pointerInside_ = false;

        QPointF lastDrag_;
        QPointF pointer_;
    };

} // namespace rationalstudio
