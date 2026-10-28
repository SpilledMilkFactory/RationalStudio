#include "MainWindow.hpp"
#include "Rational.hpp"
#include "VariableSliderPanel.hpp"
#include "MathExpressionEdit.hpp"
#include "MathExpressionInput.hpp"
#include "GraphWorkspacePersistence.hpp"
#include "PolarExpression.hpp"

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QDoubleSpinBox>
#include <QSaveFile>
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
// The core parser owns syntax validity and byte locations. This presentation
// helper maps normalized offsets back to readable Unicode character columns.
bool isIncompleteEdit(const Expression& expression, const std::string& normalized) {
    const auto* diagnostic = expression.diagnostic();
    return diagnostic && diagnostic->kind == ExpressionErrorKind::Syntax &&
        MathExpressionInput::looksIncomplete(normalized);
}

QString inlineErrorMessage(const Expression& expression, const QString& visibleBody) {
    const auto* diagnostic = expression.diagnostic();
    if (!diagnostic) return QString::fromStdString(expression.error());
    const auto original = visibleBody.toStdString();
    const auto column = MathExpressionInput::sourceColumn(original, diagnostic->offset);
    QString message = QString("Column %1: %2")
        .arg(static_cast<qulonglong>(column))
        .arg(QString::fromStdString(diagnostic->message));
    if (diagnostic->kind == ExpressionErrorKind::UnknownIdentifier)
        message += " — check the name or declare a slider variable.";
    return message;
}

