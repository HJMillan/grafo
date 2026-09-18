#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "GraphConstants.h"
#include "GraphFormat.h"
#include "Theme.h"

#include <QApplication>
#include <QAbstractItemView>
#include <QComboBox>
#include <QEvent>
#include <QFrame>
#include <QGuiApplication>
#include <QKeySequence>
#include <QLabel>
#include <QRadioButton>
#include <QScreen>
#include <QShortcut>
#include <cmath>
#include <vector>

namespace {

QString joinPath(const QVector<QString> &names, const QVector<int> &path) {
    QString out;
    for (int i = 0; i < path.size(); ++i) {
        out += names[path[i]];
        if (i + 1 < path.size()) out += QStringLiteral(" → ");
    }
    return out;
}

QVector<int> toQVec(const std::vector<int> &v) {
    QVector<int> out;
    out.reserve(static_cast<int>(v.size()));
    for (int x : v) out.append(x);
    return out;
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    ui->txtNodeName->setMaxLength(1);

    initGraphWidget();
    applyTheme();

    uppercaseDefault = ui->chkUppercase->isChecked();
    model.setDirected(ui->chkDirected->isChecked());

    connect(graph, &GraphWidget::canvasClicked, this, &MainWindow::handleCanvasClick);
    connect(graph, &GraphWidget::nodeAboutToMove, this, &MainWindow::handleNodeAboutToMove);
    connect(graph, &GraphWidget::nodeMoved, this, &MainWindow::handleNodeMoved);
    connect(graph, &GraphWidget::positionsChanged, this, &MainWindow::handlePositionsChanged);
    connect(graph, &GraphWidget::nodesLinked, this, &MainWindow::handleNodesLinked);
    connect(ui->cbOrigin, &QComboBox::currentIndexChanged, this, &MainWindow::syncQueryEndpoints);
    connect(ui->cbDestination, &QComboBox::currentIndexChanged, this, &MainWindow::syncQueryEndpoints);

    auto *undoShortcut = new QShortcut(QKeySequence::Undo, this);
    connect(undoShortcut, &QShortcut::activated, this, &MainWindow::on_btnUndo_clicked);

    auto *escapeShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(escapeShortcut, &QShortcut::activated, this, [this]() {
        if (graph) graph->clearSelection();
    });

    refreshStatus();
    updateActionStates();
    fitToScreen();
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::changeEvent(QEvent *event) {
    QMainWindow::changeEvent(event);
    if (applyingTheme) return;
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::ApplicationPaletteChange) {
        applyTheme();
        if (graph) graph->update();
    }
}

void MainWindow::initGraphWidget() {
    graph = ui->frameGraph;
}

void MainWindow::fitToScreen() {
    // Única máquina objetivo: 1920×1080 @ 125% (~1536×864 lógicos, ~800 px útiles).
    const QSize target(1180, 780);

    QScreen *screen = this->screen();
    if (!screen) screen = QGuiApplication::primaryScreen();
    if (!screen) {
        resize(target);
        return;
    }

    const QRect avail = screen->availableGeometry();
    resize(target);
    move(avail.center() - QPoint(target.width() / 2, target.height() / 2));
}

void MainWindow::applyTheme() {
    if (applyingTheme) return;
    applyingTheme = true;

    ui->lblTitle->setFont(Theme::title());

    ui->lblSubtitle->setFont(Theme::caption());
    ui->lblSubtitle->setProperty("role", "secondary");

    ui->lblResultCaption->setFont(Theme::headline());
    ui->lblSectionBuild->setFont(Theme::headline());
    ui->lblSectionCalc->setFont(Theme::headline());
    ui->lblEdgeCaption->setFont(Theme::headline());
    ui->txtOutput->setFont(Theme::body());

    const QFont captionFont = Theme::caption();
    for (QLabel *l : {ui->lblEdgeFrom, ui->lblEdgeTo, ui->lblWeight, ui->lblOrigin, ui->lblDestination}) {
        l->setFont(captionFont);
        l->setProperty("role", "secondary");
    }

    const QFont radioFont = Theme::caption();
    for (auto *r : {ui->rbtnDijkstra, ui->rbtnBellman, ui->rbtnFloyd}) {
        r->setFont(radioFont);
    }

    for (QFrame *f : {ui->sepBuildB, ui->sepCalc, ui->sepFooter}) {
        f->setProperty("role", "separator");
    }

    ui->btnCalculate->setProperty("role", "primary");
    ui->btnClearAll->setProperty("role", "destructive");

    ui->btnUndo->setToolTip(QStringLiteral("Revierte la última modificación del grafo (%1).")
                                    .arg(QKeySequence(QKeySequence::Undo).toString(QKeySequence::NativeText)));

    const QString sheet = Theme::styleSheet();
    qApp->setStyleSheet(QString());
    setStyleSheet(sheet);
    for (QComboBox *cb : findChildren<QComboBox*>()) {
        if (cb->view()) cb->view()->setStyleSheet(sheet);
    }
    applyingTheme = false;
}

void MainWindow::updateActionStates() {
    const bool hasNodes = model.nodeCount() > 0;
    const bool hasEdges = !model.userEdges().isEmpty();
    const bool roomForMore = model.nodeCount() < GraphModel::MAX_NODES;

    ui->btnAddNode->setEnabled(roomForMore);
    ui->txtNodeName->setEnabled(roomForMore);
    ui->btnRemoveLast->setEnabled(hasNodes);
    ui->btnAddEdge->setEnabled(hasNodes);
    ui->btnRemoveEdge->setEnabled(hasEdges);
    ui->btnCalculate->setEnabled(hasEdges);
    ui->btnCalculate->setToolTip(hasEdges
            ? QStringLiteral("Calcula el camino entre origen y destino.")
            : QStringLiteral("Conecta al menos dos nodos para poder calcular."));
    ui->btnClearAll->setEnabled(hasNodes || hasEdges);
    ui->btnUndo->setEnabled(model.canUndo());
    updateGuidance();
}

void MainWindow::updateGuidance() {
    if (model.nodeCount() == 0) {
        ui->lblSubtitle->setText(QStringLiteral("Haz clic en el lienzo para crear un nodo."));
    } else {
        ui->lblSubtitle->setText(QStringLiteral("Anillo azul: unir. Relleno: origen/destino."));
    }
}

void MainWindow::showNotice(const QString &text) {
    ui->txtOutput->setPlainText(text);
}

void MainWindow::refreshStatus() {
    if (statusLocked) return;
    showNotice(graphSummary());
}

QString MainWindow::graphSummary() const {
    if (model.nodeCount() == 0) {
        if (model.canUndo()) {
            return QStringLiteral("El grafo está vacío.\nDeshacer para recuperar.");
        }
        return QStringLiteral("Haz clic en el lienzo para crear un nodo.");
    }

    const QString nodesCaption = QStringLiteral("%1/%2 %3")
            .arg(model.nodeCount())
            .arg(GraphModel::MAX_NODES)
            .arg(model.nodeCount() == 1 ? QStringLiteral("nodo") : QStringLiteral("nodos"));

    if (model.userEdges().isEmpty()) {
        return QStringLiteral("%1 · sin aristas.\nSelecciona un nodo y luego otro para unirlos.")
                .arg(nodesCaption);
    }
    return QStringLiteral("%1 · %2 %3.\nElige origen y destino, y pulsa Calcular.")
            .arg(nodesCaption)
            .arg(model.userEdges().size())
            .arg(model.userEdges().size() == 1 ? QStringLiteral("arista") : QStringLiteral("aristas"));
}

QString MainWindow::nextSuggestedName() const {
    return model.nextSuggestedName(uppercaseDefault);
}

QPoint MainWindow::suggestedPosition(int idx) const {
    if (!graph) return QPoint(50, 50);
    const QRect r = graph->rect();
    const QPoint center = r.center();
    const int radius = qMax(80, qMin(r.width(), r.height()) / 2 - 48);
    const int denom = qMax(1, model.nodeCount() + 1);
    constexpr double PI = 3.14159265358979323846;
    const double angle = 2.0 * PI * (idx % denom) / denom - PI / 2.0;
    const int x = center.x() + static_cast<int>(radius * std::cos(angle));
    const int y = center.y() + static_cast<int>(radius * std::sin(angle));
    return graph->placeWithoutOverlap(QPoint(x, y));
}

bool MainWindow::addNodeInternal(const QString &name, const QPoint &pos) {
    QString reason;
    if (!model.canAddNode(name, reason)) {
        showNotice(reason);
        return false;
    }

    pushUndo();
    model.addNode(name, pos);

    syncNodesFromModel();
    statusLocked = false;
    refreshNodeSelectors();
    rebuildEdgesForView();
    updateActionStates();
    refreshStatus();
    return true;
}

void MainWindow::removeLastNode() {
    if (model.nodeCount() == 0) return;

    pushUndo();
    model.removeLastNode();

    syncNodesFromModel();
    statusLocked = false;
    refreshNodeSelectors();
    rebuildEdgesForView();
    clearHighlights();
    updateActionStates();
    refreshStatus();
}

void MainWindow::clearGraph() {
    if (model.isEmpty()) return;

    pushUndo();
    model.clear();

    graph->clearAll();
    graph->clearSelection();
    statusLocked = false;
    refreshNodeSelectors();
    clearHighlights();
    updateActionStates();
    refreshStatus();
}

void MainWindow::refreshNodeSelectors() {
    const auto fillCombo = [&](QComboBox *cb, int fallbackIndex) {
        const QString previous = cb->currentText();
        cb->blockSignals(true);
        cb->clear();
        for (const auto &name : model.nodeNames()) cb->addItem(name);
        int restore = previous.isEmpty() ? -1 : cb->findText(previous);
        if (restore < 0) restore = fallbackIndex;
        if (restore >= cb->count()) restore = cb->count() - 1;
        if (restore >= 0) cb->setCurrentIndex(restore);
        cb->blockSignals(false);
    };

    const int last = model.nodeCount() == 0 ? -1 : model.nodeCount() - 1;
    const int prev = (last > 0) ? last - 1 : last;
    fillCombo(ui->cbEdgeFrom, prev);
    fillCombo(ui->cbEdgeTo, last);
    fillCombo(ui->cbOrigin, 0);
    fillCombo(ui->cbDestination, last);

    if (ui->cbEdgeFrom->count() > 1 && ui->cbEdgeFrom->currentIndex() == ui->cbEdgeTo->currentIndex()) {
        ui->cbEdgeTo->setCurrentIndex(last);
        if (ui->cbEdgeFrom->currentIndex() == ui->cbEdgeTo->currentIndex()) {
            ui->cbEdgeFrom->setCurrentIndex(0);
        }
    }
    if (ui->cbOrigin->count() > 1 && ui->cbOrigin->currentIndex() == ui->cbDestination->currentIndex()) {
        ui->cbDestination->setCurrentIndex(last);
        if (ui->cbOrigin->currentIndex() == ui->cbDestination->currentIndex()) {
            ui->cbOrigin->setCurrentIndex(0);
        }
    }

    syncQueryEndpoints();
}

void MainWindow::syncQueryEndpoints() {
    if (!graph) return;
    graph->setQueryEndpoints(ui->cbOrigin->currentIndex(), ui->cbDestination->currentIndex());
}

void MainWindow::rebuildEdgesForView() {
    graph->setAdjacency(model.adjacency(), model.nodeCount(), GraphModel::INF, model.isDirected());
}

void MainWindow::syncNodesFromModel() {
    graph->setNodes(model.nodeNames(), model.positions());
}

bool MainWindow::parseWeight(double &w) const {
    bool ok = false;
    QString text = ui->txtWeight->text().trimmed();
    text.replace(',', '.');
    w = text.toDouble(&ok);
    return ok;
}

void MainWindow::addEdge(int from, int to, double weight) {
    if (from < 0 || to < 0 || from >= model.nodeCount() || to >= model.nodeCount()) return;

    const int existing = model.findUserEdge(from, to);
    const QString pair = model.edgePairLabel(from, to);

    if (existing >= 0 && GraphConstants::sameWeight(model.userEdges()[existing].weight, weight)) {
        showNotice(QStringLiteral("Arista %1 ya existe (peso %2).")
                           .arg(pair, formatWeight(weight)));
        return;
    }

    pushUndo();
    double oldWeight = 0.0;
    const bool updated = model.addEdge(from, to, weight, &oldWeight);

    rebuildEdgesForView();
    clearHighlights();
    statusLocked = false;
    updateActionStates();

    if (updated) {
        showNotice(QStringLiteral("Arista %1 actualizada: %2 → %3.")
                           .arg(pair, formatWeight(oldWeight), formatWeight(weight)));
    } else if (ui->rbtnDijkstra->isChecked() && weight < 0) {
        showNotice(QStringLiteral(
                "Arista agregada con peso negativo. Dijkstra no puede usarse con pesos "
                "negativos: elige Bellman-Ford o Floyd-Warshall para calcular."));
    } else {
        refreshStatus();
    }
}

void MainWindow::removeEdge(int from, int to) {
    if (model.findUserEdge(from, to) < 0) {
        showNotice(QStringLiteral("No hay una arista entre los nodos seleccionados."));
        return;
    }

    pushUndo();
    model.removeEdge(from, to);

    rebuildEdgesForView();
    clearHighlights();
    statusLocked = false;
    updateActionStates();
    refreshStatus();
}

void MainWindow::clearHighlights() {
    graph->highlightPath({});
}

void MainWindow::pushUndo() {
    model.pushUndo();
}

void MainWindow::applySnapshot(const GraphSnapshot &s) {
    model.apply(s);

    ui->chkDirected->blockSignals(true);
    ui->chkDirected->setChecked(model.isDirected());
    ui->chkDirected->blockSignals(false);

    graph->restoreNodes(model.nodeNames(), model.positions());
    graph->clearSelection();
    refreshNodeSelectors();
    rebuildEdgesForView();
    clearHighlights();
    statusLocked = false;
    updateActionStates();
    refreshStatus();
}

void MainWindow::handleCanvasClick(const QPoint &p) {
    if (model.nodeCount() >= GraphModel::MAX_NODES) {
        showNotice(QStringLiteral("Límite de 26 nodos alcanzado."));
        return;
    }

    QString name = ui->txtNodeName->text().trimmed();
    uppercaseDefault = ui->chkUppercase->isChecked();
    if (name.isEmpty()) name = nextSuggestedName();
    if (addNodeInternal(name, graph->placeWithoutOverlap(p))) {
        ui->txtNodeName->clear();
    }
}

void MainWindow::handleNodeAboutToMove() {
    pushUndo();
    updateActionStates();
}

void MainWindow::handleNodeMoved() {
    model.setPositions(graph->nodePositions());
}

void MainWindow::handlePositionsChanged() {
    model.setPositions(graph->nodePositions());
}

void MainWindow::handleNodesLinked(int from, int to) {
    double w = 0.0;
    if (!parseWeight(w)) {
        showNotice(QStringLiteral("Ingresa un peso numérico. Puedes usar punto o coma decimal."));
        if (from >= 0 && from < ui->cbEdgeFrom->count()) ui->cbEdgeFrom->setCurrentIndex(from);
        if (to >= 0 && to < ui->cbEdgeTo->count()) ui->cbEdgeTo->setCurrentIndex(to);
        return;
    }
    addEdge(from, to, w);
    if (from >= 0 && from < ui->cbEdgeFrom->count()) ui->cbEdgeFrom->setCurrentIndex(from);
    if (to >= 0 && to < ui->cbEdgeTo->count()) ui->cbEdgeTo->setCurrentIndex(to);
}

void MainWindow::on_btnAddNode_clicked() {
    QString name = ui->txtNodeName->text().trimmed();
    uppercaseDefault = ui->chkUppercase->isChecked();
    if (name.isEmpty()) name = nextSuggestedName();

    const int idx = model.nodeCount();
    const QPoint pos = suggestedPosition(idx);
    if (addNodeInternal(name, pos)) {
        ui->txtNodeName->clear();
    }
}

void MainWindow::on_btnRemoveLast_clicked() { removeLastNode(); }

void MainWindow::on_btnClearAll_clicked() { clearGraph(); }

void MainWindow::on_btnUndo_clicked() {
    GraphSnapshot s;
    if (!model.popUndo(s)) return;
    applySnapshot(s);
}

void MainWindow::on_btnAddEdge_clicked() {
    const int from = ui->cbEdgeFrom->currentIndex();
    const int to = ui->cbEdgeTo->currentIndex();

    double w = 0.0;
    if (!parseWeight(w)) {
        showNotice(QStringLiteral("Ingresa un peso numérico. Puedes usar punto o coma decimal."));
        return;
    }

    addEdge(from, to, w);
}

void MainWindow::on_btnRemoveEdge_clicked() {
    removeEdge(ui->cbEdgeFrom->currentIndex(), ui->cbEdgeTo->currentIndex());
}

void MainWindow::on_btnCalculate_clicked() {
    runSelectedAlgorithm();
}

void MainWindow::on_chkDirected_toggled(bool checked) {
    if (checked == model.isDirected()) return;

    pushUndo();

    QString notice;
    if (checked && !model.isDirected()) {
        notice = model.expandUndirectedEdges();
    } else if (!checked && model.isDirected()) {
        notice = model.collapseDirectedEdges();
    }

    model.setDirected(checked);
    if (graph) graph->clearSelection();

    rebuildEdgesForView();
    clearHighlights();
    statusLocked = false;
    updateActionStates();
    if (!notice.isEmpty()) showNotice(notice);
    else refreshStatus();
}

void MainWindow::showResultNotice(const QString &text) {
    statusLocked = true;
    showNotice(text);
}

int MainWindow::countNegativeEdges() const {
    return ShortestPath::countNegativeEdges(model.adjacencyStd(), model.nodeCount(), GraphModel::INF);
}

void MainWindow::showPathResult(const QVector<int> &path, double distance) {
    statusLocked = true;
    if (path.size() == 1) {
        showNotice(QStringLiteral("Mismo nodo · distancia %1").arg(formatWeight(distance)));
    } else {
    showNotice(QStringLiteral("Camino encontrado. Distancia = %1\nCamino: %2")
                           .arg(formatWeight(distance), joinPath(model.nodeNames(), path)));
    }
    graph->highlightPath(path);
}

void MainWindow::showNegativeCycle(const QVector<int> &cycle) {
    statusLocked = true;
    double cycleWeight = 0.0;
    const auto &adj = model.adjacency();
    for (int i = 0; i + 1 < cycle.size(); ++i) {
        cycleWeight += adj[cycle[i]][cycle[i + 1]];
    }
    showNotice(QStringLiteral("Un ciclo negativo detectado: %1 (peso %2).\n"
                              "El camino mínimo no está definido.")
                       .arg(joinPath(model.nodeNames(), cycle), formatWeight(cycleWeight)));
    graph->highlightPath(cycle);
}

void MainWindow::runSelectedAlgorithm() {
    const int src = ui->cbOrigin->currentIndex();
    const int dest = ui->cbDestination->currentIndex();

    if (src < 0 || dest < 0 || src >= model.nodeCount() || dest >= model.nodeCount()) {
        showNotice(QStringLiteral("Selecciona un origen y un destino."));
        return;
    }

    if (ui->rbtnDijkstra->isChecked()) {
        if (countNegativeEdges() > 0) {
            showNotice(QStringLiteral(
                    "Dijkstra no admite pesos negativos. Elige Bellman-Ford o Floyd-Warshall."));
            return;
        }
        runDijkstra(src, dest);
        return;
    }

    if (ui->rbtnBellman->isChecked()) {
        runBellmanFord(src, dest);
        return;
    }

    runFloydWarshall(src, dest);
}

void MainWindow::presentAlgorithmResult(const ShortestPath::PathResult &result) {
    if (result.negativeCycle) {
        if (!result.cycle.empty()) {
            showNegativeCycle(toQVec(result.cycle));
            return;
        }
        showResultNotice(QStringLiteral(
                "Se detectó un ciclo de peso negativo, pero no se pudo reconstruir.\n"
                "El camino mínimo no está definido."));
        clearHighlights();
        return;
    }

    if (!result.ok) {
        if (!result.reconstructed) {
            showResultNotice(QStringLiteral("No se pudo reconstruir el camino entre los nodos seleccionados."));
        } else {
            showResultNotice(QStringLiteral("No existe camino entre los nodos seleccionados."));
        }
        clearHighlights();
        return;
    }

    showPathResult(toQVec(result.path), result.distance);
}

void MainWindow::runDijkstra(int src, int dest) {
    presentAlgorithmResult(ShortestPath::dijkstra(model.adjacencyStd(), model.nodeCount(), GraphModel::INF, src, dest));
}

void MainWindow::runBellmanFord(int src, int dest) {
    presentAlgorithmResult(ShortestPath::bellmanFord(model.adjacencyStd(), model.nodeCount(), GraphModel::INF, src, dest));
}

void MainWindow::runFloydWarshall(int src, int dest) {
    presentAlgorithmResult(ShortestPath::floydWarshall(model.adjacencyStd(), model.nodeCount(), GraphModel::INF, src, dest));
}
