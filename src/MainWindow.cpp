#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "Messages.h"
#include "Weight.h"
#include "Layout.h"
#include "Theme.h"

#include <QAbstractButton>
#include <QComboBox>
#include <QAbstractItemView>
#include <QEvent>
#include <QGuiApplication>
#include <QScreen>
#include <QShortcut>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QMenu>
#include <QRegularExpressionValidator>
#include <QCheckBox>
#include <QSignalBlocker>
#include <QStyle>

namespace {
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
    ui->txtNodeName->setMaxLength(1);
    ui->txtNodeName->setValidator(new QRegularExpressionValidator(
            QRegularExpression(QStringLiteral("[A-Za-z]")), ui->txtNodeName));
    // Solo signo, dígitos y un separador decimal (coma o punto).
    ui->txtWeight->setValidator(new QRegularExpressionValidator(
            QRegularExpression(QStringLiteral(R"([+-]?\d{0,10}([.,]\d{0,6})?)")), ui->txtWeight));

    applyTheme();
    fitToScreen();

    graph.setDirected(ui->chkDirected->isChecked());
    ui->graphView->setGraph(&graph);

    const Msg::Message hint = Msg::initialHint();
    ui->resultBanner->setMessage(ResultBanner::Info, hint.title, hint.text);

    setupConnections();
    updateControls();
}

MainWindow::~MainWindow() { delete ui; }

// --------------------- Apariencia ---------------------
void MainWindow::applyTheme() {
    if (applyingTheme) return;
    applyingTheme = true;

    ui->lblTitle->setFont(Theme::title());
    ui->lblSubtitle->setFont(Theme::caption());
    ui->lblSubtitle->setProperty("role", "secondary");
    for (QLabel *l : {ui->lblResultCaption, ui->lblSectionBuild, ui->lblSectionEdge, ui->lblSectionCalc})
        l->setFont(Theme::headline());
    for (QLabel *l : {ui->lblEdgeFrom, ui->lblEdgeTo, ui->lblWeight, ui->lblOrigin, ui->lblDestination}) {
        l->setFont(Theme::caption());
        l->setProperty("role", "secondary");
    }
    for (QLabel *l : {ui->lblNodeError, ui->lblWeightError}) {
        l->setFont(Theme::caption());
        l->setProperty("role", "error");
    }
    ui->lblEdgeInfo->setFont(Theme::caption());
    ui->lblEdgeInfo->setProperty("role", "secondary");
    ui->lblAlgoWarning->setFont(Theme::caption());
    ui->lblAlgoWarning->setProperty("role", "warning");
    for (QFrame *f : {ui->sepBuild, ui->sepEdge, ui->sepFooter}) f->setProperty("role", "separator");

    ui->btnCalculate->setProperty("role", "primary");
    ui->btnClearAll->setProperty("role", "destructive");

    const QString sheet = Theme::styleSheet();
    setStyleSheet(sheet);
    for (QComboBox *cb : findChildren<QComboBox *>())
        if (cb->view()) cb->view()->setStyleSheet(sheet);
    ui->resultBanner->refreshStyle();
    ui->graphView->update();

    applyingTheme = false;
}

void MainWindow::changeEvent(QEvent *event) {
    QMainWindow::changeEvent(event);
    // Seguir la apariencia clara/oscura del sistema si cambia con la app abierta.
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::ApplicationPaletteChange)
        applyTheme();
}

void MainWindow::fitToScreen() {
    QScreen *screen = this->screen() ? this->screen() : QGuiApplication::primaryScreen();
    if (!screen) return;
    const QRect avail = screen->availableGeometry();
    const QSize size = QSize(1180, 780).boundedTo(avail.size() - QSize(40, 40));
    resize(size);
    move(avail.center() - QPoint(size.width() / 2, size.height() / 2));
}

