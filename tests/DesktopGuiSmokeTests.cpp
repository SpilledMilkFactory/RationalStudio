// M5-09 / #63 — Representative, isolated GUI interaction and real-process startup smoke.
// Intentionally uses Qt Widgets/Core only, not QtTest or custom Q_OBJECT macros.
#include "MainWindow.hpp"
#include "GraphCanvas.hpp"
#include "MathExpressionEdit.hpp"

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QProcess>
#include <QProcessEnvironment>
#include <QPushButton>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QWidget>

#include <iostream>

using namespace rationalstudio;
namespace {
int checks = 0;
int failures = 0;
void expect(bool good, const char* text) {
    ++checks;
    if (!good) { ++failures; std::cerr << "FAIL: " << text << '\n'; }
}
void events() { QApplication::processEvents(); }

GraphCanvas* findCanvas(QWidget& parent) {
    // GraphCanvas has no Q_OBJECT: Qt6's findChild<GraphCanvas*>() cannot compile.
    for (QWidget* widget : parent.findChildren<QWidget*>())
        if (auto* canvas = dynamic_cast<GraphCanvas*>(widget)) return canvas;
    return nullptr;
}

int startup(const QString& executable) {
    QTemporaryDir isolation;
    expect(isolation.isValid(), "isolated process home exists");
    expect(QFile::exists(executable), "real desktop executable exists");
    if (!isolation.isValid() || !QFile::exists(executable)) return 2;
    QDir().mkpath(isolation.filePath("config"));
    QDir().mkpath(isolation.filePath("cache"));
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("HOME", isolation.path());
    env.insert("XDG_CONFIG_HOME", isolation.filePath("config"));
    env.insert("XDG_CACHE_HOME", isolation.filePath("cache"));
    if (!env.contains("QT_QPA_PLATFORM")) env.insert("QT_QPA_PLATFORM", "offscreen");
    QProcess process;
    process.setProcessEnvironment(env);
    process.setProgram(executable);
    process.start();
    const bool started = process.waitForStarted(5000);
    expect(started, "desktop executable process starts");
    if (!started) {
        std::cerr << process.errorString().toStdString() << '\n';
        return 1;
    }
    // A dead-on-start application is not a valid startup; an indefinitely running
    // application is normal for a Qt desktop event loop.
    const bool terminatedEarly = process.waitForFinished(1700);
    const bool running = !terminatedEarly && process.state() == QProcess::Running;
    expect(running, "actual desktop executable remains alive after initialization");
    if (!running) {
        std::cerr << "Early-exit stderr: "
                  << process.readAllStandardError().toStdString() << '\n';
    }
    if (process.state() != QProcess::NotRunning) {
        process.terminate();
        if (!process.waitForFinished(3000)) {
            process.kill();
            process.waitForFinished(3000);
        }
    }
    expect(process.state() == QProcess::NotRunning, "spawned process cleaned up");
    std::cout << (failures ? "FAIL: " : "PASS: ") << checks
              << " desktop startup smoke assertions\n";
    return failures ? 1 : 0;
}

int interaction() {
    QTemporaryDir isolation;
    expect(isolation.isValid(), "temporary GUI workspace is available");
    if (!isolation.isValid()) return 2;
    MainWindow window(nullptr, isolation.filePath("desktop-smoke.ini"));
    window.resize(1130, 750);
    window.show();
    events();
    auto* tabs = window.findChild<QTabWidget*>();
    expect(window.isVisible() && tabs && tabs->count() == 4,
           "real MainWindow displays all four workspaces");
    if (!tabs || tabs->count() != 4) return 2;

    // Scientific: real keyboard Return, result and history, invalid-input recovery.
    tabs->setCurrentIndex(0);
    auto* scientific = window.findChild<MathExpressionEdit*>("ScientificExpression");
    auto* scientificPage = tabs->widget(0);
    auto* history = scientificPage ? scientificPage->findChild<QListWidget*>() : nullptr;
    expect(scientific && history, "scientific editor and history are discoverable");
    if (scientific && history) {
        scientific->setFocus();
        scientific->setText("2+3*4");
        QKeyEvent press(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
        QKeyEvent release(QEvent::KeyRelease, Qt::Key_Return, Qt::NoModifier);
        QApplication::sendEvent(scientific, &press);
        QApplication::sendEvent(scientific, &release);
        events();
        expect(history->count() == 1 && history->item(0)->text().contains("14"),
               "Return evaluates scientific expression and commits to history");
        scientific->setText("not_a_known_symbol(");
        events();
        expect(history->count() == 1 &&
                   scientific->property("syntaxState").toString() != "valid",
               "bad expression does not create a successful calculation");
        scientific->setText("sqrt(81)");
        QApplication::sendEvent(scientific, &press);
        QApplication::sendEvent(scientific, &release);
        events();
        expect(history->count() == 2 && history->item(0)->text().contains("9"),
               "scientific editor recovers and evaluates after an error");
    }

    // Graphing: UI row creation, editable expression, actual canvas, Save/Open.
    tabs->setCurrentIndex(1);
    events();
    auto* addRow = window.findChild<QPushButton*>("addCartesianGraphRow");
    auto* feature = window.findChild<QPushButton*>("graphFindFeatures");
    auto* canvas = findCanvas(window);
    int previousRows = 0;
    for (auto* editor : window.findChildren<MathExpressionEdit*>())
        if (editor->objectName().startsWith("GraphExpression_")) ++previousRows;
    expect(addRow && feature && canvas, "graph controls and genuine canvas exist");
    if (addRow && feature && canvas) {
        addRow->click();
        events();
        int newRows = 0;
        MathExpressionEdit* created = nullptr;
        for (auto* editor : window.findChildren<MathExpressionEdit*>()) {
            if (editor->objectName().startsWith("GraphExpression_")) {
                ++newRows;
                if (editor->text().trimmed().isEmpty()) created = editor;
            }
        }
        expect(newRows == previousRows + 1 && created,
               "graph creation button appends a new editable Cartesian row");
        if (created) {
            created->setText("y = x^3");
            events();
            expect(created->property("syntaxState").toString() == "valid",
                   "new graph expression passes inline syntax validation");
        }
        expect(canvas->width() > 100 && canvas->height() > 100,
               "graph viewport has drawable dimensions");
        expect(!canvas->grab().isNull(), "graph canvas produces a real rendered pixmap");
        const QString file = isolation.filePath("smoke-workspace.json");
        QString error;
        const bool saved = window.saveWorkspace(file, &error);
        expect(saved && QFile::exists(file) && QFileInfo(file).size() > 0,
               "graph workspace persists to an isolated JSON file");
        if (saved) expect(window.loadWorkspace(file, &error),
                          "graph workspace reopens using the real MainWindow API");
        feature->click();
        events();
        auto* report = window.findChild<QLabel*>("graphFeatureResults");
        expect(report && report->text().contains("Features"),
               "graph feature inspection produces an analysis report");
    }

    // Exact fractions: operate through actual form controls and action button.
    tabs->setCurrentIndex(2);
    events();
    auto* fractions = tabs->widget(2);
    if (fractions) {
        const auto edits = fractions->findChildren<QLineEdit*>();
        auto* op = fractions->findChild<QComboBox*>();
        QPushButton* calculate = nullptr;
        for (auto* button : fractions->findChildren<QPushButton*>())
            if (button->text() == "Calculate exact result") calculate = button;
        expect(edits.size() >= 2 && op && calculate,
               "fraction inputs, operator and Calculate button are discoverable");
        if (edits.size() >= 2 && op && calculate) {
            edits[0]->setText("1/2");
            edits[1]->setText("1/3");
            op->setCurrentIndex(0);
            calculate->click();
            events();
            bool exact = false;
            for (auto* label : fractions->findChildren<QLabel*>())
                if (label->text() == "5/6") exact = true;
            expect(exact, "fraction GUI performs exact rational addition");
        }
    }

    // Advanced Math: a user-invoked calculation and domain-error feedback.
    tabs->setCurrentIndex(3);
    events();
    auto* inner = window.findChild<QTabWidget*>("advancedMathTabs");
    auto* run = window.findChild<QPushButton*>("advancedComplexRun");
    auto* expression = window.findChild<QLineEdit*>("advancedComplexExpression");
    auto* output = window.findChild<QPlainTextEdit*>("advancedComplexOutput");
    expect(inner && inner->count() == 6 && run && expression && output,
           "six advanced workspaces and complex calculator controls exist");
    if (inner && run && expression && output) {
        inner->setCurrentIndex(0);
        expression->setText("sqrt(-1)");
        run->click();
        events();
        expect(output->toPlainText().contains("i"),
               "advanced complex calculation produces imaginary result");
        expression->setText("not_a_known_symbol");
        run->click();
        events();
        expect(output->toPlainText().startsWith("Error:"),
               "advanced calculator communicates invalid input");
    }

    window.close();
    events();
    expect(!window.isVisible(), "main window closes normally");
    std::cout << (failures ? "FAIL: " : "PASS: ") << checks
              << " desktop GUI interaction smoke assertions\n";
    return failures ? 1 : 0;
}
} // namespace

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    if (argc == 3 && QString::fromLocal8Bit(argv[1]) == "--startup")
        return startup(QString::fromLocal8Bit(argv[2]));
    if (argc == 2 && QString::fromLocal8Bit(argv[1]) == "--interactions")
        return interaction();
    std::cerr << "Usage: RationalDesktopGuiSmokeTests --interactions | --startup <desktop-executable>\n";
    return 2;
}
