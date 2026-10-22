#include "GraphCanvas.hpp"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QFont>
#include <QFontMetrics>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
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
            if (std::abs(value) < 1e-12) {
                value = 0;
            }

            return QString::number(value, 'g', 6);
        }

        // Grid lines get closer/farther by 1, 2, 5,
        // or 10 times a power of ten.
        double gridStep(double unitsPerPixel) {
            const double target = 85.0 * unitsPerPixel;

            const double power = std::pow(
                10.0,
                std::floor(std::log10(target))
            );

            const double m = target / power;

            return (
                m <= 1 ? 1 :
                (m <= 2 ? 2 :
                (m <= 5 ? 5 : 10))
            ) * power;
        }

    } // namespace

    GraphCanvas::GraphCanvas(QWidget* parent)
    : QWidget(parent) {

        setMouseTracking(true);

        setMinimumSize(350, 300);

        setCursor(Qt::CrossCursor);

        setAttribute(Qt::WA_OpaquePaintEvent);

        (void)viewport_.setDimensions(
            std::max(1, width()),
                                      std::max(1, height())
        );
    }

    void GraphCanvas::setCurves(
        std::vector<GraphCurve> curves
    ) {
        curves_ = std::move(curves);

        update();
    }

    void GraphCanvas::setAngleMode(AngleMode mode) {
        angleMode_ = mode;

        update();
    }

    void GraphCanvas::setLastAnswer(double answer) {
        lastAnswer_ = answer;

        update();
    }

    void GraphCanvas::resetView() {
        setView(0, 0, 58);
    }

    void GraphCanvas::setView(
        double x,
        double y,
        double pixelsPerUnit
    ) {
        if (viewport_.setView(x, y, pixelsPerUnit)) {
            update();
        }
    }

    QPointF GraphCanvas::worldToScreen(
        const QPointF& p
    ) const {
        return {
            viewport_.worldToScreenX(p.x()),
            viewport_.worldToScreenY(p.y())
        };
    }

    QPointF GraphCanvas::screenToWorld(
        const QPointF& p
    ) const {
        return {
            viewport_.screenToWorldX(p.x()),
            viewport_.screenToWorldY(p.y())
        };
    }

    void GraphCanvas::paintEvent(QPaintEvent*) {
        QPainter p(this);

        p.fillRect(
            rect(),
                   QColor("#101722")
        );

        p.setRenderHint(
            QPainter::Antialiasing,
            true
        );

        if (showGrid_ || showAxes_) {
            drawGrid(p);
        }

        drawCurves(p);

        if (pointerInside_ && !dragging_) {
            drawTrace(p);
        }

        p.setPen(QColor("#8395aa"));

        p.setFont(QFont("Sans Serif", 9));

        p.drawText(
            QRect(
                10,
                height() - 28,
                  width() - 20,
                  22
            ),
            Qt::AlignLeft | Qt::AlignVCenter,
            QString::fromUtf8(
                "Drag to pan  •  Scroll to zoom  •  Hover to inspect"
            )
        );
    }

    void GraphCanvas::drawGrid(QPainter& p) const {
        const QPointF min = screenToWorld({
            0.0,
            static_cast<qreal>(height())
        });

        const QPointF max = screenToWorld({
            static_cast<qreal>(width()),
                                          0.0
        });

        const double step = gridStep(
            1.0 / viewport_.pixelsPerUnit()
        );

        p.setFont(QFont("Sans Serif", 9));

        if (showGrid_) {
            p.setPen(
                QPen(QColor("#253243"), 1)
            );

            double firstX =
            std::ceil(min.x() / step) * step;

            for (int i = 0; i < 250; ++i) {
                const double x = firstX + i * step;

                if (x > max.x()) {
                    break;
                }

                const double sx =
                worldToScreen({x, 0}).x();

                p.drawLine(
                    QPointF(sx, 0),
                           QPointF(sx, height())
                );
            }

            double firstY =
            std::ceil(min.y() / step) * step;

            for (int i = 0; i < 250; ++i) {
                const double y = firstY + i * step;

                if (y > max.y()) {
                    break;
                }

                const double sy =
                worldToScreen({0, y}).y();

                p.drawLine(
                    QPointF(0, sy),
                           QPointF(width(), sy)
                );
            }
        }

        if (showAxes_) {
            p.setPen(
                QPen(QColor("#8396ad"), 1.4)
            );

            const QPointF origin =
            worldToScreen({0, 0});

            if (
                origin.y() > -2 &&
                origin.y() < height() + 2
            ) {
                p.drawLine(
                    QPointF(0, origin.y()),
                           QPointF(width(), origin.y())
                );
            }

            if (
                origin.x() > -2 &&
                origin.x() < width() + 2
            ) {
                p.drawLine(
                    QPointF(origin.x(), 0),
                           QPointF(origin.x(), height())
                );
            }
        }

        if (!showGrid_) {
            return;
        }

        p.setPen(QColor("#a1b3c7"));

        const QPointF origin =
        worldToScreen({0, 0});

        const double labelY = std::clamp(
            origin.y() + 6,
                                         4.0,
                                         double(height() - 20)
        );

        const double labelX = std::clamp(
            origin.x() + 8,
                                         4.0,
                                         double(width() - 78)
        );

        const double firstX =
        std::ceil(min.x() / step) * step;

        for (int i = 0; i < 250; ++i) {
            const double x = firstX + i * step;

            if (x > max.x()) {
                break;
            }

            if (std::abs(x) < step * 0.01) {
                continue;
            }

            const int px = int(
                worldToScreen({x, 0}).x()
            );

            p.drawText(
                QRect(
                    px - 32,
                    int(labelY),
                      64,
                      16
                ),
                Qt::AlignHCenter,
                numberLabel(x)
            );
        }

        const double firstY =
        std::ceil(min.y() / step) * step;

        for (int i = 0; i < 250; ++i) {
            const double y = firstY + i * step;

            if (y > max.y()) {
                break;
            }

            if (std::abs(y) < step * 0.01) {
                continue;
            }

            const int py = int(
                worldToScreen({0, y}).y()
            );

            p.drawText(
                QRect(
                    int(labelX),
                      py - 9,
                      72,
                      18
                ),
                Qt::AlignLeft,
                numberLabel(y)
            );
        }
    }

    void GraphCanvas::drawCurves(QPainter& p) const {
        p.save();

        p.setClipRect(rect());

        for (const auto& curve : curves_) {
            if (
                !curve.enabled ||
                !curve.function ||
                !curve.function->valid()
            ) {
                continue;
            }

            p.setPen(
                QPen(
                    curve.color,
                     2.5,
                     Qt::SolidLine,
                     Qt::RoundCap,
                     Qt::RoundJoin
                )
            );

            QPainterPath path;

            bool prevValid = false;
            double prevY = 0;

            // Two physical pixels per evaluation;
            // expressions are parsed only on edit.
            for (
                int sx = 0;
            sx <= width() + 2;
            sx += 2
            ) {
                const double x =
                screenToWorld({double(sx), 0}).x();

                const double y =
                curve.function->evaluate(
                    x,
                    lastAnswer_,
                    angleMode_
                );

                const double sy =
                worldToScreen({0, y}).y();

                const bool finite =
                std::isfinite(sy) &&
                std::abs(sy) < 1e7;

                if (!finite) {
                    prevValid = false;
                    continue;
                }

                // Discontinuous graphs are deliberately
                // split rather than connected.
                const bool jump =
                prevValid &&
                std::abs(sy - prevY) > height() * 0.7;

                if (!prevValid || jump) {
                    path.moveTo(double(sx), sy);
                } else {
                    path.lineTo(double(sx), sy);
                }

                prevValid = true;
                prevY = sy;
            }

            p.drawPath(path);
        }

        p.restore();
    }

    void GraphCanvas::drawTrace(QPainter& p) const {
        const QPointF world =
        screenToWorld(pointer_);

        const GraphCurve* nearest = nullptr;

        double nearestY = 0;
        double distance = 24;

        for (const auto& curve : curves_) {
            if (
                !curve.enabled ||
                !curve.function ||
                !curve.function->valid()
            ) {
                continue;
            }

            const double y =
            curve.function->evaluate(
                world.x(),
                                     lastAnswer_,
                                     angleMode_
            );

            if (!std::isfinite(y)) {
                continue;
            }

            const double screenY =
            worldToScreen({
                world.x(),
                          y
            }).y();

            const double dy =
            std::abs(screenY - pointer_.y());

            if (dy < distance) {
                distance = dy;
                nearest = &curve;
                nearestY = y;
            }
        }

        if (!nearest) {
            return;
        }

        const QPointF point = worldToScreen({
            world.x(),
                                            nearestY
        });

        p.setPen(
            QPen(nearest->color, 2)
        );

        p.setBrush(QColor("#101722"));

        p.drawEllipse(
            point,
            5,
            5
        );

        const QString tip =
        QString("(%1, %2)").arg(
            numberLabel(world.x()),
                                numberLabel(nearestY)
        );

        const QFontMetrics fm(p.font());

        const int boxWidth =
        fm.horizontalAdvance(tip) + 20;

        const double left = std::clamp(
            point.x() + 12,
                                       2.0,
                                       double(
                                           std::max(
                                               2,
                                               width() - boxWidth - 2
                                           )
                                       )
        );

        const double top = std::clamp(
            point.y() - 34,
                                      2.0,
                                      double(
                                          std::max(
                                              2,
                                              height() - 30
                                          )
                                      )
        );

        p.setPen(Qt::NoPen);

        p.setBrush(QColor("#263649"));

        p.drawRoundedRect(
            QRectF(
                left,
                top,
                boxWidth,
                27
            ),
            5,
            5
        );

        p.setPen(QColor("#f4f7fb"));

        p.drawText(
            QRectF(
                left,
                top,
                boxWidth,
                27
            ),
            Qt::AlignCenter,
            tip
        );
    }

    void GraphCanvas::updatePointerStatus() const {
        if (!onPointerInfo) {
            return;
        }

        const QPointF world =
        screenToWorld(pointer_);

        const GraphCurve* nearest = nullptr;

        double nearestY = 0;
        double bestDistance = 24;

        for (const auto& curve : curves_) {
            if (
                !curve.enabled ||
                !curve.function ||
                !curve.function->valid()
            ) {
                continue;
            }

            const double y =
            curve.function->evaluate(
                world.x(),
                                     lastAnswer_,
                                     angleMode_
            );

            if (!std::isfinite(y)) {
                continue;
            }

            const double dy = std::abs(
                worldToScreen({
                    world.x(),
                              y
                }).y() - pointer_.y()
            );

            if (dy < bestDistance) {
                bestDistance = dy;
                nearest = &curve;
                nearestY = y;
            }
        }

        if (nearest) {
            onPointerInfo(
                nearest->label +
                QString("  (%1, %2)").arg(
                    numberLabel(world.x()),
                                          numberLabel(nearestY)
                )
            );
        } else {
            onPointerInfo(
                QString("x = %1,  y = %2").arg(
                    numberLabel(world.x()),
                                               numberLabel(world.y())
                )
            );
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
            const QPointF delta =
            pointer_ - lastDrag_;

            (void)viewport_.panPixels(
                delta.x(),
                                      delta.y()
            );

            lastDrag_ = pointer_;
        }

        updatePointerStatus();

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

        if (onPointerInfo) {
            onPointerInfo("Ready");
        }

        update();

        QWidget::leaveEvent(e);
    }

    void GraphCanvas::wheelEvent(QWheelEvent* e) {
        const QPointF at = wheelPoint(e);

        const double factor = std::pow(
            1.12,
            e->angleDelta().y() / 120.0
        );

        (void)viewport_.zoomAt(
            factor,
            at.x(),
                               at.y()
        );

        update();

        e->accept();
    }

    void GraphCanvas::resizeEvent(QResizeEvent* e) {
        QWidget::resizeEvent(e);

        (void)viewport_.setDimensions(
            e->size().width(),
                                      e->size().height()
        );

        update();
    }

} // namespace rationalstudio
