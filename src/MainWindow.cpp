#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QComboBox>
#include <QMessageBox>
#include <QFrame>
#include <QLayout>
#include <QtGlobal>
#include <QGraphicsDropShadowEffect>
#include <QAbstractButton>
#include <cmath>

namespace {
QString joinPath(const Graph &g, const QVector<int> &path) {
    QString out;
    for (int i = 0; i < path.size(); ++i) {
        out += g.node(path[i]).name;
        if (i + 1 < path.size()) out += " -> ";
    }
    return out;
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    ui->txtNodeName->setMaxLength(1);

    // Sombra cian y efecto presionado para botones
    auto stylizeButton = [](QAbstractButton *btn) {
        if (!btn) return;
        auto *shadow = new QGraphicsDropShadowEffect(btn);
        shadow->setBlurRadius(0);
        shadow->setOffset(2, 2);
        shadow->setColor(QColor("#00B2A9"));
        btn->setGraphicsEffect(shadow);
        btn->setStyleSheet(
                "QPushButton { color: white; }"
                "QPushButton:hover { color: black; }"
                "QPushButton:pressed { color: black; }"
        );
        QObject::connect(btn, &QAbstractButton::pressed, btn, [shadow]() {
            shadow->setOffset(0, 0);
        });
        QObject::connect(btn, &QAbstractButton::released, btn, [shadow]() {
            shadow->setOffset(2, 2);
        });
    };

    stylizeButton(ui->btnCalculate);
    stylizeButton(ui->btnAddNode);
    stylizeButton(ui->btnRemoveLast);
    stylizeButton(ui->btnClearAll);
    stylizeButton(ui->btnAddEdge);

    initGraphWidget();

    uppercaseDefault = ui->chkUppercase->isChecked();
    graph.setDirected(ui->chkDirected->isChecked());
    graphView->setGraph(&graph);
    ui->txtOutput->setText("Usa el panel derecho para configurar.\n\nDistancia total: (sin camino)");

    connect(graphView, &GraphWidget::canvasClicked, this, &MainWindow::handleCanvasClick);
}

MainWindow::~MainWindow() { delete ui; }

// --------------------- Init ---------------------
void MainWindow::initGraphWidget() {
    graphView = qobject_cast<GraphWidget*>(ui->frameGraph);
    if (!graphView) {
        QFrame *placeholder = ui->frameGraph;
        graphView = new GraphWidget(placeholder->parentWidget());
        graphView->setObjectName("frameGraph");

        if (auto lay = placeholder->parentWidget()->layout()) {
            lay->replaceWidget(placeholder, graphView);
        } else {
            graphView->setGeometry(placeholder->geometry());
        }
        placeholder->deleteLater();
    }
}

// --------------------- Helpers de nodos ---------------------
QString MainWindow::nextSuggestedName() const {
    const QString alphabet = uppercaseDefault ? "ABCDEFGHIJKLMNOPQRSTUVWXYZ" : "abcdefghijklmnopqrstuvwxyz";
    for (int i = 0; i < alphabet.size(); ++i) {
        QString candidate(alphabet[i]);
        if (graph.indexOf(candidate) < 0) return candidate;
    }
    // Si se agotaron las letras básicas, volver a A/B/.../AA.
    int idx = graph.nodeCount();
    QString name;
    int base = alphabet.size();
    while (idx >= 0) {
        int r = idx % base;
        name.prepend(alphabet[r]);
        idx = idx / base - 1;
    }
    return name;
}

QPoint MainWindow::suggestedPosition(int idx) const {
    if (!graphView) return QPoint(50, 50);
    const QRect r = graphView->rect();
    const QPoint center = r.center();
    const int radius = qMax(80, qMin(r.width(), r.height()) / 2 - 40);
    const int denom = qMax(1, graph.nodeCount() + 1);
    constexpr double PI = 3.14159265358979323846;
    const double angle = 2.0 * PI * (idx % denom) / denom;
    const int x = center.x() + static_cast<int>(radius * std::cos(angle));
    const int y = center.y() + static_cast<int>(radius * std::sin(angle));
    return QPoint(x, y);
}

bool MainWindow::canAddNode(const QString &name, QString &reason) const {
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty()) {
        reason = "El nombre no puede estar vacío.";
        return false;
    }

    if (trimmed.length() != 1 || !trimmed.at(0).isLetter()) {
        reason = "El nombre del nodo debe ser una única letra (A-Z, a-z).";
        return false;
    }

    if (graph.nodeCount() >= Graph::MaxNodes) {
        reason = "Límite de 26 nodos alcanzado.";
        return false;
    }
    if (graph.indexOf(trimmed) >= 0) {
        reason = "El nombre de nodo ya existe (sensible a mayúsculas).";
        return false;
    }
    return true;
}

bool MainWindow::addNodeInternal(const QString &name, const QPoint &pos, bool autoPlaced) {
    QString reason;
    if (!canAddNode(name, reason)) {
        QMessageBox::warning(this, "No se puede agregar el nodo", reason);
        return false;
    }

    graph.addNode(name.trimmed(), pos, autoPlaced);
    refreshNodeSelectors();
    graphChanged();
    return true;
}

void MainWindow::removeLastNode() {
    if (graph.nodeCount() == 0) return;
    graph.removeNode(graph.nodeCount() - 1);
    refreshNodeSelectors();
    clearHighlights();
}

void MainWindow::clearGraph() {
    graph.clear();
    refreshNodeSelectors();
    clearHighlights();
    ui->txtOutput->clear();
}

