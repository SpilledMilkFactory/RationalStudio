// Issue #40 — real MainWindow intersection search integration (Qt offscreen).
#include "MainWindow.hpp"
#include "MathExpressionEdit.hpp"

#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include <QRegularExpression>
#include <QTabWidget>
#include <QThread>
#include <QWidget>

#include <iostream>
#include <cmath>
#include <vector>

using namespace rationalstudio;
namespace {
int checks=0, failures=0;
void check(bool ok, const char* label) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
bool hasPointNear(const QString& text, double targetX, double targetY) {
    const QRegularExpression pointRe(R"(Rows 1 and 2: \(([-+0-9.eE]+), ([-+0-9.eE]+)\))");
    auto it=pointRe.globalMatch(text);
    while(it.hasNext()) {
        const auto match=it.next();
        bool xOk=false, yOk=false;
        const double x=match.captured(1).toDouble(&xOk);
        const double y=match.captured(2).toDouble(&yOk);
        if(xOk && yOk && std::abs(x-targetX)<1e-5 && std::abs(y-targetY)<1e-5)
            return true;
    }
    return false;
}
void settle() {
    QThread::msleep(190);
    QApplication::processEvents();
    QThread::msleep(20);
    QApplication::processEvents();
}
std::vector<MathExpressionEdit*> editors(MainWindow& window) {
    std::vector<MathExpressionEdit*> result;
    for (auto* widget: window.findChildren<QWidget*>())
        if (auto* editor=dynamic_cast<MathExpressionEdit*>(widget))
            result.push_back(editor);
    return result;
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    MainWindow window;
    window.show();
    auto* tabs=window.findChild<QTabWidget*>();
    check(tabs!=nullptr,"main tab control exists");
    if (tabs) tabs->setCurrentIndex(1);
    settle();
    auto* button=window.findChild<QPushButton*>("graphFindIntersections");
    auto* result=window.findChild<QLabel*>("graphIntersectionResults");
    check(button!=nullptr,"intersections toolbar button registered");
    check(result!=nullptr,"text output label registered");
    if (!button || !result) return 1;
    check(result->text().isEmpty(),"intersection label initially empty");
    button->click();
    QApplication::processEvents();
    check(result->text().contains("tolerance"),"numerical tolerances shown");
    check(result->text().contains("Rows 1 and 2") || result->text().contains("No isolated"),
          "initial Cartesian pair searched");
    check(result->text().contains("Other graph types"),"scope limitation displayed");
    auto inputs=editors(window);
    check(inputs.size()>=3,"scientific and two graph editors exist");
    if (inputs.size()>=3) {
        // First editor is scientific. Graph editors follow their creation order.
        inputs[1]->setText("y = x");
        inputs[2]->setText("y = 0");
        settle();
        check(result->text().contains("stale"),"source edit invalidates prior report");
        button->click();
        QApplication::processEvents();
        check(result->text().contains("Rows 1 and 2"),"linear intersection listed");
        check(hasPointNear(result->text(),0,0),"analytic origin reported");
        inputs[2]->setText("y = x");
        settle();
        button->click();
        QApplication::processEvents();
        check(result->text().contains("coincident"),"identical curves identified");
        inputs[2]->setText("y = 2+*");
        settle();
        button->click();
        QApplication::processEvents();
        check(result->text().contains("at least two visible"),"invalid row excluded");
        inputs[2]->setText("y = 2");
        settle();
        button->click();
        QApplication::processEvents();
        check(result->text().contains("Rows 1 and 2"),"repaired row included again");
        check(hasPointNear(result->text(),2,2),"repaired graph analytical result");
    }
    check(window.isVisible(),"application remains alive after searches");
    std::cout << (failures?"FAIL: ":"PASS: ")<<checks<<" IntersectionGraph assertions\n";
    return failures?1:0;
}
