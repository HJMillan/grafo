#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "Messages.h"
#include "Weight.h"
#include "Layout.h"

#include <QAbstractButton>
#include <QComboBox>
#include <QGraphicsDropShadowEffect>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpressionValidator>
#include <QCheckBox>
#include <QSignalBlocker>
#include <QStyle>

namespace {
const char *kWindowStyle =
        "QLineEdit[error=\"true\"] { border: 2px solid #d64545; border-radius: 3px; padding: 1px; }"
        "QLabel#lblNodeError, QLabel#lblWeightError { color: #e04848; }"
        "QLabel#lblAlgoWarning { background: rgba(209, 139, 0, 0.14);"
        "  border-left: 4px solid #d18b00; border-radius: 3px; padding: 6px; }"
        "QLabel#lblNodeHint { color: palette(placeholder-text); }";

// Pide confirmación para una acción que borra datos. «Cancelar» es la opción por defecto.
bool confirmDestructive(QWidget *parent, const Msg::Message &m, const QString &acceptText) {
    QMessageBox box(QMessageBox::Warning, m.title, m.title, QMessageBox::NoButton, parent);
    box.setInformativeText(m.text);
    QPushButton *accept = box.addButton(acceptText, QMessageBox::DestructiveRole);
    QPushButton *cancel = box.addButton(QStringLiteral("Cancelar"), QMessageBox::RejectRole);
    box.setDefaultButton(cancel);
    box.setEscapeButton(cancel);
    box.exec();
    return box.clickedButton() == accept;
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    setStyleSheet(kWindowStyle);
    ui->txtNodeName->setMaxLength(1);
    // Solo signo, dígitos y un separador decimal (coma o punto).
    ui->txtWeight->setValidator(new QRegularExpressionValidator(
            QRegularExpression(QStringLiteral(R"([+-]?\d{0,10}([.,]\d{0,6})?)")), ui->txtWeight));

    // Sombra cian y efecto presionado para botones
    auto stylizeButton = [](QAbstractButton *btn) {
        auto *shadow = new QGraphicsDropShadowEffect(btn);
        shadow->setBlurRadius(0);
        shadow->setOffset(2, 2);
        shadow->setColor(QColor("#00B2A9"));
        btn->setGraphicsEffect(shadow);
        btn->setStyleSheet(
                "QPushButton { color: white; }"
                "QPushButton:hover { color: black; }"
                "QPushButton:pressed { color: black; }"
                "QPushButton:disabled { color: #7a7f87; }"
        );
        QObject::connect(btn, &QAbstractButton::pressed, btn, [shadow]() {
            shadow->setOffset(0, 0);
        });
        QObject::connect(btn, &QAbstractButton::released, btn, [shadow]() {
            shadow->setOffset(2, 2);
        });
    };
    for (QAbstractButton *b : QList<QAbstractButton *>{ui->btnCalculate, ui->btnAddNode, ui->btnRemoveLast,
                                                       ui->btnClearAll, ui->btnAddEdge})
        stylizeButton(b);

    graph.setDirected(ui->chkDirected->isChecked());
    ui->graphView->setGraph(&graph);

    const Msg::Message hint = Msg::initialHint();
    ui->resultBanner->setMessage(ResultBanner::Info, hint.title, hint.text);

    setupConnections();
    updateControls();
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::setupConnections() {
    connect(ui->btnAddNode, &QPushButton::clicked, this, &MainWindow::onAddNode);
    connect(ui->btnRemoveLast, &QPushButton::clicked, this, &MainWindow::onRemoveLast);
    connect(ui->btnClearAll, &QPushButton::clicked, this, &MainWindow::onClearAll);
    connect(ui->btnAddEdge, &QPushButton::clicked, this, &MainWindow::onAddEdge);
    connect(ui->btnCalculate, &QPushButton::clicked, this, &MainWindow::onCalculate);
    connect(ui->chkDirected, &QCheckBox::toggled, this, &MainWindow::onDirectedToggled);
    connect(ui->graphView, &GraphWidget::canvasClicked, this, &MainWindow::onCanvasClicked);
    connect(ui->graphView, &GraphWidget::placementBlocked, this, &MainWindow::onPlacementBlocked);

    // Validación mientras se escribe o cambia el contexto
    connect(ui->txtNodeName, &QLineEdit::textChanged, this, &MainWindow::updateControls);
    connect(ui->chkUppercase, &QCheckBox::toggled, this, &MainWindow::updateControls);
    connect(ui->txtWeight, &QLineEdit::textChanged, this, &MainWindow::updateControls);
    connect(ui->cbEdgeFrom, &QComboBox::currentIndexChanged, this, &MainWindow::updateControls);
    connect(ui->cbEdgeTo, &QComboBox::currentIndexChanged, this, &MainWindow::updateControls);
    for (QAbstractButton *rb : QList<QAbstractButton *>{ui->rbtnDijkstra, ui->rbtnBellman, ui->rbtnFloyd})
        connect(rb, &QAbstractButton::toggled, this, &MainWindow::updateControls);
}

// --------------------- Nodos ---------------------
QString MainWindow::nextSuggestedName() const {
    const QString alphabet = ui->chkUppercase->isChecked() ? "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                                           : "abcdefghijklmnopqrstuvwxyz";
    for (const QChar c : alphabet) {
        if (graph.indexOf(QString(c)) < 0) return QString(c);
    }
    return {};
}

// Devuelve el error que impide agregar un nodo con el nombre escrito
// (o con el automático, si el campo está vacío), o una cadena vacía.
QString MainWindow::nodeNameError(const QString &typed) const {
    if (graph.nodeCount() >= Graph::MaxNodes) return Msg::nodeLimit(Graph::MaxNodes);
    if (typed.isEmpty()) {
        return nextSuggestedName().isEmpty() ? Msg::nodeNoFreeName(ui->chkUppercase->isChecked())
                                             : QString();
    }
    if (typed.length() != 1 || !typed.at(0).isLetter()) return Msg::nodeNameInvalid();
    if (graph.indexOf(typed) >= 0) {
        const QChar c = typed.at(0);
        const QString swapped(c.isUpper() ? c.toLower() : c.toUpper());
        return Msg::nodeNameDuplicate(typed, graph.indexOf(swapped) < 0 ? swapped : QString());
    }
    return {};
}

bool MainWindow::addNode(const QPointF &pos, bool autoPlaced) {
    const QString typed = ui->txtNodeName->text().trimmed();
    if (!nodeNameError(typed).isEmpty()) {
        updateControls(); // el error ya está visible bajo el campo
        ui->txtNodeName->setFocus();
        return false;
    }
    const QString name = typed.isEmpty() ? nextSuggestedName() : typed;
    graph.addNode(name, pos, autoPlaced);
    if (autoPlaced) relayoutAutoPlaced();
    ui->txtNodeName->clear();
    refreshNodeSelectors();
    graphChanged();
    return true;
}

void MainWindow::refreshNodeSelectors() {
    const auto fillCombo = [&](QComboBox *cb) {
        const QString previous = cb->currentText();
        QSignalBlocker block(cb);
        cb->clear();
        for (const auto &name : graph.names()) cb->addItem(name);
        const int keep = cb->findText(previous);
        cb->setCurrentIndex(keep >= 0 ? keep : cb->count() - 1);
    };
    fillCombo(ui->cbEdgeFrom);
    fillCombo(ui->cbEdgeTo);
    fillCombo(ui->cbOrigin);
    fillCombo(ui->cbDestination);
}

void MainWindow::onCanvasClicked(const QPointF &normalizedPos) {
    addNode(normalizedPos, false);
}

void MainWindow::onPlacementBlocked() {
    ui->resultBanner->setMessage(ResultBanner::Warning, QStringLiteral("No se puede colocar el nodo"),
                                 Msg::nodeOverlap());
    resultShown = true;
}

void MainWindow::onAddNode() {
    addNode(QPointF(0.5, 0.5), true); // la posición final la decide relayoutAutoPlaced
}

void MainWindow::relayoutAutoPlaced() {
    layoutAutoPlaced(graph, ui->graphView->canvasGeometry(), GraphWidget::minNodeDistance);
}

void MainWindow::onRemoveLast() {
    if (graph.nodeCount() == 0) return;
    const bool wasAuto = graph.node(graph.nodeCount() - 1).autoPlaced;
    graph.removeNode(graph.nodeCount() - 1);
    if (wasAuto) relayoutAutoPlaced();
    refreshNodeSelectors();
    graphChanged();
}

void MainWindow::onClearAll() {
    if (graph.nodeCount() == 0) return;
    if (!confirmDestructive(this, Msg::confirmClearAll(graph.nodeCount(), graph.edgeList().size()),
                            Msg::confirmClearAllAccept()))
        return;
    graph.clear();
    refreshNodeSelectors();
    graphChanged();
    const Msg::Message hint = Msg::initialHint();
    ui->resultBanner->setMessage(ResultBanner::Info, hint.title, hint.text);
}

// --------------------- Aristas ---------------------
MainWindow::WeightCheck MainWindow::checkWeight() const {
    const QString text = ui->txtWeight->text().trimmed();
    const WeightParse p = parseWeight(text);
    switch (p.error) {
    case WeightParse::Empty:      return {std::nullopt, Msg::weightEmpty()};
    case WeightParse::NotNumber:  return {std::nullopt, Msg::weightNotNumber(text)};
    case WeightParse::NotFinite:  return {std::nullopt, Msg::weightNotFinite()};
    case WeightParse::OutOfRange: return {std::nullopt, Msg::weightOutOfRange(WeightParse::MaxAbs)};
    case WeightParse::None:       break;
    }

    if (p.value < 0) {
        // En no dirigido ningún algoritmo sirve, así que este aviso va primero.
        if (!graph.isDirected()) {
            return {std::nullopt, Msg::weightNegativeUndirected(ui->cbEdgeFrom->currentText(),
                                                                ui->cbEdgeTo->currentText(), p.value)};
        }
        if (ui->rbtnDijkstra->isChecked()) return {std::nullopt, Msg::weightNegativeDijkstra()};
    }
    return {p.value, {}};
}

void MainWindow::onAddEdge() {
    const int from = ui->cbEdgeFrom->currentIndex();
    const int to = ui->cbEdgeTo->currentIndex();
    const WeightCheck w = checkWeight();
    if (from < 0 || to < 0 || !w.value) return; // el botón ya estaba desactivado

    graph.setEdge(from, to, *w.value);
    graphChanged();
}

void MainWindow::onDirectedToggled(bool checked) {
    if (!checked) {
        // Pasar a no dirigido solo es posible si todas las aristas son simétricas
        // y no negativas; si no, hay que borrarlas o cancelar.
        const auto asymmetric = graph.asymmetricArcs();
        const auto negatives = graph.negativeEdges();
        if (!asymmetric.isEmpty() || !negatives.isEmpty()) {
            if (!confirmDestructive(this, Msg::confirmToUndirected(graph, asymmetric, negatives),
                                    Msg::confirmToUndirectedAccept())) {
                QSignalBlocker block(ui->chkDirected);
                ui->chkDirected->setChecked(true);
                return;
            }
            graph.clearEdges();
        }
    }
    graph.setDirected(checked);
    graphChanged();
}

// --------------------- Estado de la interfaz ---------------------
void MainWindow::graphChanged() {
    ui->graphView->highlightPath({});
    ui->graphView->update();
    if (resultShown) {
        const Msg::Message m = Msg::graphChanged();
        ui->resultBanner->setMessage(ResultBanner::Info, m.title, m.text);
        resultShown = false;
    }
    updateControls();
}

void MainWindow::updateControls() {
    const bool hasNodes = graph.nodeCount() > 0;

    // Nodos: solo se marca el campo si el usuario escribió algo; los errores
    // que no dependen del texto (límite, sin letras libres) se muestran igual.
    const QString typed = ui->txtNodeName->text().trimmed();
    const QString nameError = nodeNameError(typed);
    setFieldError(ui->txtNodeName, ui->lblNodeError, nameError);
    setButtonEnabled(ui->btnAddNode, nameError.isEmpty(), nameError);
    setButtonEnabled(ui->btnRemoveLast, hasNodes, QStringLiteral("No hay nodos que borrar."));
    setButtonEnabled(ui->btnClearAll, hasNodes, QStringLiteral("No hay nodos que borrar."));

    // Aristas
    const WeightCheck w = checkWeight();
    setFieldError(ui->txtWeight, ui->lblWeightError, hasNodes ? w.error : QString());
    if (!hasNodes)
        setButtonEnabled(ui->btnAddEdge, false, Msg::needsNodesForEdge());
    else
        setButtonEnabled(ui->btnAddEdge, w.value.has_value(), w.error);

    // Cálculo: Dijkstra queda bloqueado mientras existan aristas negativas.
    const auto negatives = graph.negativeEdges();
    const QString algoWarning = ui->rbtnDijkstra->isChecked() && !negatives.isEmpty()
            ? Msg::dijkstraWithNegatives(graph, negatives) : QString();
    ui->lblAlgoWarning->setText(algoWarning);
    ui->lblAlgoWarning->setVisible(!algoWarning.isEmpty());
    if (!hasNodes)
        setButtonEnabled(ui->btnCalculate, false, Msg::needsNodesForCalculate());
    else
        setButtonEnabled(ui->btnCalculate, algoWarning.isEmpty(), algoWarning);
}

void MainWindow::setFieldError(QLineEdit *field, QLabel *label, const QString &error) {
    const bool hasError = !error.isEmpty();
    if (field->property("error").toBool() != hasError) {
        field->setProperty("error", hasError);
        field->style()->unpolish(field);
        field->style()->polish(field);
    }
    label->setText(error);
    label->setVisible(hasError);
}

void MainWindow::setButtonEnabled(QAbstractButton *button, bool enabled, const QString &reason) {
    button->setEnabled(enabled);
    button->setToolTip(enabled ? QString() : reason);
    if (auto *effect = button->graphicsEffect()) effect->setEnabled(enabled);
}

// --------------------- Cálculo ---------------------
QString MainWindow::selectedAlgorithmName() const {
    if (ui->rbtnDijkstra->isChecked()) return ui->rbtnDijkstra->text();
    if (ui->rbtnBellman->isChecked()) return ui->rbtnBellman->text();
    return ui->rbtnFloyd->text();
}

void MainWindow::onCalculate() {
    const int src = ui->cbOrigin->currentIndex();
    const int dest = ui->cbDestination->currentIndex();
    if (src < 0 || dest < 0) return; // el botón ya estaba desactivado

    if (ui->rbtnDijkstra->isChecked()) {
        const auto negatives = graph.negativeEdges();
        if (!negatives.isEmpty()) {
            ui->resultBanner->setMessage(ResultBanner::Error, QStringLiteral("No se puede usar Dijkstra"),
                                         Msg::dijkstraWithNegatives(graph, negatives));
            resultShown = true;
            return;
        }
        showResult(dijkstra(graph, src, dest), src, dest);
    } else if (ui->rbtnBellman->isChecked()) {
        showResult(bellmanFord(graph, src, dest), src, dest);
    } else {
        showResult(floydWarshall(graph, src, dest), src, dest);
    }
}

void MainWindow::showResult(const PathResult &result, int src, int dest) {
    Msg::Message m;
    switch (result.status) {
    case PathResult::Ok:
        m = Msg::pathFound(graph, result.path, result.distance, selectedAlgorithmName());
        ui->resultBanner->setMessage(ResultBanner::Success, m.title, m.text);
        break;
    case PathResult::NoPath:
        m = Msg::noPath(graph, src, dest);
        ui->resultBanner->setMessage(ResultBanner::Info, m.title, m.text);
        break;
    case PathResult::NegativeCycle:
        m = Msg::negativeCycle(graph, src, dest, result.path, result.distance);
        ui->resultBanner->setMessage(ResultBanner::Error, m.title, m.text);
        break;
    }
    ui->graphView->highlightPath(result.path, result.status == PathResult::NegativeCycle
                                                      ? GraphWidget::Highlight::NegativeCycle
                                                      : GraphWidget::Highlight::Path);
    resultShown = true;
}