void MainWindow::refreshNodeSelectors() {
    const auto fillCombo = [&](QComboBox *cb) {
        cb->clear();
        for (const auto &name : graph.names()) cb->addItem(name);
        if (cb->count() > 0) cb->setCurrentIndex(cb->count() - 1);
    };
    fillCombo(ui->cbEdgeFrom);
    fillCombo(ui->cbEdgeTo);
    fillCombo(ui->cbOrigin);
    fillCombo(ui->cbDestination);
}

void MainWindow::graphChanged() {
    graphView->update();
}

bool MainWindow::parseWeight(double &w) const {
    bool ok = false;
    w = ui->txtWeight->text().trimmed().toDouble(&ok);
    return ok;
}

void MainWindow::addEdge(int from, int to, double weight) {
    if (from < 0 || to < 0 || from >= graph.nodeCount() || to >= graph.nodeCount()) return;

    if (ui->rbtnDijkstra->isChecked() && weight < 0) {
        QMessageBox::warning(this, "Restricción de Dijkstra",
                             "Dijkstra no permite pesos negativos. Modifique el peso o elija otro algoritmo.");
        return;
    }

    graph.setEdge(from, to, weight);
    clearHighlights();
}

void MainWindow::clearHighlights() {
    graphView->highlightPath({});
}

// --------------------- Slots UI ---------------------
void MainWindow::handleCanvasClick(const QPoint &p) {
    QString name = ui->txtNodeName->text().trimmed();
    uppercaseDefault = ui->chkUppercase->isChecked();
    if (name.isEmpty()) name = nextSuggestedName();
    if (addNodeInternal(name, p, false)) {
        ui->txtNodeName->clear();
    }
}

void MainWindow::on_btnAddNode_clicked() {
    QString name = ui->txtNodeName->text().trimmed();
    uppercaseDefault = ui->chkUppercase->isChecked();
    if (name.isEmpty()) name = nextSuggestedName();

    const QPoint pos = suggestedPosition(graph.nodeCount());
    if (addNodeInternal(name, pos, true)) {
        ui->txtNodeName->clear();
    }
}

void MainWindow::on_btnRemoveLast_clicked() { removeLastNode(); }

void MainWindow::on_btnClearAll_clicked() { clearGraph(); }

void MainWindow::on_btnAddEdge_clicked() {
    const int from = ui->cbEdgeFrom->currentIndex();
    const int to = ui->cbEdgeTo->currentIndex();

    double w = 0.0;
    if (!parseWeight(w)) {
        QMessageBox::warning(this, "Peso inválido", "Ingrese un peso numérico.");
        return;
    }

    graph.setDirected(ui->chkDirected->isChecked());
    addEdge(from, to, w);
}

void MainWindow::on_btnCalculate_clicked() {
    runSelectedAlgorithm();
}

void MainWindow::on_chkDirected_toggled(bool checked) {
    graph.setDirected(checked);
    graphChanged();

    QMessageBox::information(this, "Cambio de Modo de Grafo",
                             "El modo del grafo ha sido cambiado.\n\n"
                             "Tome en cuenta que este cambio no modifica las aristas ya existentes. "
                             "Si creó aristas en modo no-dirigido, estas seguirán siendo simétricas internamente. "
                             "Para evitar inconsistencias, se recomienda limpiar el grafo y volver a añadir las aristas.");
}

// --------------------- Algoritmos ---------------------
void MainWindow::runSelectedAlgorithm() {
    ui->txtOutput->clear();
    graph.setDirected(ui->chkDirected->isChecked());
    const int src = ui->cbOrigin->currentIndex();
    const int dest = ui->cbDestination->currentIndex();

    if (src < 0 || dest < 0 || src >= graph.nodeCount() || dest >= graph.nodeCount()) {
        QMessageBox::warning(this, "Selección inválida", "Seleccione origen y destino.");
        return;
    }

    if (ui->rbtnDijkstra->isChecked()) {
        if (!graph.negativeEdges().isEmpty()) {
            QMessageBox::warning(this, "Restricción de Dijkstra",
                                 "Dijkstra no permite pesos negativos. Modifique los caminos o elija otro algoritmo.");
            return;
        }
        showResult(dijkstra(graph, src, dest));
    } else if (ui->rbtnBellman->isChecked()) {
        showResult(bellmanFord(graph, src, dest));
    } else {
        showResult(floydWarshall(graph, src, dest));
    }
}

void MainWindow::showResult(const PathResult &result) {
    switch (result.status) {
    case PathResult::Ok:
        ui->txtOutput->setText(
                QString("Camino encontrado. Distancia = %1\nCamino: %2")
                .arg(result.distance, 0, 'f', 2)
                .arg(joinPath(graph, result.path)));
        graphView->highlightPath(result.path);
        break;
    case PathResult::NoPath:
        ui->txtOutput->setText("No existe camino entre los nodos seleccionados.");
        graphView->highlightPath({});
        break;
    case PathResult::NegativeCycle:
        if (!result.path.isEmpty()) {
            ui->txtOutput->setText(QString("Ciclo negativo: %1, peso %2")
                                   .arg(joinPath(graph, result.path))
                                   .arg(result.distance, 0, 'f', 2));
            QMessageBox::warning(this, "Ciclo negativo",
                                 "Se detectó un ciclo de peso negativo. El camino mínimo no está definido.");
        } else {
            QMessageBox::warning(this, "Ciclo negativo",
                                 "Se detectó un ciclo de peso negativo, pero no se pudo reconstruir. "
                                 "El camino mínimo no está definido.");
        }
        graphView->highlightPath(result.path);
        break;
    }
}
