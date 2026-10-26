#include "MainWindow.hpp"
#include "Rational.hpp"
#include "VariableSliderPanel.hpp"

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QFont>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QStatusBar>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSplitter>
#include <QTabWidget>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <cmath>
#include <exception>
#include <memory>
#include <vector>

namespace rationalstudio {
namespace {
const QColor seriesColors[] = {
    QColor("#f07882"), QColor("#50c4ef"), QColor("#6fd6a4"),
    QColor("#ffc76c"), QColor("#c8a2ff"), QColor("#ed88d4"), QColor("#8ba9ff")
};

QString prettyDouble(double value) {
    if (!std::isfinite(value)) return "Undefined (non-real result or division by zero)";
    if (std::abs(value) < 1e-13) value = 0;
    return QString::number(value, 'g', 15);
}

QLabel* heading(const QString& title, const QString& subtitle) {
    auto* text = new QLabel(QString("<span style='font-size:23px;font-weight:700'>%1</span><br/>"
                                     "<span style='font-size:12px;color:#9aadc5'>%2</span>")
                            .arg(title.toHtmlEscaped(), subtitle.toHtmlEscaped()));
    text->setTextFormat(Qt::RichText);
    text->setWordWrap(true);
    return text;
}

QPushButton* makeButton(const QString& name, const QString& property = {}) {
    auto* button = new QPushButton(name);
    button->setMinimumHeight(45);
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    if (!property.isEmpty()) button->setProperty("kind", property);
    return button;
}
QString normalizedGraphInput(QString input) {
    input = input.trimmed();
    if (input.startsWith("y", Qt::CaseInsensitive)) {
        int equal = input.indexOf('=');
        if (equal > 0 && input.left(equal).trimmed().compare("y", Qt::CaseInsensitive) == 0)
            return input.mid(equal + 1).trimmed();
    }
    const int equal = input.indexOf('=');
    if (equal > 0 && input.left(equal).remove(' ').compare("f(x)", Qt::CaseInsensitive) == 0)
        return input.mid(equal + 1).trimmed();
    return input;
}
} // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Rational Studio — Scientific & Graphing Calculator");
    setMinimumSize(1000, 650);
    resize(1250, 790);
    setStyleSheet(R"(
        QMainWindow, QWidget { background:#121c2a; color:#e8eff9; font-family:"Noto Sans","DejaVu Sans",sans-serif; font-size:13px; }
        QTabWidget::pane { border:none; }
        QTabBar::tab { background:#1b2839; border-radius:8px; padding:12px 24px; margin:5px 5px 10px 0; color:#a3b6cc; }
        QTabBar::tab:selected { background:#34587a; color:white; }
        QLineEdit, QComboBox, QListWidget { background:#0e1723; border:1px solid #34475e; border-radius:8px; padding:10px; selection-background-color:#426e94; }
        QLineEdit:focus { border:1px solid #60b5eb; }
        QPushButton, QToolButton { background:#25364a; color:#edf4fc; border:1px solid #32465c; border-radius:8px; padding:8px; }
        QPushButton:hover, QToolButton:hover { background:#354f69; }
        QPushButton:pressed, QToolButton:pressed { background:#486e91; }
        QPushButton[kind="accent"] { background:#357cb2; font-weight:bold; }
        QPushButton[kind="accent"]:hover { background:#4593c8; }
        QPushButton[kind="operator"] { background:#2b465f; color:#8ed7ff; }
        QToolButton:checked { background:#416a86; }
        QScrollArea { border:0; }
        QScrollBar:vertical { width:10px; background:#182535; }
        QScrollBar::handle:vertical { background:#435974; border-radius:5px; }
        QSplitter::handle { background:#24354a; width:3px; }
    )");

    tabs_ = new QTabWidget(this);
    tabs_->addTab(makeScientificPage(), "Scientific");
    tabs_->addTab(makeGraphPage(), "Graphing");
    tabs_->addTab(makeRationalPage(), "Exact Fractions");
    setCentralWidget(tabs_);
    graphDebounce_ = new QTimer(this);
    graphDebounce_->setSingleShot(true);
    graphDebounce_->setInterval(130);
    connect(graphDebounce_, &QTimer::timeout, this, [this] { rebuildGraph(); });
    addGraphRow("y = sin(x)");
    addGraphRow("y = x^2 / 4 - 2");
    statusBar()->showMessage("Rational Studio  •  Mathematical results use double precision; fractions use exact integers.");
}

QWidget* MainWindow::makeScientificPage() {
    auto* page = new QWidget;
    auto* outer = new QHBoxLayout(page);
    outer->setContentsMargins(16, 8, 16, 15);
    outer->setSpacing(16);

    auto* calculator = new QWidget;
    auto* layout = new QVBoxLayout(calculator);
    layout->setSpacing(12);
    layout->addWidget(heading("Scientific calculator", "Type expressions directly, or use the keypad."));

    scientificInput_ = new QLineEdit;
    scientificInput_->setObjectName("ScientificExpression");
    scientificInput_->setPlaceholderText("Try: 2sin(pi/4)^2 + sqrt(9)");
    scientificInput_->setMinimumHeight(63);
    scientificInput_->setMaxLength(2048);
    QFont inputFont = scientificInput_->font();
    inputFont.setPointSize(18);
    scientificInput_->setFont(inputFont);
    layout->addWidget(scientificInput_);

    scientificResult_ = new QLabel("0");
    scientificResult_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    scientificResult_->setMinimumHeight(62);
    QFont answerFont = scientificResult_->font();
    answerFont.setPointSize(25);
    answerFont.setBold(true);
    scientificResult_->setFont(answerFont);
    scientificResult_->setStyleSheet("color:#8fe3bd; padding-right:12px;");
    scientificResult_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(scientificResult_);

    scientificHint_ = new QLabel("Enter to calculate  ·  ^ for powers  ·  ! for factorial");
    scientificHint_->setStyleSheet("color:#98aac0;");
    layout->addWidget(scientificHint_);

    // Button labels and inserted expressions are separated so the UI remains readable.
    const std::vector<std::vector<std::pair<QString, QString>>> keys = {
        {{"sin", "sin("}, {"cos", "cos("}, {"tan", "tan("}, {"asin", "asin("}, {"acos", "acos("}, {"atan", "atan("}},
        {{"ln", "ln("}, {"log₁₀", "log("}, {"√", "sqrt("}, {"|x|", "abs("}, {"x!", "!"}, {"xʸ", "^"}},
        {{"(", "("}, {")", ")"}, {"π", "pi"}, {"e", "e"}, {"%", "%"}, {"÷", "/"}},
        {{"7", "7"}, {"8", "8"}, {"9", "9"}, {"⌫", "BACK"}, {"C", "CLEAR"}, {"×", "*"}},
        {{"4", "4"}, {"5", "5"}, {"6", "6"}, {"1/x", "1/("}, {"x²", "^2"}, {"−", "-"}},
        {{"1", "1"}, {"2", "2"}, {"3", "3"}, {"ANS", "ans"}, {".", "."}, {"+", "+"}},
        {{"0", "0"}, {"00", "00"}, {"10ˣ", "10^("}, {"exp", "exp("}, {"Graph ↗", "GRAPH"}, {"=", "EQUAL"}}
    };
    auto* pad = new QGridLayout;
    pad->setSpacing(7);
    for (int r = 0; r < int(keys.size()); ++r) {
        for (int c = 0; c < int(keys[r].size()); ++c) {
            const auto& [label, action] = keys[r][c];
            const QString kind = action == "EQUAL" ? "accent" :
                (QString("+-*/^%").contains(action) && action.size() == 1 ? "operator" : "");
            auto* b = makeButton(label, kind);
            pad->addWidget(b, r, c);
            connect(b, &QPushButton::clicked, this, [this, action] {
                if (action == "EQUAL") calculateScientific();
                else if (action == "GRAPH") graphScientificExpression();
                else if (action == "CLEAR") scientificInput_->clear();
                else if (action == "BACK") scientificInput_->backspace();
                else appendScientificText(action);
            });
        }
    }
    layout->addLayout(pad, 1);
    connect(scientificInput_, &QLineEdit::returnPressed, this, [this] { calculateScientific(); });
    connect(scientificInput_, &QLineEdit::textChanged, this, [this] { updateScientificPreview(); });

    auto* sidebar = new QWidget;
    sidebar->setMinimumWidth(240);
    sidebar->setMaximumWidth(350);
    auto* right = new QVBoxLayout(sidebar);
    right->addWidget(heading("History", "Click a result to reuse it."));
    auto* modes = new QHBoxLayout;
    auto* radians = radiansButton_ = makeButton("RAD");
    auto* degrees = degreesButton_ = makeButton("DEG");
    radians->setCheckable(true);
    degrees->setCheckable(true);
    radians->setChecked(true);
    modes->addWidget(radians);
    modes->addWidget(degrees);
    connect(radians, &QPushButton::clicked, this, [this, radians, degrees] {
        radians->setChecked(true); degrees->setChecked(false); setAngleMode(AngleMode::Radians);
    });
    connect(degrees, &QPushButton::clicked, this, [this, radians, degrees] {
        degrees->setChecked(true); radians->setChecked(false); setAngleMode(AngleMode::Degrees);
    });
    right->addLayout(modes);
    right->addWidget(new QLabel("Angle mode applies to scientific and graphing functions."));
    history_ = new QListWidget;
    history_->setWordWrap(true);
    history_->setAlternatingRowColors(true);
    right->addWidget(history_, 1);
    connect(history_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        scientificInput_->setText(item->data(Qt::UserRole).toString());
        scientificInput_->setFocus();
    });
    auto* clearHistory = makeButton("Clear history");
    connect(clearHistory, &QPushButton::clicked, history_, &QListWidget::clear);
    right->addWidget(clearHistory);
    outer->addWidget(calculator, 3);
    outer->addWidget(sidebar, 1);
    return page;
}

void MainWindow::appendScientificText(const QString& text) {
    scientificInput_->insert(text);
    scientificInput_->setFocus();
}
void MainWindow::updateScientificPreview() {
    const QString source = scientificInput_->text().trimmed();
    if (source.isEmpty()) {
        scientificResult_->setText("0"); scientificHint_->setText("Enter to calculate  ·  ^ for powers  ·  ! for factorial");
        return;
    }
    Expression expression(source.toStdString());
    if (!expression.valid()) {
        scientificResult_->setText("…");
        scientificHint_->setText(QString::fromStdString(expression.error()));
        return;
    }
    if (expression.dependsOnX()) {
        scientificResult_->setText("f(x)");
        scientificHint_->setText("Contains x — use the Graphing tab to plot this expression.");
        return;
    }
    const double value = expression.evaluate(0, lastAnswer_, angleMode_);
    scientificResult_->setText(prettyDouble(value));
    scientificHint_->setText("Live preview  ·  Press Enter or = to commit the answer.");
}
void MainWindow::calculateScientific() {
    const QString source = scientificInput_->text().trimmed();
    if (source.isEmpty()) return;
    Expression expression(source.toStdString());
    if (!expression.valid()) { scientificHint_->setText(QString::fromStdString(expression.error())); return; }
    if (expression.dependsOnX()) {
        scientificHint_->setText("Use the Graphing tab for expressions containing x."); return;
    }
    const double answer = expression.evaluate(0, lastAnswer_, angleMode_);
    if (!std::isfinite(answer)) {
        scientificResult_->setText("Undefined");
        scientificHint_->setText("Result is not a finite real number.");
        return;
    }
    const QString result = prettyDouble(answer);
    auto* item = new QListWidgetItem(source + "\n= " + result);
    item->setData(Qt::UserRole, source);
    history_->insertItem(0, item);
    if (history_->count() > 150) delete history_->takeItem(history_->count() - 1);
    lastAnswer_ = answer;
    if (graph_) graph_->setLastAnswer(answer);
    scientificResult_->setText(result);
    scientificHint_->setText("Saved to history  ·  Use ans to reference this result.");
}

QWidget* MainWindow::makeGraphPage() {
    auto* page = new QWidget;
    auto* outer = new QVBoxLayout(page);
    outer->setContentsMargins(12, 4, 12, 10);
    auto* tools = new QHBoxLayout;
    auto* title = heading("Graphing workspace", "Multiple functions · instant editing · drag to pan · scroll to zoom");
    tools->addWidget(title, 1);
    auto addToolbarButton = [this, tools](const QString& text, const std::function<void()>& action) {
        auto* button = makeButton(text);
        button->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        tools->addWidget(button);
        connect(button, &QPushButton::clicked, this, action);
    };
    addToolbarButton("+ Zoom", [this] { graph_->setView(graph_->centerX(), graph_->centerY(), graph_->pixelsPerUnit() * 1.25); });
    addToolbarButton("− Zoom", [this] { graph_->setView(graph_->centerX(), graph_->centerY(), graph_->pixelsPerUnit() / 1.25); });
    addToolbarButton("Home", [this] { graph_->resetView(); });
    addToolbarButton("Save", [this] { saveGraph(); });
    addToolbarButton("Open", [this] { loadGraph(); });
    addToolbarButton("PNG", [this] { exportImage(); });
    outer->addLayout(tools);

    auto* splitter = new QSplitter(Qt::Horizontal);
    auto* side = new QWidget;
    side->setMinimumWidth(255);
    side->setMaximumWidth(470);
    auto* sv = new QVBoxLayout(side);
    sv->setContentsMargins(5, 6, 8, 4);
    auto* inputHead = new QHBoxLayout;
    auto* label = new QLabel("Expressions");
    QFont labelFont = label->font(); labelFont.setPointSize(16); labelFont.setBold(true);
    label->setFont(labelFont);
    inputHead->addWidget(label, 1);
    auto* add = makeButton("+ Add", "accent");
    add->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    inputHead->addWidget(add);
    connect(add, &QPushButton::clicked, this, [this] { addGraphRow(); });
    sv->addLayout(inputHead);
    // Issue #30: Session-local interactive variable sliders.
    // Issue #31 supplies scalar bindings and selective graph invalidation.
    // Workspace persistence is handled in Issue #35.   

    variableSliders_ = new VariableSliderPanel(side);   

    variableSliders_->onDefinitionsChanged =
        [this](const std::vector<std::string>& changed, bool scopeChanged) {
            onVariableDefinitionsChanged(changed, scopeChanged);
        };
    variableSliders_->onValueChanged =
        [this](const std::string& name, double value) {
            if (graphStatus_) {
                graphStatus_->setText("Variable " + QString::fromStdString(name) +
                                      " = " + QString::number(value, 'g', 12));
            }
        };  

    // Place the slider panel inside its own scrollable container.
    auto* sliderScroll = new QScrollArea(side); 

    sliderScroll->setWidgetResizable(true);
    sliderScroll->setWidget(variableSliders_);  

    sliderScroll->setMinimumHeight(125);
    sliderScroll->setMaximumHeight(265);    

    // Insert the slider controls above the expression list.
    sv->addWidget(sliderScroll);
    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    auto* scrollContent = new QWidget;
    graphList_ = new QVBoxLayout(scrollContent);
    graphList_->setContentsMargins(0, 0, 0, 0);
    graphList_->setSpacing(12);
    graphList_->addStretch(1);
    scroll->setWidget(scrollContent);
    sv->addWidget(scroll, 1);
    auto* sideHelp = new QLabel("Examples: sin(x), x^2, 2x+1, sqrt(abs(x))\n"
                                 "Supported: y = ..., f(x) = ..., constants, functions.");
    sideHelp->setWordWrap(true);
    sideHelp->setStyleSheet("color:#91a6be; font-size:12px;");
    sv->addWidget(sideHelp);
    splitter->addWidget(side);

    graph_ = new GraphCanvas;
    graph_->onPointerInfo = [this](const QString& value) { if (graphStatus_) graphStatus_->setText(value); };
    splitter->addWidget(graph_);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({320, 750});
    outer->addWidget(splitter, 1);
    graphStatus_ = new QLabel("Ready");
    graphStatus_->setStyleSheet("color:#8eacc8; padding-left:8px;");
    outer->addWidget(graphStatus_);
    return page;
}

void MainWindow::addGraphRow(const QString& text, bool visible, QColor color) {
    if (graphRows_.size() >= 40) {
        QMessageBox::information(this, "Graph limit", "This version supports up to 40 expressions per workspace.");
        return;
    }
    auto* row = new GraphRow;
    const QColor next = seriesColors[nextColor_++ % (sizeof(seriesColors) / sizeof(seriesColors[0]))];
    row->color = color.isValid() ? color : next;
    row->widget = new QWidget;
    auto* frame = new QVBoxLayout(row->widget);
    frame->setContentsMargins(3, 4, 3, 4);
    auto* controls = new QHBoxLayout;
    row->visible = new QToolButton;
    row->visible->setText("●");
    row->visible->setToolTip("Toggle curve visibility");
    row->visible->setCheckable(true);
    row->visible->setChecked(visible);
    row->visible->setStyleSheet(QString("color:%1; font-size:18px;").arg(row->color.name()));
    controls->addWidget(row->visible);
    row->input = new QLineEdit;
    row->input->setPlaceholderText("y = f(x)");
    row->input->setText(text);
    row->input->setMinimumHeight(46);
    row->input->setMaxLength(2048);
    controls->addWidget(row->input, 1);
    auto* remove = new QToolButton;
    remove->setText("×");
    remove->setToolTip("Remove expression");
    controls->addWidget(remove);
    frame->addLayout(controls);
    row->info = new QLabel;
    row->info->setWordWrap(true);
    row->info->setStyleSheet("color:#9aadbf; padding-left:9px; font-size:11px;");
    frame->addWidget(row->info);
    graphList_->insertWidget(graphList_->count() - 1, row->widget);
    graphRows_.push_back(row);
    connect(row->input, &QLineEdit::textChanged, this, [this] { graphDebounce_->start(); });
    connect(row->visible, &QToolButton::toggled, this, [this] { rebuildGraph(); });
    connect(remove, &QToolButton::clicked, this, [this, row] { removeGraphRow(row); });
    if (graphDebounce_) graphDebounce_->start();
}

void MainWindow::removeGraphRow(GraphRow* row) {
    auto it = std::find(graphRows_.begin(), graphRows_.end(), row);
    if (it == graphRows_.end()) return;
    graphRows_.erase(it);
    row->widget->deleteLater();
    delete row;
    rebuildGraph();
}

void MainWindow::onVariableDefinitionsChanged(
    const std::vector<std::string>& changed, bool scopeChanged) {
    if (!variableSliders_ || !graph_) return;
    graph_->setVariableBindings(variableSliders_->definitions().bindings(), changed);
    if (scopeChanged) rebuildGraph(changed);
}

void MainWindow::rebuildGraph(const std::vector<std::string>& changedScope) {
    if (!graph_) return;
    const VariableScope scope = variableSliders_
        ? variableSliders_->definitions().scope() : VariableScope{};
    std::vector<GraphCurve> curves;
    curves.reserve(graphRows_.size());
    for (auto* row : graphRows_) {
        const QString raw = row->input->text().trimmed();
        const QString source = normalizedGraphInput(raw);
        if (source.isEmpty()) {
            // Emptying a row drops the old compiled scope. Restoring identical
            // text after a declaration was removed must not revive a stale AST.
            row->parsedExpression.reset();
            row->parsedSource.clear();
            row->info->setText("Enter an expression to plot.");
            row->info->setStyleSheet("color:#93a4b9; font-size:11px;");
            continue;
        }
        // Preserve the compiled expression object when only visibility,
        // display syntax or other presentation metadata changes. Geometry
        // caching uses this identity, so reparsing every row would defeat it.
        // On a declaration/removal, only retry invalid rows or recompile
        // rows that actually named the removed variable. Unrelated compiled
        // AST identities survive scope edits, visibility and style changes.
        bool scopeAffectsRow = false;
        if (!changedScope.empty() && row->parsedExpression) {
            if (!row->parsedExpression->valid()) {
                scopeAffectsRow = true;
            } else {
                const auto& referenced = row->parsedExpression->referencedVariables();
                for (const auto& name : changedScope) {
                    if (std::find(referenced.begin(), referenced.end(), name) != referenced.end()) {
                        scopeAffectsRow = true;
                        break;
                    }
                }
            }
        }
        if (!row->parsedExpression || row->parsedSource != source || scopeAffectsRow) {
            row->parsedExpression = std::make_shared<Expression>(source.toStdString(), scope);
            row->parsedSource = source;
        }
        const auto& function = row->parsedExpression;
        if (!function->valid()) {
            row->info->setText(QString::fromStdString(function->error()));
            row->info->setStyleSheet("color:#ff9299; font-size:11px;");
            continue;
        }
        row->info->setText(row->visible->isChecked() ? "Visible · drag graph to explore" : "Hidden");
        row->info->setStyleSheet("color:#91baa4; font-size:11px;");
        curves.push_back({row->color, function, row->visible->isChecked(), raw});
    }
    graph_->setCurves(std::move(curves));
}

void MainWindow::setAngleMode(AngleMode mode) {
    angleMode_ = mode;
    // Keep the scalar definition owner in the same angle mode as the graph.
    // A scalar defined from trigonometry may itself change on a mode switch.
    if (variableSliders_) {
        const auto update = variableSliders_->definitions().setAngleMode(mode);
        if (graph_ && update.accepted)
            graph_->setVariableBindings(variableSliders_->definitions().bindings(), update.changed);
    }
    if (graph_) graph_->setAngleMode(mode);
    if (radiansButton_) radiansButton_->setChecked(mode == AngleMode::Radians);
    if (degreesButton_) degreesButton_->setChecked(mode == AngleMode::Degrees);
    updateScientificPreview();
}
void MainWindow::graphScientificExpression() {
    QString expression = scientificInput_->text().trimmed();
    if (expression.isEmpty()) expression = "sin(x)";
    addGraphRow("y = " + expression);
    tabs_->setCurrentIndex(1);
}

void MainWindow::saveGraph() {
    const QString path = QFileDialog::getSaveFileName(this, "Save graph workspace", "graph.json", "Graph JSON (*.json)");
    if (path.isEmpty()) return;
    QJsonObject root;
    root["format"] = "RationalStudioGraph";
    root["version"] = 1;
    root["centerX"] = graph_->centerX();
    root["centerY"] = graph_->centerY();
    root["pixelsPerUnit"] = graph_->pixelsPerUnit();
    root["angleMode"] = angleMode_ == AngleMode::Degrees ? "degrees" : "radians";
    QJsonArray expressions;
    for (const auto* row : graphRows_) {
        QJsonObject entry;
        entry["expression"] = row->input->text();
        entry["visible"] = row->visible->isChecked();
        entry["color"] = row->color.name();
        expressions.append(entry);
    }
    root["expressions"] = expressions;
    QFile file(path);
    const QByteArray output = QJsonDocument(root).toJson(QJsonDocument::Indented);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate) ||
        file.write(output) != output.size()) {
        QMessageBox::warning(this, "Save failed", "Could not write the graph workspace.");
    }
}
void MainWindow::loadGraph() {
    const QString path = QFileDialog::getOpenFileName(this, "Open graph workspace", QString(), "Graph JSON (*.json)");
    if (path.isEmpty()) return;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.size() > 512 * 1024) {
        QMessageBox::warning(this, "Open failed", "Cannot read the file or file is too large."); return;
    }
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        QMessageBox::warning(this, "Invalid graph", "Invalid JSON workspace."); return;
    }
    const QJsonObject root = doc.object();
    if (root.value("format").toString() != "RationalStudioGraph" ||
        root.value("version").toInt() != 1 || !root.value("expressions").isArray() ||
        root.value("expressions").toArray().size() > 40) {
        QMessageBox::warning(this, "Invalid graph", "Unsupported graph workspace format."); return;
    }
    const QJsonArray expressions = root.value("expressions").toArray();
    // Parse and validate first; only replace the workspace after basic validation succeeds.
    for (const auto& value : expressions) {
        if (!value.isObject() || !value.toObject().value("expression").isString() ||
            value.toObject().value("expression").toString().size() > 2048) {
            QMessageBox::warning(this, "Invalid graph", "Workspace contains an invalid expression entry.");
            return;
        }
    }
    for (auto* row : graphRows_) { row->widget->deleteLater(); delete row; }
    graphRows_.clear();
    for (const auto& value : expressions) {
        const QJsonObject obj = value.toObject();
        addGraphRow(obj.value("expression").toString(), obj.value("visible").toBool(true),
                    QColor(obj.value("color").toString()));
    }
    graph_->setView(root.value("centerX").toDouble(0), root.value("centerY").toDouble(0),
                    root.value("pixelsPerUnit").toDouble(58));
    // Angle mode is shared across tabs; update graph and the scientific preview.
    setAngleMode(root.value("angleMode").toString() == "degrees" ? AngleMode::Degrees : AngleMode::Radians);
    rebuildGraph();
}
void MainWindow::exportImage() {
    const QString path = QFileDialog::getSaveFileName(this, "Export graph image", "graph.png", "PNG image (*.png)");
    if (path.isEmpty()) return;
    if (!graph_->grab().save(path, "PNG"))
        QMessageBox::warning(this, "Export failed", "Could not save the graph image.");
}

QWidget* MainWindow::makeRationalPage() {
    auto* page = new QWidget;
    auto* outer = new QVBoxLayout(page);
    outer->setContentsMargins(35, 20, 35, 30);
    outer->setSpacing(20);
    outer->addWidget(heading("Exact fraction arithmetic", "The original 2019 calculator, rebuilt with arbitrary-precision integers."));
    auto* form = new QFormLayout;
    form->setHorizontalSpacing(20);
    form->setVerticalSpacing(18);
    fractionA_ = new QLineEdit("5/2");
    fractionA_->setMinimumHeight(55);
    fractionA_->setMaxLength(2048);
    fractionA_->setPlaceholderText("e.g. -42/15");
    fractionB_ = new QLineEdit("7/4");
    fractionB_->setMinimumHeight(55);
    fractionB_->setMaxLength(2048);
    fractionB_->setPlaceholderText("e.g. 7/4");
    fractionOp_ = new QComboBox;
    fractionOp_->addItem("Add  (+)", "+");
    fractionOp_->addItem("Subtract  (−)", "-");
    fractionOp_->addItem("Multiply  (×)", "*");
    fractionOp_->addItem("Divide  (÷)", "/");
    fractionOp_->setMinimumHeight(52);
    form->addRow("First rational", fractionA_);
    form->addRow("Operation", fractionOp_);
    form->addRow("Second rational", fractionB_);
    outer->addLayout(form);
    auto* calculate = makeButton("Calculate exact result", "accent");
    calculate->setMinimumHeight(58);
    outer->addWidget(calculate);
    auto* resultTitle = new QLabel("SIMPLIFIED RESULT");
    resultTitle->setStyleSheet("color:#a8b9cc; letter-spacing:2px; margin-top:20px;");
    outer->addWidget(resultTitle);
    fractionResult_ = new QLabel("17/4");
    fractionResult_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    fractionResult_->setWordWrap(true);
    fractionResult_->setStyleSheet("color:#8fe3bd; font-size:32px; font-weight:bold;");
    outer->addWidget(fractionResult_);
    fractionDecimal_ = new QLabel("Decimal approximation: 4.25");
    fractionDecimal_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    fractionDecimal_->setWordWrap(true);
    fractionDecimal_->setStyleSheet("color:#b4c4d7; font-size:16px;");
    outer->addWidget(fractionDecimal_);
    auto* note = new QLabel("Fractions can contain arbitrarily large integer numerators and denominators. "
                            "Results are reduced exactly; decimal output is rounded to at most 15 places.");
    note->setWordWrap(true);
    note->setStyleSheet("color:#94a9c0;");
    outer->addWidget(note);
    outer->addStretch(1);
    connect(calculate, &QPushButton::clicked, this, [this] { calculateFraction(); });
    connect(fractionA_, &QLineEdit::returnPressed, this, [this] { calculateFraction(); });
    connect(fractionB_, &QLineEdit::returnPressed, this, [this] { calculateFraction(); });
    return page;
}

void MainWindow::calculateFraction() {
    try {
        const Rational a = Rational::parse(fractionA_->text().toStdString());
        const Rational b = Rational::parse(fractionB_->text().toStdString());
        Rational answer;
        const QString op = fractionOp_->currentData().toString();
        if (op == "+") answer = a + b;
        else if (op == "-") answer = a - b;
        else if (op == "*") answer = a * b;
        else answer = a / b;
        fractionResult_->setText(QString::fromStdString(answer.toString()));
        fractionDecimal_->setText("Decimal approximation: " + QString::fromStdString(answer.toDecimal(15)));
    } catch (const std::exception& e) {
        fractionResult_->setText("Invalid fraction");
        fractionDecimal_->setText(QString::fromUtf8(e.what()));
    }
}

} // namespace rationalstudio
