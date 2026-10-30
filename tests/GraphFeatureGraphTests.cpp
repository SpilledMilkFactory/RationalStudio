// M3-05: real MainWindow feature inspection in offscreen Qt.
#include "MainWindow.hpp"
#include "GraphCanvas.hpp"
#include "MathExpressionEdit.hpp"

#include <QApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QImage>
#include <QTimer>
#include <QLabel>
#include <QPushButton>
#include <QRegularExpression>
#include <QTabWidget>
#include <QWidget>

#include <cmath>
#include <iostream>
#include <vector>

using namespace rationalstudio;
namespace {
int checks=0,failures=0;
void check(bool ok,const char* what) {
    ++checks;
    if(!ok){++failures;std::cerr<<"FAIL: "<<what<<'\n';}
}
// A blocking sleep does not run Qt timers. In the old test, sleeping for
// 180 ms and then calling processEvents() only *started* the 130-ms debounced
// graph rebuild, so the subsequent Features click could complete legitimate
// startup geometry and incorrectly be blamed for resampling.
void pumpEvents(int milliseconds) {
    QEventLoop loop;
    QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
    loop.exec();
    QApplication::processEvents();
}
void settle() { pumpEvents(220); }

// Force real GraphCanvas paintEvents rather than relying on timing of the
// offscreen platform's exposure events. Rendering the canvas does not resize
// it, update the viewport, or invalidate the geometry cache.
void renderCanvas(GraphCanvas& canvas) {
    QImage image(canvas.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    canvas.render(&image);
}

// Establish a quiescent baseline before asserting that a *new* action did
// not resample. A completed-build count alone is insufficient: the canvas
// may have additional pending jobs. Timeouts fail the test explicitly.
bool awaitInitialGeometry(GraphCanvas& canvas, int milliseconds = 6000) {
    QElapsedTimer elapsed;
    elapsed.start();
    while (elapsed.elapsed() < milliseconds) {
        QApplication::processEvents();
        if (canvas.width() > 0 && canvas.height() > 0)
            renderCanvas(canvas);
        if (canvas.geometryBuildCount() >= 2 && canvas.visibleGeometryReady()) {
            pumpEvents(25);
            renderCanvas(canvas);
            if (canvas.visibleGeometryReady()) return true;
        }
        pumpEvents(15);
    }
    std::cerr << "DIAGNOSTIC: startup graph not fully ready after "
              << elapsed.elapsed() << " ms; completed paths="
              << canvas.geometryBuildCount() << ", last-paint deferred="
              << canvas.samplingDiagnostics().lastPaintDeferredCurves << '\n';
    return false;
}
std::vector<MathExpressionEdit*> editors(MainWindow& w) {
    std::vector<MathExpressionEdit*> result;
    for(auto* child:w.findChildren<QWidget*>())
        if(auto* e=dynamic_cast<MathExpressionEdit*>(child)) result.push_back(e);
    return result;
}
// GraphCanvas inherits QWidget without declaring Q_OBJECT.  Qt 6 rejects
// findChild<GraphCanvas*>(), so find QWidget children then use C++ RTTI.
GraphCanvas* findCanvas(MainWindow& window) {
    for (QWidget* child : window.findChildren<QWidget*>()) {
        if (auto* canvas = dynamic_cast<GraphCanvas*>(child))
            return canvas;
    }
    return nullptr;
}
bool containsFeature(const QString& report,const QString& label,double x,double y,double tol) {
    const QRegularExpression regex(
        QString(R"(Row 1: %1 \(([-+0-9.eE]+), ([-+0-9.eE]+)\))").arg(label));
    auto it=regex.globalMatch(report);
    while(it.hasNext()) {
        const auto match=it.next();
        bool okx=false,oky=false;
        const double foundX=match.captured(1).toDouble(&okx);
        const double foundY=match.captured(2).toDouble(&oky);
        if(okx&&oky&&std::abs(foundX-x)<tol&&std::abs(foundY-y)<tol) return true;
    }
    return false;
}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);
    MainWindow window;
    window.show();
    auto* tabs=window.findChild<QTabWidget*>();
    check(tabs!=nullptr,"tab widget exists");
    if(tabs) tabs->setCurrentIndex(1);
    settle();
    auto* button=window.findChild<QPushButton*>("graphFindFeatures");
    auto* results=window.findChild<QLabel*>("graphFeatureResults");
    auto* canvas=findCanvas(window);
    check(button!=nullptr,"feature toolbar control registered");
    check(results!=nullptr,"feature report registered");
    check(canvas!=nullptr,"shared renderer still exists");
    if(!button||!results||!canvas) return 1;
    check(!results->isVisible(),"report hidden until requested");
    // Both default Cartesian rows must finish their incremental paint jobs
    // before we can attribute a subsequent build to the analysis action.
    check(awaitInitialGeometry(*canvas), "initial graph geometry completed before measurement");
    const auto countBefore=canvas->geometryBuildCount();
    const auto evaluationsBefore=canvas->samplingDiagnostics().totalEvaluations;
    const QSize graphSizeBefore=canvas->size();
    button->click();
    pumpEvents(35);
    renderCanvas(*canvas);
    if (results->text().contains("stale"))
        std::cerr << "DIAGNOSTIC: first report was invalidated: "
                  << results->text().toStdString() << '\n';
    check(results->isVisible(),"report visible after click");
    check(!results->text().contains("stale"),"fresh report is not immediately marked stale");
    check(canvas->size()==graphSizeBefore,"analysis report does not resize canvas");
    check(results->text().contains("tolerance"),"root numeric tolerance shown");
    check(results->text().contains("stationary slope"),"derivative tolerance shown");
    check(results->text().contains("not searched"),"unsupported representations disclosed");
    if (canvas->geometryBuildCount()!=countBefore ||
        canvas->samplingDiagnostics().totalEvaluations!=evaluationsBefore)
        std::cerr << "DIAGNOSTIC: Features changed rendering counters: completed paths "
                  << countBefore << " -> " << canvas->geometryBuildCount()
                  << ", sampling evaluations " << evaluationsBefore << " -> "
                  << canvas->samplingDiagnostics().totalEvaluations << '\n';
    check(canvas->geometryBuildCount()==countBefore,
          "Features does not rebuild completed cached geometry");
    check(canvas->samplingDiagnostics().totalEvaluations==evaluationsBefore,
          "Features does not invoke geometry sampling");
    check(canvas->visibleGeometryReady(),
          "Features preserves completed geometry cache entries");
    auto inputs=editors(window);
    check(inputs.size()>=3,"graph rows available");
    if(inputs.size()>=3) {
        inputs[1]->setText("y = x^2 - 1");
        settle();
        check(results->text().contains("stale"),"editing invalidates report");
        button->click(); QApplication::processEvents();
        check(containsFeature(results->text(),"root",-1,0,1e-5),"negative x-axis root");
        check(containsFeature(results->text(),"root",1,0,1e-5),"positive x-axis root");
        check(containsFeature(results->text(),"local minimum",0,-1,1e-5),"parabolic minimum");
        inputs[1]->setText("y = 4 - x^2");
        settle(); button->click(); QApplication::processEvents();
        check(containsFeature(results->text(),"local maximum",0,4,1e-5),"parabolic maximum");
        inputs[1]->setText("y = x^3");
        settle();button->click();QApplication::processEvents();
        check(containsFeature(results->text(),"stationary inflection candidate",0,0,1e-4),
              "stationary inflection shown");
        inputs[1]->setText("y = x + *");
        settle();button->click();QApplication::processEvents();
        check(!results->text().contains("Row 1: root"),"invalid input excluded");
        inputs[1]->setText("y = x");
        settle();button->click();QApplication::processEvents();
        check(containsFeature(results->text(),"root",0,0,1e-5),"repaired expression analyzed");
    }
    canvas->setView(1,0,58);
    QApplication::processEvents();
    check(results->text().contains("stale"),"navigation invalidates result");
    button->click();QApplication::processEvents();
    check(!results->text().contains("stale"),"recompute after navigation succeeds");
    // Both analysis actions share this pane; switching between them must not
    // reflow the graph or invalidate the just-created report.
    auto* intersections=window.findChild<QPushButton*>("graphFindIntersections");
    auto* intersectionReport=window.findChild<QLabel*>("graphIntersectionResults");
    check(intersections!=nullptr && intersectionReport!=nullptr,
          "legacy intersection controls remain available");
    if(intersections && intersectionReport) {
        const QSize beforeSwitch=canvas->size();
        intersections->click(); QApplication::processEvents();
        check(intersectionReport->isVisible() &&
              intersectionReport->text().contains("tolerance") &&
              !intersectionReport->text().contains("stale"),
              "intersection analysis remains current when displayed");
        check(canvas->size()==beforeSwitch,
              "switching reports preserves graph viewport size");
        button->click(); QApplication::processEvents();
        check(results->isVisible() && !results->text().contains("stale"),
              "switching back to features computes a current report");
    }
    check(window.isVisible(),"desktop remains operational");
    std::cout<<(failures?"FAIL: ":"PASS: ")<<checks<<" GraphFeatureGraph assertions\n";
    return failures?1:0;
}