// The parametric equation labels are presentation syntax. The coordinate
// expressions are compiled against a declared local t, not an extra global
// variable or a new parser grammar.
QString parametricBody(QString input, QChar coordinate) {
    input = input.trimmed();
    const int equal = input.indexOf('=');
    if (equal >= 0) {
        QString name = input.left(equal).trimmed();
        name.remove(' ');
        if (name.compare(QString(coordinate) + "(t)", Qt::CaseInsensitive) != 0)
            return QString();
        return input.mid(equal + 1).trimmed();
    }
    return input;
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
        QLineEdit[syntaxState="invalid"] { border:2px solid #f07d86; }
        QLineEdit[syntaxState="incomplete"] { border:1px solid #d7ad63; }
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
    layout->addWidget(heading("Scientific calculator", "Type directly or use the keypad · Alt+P π · Alt+R √ · Alt+2 ² · Alt+S sin"));

    scientificInput_ = new MathExpressionEdit;
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
    scientificHint_->setTextFormat(Qt::PlainText);
    scientificHint_->setWordWrap(true);
    scientificHint_->setStyleSheet("color:#98aac0;");
    layout->addWidget(scientificHint_);

    // Button labels and inserted expressions are separated so the UI remains readable.
    const std::vector<std::vector<std::pair<QString, QString>>> keys = {
        {{"sin", "sin("}, {"cos", "cos("}, {"tan", "tan("}, {"asin", "asin("}, {"acos", "acos("}, {"atan", "atan("}},
        {{"ln", "ln("}, {"log₁₀", "log("}, {"√", "√"}, {"|x|", "abs("}, {"x!", "!"}, {"xʸ", "^"}},
        {{"(", "("}, {")", ")"}, {"π", "π"}, {"e", "e"}, {"%", "%"}, {"÷", "÷"}},
        {{"7", "7"}, {"8", "8"}, {"9", "9"}, {"⌫", "BACK"}, {"C", "CLEAR"}, {"×", "×"}},
        {{"4", "4"}, {"5", "5"}, {"6", "6"}, {"1/x", "1/("}, {"x²", "²"}, {"−", "−"}},
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
    scientificInput_->insertMathSymbol(text);
}
void MainWindow::updateScientificPreview() {
    const QString source = scientificInput_->text().trimmed();
    if (source.isEmpty()) {
        scientificInput_->setSyntaxState("empty");
        scientificInput_->setToolTip(QString());
        scientificResult_->setText("0"); scientificHint_->setText("Enter to calculate  ·  ^ for powers  ·  ! for factorial");
        return;
    }
    Expression expression(MathExpressionInput::normalize(source.toStdString()));
    if (!expression.valid()) {
        const std::string canonical = MathExpressionInput::normalize(source.toStdString());
        const bool incomplete = isIncompleteEdit(expression, canonical);
        scientificInput_->setSyntaxState(incomplete ? "incomplete" : "invalid");
        scientificResult_->setText("…");
        const QString message = incomplete
            ? "Incomplete expression — " + inlineErrorMessage(expression, source)
            : inlineErrorMessage(expression, source);
        scientificHint_->setText(message);
        scientificInput_->setToolTip(message);
        return;
    }
    scientificInput_->setSyntaxState("valid");
    scientificInput_->setToolTip(QString());
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
    Expression expression(MathExpressionInput::normalize(source.toStdString()));
    if (!expression.valid()) {
        const QString message = inlineErrorMessage(expression, source);
        scientificInput_->setSyntaxState("invalid");
        scientificHint_->setText(message);
        scientificInput_->setToolTip(message);
        return;
    }
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
    auto* addParametric = makeButton("+ Parametric");
    addParametric->setObjectName("addParametricGraphRow");
    addParametric->setToolTip("Add X(t), Y(t) coordinate expressions and a parameter interval");
    addParametric->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    connect(addParametric, &QPushButton::clicked, this, [this] { addParametricRow(); });
    inputHead->addWidget(addParametric);
    auto* addPolar = makeButton("+ Polar");
    addPolar->setObjectName("addPolarGraphRow");
    addPolar->setToolTip("Add r = f(θ) with an angle interval");
    connect(addPolar, &QPushButton::clicked, this, [this] { addPolarRow(); });
    inputHead->addWidget(addPolar);
    sv->addLayout(inputHead);

    // Issue #32: buttons insert genuine mathematical glyphs into the
    // currently active row without stealing or resetting its cursor.
    auto* mathSymbols = new QHBoxLayout;
    mathSymbols->setSpacing(3);
    const std::vector<std::pair<QString, QString>> symbolButtons = {
        {"π", "Pi"}, {"√", "Root"}, {"²", "Squared"},
        {"×", "Multiply"}, {"÷", "Divide"}, {"−", "Minus"}
    };
    for (const auto& [symbol, name] : symbolButtons) {
        auto* button = new QToolButton(side);
        button->setObjectName("graphMathSymbol" + name);
        button->setText(symbol);
        const QString shortcut =
            name == "Pi" ? "Alt+P" : name == "Root" ? "Alt+R" :
            name == "Squared" ? "Alt+2" : name == "Multiply" ? "Alt+M" :
            name == "Divide" ? "Alt+D" : QString();
        button->setToolTip("Insert " + symbol + " at the expression cursor" +
                           (shortcut.isEmpty() ? QString() : " (" + shortcut + ")"));
        button->setMinimumHeight(28);
        mathSymbols->addWidget(button);
        connect(button, &QToolButton::clicked, this, [this, symbol] {
            if (activeGraphInput_) activeGraphInput_->insertMathSymbol(symbol);
        });
    }
    sv->addLayout(mathSymbols);
    // Issue #30: Session-local interactive variable sliders.
    // Issue #31 supplies scalar bindings and selective graph invalidation.
    // Workspace persistence is handled in Issue #35.   

    variableSliders_ = new VariableSliderPanel(side);   

    variableSliders_->mayAddVariable = [this](const std::string& name, std::string* error) {
        if (functionDefinitions_.contains(name)) {
            if (error) *error = "Name conflicts with workspace function";
            return false;
        }
        return true;
    };
    variableSliders_->mayRemoveVariable = [this](const std::string& name, std::string* error) {
        for (const auto& function : functionDefinitions_.names()) {
            const auto deps = functionDefinitions_.variableDependencies(function);
            if (std::find(deps.begin(), deps.end(), name) != deps.end()) {
                if (error) *error = "Slider is referenced by function '" + function + "'";
                return false;
            }
        }
        return true;
    };
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
    auto* sideHelp = new QLabel("Cartesian: y = sin(x) · Parametric: x(t) = cos(t), y(t) = sin(t)\n"
                                 "Polar: r = 2cos(θ) · angle intervals use current RAD/DEG mode.\n"
                                 "Parametric and polar intervals are finite and configurable.\n"
                                 "Supported: y = ..., f(x) = ..., constants, functions.\n"
                                 "Alt+P π · Alt+R √ · Alt+2 ² · Alt+S sin\n"
                                 "Alt+↑/↓ switch rows · Ctrl+Enter add a row.");
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
    row->input = new MathExpressionEdit;
    row->input->setObjectName(QString("GraphExpression_%1").arg(nextColor_));
    row->input->setPlaceholderText("y = f(x)");
    row->input->setGraphEditingMode(true);
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
    row->info->setTextFormat(Qt::PlainText);
    row->info->setWordWrap(true);
    row->info->setStyleSheet("color:#9aadbf; padding-left:9px; font-size:11px;");
    frame->addWidget(row->info);
    graphList_->insertWidget(graphList_->count() - 1, row->widget);
    graphRows_.push_back(row);
    activeGraphInput_ = row->input;
    // Keyboard-only graph authoring. Sender lifetime is owned by the row;
    // navigation is scoped to the current row order and wraps at both ends.
    connect(row->input, &MathExpressionEdit::graphNavigationRequested, this,
            [this, row](int direction) {
                auto it = std::find(graphRows_.begin(), graphRows_.end(), row);
                if (it == graphRows_.end() || graphRows_.empty()) return;
                const auto index = static_cast<std::size_t>(it - graphRows_.begin());
                const auto count = graphRows_.size();
                const auto target = direction < 0 ? (index + count - 1) % count
                                                  : (index + 1) % count;
                activeGraphInput_ = graphRows_[target]->input;
                activeGraphInput_->setFocus(Qt::ShortcutFocusReason);
            });
    connect(row->input, &MathExpressionEdit::graphRowRequested, this, [this] {
        const auto count = graphRows_.size();
        addGraphRow();
        if (graphRows_.size() > count) {
            activeGraphInput_ = graphRows_.back()->input;
            activeGraphInput_->setFocus(Qt::ShortcutFocusReason);
        }
    });
    connect(row->input, &QLineEdit::cursorPositionChanged, this,
            [this, row] { activeGraphInput_ = row->input; });
    connect(row->input, &QLineEdit::selectionChanged, this,
            [this, row] { activeGraphInput_ = row->input; });
    connect(row->input, &QLineEdit::textChanged, this, [this, row] {
        activeGraphInput_ = row->input;
        updateGraphEditorFeedback(row); // immediate, before debounced rendering
        if (graphDebounce_) graphDebounce_->start();
    });
    connect(row->visible, &QToolButton::toggled, this, [this] { rebuildGraph(); });
    connect(remove, &QToolButton::clicked, this, [this, row] { removeGraphRow(row); });
    if (graphDebounce_) graphDebounce_->start();
}

void MainWindow::addParametricRow(const QString& x, const QString& y,
                                  double minimum, double maximum,
                                  bool visible, QColor color) {
    if (graphRows_.size() >= 40) {
        QMessageBox::information(this, "Graph limit", "This version supports up to 40 expressions per workspace.");
        return;
    }
    auto* row = new GraphRow;
    row->parametric = true;
    const QColor next = seriesColors[nextColor_++ % (sizeof(seriesColors) / sizeof(seriesColors[0]))];
    row->color = color.isValid() ? color : next;
    row->widget = new QWidget;
    auto* frame = new QVBoxLayout(row->widget);
    frame->setContentsMargins(3, 4, 3, 4);
    auto* headingRow = new QHBoxLayout;
    row->visible = new QToolButton;
    row->visible->setText("●");
    row->visible->setCheckable(true);
    row->visible->setChecked(visible);
    row->visible->setToolTip("Toggle parametric curve visibility");
    row->visible->setStyleSheet(QString("color:%1; font-size:18px;").arg(row->color.name()));
    headingRow->addWidget(row->visible);
    auto* label = new QLabel("Parametric X(t), Y(t)");
    headingRow->addWidget(label, 1);
    auto* remove = new QToolButton;
    remove->setText("×");
    remove->setToolTip("Remove parametric curve");
    headingRow->addWidget(remove);
    frame->addLayout(headingRow);
    row->input = new MathExpressionEdit;
    row->input->setObjectName(QString("ParametricX_%1").arg(nextColor_));
    row->input->setGraphEditingMode(true);
    row->input->setPlaceholderText("x(t) = cos(t)");
    row->input->setMaxLength(2048);
    row->input->setText(x);
    row->yInput = new MathExpressionEdit;
    row->yInput->setObjectName(QString("ParametricY_%1").arg(nextColor_));
    row->yInput->setGraphEditingMode(true);
    row->yInput->setPlaceholderText("y(t) = sin(t)");
    row->yInput->setMaxLength(2048);
    row->yInput->setText(y);
    frame->addWidget(row->input);
    frame->addWidget(row->yInput);
    auto* range = new QHBoxLayout;
    range->addWidget(new QLabel("t min:"));
    row->tMinimum = new QDoubleSpinBox;
    row->tMinimum->setObjectName("parametricMinimum");
    row->tMinimum->setRange(-1e8, 1e8);
    row->tMinimum->setDecimals(12);
    row->tMinimum->setValue(minimum);
    range->addWidget(row->tMinimum);
    range->addWidget(new QLabel("t max:"));
    row->tMaximum = new QDoubleSpinBox;
    row->tMaximum->setObjectName("parametricMaximum");
    row->tMaximum->setRange(-1e8, 1e8);
    row->tMaximum->setDecimals(12);
    row->tMaximum->setValue(maximum);
    range->addWidget(row->tMaximum);
    frame->addLayout(range);
    row->info = new QLabel("Parameter interval must satisfy min < max.");
    row->info->setTextFormat(Qt::PlainText);
    row->info->setWordWrap(true);
    frame->addWidget(row->info);
    graphList_->insertWidget(graphList_->count() - 1, row->widget);
    graphRows_.push_back(row);
    activeGraphInput_ = row->input;
    for (auto* input : {row->input, row->yInput}) {
        connect(input, &QLineEdit::cursorPositionChanged, this, [this, input] {
            activeGraphInput_ = input;
        });
        connect(input, &QLineEdit::selectionChanged, this, [this, input] {
            activeGraphInput_ = input;
        });
        connect(input, &QLineEdit::textChanged, this, [this, row, input] {
            activeGraphInput_ = input;
            updateGraphEditorFeedback(row);
            if (graphDebounce_) graphDebounce_->start();
        });
        connect(input, &MathExpressionEdit::graphRowRequested, this, [this] {
            addParametricRow();
            if (activeGraphInput_) activeGraphInput_->setFocus(Qt::ShortcutFocusReason);
        });
        connect(input, &MathExpressionEdit::graphNavigationRequested, this, [this, row](int direction) {
            auto it = std::find(graphRows_.begin(), graphRows_.end(), row);
            if (it == graphRows_.end() || graphRows_.empty()) return;
            const auto index = static_cast<std::size_t>(it - graphRows_.begin());
            const auto target = direction < 0
                ? (index + graphRows_.size() - 1) % graphRows_.size()
                : (index + 1) % graphRows_.size();
            activeGraphInput_ = graphRows_[target]->input;
            activeGraphInput_->setFocus(Qt::ShortcutFocusReason);
        });
    }
    for (auto* spin : {row->tMinimum, row->tMaximum}) {
        connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
                [this, row](double) {
                    updateGraphEditorFeedback(row);
                    if (graphDebounce_) graphDebounce_->start();
                });
    }
    connect(row->visible, &QToolButton::toggled, this, [this] { rebuildGraph(); });
    connect(remove, &QToolButton::clicked, this, [this, row] { removeGraphRow(row); });
    if (graphDebounce_) graphDebounce_->start();
}

void MainWindow::addPolarRow(const QString& expression, double minimum, double maximum,
                             bool visible, QColor color) {
    if (graphRows_.size() >= 40) {
        QMessageBox::information(this, "Graph limit", "This version supports up to 40 expressions per workspace.");
        return;
    }
    auto* row = new GraphRow;
    row->polar = true;
    const QColor next = seriesColors[nextColor_++ % (sizeof(seriesColors) / sizeof(seriesColors[0]))];
    row->color = color.isValid() ? color : next;
    row->widget = new QWidget;
    auto* frame = new QVBoxLayout(row->widget);
    frame->setContentsMargins(3, 4, 3, 4);
    auto* headingRow = new QHBoxLayout;
    row->visible = new QToolButton;
    row->visible->setText("●");
    row->visible->setCheckable(true);
    row->visible->setChecked(visible);
    row->visible->setStyleSheet(QString("color:%1; font-size:18px;").arg(row->color.name()));
    headingRow->addWidget(row->visible);
    headingRow->addWidget(new QLabel("Polar r(θ)"), 1);
    auto* remove = new QToolButton;
    remove->setText("×");
    remove->setToolTip("Remove polar curve");
    headingRow->addWidget(remove);
    frame->addLayout(headingRow);
    row->input = new MathExpressionEdit;
    row->input->setObjectName(QString("PolarRadius_%1").arg(nextColor_));
    row->input->setGraphEditingMode(true);
    row->input->setPlaceholderText("r = 2cos(θ)");
    row->input->setMaxLength(2048);
    row->input->setText(expression);
    frame->addWidget(row->input);
    auto* interval = new QHBoxLayout;
    interval->addWidget(new QLabel("θ min:"));
    row->tMinimum = new QDoubleSpinBox;
    row->tMinimum->setObjectName("polarMinimum");
    row->tMinimum->setRange(-1e8, 1e8);
    row->tMinimum->setDecimals(12);
    row->tMinimum->setValue(minimum);
    interval->addWidget(row->tMinimum);
    interval->addWidget(new QLabel("θ max:"));
    row->tMaximum = new QDoubleSpinBox;
    row->tMaximum->setObjectName("polarMaximum");
    row->tMaximum->setRange(-1e8, 1e8);
    row->tMaximum->setDecimals(12);
    row->tMaximum->setValue(maximum);
    interval->addWidget(row->tMaximum);
    frame->addLayout(interval);
    row->info = new QLabel;
    row->info->setTextFormat(Qt::PlainText);
    row->info->setWordWrap(true);
    frame->addWidget(row->info);
    graphList_->insertWidget(graphList_->count() - 1, row->widget);
    graphRows_.push_back(row);
    activeGraphInput_ = row->input;
    connect(row->input, &QLineEdit::textChanged, this, [this, row] {
        activeGraphInput_ = row->input;
        updateGraphEditorFeedback(row);
        if (graphDebounce_) graphDebounce_->start();
    });
    connect(row->input, &QLineEdit::cursorPositionChanged, this, [this, row] {
        activeGraphInput_ = row->input;
    });
    connect(row->input, &MathExpressionEdit::graphRowRequested, this, [this] {
        addPolarRow();
        if (activeGraphInput_) activeGraphInput_->setFocus(Qt::ShortcutFocusReason);
    });
    connect(row->input, &MathExpressionEdit::graphNavigationRequested, this, [this, row](int direction) {
        const auto it = std::find(graphRows_.begin(), graphRows_.end(), row);
        if (it == graphRows_.end()) return;
        const auto index = static_cast<std::size_t>(it - graphRows_.begin());
        const auto n = graphRows_.size();
        activeGraphInput_ = graphRows_[direction < 0 ? (index + n - 1) % n : (index + 1) % n]->input;
        activeGraphInput_->setFocus(Qt::ShortcutFocusReason);
    });
    for (auto* spin : {row->tMinimum, row->tMaximum}) {
        connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
                [this, row](double) {
                    updateGraphEditorFeedback(row);
                    if (graphDebounce_) graphDebounce_->start();
                });
    }
    connect(row->visible, &QToolButton::toggled, this, [this] { rebuildGraph(); });
    connect(remove, &QToolButton::clicked, this, [this, row] { removeGraphRow(row); });
    updateGraphEditorFeedback(row);
    if (graphDebounce_) graphDebounce_->start();
}

void MainWindow::removeGraphRow(GraphRow* row) {
    auto it = std::find(graphRows_.begin(), graphRows_.end(), row);
    if (it == graphRows_.end()) return;
    graphRows_.erase(it);
    if (activeGraphInput_ == row->input || activeGraphInput_ == row->yInput)
        activeGraphInput_ = graphRows_.empty() ? nullptr : graphRows_.back()->input;
    row->widget->deleteLater();
    delete row;
    rebuildGraph();
}

void MainWindow::updateGraphEditorFeedback(GraphRow* row) {
    if (!row || !row->input) return;
    if (row->polar) {
        const auto coordinates = polarCoordinateSources(row->input->text().toStdString());
        QString error;
        if (!coordinates.valid()) {
            error = "Enter a polar expression such as r = 2cos(θ).";
        } else if (!validParameterInterval(row->tMinimum->value(), row->tMaximum->value())) {
            error = "Angle interval requires finite -1e8 <= min < max <= 1e8.";
        } else {
            VariableScope local = variableSliders_ ?
                variableSliders_->definitions().scope() : VariableScope{};
            if (!local.contains("t")) (void)local.declare("t");
            const auto x = QString::fromStdString(coordinates.x);
            const auto y = QString::fromStdString(coordinates.y);
            if (!row->parsedExpression || !row->parsedYExpression ||
                row->parsedSource != x || row->parsedYSource != y ||
                row->compiledMinimum != row->tMinimum->value() ||
                row->compiledMaximum != row->tMaximum->value()) {
                row->parsedExpression = std::make_shared<Expression>(coordinates.x, local, functionDefinitions_.scope());
                row->parsedYExpression = std::make_shared<Expression>(coordinates.y, local, functionDefinitions_.scope());
                row->parsedSource = x;
                row->parsedYSource = y;
                row->compiledMinimum = row->tMinimum->value();
                row->compiledMaximum = row->tMaximum->value();
            }
            if (!row->parsedExpression->valid())
                error = "Polar radius: " + QString::fromStdString(row->parsedExpression->error());
            else if (!row->parsedYExpression->valid())
                error = "Polar radius: " + QString::fromStdString(row->parsedYExpression->error());
            else if (row->parsedExpression->dependsOnX() || row->parsedYExpression->dependsOnX())
                error = "Use θ as the polar parameter, not Cartesian x.";
        }
        const bool valid = error.isEmpty();
        row->input->setSyntaxState(valid ? "valid" : "invalid");
        row->input->setToolTip(error);
        row->info->setText(valid ? "Polar radius · θ interval uses selected angle mode" : error);
        row->info->setStyleSheet(valid ? "color:#91baa4; font-size:11px;" :
                                        "color:#ff9299; font-size:11px;");
        return;
    }
    if (row->parametric) {
        const auto xRaw = parametricBody(row->input->text(), 'x');
        const auto yRaw = parametricBody(row->yInput->text(), 'y');
        const auto min = row->tMinimum->value(), max = row->tMaximum->value();
        QString error;
        if (xRaw.isEmpty() || yRaw.isEmpty()) {
            error = "Enter x(t) = ... and y(t) = ... with valid labels.";
        } else if (!validParameterInterval(min, max)) {
            error = "Parameter interval requires finite -1e8 <= min < max <= 1e8.";
        } else {
            VariableScope scope = variableSliders_ ?
                variableSliders_->definitions().scope() : VariableScope{};
            if (!scope.contains("t")) (void)scope.declare("t");
            const auto nx = MathExpressionInput::normalize(xRaw.toStdString());
            const auto ny = MathExpressionInput::normalize(yRaw.toStdString());
            if (!row->parsedExpression || !row->parsedYExpression ||
                row->parsedSource != QString::fromStdString(nx) ||
                row->parsedYSource != QString::fromStdString(ny) ||
                row->compiledMinimum != min || row->compiledMaximum != max) {
                row->parsedExpression = std::make_shared<Expression>(nx, scope, functionDefinitions_.scope());
                row->parsedYExpression = std::make_shared<Expression>(ny, scope, functionDefinitions_.scope());
                row->parsedSource = QString::fromStdString(nx);
                row->parsedYSource = QString::fromStdString(ny);
                row->compiledMinimum = min;
                row->compiledMaximum = max;
            }
            if (!row->parsedExpression->valid())
                error = "X(t): " + inlineErrorMessage(*row->parsedExpression, xRaw);
            else if (!row->parsedYExpression->valid())
                error = "Y(t): " + inlineErrorMessage(*row->parsedYExpression, yRaw);
            else if (row->parsedExpression->dependsOnX() || row->parsedYExpression->dependsOnX())
                error = "Use t as the parameter; x is reserved for Cartesian graphing.";
        }
        const bool valid = error.isEmpty();
        for (auto* input : {row->input, row->yInput}) {
            input->setSyntaxState(valid ? "valid" : "invalid");
            input->setToolTip(error);
        }
        row->info->setText(valid ? "Parametric curve · drag to pan · scroll to zoom" : error);
        row->info->setStyleSheet(valid ? "color:#91baa4; font-size:11px;" :
                                       "color:#ff9299; font-size:11px;");
        return;
    }
    const QString normalized = normalizedGraphInput(row->input->text());
    const QString source = QString::fromStdString(
        MathExpressionInput::normalize(normalized.toStdString()));
    if (source.isEmpty()) {
        row->parsedSource.clear();
        row->parsedExpression.reset();
        row->input->setSyntaxState("empty");
        row->input->setToolTip(QString());
        row->info->setText("Enter an expression to plot.");
        row->info->setStyleSheet("color:#93a4b9; font-size:11px;");
        return;
    }
    if (!row->parsedExpression || row->parsedSource != source) {
        const VariableScope scope = variableSliders_ ?
            variableSliders_->definitions().scope() : VariableScope{};
        row->parsedExpression = std::make_shared<Expression>(source.toStdString(), scope, functionDefinitions_.scope());
        row->parsedSource = source;
    }
    if (row->parsedExpression->valid()) {
        row->input->setSyntaxState("valid");
        row->input->setToolTip(QString());
        row->info->setText("Valid expression · updating curve…");
        row->info->setStyleSheet("color:#91baa4; font-size:11px;");
    } else {
        const bool incomplete = isIncompleteEdit(*row->parsedExpression, source.toStdString());
        row->input->setSyntaxState(incomplete ? "incomplete" : "invalid");
        const QString message = incomplete
            ? "Incomplete expression — " + inlineErrorMessage(*row->parsedExpression, normalized)
            : inlineErrorMessage(*row->parsedExpression, normalized);
        row->info->setText(message);
        row->input->setToolTip(message);
        row->info->setStyleSheet(incomplete ? "color:#e4bd7a; font-size:11px;" :
                                            "color:#ff9299; font-size:11px;");
    }
}

void MainWindow::onVariableDefinitionsChanged(
    const std::vector<std::string>& changed, bool scopeChanged) {
    if (!variableSliders_ || !graph_) return;
    graph_->setVariableBindings(variableSliders_->definitions().bindings(), changed);
    if (scopeChanged) {
        const auto update = functionDefinitions_.setVariableScope(variableSliders_->definitions().scope());
        if (update.accepted && !update.recompiled.empty()) {
            // Preserve unrelated compiled graph expressions. Only expressions
            // referencing a recompiled function need a fresh function scope.
            for (auto* row : graphRows_) {
                if (!row->parsedExpression) continue;
                const auto affects = [&](const std::shared_ptr<const Expression>& expression) {
                    if (!expression) return false;
                    const auto& referenced = expression->referencedFunctions();
                    return std::any_of(referenced.begin(), referenced.end(), [&](const std::string& name) {
                        return std::find(update.recompiled.begin(), update.recompiled.end(), name) !=
                               update.recompiled.end();
                    });
                };
                if (affects(row->parsedExpression) || affects(row->parsedYExpression)) {
                    row->parsedExpression.reset();
                    row->parsedYExpression.reset();
                }
            }
        }
        rebuildGraph(changed);
    }
}

void MainWindow::rebuildGraph(const std::vector<std::string>& changedScope) {
    if (!graph_) return;
    const VariableScope scope = variableSliders_
        ? variableSliders_->definitions().scope() : VariableScope{};
    std::vector<GraphCurve> curves;
    curves.reserve(graphRows_.size());
    for (auto* row : graphRows_) {
        if (row->polar) {
            const auto sources = polarCoordinateSources(row->input->text().toStdString());
            if (sources.valid() && validParameterInterval(row->tMinimum->value(), row->tMaximum->value())) {
                VariableScope local = scope;
                if (!local.contains("t")) (void)local.declare("t");
                const auto nx = QString::fromStdString(sources.x);
                const auto ny = QString::fromStdString(sources.y);
                bool refresh = !row->parsedExpression || !row->parsedYExpression ||
                    row->parsedSource != nx || row->parsedYSource != ny ||
                    row->compiledMinimum != row->tMinimum->value() ||
                    row->compiledMaximum != row->tMaximum->value();
                if (!changedScope.empty()) {
                    for (const auto& name : changedScope) {
                        for (const auto& expression : {row->parsedExpression, row->parsedYExpression}) {
                            if (!expression || !expression->valid() ||
                                std::find(expression->referencedVariables().begin(),
                                          expression->referencedVariables().end(), name) !=
                                    expression->referencedVariables().end()) refresh = true;
                        }
                    }
                }
                if (refresh) {
                    row->parsedExpression = std::make_shared<Expression>(sources.x, local, functionDefinitions_.scope());
                    row->parsedYExpression = std::make_shared<Expression>(sources.y, local, functionDefinitions_.scope());
                    row->parsedSource = nx;
                    row->parsedYSource = ny;
                    row->compiledMinimum = row->tMinimum->value();
                    row->compiledMaximum = row->tMaximum->value();
                }
                if (row->parsedExpression->valid() && row->parsedYExpression->valid() &&
                    !row->parsedExpression->dependsOnX() && !row->parsedYExpression->dependsOnX()) {
                    GraphCurve curve{row->color, row->parsedExpression, row->visible->isChecked(),
                                     row->input->text(), row->parsedYExpression,
                                     row->tMinimum->value(), row->tMaximum->value()};
                    curve.polarCurve = true;
                    curves.push_back(std::move(curve));
                }
            }
            updateGraphEditorFeedback(row);
            continue;
        }
        if (row->parametric) {
            const auto xRaw = parametricBody(row->input->text(), 'x');
            const auto yRaw = parametricBody(row->yInput->text(), 'y');
            if (!xRaw.isEmpty() && !yRaw.isEmpty() &&
                validParameterInterval(row->tMinimum->value(), row->tMaximum->value())) {
                VariableScope pScope = scope;
                if (!pScope.contains("t")) (void)pScope.declare("t");
                const QString nx = QString::fromStdString(MathExpressionInput::normalize(xRaw.toStdString()));
                const QString ny = QString::fromStdString(MathExpressionInput::normalize(yRaw.toStdString()));
                bool refresh = !row->parsedExpression || !row->parsedYExpression ||
                    row->parsedSource != nx || row->parsedYSource != ny ||
                    row->compiledMinimum != row->tMinimum->value() ||
                    row->compiledMaximum != row->tMaximum->value();
                if (!changedScope.empty()) {
                    for (const auto& name : changedScope) {
                        for (const auto& expression : {row->parsedExpression, row->parsedYExpression}) {
                            if (!expression || !expression->valid() ||
                                std::find(expression->referencedVariables().begin(),
                                          expression->referencedVariables().end(), name) !=
                                    expression->referencedVariables().end()) refresh = true;
                        }
                    }
                }
                if (refresh) {
                    row->parsedExpression = std::make_shared<Expression>(nx.toStdString(), pScope, functionDefinitions_.scope());
                    row->parsedYExpression = std::make_shared<Expression>(ny.toStdString(), pScope, functionDefinitions_.scope());
                    row->parsedSource = nx;
                    row->parsedYSource = ny;
                    row->compiledMinimum = row->tMinimum->value();
                    row->compiledMaximum = row->tMaximum->value();
                }
                if (row->parsedExpression->valid() && row->parsedYExpression->valid() &&
                    !row->parsedExpression->dependsOnX() && !row->parsedYExpression->dependsOnX()) {
                    curves.push_back({row->color, row->parsedExpression, row->visible->isChecked(),
                        row->input->text() + "; " + row->yInput->text(),
                        row->parsedYExpression, row->tMinimum->value(), row->tMaximum->value()});
                    updateGraphEditorFeedback(row);
                    continue;
                }
            }
            updateGraphEditorFeedback(row);
            continue;
        }
        const QString raw = row->input->text().trimmed();
        const QString source = QString::fromStdString(
            MathExpressionInput::normalize(normalizedGraphInput(raw).toStdString()));
        if (source.isEmpty()) {
            // Emptying a row drops the old compiled scope. Restoring identical
            // text after a declaration was removed must not revive a stale AST.
            row->parsedExpression.reset();
            row->parsedSource.clear();
            row->input->setSyntaxState("empty");
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
            row->parsedExpression = std::make_shared<Expression>(source.toStdString(), scope, functionDefinitions_.scope());
            row->parsedSource = source;
        }
        const auto& function = row->parsedExpression;
        if (!function->valid()) {
            const bool incomplete = isIncompleteEdit(*function, source.toStdString());
            row->input->setSyntaxState(incomplete ? "incomplete" : "invalid");
            const QString message = incomplete
                ? "Incomplete expression — " + inlineErrorMessage(*function, normalizedGraphInput(raw))
                : inlineErrorMessage(*function, normalizedGraphInput(raw));
            row->info->setText(message);
            row->input->setToolTip(message);
            row->info->setStyleSheet(incomplete ? "color:#e4bd7a; font-size:11px;" :
                                                "color:#ff9299; font-size:11px;");
            continue;
        }
        row->input->setSyntaxState("valid");
        row->input->setToolTip(QString());
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

bool MainWindow::defineWorkspaceFunction(const QString& statement, QString* error) {
    if (!variableSliders_) {
        if (error) *error = "Graph workspace is not initialized.";
        return false;
    }
    auto staged = functionDefinitions_;
    const auto result = staged.define(statement.toStdString());
    if (!result.accepted) {
        if (error) *error = QString::fromStdString(result.error);
        return false;
    }
    functionDefinitions_ = std::move(staged);
    for (auto* row : graphRows_) {
        row->parsedExpression.reset();
        row->parsedYExpression.reset();
    }
    rebuildGraph();
    if (error) error->clear();
    return true;
}

bool MainWindow::saveWorkspace(const QString& path, QString* error) const {
    if (!variableSliders_ || !graph_) {
        if (error) *error = "Graph workspace is not initialized.";
        return false;
    }
    GraphWorkspaceState state;
    state.centerX = graph_->centerX();
    state.centerY = graph_->centerY();
    state.pixelsPerUnit = graph_->pixelsPerUnit();
    state.angle = angleMode_;
    for (const auto* row : graphRows_) {
        WorkspaceCurve saved{row->input->text(), row->visible->isChecked(), row->color};
        if (row->polar) {
            saved.polar = true;
            saved.tMinimum = row->tMinimum->value();
            saved.tMaximum = row->tMaximum->value();
        } else if (row->parametric) {
            saved.parametric = true;
            saved.yExpression = row->yInput->text();
            saved.tMinimum = row->tMinimum->value();
            saved.tMaximum = row->tMaximum->value();
        }
        state.curves.push_back(std::move(saved));
    }
    state.sliders = variableSliders_->snapshotSliders();
    state.assignments = GraphWorkspacePersistence::orderedAssignments(
        variableSliders_->definitions(), state.sliders);
    state.functions = GraphWorkspacePersistence::orderedFunctions(functionDefinitions_);
    const QByteArray bytes = GraphWorkspacePersistence::encode(state);
    if (bytes.size() > GraphWorkspacePersistence::MaxFileBytes) {
        if (error) *error = "Workspace exceeds maximum file size.";
        return false;
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() ||
        !file.commit()) {
        file.cancelWriting();
        if (error) *error = "Unable to atomically save workspace.";
        return false;
    }
    if (error) error->clear();
    return true;
}

bool MainWindow::loadWorkspace(const QString& path, QString* error) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.size() > GraphWorkspacePersistence::MaxFileBytes) {
        if (error) *error = "Cannot read workspace or file is too large.";
        return false;
    }
    GraphWorkspaceState staged;
    if (!GraphWorkspacePersistence::decode(file.readAll(), staged, error)) return false;
    GraphViewport checkedView;
    if (!checkedView.setDimensions(graph_->width(), graph_->height()) ||
        !checkedView.setView(staged.centerX, staged.centerY, staged.pixelsPerUnit)) {
        if (error) *error = "Workspace viewport cannot be represented at the current graph size.";
        return false;
    }
    // All scalar/function dependencies and settings have been validated in
    // scratch owners before touching any existing UI state.
    functionDefinitions_ = std::move(staged.validatedFunctions);
    std::string panelError;
    if (!variableSliders_->replaceWorkspace(staged.sliders,
                                            staged.validatedVariables, &panelError)) {
        if (error) *error = "Could not restore slider controls: " + QString::fromStdString(panelError);
        return false;
    }
    if (graphDebounce_) graphDebounce_->stop();
    for (auto* row : graphRows_) { delete row->widget; delete row; }
    graphRows_.clear();
    activeGraphInput_ = nullptr;
    for (const auto& row : staged.curves) {
        if (row.polar) addPolarRow(row.expression, row.tMinimum, row.tMaximum,
            row.visible, row.color);
        else if (row.parametric) addParametricRow(row.expression, row.yExpression,
            row.tMinimum, row.tMaximum, row.visible, row.color);
        else addGraphRow(row.expression, row.visible, row.color);
    }
    graph_->setView(staged.centerX, staged.centerY, staged.pixelsPerUnit);
    setAngleMode(staged.angle);
    rebuildGraph();
    if (error) error->clear();
    return true;
}

void MainWindow::saveGraph() {
    const QString path = QFileDialog::getSaveFileName(this, "Save graph workspace", "graph.json", "Graph JSON (*.json)");
    if (path.isEmpty()) return;
    QString error;
    if (!saveWorkspace(path, &error)) QMessageBox::warning(this, "Save failed", error);
}

void MainWindow::loadGraph() {
    const QString path = QFileDialog::getOpenFileName(this, "Open graph workspace", QString(), "Graph JSON (*.json)");
    if (path.isEmpty()) return;
    QString error;
    if (!loadWorkspace(path, &error)) QMessageBox::warning(this, "Open failed", error);
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