void MainWindow::setupConnections() {
    connect(ui->btnAddNode, &QPushButton::clicked, this, &MainWindow::onAddNode);
    connect(ui->btnRemoveLast, &QPushButton::clicked, this, &MainWindow::onRemoveLast);
    connect(ui->btnClearAll, &QPushButton::clicked, this, &MainWindow::onClearAll);
    connect(ui->btnAddEdge, &QPushButton::clicked, this, &MainWindow::onAddEdge);
    connect(ui->btnRemoveEdge, &QPushButton::clicked, this, &MainWindow::onRemoveEdge);
    connect(ui->btnCalculate, &QPushButton::clicked, this, &MainWindow::onCalculate);
    connect(ui->btnUndo, &QPushButton::clicked, this, &MainWindow::onUndo);
    auto *undoShortcut = new QShortcut(QKeySequence::Undo, this);
    connect(undoShortcut, &QShortcut::activated, this, &MainWindow::onUndo);
    connect(ui->chkDirected, &QCheckBox::toggled, this, &MainWindow::onDirectedToggled);
    connect(ui->graphView, &GraphWidget::canvasClicked, this, &MainWindow::onCanvasClicked);
    connect(ui->graphView, &GraphWidget::placementBlocked, this, &MainWindow::onPlacementBlocked);
    connect(ui->graphView, &GraphWidget::nodeDragStarted, this, &MainWindow::onNodeDragStarted);
    connect(ui->graphView, &GraphWidget::nodeDragged, this, &MainWindow::onNodeDragged);
    connect(ui->graphView, &GraphWidget::nodeMenuRequested, this, &MainWindow::onNodeMenuRequested);

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
// Primera letra libre del caso marcado; si ya se usaron todas, del otro caso.
QString MainWindow::nextSuggestedName() const {
    const QString upper = QStringLiteral("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    const QString lower = upper.toLower();
    const QString alphabet = ui->chkUppercase->isChecked() ? upper + lower : lower + upper;
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
        // Hay tantas letras como MaxNodes: por debajo del límite siempre queda una libre.
        Q_ASSERT(!nextSuggestedName().isEmpty());
        return {};
    }
    const QChar c = typed.at(0);
    const bool asciiLetter = (c >= u'A' && c <= u'Z') || (c >= u'a' && c <= u'z');
    if (typed.length() != 1 || !asciiLetter) return Msg::nodeNameInvalid();
    if (graph.indexOf(typed) >= 0) {
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
    pushUndo(Msg::undoAddNode(name));
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
    removeNode(graph.nodeCount() - 1);
}

void MainWindow::removeNode(int index) {
    pushUndo(Msg::undoRemoveNode(graph.node(index).name));
    const bool wasAuto = graph.node(index).autoPlaced;
    graph.removeNode(index);
    if (wasAuto) relayoutAutoPlaced();
    refreshNodeSelectors();
    graphChanged();
}

void MainWindow::onNodeMenuRequested(int index, const QPoint &globalPos) {
    const QString name = graph.node(index).name;
    QMenu menu(this);
    QAction *remove = menu.addAction(style()->standardIcon(QStyle::SP_TrashIcon), Msg::removeNodeAction(name));
    if (menu.exec(globalPos) != remove) return;

    int edges = 0;
    for (const auto &e : graph.edgeList())
        if (e.from == index || e.to == index) ++edges;
    if (edges > 0 && !confirmDestructive(this, Msg::confirmRemoveNode(name, edges),
                                         Msg::confirmRemoveNodeAccept()))
        return;
    removeNode(index);
}

void MainWindow::onNodeDragStarted(int index) {
    pushUndo(Msg::undoMoveNode(graph.node(index).name));
}

void MainWindow::onNodeDragged(int index, const QPointF &normalizedPos) {
    // Mover un nodo no cambia distancias: no invalida el resultado.
    graph.setNodePos(index, normalizedPos);
    graph.setNodeAutoPlaced(index, false); // el usuario decidió dónde va: queda fijo
    ui->graphView->update();
}

void MainWindow::onClearAll() {
    if (graph.nodeCount() == 0) return;
    if (!confirmDestructive(this, Msg::confirmClearAll(graph.nodeCount(), graph.edgeList().size()),
                            Msg::confirmClearAllAccept()))
        return;
    pushUndo(Msg::undoClearAll());
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

    const QString label = Msg::edge(graph, {from, to, *w.value});
    const std::optional<double> previous = graph.edge(from, to);
    pushUndo(previous ? Msg::undoChangeWeight(label) : Msg::undoAddEdge(label));
    graph.setEdge(from, to, *w.value);
    graphChanged();
    if (previous) {
        const Msg::Message m = Msg::edgeUpdated(label, *previous, *w.value);
        ui->resultBanner->setMessage(ResultBanner::Info, m.title, m.text);
        resultShown = true; // el próximo cambio reemplaza este aviso
    }
}

void MainWindow::onRemoveEdge() {
    const int from = ui->cbEdgeFrom->currentIndex();
    const int to = ui->cbEdgeTo->currentIndex();
    if (from < 0 || to < 0 || !graph.hasEdge(from, to)) return; // el botón ya estaba desactivado
    pushUndo(Msg::undoRemoveEdge(Msg::edge(graph, {from, to, 0.0})));
    graph.removeEdge(from, to);
    graphChanged();
}

void MainWindow::onDirectedToggled(bool checked) {
    const Graph before = graph;
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
    pushUndo(Msg::undoSetDirected(checked), before);
    graphChanged();
}

// --------------------- Deshacer ---------------------
void MainWindow::pushUndo(const QString &action, const std::optional<Graph> &snapshot) {
    undoStack.append({snapshot ? *snapshot : graph, action});
    if (undoStack.size() > MaxUndo) undoStack.removeFirst();
}

void MainWindow::onUndo() {
    if (undoStack.isEmpty()) return;
    const UndoEntry entry = undoStack.takeLast();
    graph = entry.graph;
    {
        QSignalBlocker block(ui->chkDirected);
        ui->chkDirected->setChecked(graph.isDirected());
    }
    refreshNodeSelectors();
    graphChanged();
    const Msg::Message m = Msg::undone(entry.action, undoStack.isEmpty() ? QString() : undoStack.last().action);
    ui->resultBanner->setMessage(ResultBanner::Info, m.title, m.text);
    resultShown = true; // el próximo cambio reemplaza este aviso
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
    setButtonEnabled(ui->btnUndo, !undoStack.isEmpty(), Msg::nothingToUndo());
    if (!undoStack.isEmpty()) ui->btnUndo->setToolTip(Msg::undoTooltip(undoStack.last().action));

    // Aristas: si ya existe, «Agregar» pasa a «Cambiar peso» y se puede quitar.
    const int from = ui->cbEdgeFrom->currentIndex();
    const int to = ui->cbEdgeTo->currentIndex();
    const bool edgeExists = from >= 0 && to >= 0 && graph.hasEdge(from, to);
    ui->btnAddEdge->setText(Msg::addEdgeText(edgeExists));
    if (!hasNodes)
        setButtonEnabled(ui->btnRemoveEdge, false, Msg::needsNodesForEdge());
    else
        setButtonEnabled(ui->btnRemoveEdge, edgeExists,
                         Msg::edgeMissing(ui->cbEdgeFrom->currentText(), ui->cbEdgeTo->currentText()));

    const WeightCheck w = checkWeight();
    setFieldError(ui->txtWeight, ui->lblWeightError, hasNodes ? w.error : QString());

    // Si la arista ya existe se informa su peso; con el mismo peso no hay nada que cambiar.
    QString edgeInfo;
    QString sameWeight;
    if (edgeExists && w.error.isEmpty()) {
        const double current = *graph.edge(from, to);
        const QString label = Msg::edge(graph, {from, to, current});
        if (w.value && *w.value == current)
            sameWeight = edgeInfo = Msg::edgeSameWeight(label, current);
        else
            edgeInfo = Msg::edgeExists(label, current);
    }
    ui->lblEdgeInfo->setText(edgeInfo);
    ui->lblEdgeInfo->setVisible(!edgeInfo.isEmpty());

    if (!hasNodes)
        setButtonEnabled(ui->btnAddEdge, false, Msg::needsNodesForEdge());
    else if (!sameWeight.isEmpty())
        setButtonEnabled(ui->btnAddEdge, false, sameWeight);
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

    fitWrappedLabels();
}

// Qt no propaga bien la altura de las etiquetas con salto de línea dentro de
// los grupos del panel: se fija a mano para que ningún mensaje quede cortado.
void MainWindow::fitWrappedLabels() {
    for (QLabel *label : ui->panelSide->findChildren<QLabel *>()) {
        if (!label->wordWrap() || label->isHidden()) continue;
        const QLayout *layout = label->parentWidget()->layout();
        const int width = layout ? layout->contentsRect().width() : label->width();
        if (width <= 0) continue;
        const int m = label->contentsMargins().left() + label->contentsMargins().right();
        label->setMinimumHeight(label->heightForWidth(width) > 0
                                    ? label->heightForWidth(width)
                                    : label->fontMetrics().boundingRect(QRect(0, 0, width - m, 10000),
                                                                        Qt::TextWordWrap, label->text()).height());
    }
}

void MainWindow::showEvent(QShowEvent *event) {
    QMainWindow::showEvent(event);
    fitWrappedLabels();
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
