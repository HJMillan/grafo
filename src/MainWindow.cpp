#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QComboBox>
#include <QMessageBox>
#include <QFrame>
#include <QLayout>
#include <QtGlobal>
#include <QGraphicsDropShadowEffect>
#include <QAbstractButton>
#include <queue>
#include <vector>
#include <limits>
#include <cmath>

namespace {
QString joinPath(const QVector<QString> &names, const QVector<int> &path) {
    QString out;
    for (int i = 0; i < path.size(); ++i) {
        out += names[path[i]];
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

    adj = QVector<QVector<double>>(MAX_NODES, QVector<double>(MAX_NODES, INF));
    uppercaseDefault = ui->chkUppercase->isChecked();
    directed = ui->chkDirected->isChecked();
    ui->txtOutput->setText("Usa el panel derecho para configurar.\n\nDistancia total: (sin camino)");

    connect(graph, &GraphWidget::nodeClicked, this, &MainWindow::handleCanvasClick);
}

MainWindow::~MainWindow() { delete ui; }

// --------------------- Init ---------------------
void MainWindow::initGraphWidget() {
    graph = qobject_cast<GraphWidget*>(ui->frameGraph);
    if (!graph) {
        QFrame *placeholder = ui->frameGraph;
        graph = new GraphWidget(placeholder->parentWidget());
        graph->setObjectName("frameGraph");

        if (auto lay = placeholder->parentWidget()->layout()) {
            lay->replaceWidget(placeholder, graph);
        } else {
            graph->setGeometry(placeholder->geometry());
        }
        placeholder->deleteLater();
    }
}

// --------------------- Helpers de nodos ---------------------
QString MainWindow::nextSuggestedName() const {
    const QString alphabet = uppercaseDefault ? "ABCDEFGHIJKLMNOPQRSTUVWXYZ" : "abcdefghijklmnopqrstuvwxyz";
    for (int i = 0; i < alphabet.size(); ++i) {
        QString candidate(alphabet[i]);
        if (!nameToIndex.contains(candidate)) return candidate;
    }
    // Si se agotaron las letras básicas, volver a A/B/.../AA.
    int idx = nodeNames.size();
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
    if (!graph) return QPoint(50, 50);
    const QRect r = graph->rect();
    const QPoint center = r.center();
    const int radius = qMax(80, qMin(r.width(), r.height()) / 2 - 40);
    const int denom = qMax(1, nodeNames.size() + 1);
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

    if (nodeNames.size() >= MAX_NODES) {
        reason = "Límite de 26 nodos alcanzado.";
        return false;
    }
    if (nameToIndex.contains(trimmed)) {
        reason = "El nombre de nodo ya existe (sensible a mayúsculas).";
        return false;
    }
    return true;
}

bool MainWindow::addNodeInternal(const QString &name, const QPoint &pos) {
    QString reason;
    if (!canAddNode(name, reason)) {
        QMessageBox::warning(this, "No se puede agregar el nodo", reason);
        return false;
    }

    const int idx = nodeNames.size();
    nodeNames.append(name.trimmed());
    nameToIndex.insert(name.trimmed(), idx);

    graph->addNodeVisual(name.trimmed(), pos);
    refreshNodeSelectors();
    rebuildEdgesForView();
    return true;
}

void MainWindow::removeLastNode() {
    if (nodeNames.isEmpty()) return;
    const int idx = nodeNames.size() - 1;
    const QString name = nodeNames.last();
    nodeNames.removeLast();
    nameToIndex.remove(name);

    for (int i = 0; i < MAX_NODES; ++i) {
        adj[idx][i] = INF;
        adj[i][idx] = INF;
    }

    graph->removeLastNode();
    refreshNodeSelectors();
    rebuildEdgesForView();
    clearHighlights();
}

void MainWindow::clearGraph() {
    nodeNames.clear();
    nameToIndex.clear();
    for (int i = 0; i < MAX_NODES; ++i)
        for (int j = 0; j < MAX_NODES; ++j)
            adj[i][j] = INF;

    graph->clearAll();
    refreshNodeSelectors();
    clearHighlights();
    ui->txtOutput->clear();
}

void MainWindow::refreshNodeSelectors() {
    const auto fillCombo = [&](QComboBox *cb) {
        cb->clear();
        for (const auto &name : nodeNames) cb->addItem(name);
        if (cb->count() > 0) cb->setCurrentIndex(cb->count() - 1);
    };
    fillCombo(ui->cbEdgeFrom);
    fillCombo(ui->cbEdgeTo);
    fillCombo(ui->cbOrigin);
    fillCombo(ui->cbDestination);
}

void MainWindow::rebuildEdgesForView() {
    graph->setAdjacency(adj, nodeNames.size(), INF, directed);
}

bool MainWindow::parseWeight(double &w) const {
    bool ok = false;
    w = ui->txtWeight->text().trimmed().toDouble(&ok);
    return ok;
}

void MainWindow::addEdge(int from, int to, double weight) {
    if (from < 0 || to < 0 || from >= nodeNames.size() || to >= nodeNames.size()) return;

    if (ui->rbtnDijkstra->isChecked() && weight < 0) {
        QMessageBox::warning(this, "Restricción de Dijkstra",
                             "Dijkstra no permite pesos negativos. Modifique el peso o elija otro algoritmo.");
        return;
    }

    adj[from][to] = weight;
    if (!directed) {
        adj[to][from] = weight;
    }

    rebuildEdgesForView();
    clearHighlights();
}

void MainWindow::clearHighlights() {
    graph->highlightPath({});
    graph->update();
}

// --------------------- Slots UI ---------------------
void MainWindow::handleCanvasClick(const QPoint &p) {
    QString name = ui->txtNodeName->text().trimmed();
    uppercaseDefault = ui->chkUppercase->isChecked();
    if (name.isEmpty()) name = nextSuggestedName();
    if (addNodeInternal(name, p)) {
        ui->txtNodeName->clear();
    }
}

void MainWindow::on_btnAddNode_clicked() {
    QString name = ui->txtNodeName->text().trimmed();
    uppercaseDefault = ui->chkUppercase->isChecked();
    if (name.isEmpty()) name = nextSuggestedName();

    const int idx = nodeNames.size();
    QPoint pos = suggestedPosition(idx);
    if (addNodeInternal(name, pos)) {
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

    directed = ui->chkDirected->isChecked();
    addEdge(from, to, w);
}

void MainWindow::on_btnCalculate_clicked() {
    runSelectedAlgorithm();
}

void MainWindow::on_chkDirected_toggled(bool checked) {
    directed = checked;
    rebuildEdgesForView();

    QMessageBox::information(this, "Cambio de Modo de Grafo",
                             "El modo del grafo ha sido cambiado.\n\n"
                             "Tome en cuenta que este cambio no modifica las aristas ya existentes. "
                             "Si creó aristas en modo no-dirigido, estas seguirán siendo simétricas internamente. "
                             "Para evitar inconsistencias, se recomienda limpiar el grafo y volver a añadir las aristas.");
}

// --------------------- Algoritmos ---------------------
int MainWindow::countNegativeEdges() const {
    int neg = 0;
    const int n = nodeNames.size();
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            if (adj[i][j] >= INF / 2) continue;
            if (adj[i][j] < 0) {
                // si es simétrico (caso no dirigido) contamos una sola vez
                if (j < i && adj[j][i] < INF / 2 && qFuzzyCompare(adj[i][j], adj[j][i])) {
                    continue;
                }
                ++neg;
            }
        }
    }
    return neg;
}

QVector<int> MainWindow::buildPathFromPrev(const QVector<int> &prev, int dest) const {
    QVector<int> path;
    if (dest < 0 || dest >= prev.size()) return path;
    for (int v = dest; v != -1; v = prev[v]) {
        path.prepend(v);
    }
    return path;
}

void MainWindow::runSelectedAlgorithm() {
    ui->txtOutput->clear();
    directed = ui->chkDirected->isChecked();
    const int src = ui->cbOrigin->currentIndex();
    const int dest = ui->cbDestination->currentIndex();

    if (src < 0 || dest < 0 || src >= nodeNames.size() || dest >= nodeNames.size()) {
        QMessageBox::warning(this, "Selección inválida", "Seleccione origen y destino.");
        return;
    }

    const int negatives = countNegativeEdges();
    if (ui->rbtnDijkstra->isChecked()) {
        if (negatives > 0) {
            QMessageBox::warning(this, "Restricción de Dijkstra",
                                 "Dijkstra no permite pesos negativos. Modifique los caminos o elija otro algoritmo.");
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

void MainWindow::runDijkstra(int src, int dest) {
    const int n = nodeNames.size();
    QVector<double> dist(n, INF);
    QVector<int> prev(n, -1);

    using P = QPair<double, int>;
    auto cmp = [](const P &a, const P &b) { return a.first > b.first; };
    std::priority_queue<P, std::vector<P>, decltype(cmp)> pq(cmp);

    dist[src] = 0.0;
    pq.push({0.0, src});

    while (!pq.empty()) {
        auto [d, u] = pq.top(); pq.pop();
        if (d != dist[u]) continue;
        if (u == dest) break;
        for (int v = 0; v < n; ++v) {
            double w = adj[u][v];
            if (w >= INF / 2) continue;
            if (dist[u] + w < dist[v]) {
                dist[v] = dist[u] + w;
                prev[v] = u;
                pq.push({dist[v], v});
            }
        }
    }

    if (dist[dest] >= INF / 2) {
        ui->txtOutput->setText("No existe camino entre los nodos seleccionados.");
        graph->highlightPath({});
        graph->update();
        return;
    }

    const QVector<int> path = buildPathFromPrev(prev, dest);
    ui->txtOutput->setText(
            QString("Camino encontrado. Distancia = %1\nCamino: %2")
            .arg(dist[dest], 0, 'f', 2)
            .arg(joinPath(nodeNames, path))
    );
    graph->highlightPath(path, adj, INF, directed);
    graph->update();
}

void MainWindow::runBellmanFord(int src, int dest) {
    const int n = nodeNames.size();
    QVector<double> dist(n, INF);
    QVector<int> prev(n, -1);

    struct Edge { int u; int v; double w; };
    QVector<Edge> edges;
    edges.reserve(n * n);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            if (adj[i][j] < INF / 2) edges.append({i, j, adj[i][j]});
        }
    }

    dist[src] = 0.0;
    for (int it = 0; it < n - 1; ++it) {
        bool changed = false;
        for (const auto &e : edges) {
            if (dist[e.u] >= INF / 2) continue;
            if (dist[e.u] + e.w < dist[e.v]) {
                dist[e.v] = dist[e.u] + e.w;
                prev[e.v] = e.u;
                changed = true;
            }
        }
        if (!changed) break;
    }

    int cycle_node = -1;
    for (const auto &e : edges) {
        if (dist[e.u] < INF / 2 && dist[e.u] + e.w < dist[e.v]) {
            prev[e.v] = e.u;
            cycle_node = e.v;
            break;
        }
    }

    if (cycle_node != -1) {
        for (int i = 0; i < n; ++i) {
            if(cycle_node == -1) break;
            cycle_node = prev[cycle_node];
        }

        QVector<int> cycle_path;
        for (int v = cycle_node; ; v = prev[v]) {
            cycle_path.prepend(v);
            if (v == cycle_node && cycle_path.size() > 1) break;
            if (prev[v] == -1 || cycle_path.size() > n + 1) {
                cycle_path.clear();
                break;
            }
        }
        
        if(!cycle_path.isEmpty()) {
            double cycle_weight = 0;
            for(size_t i = 0; i < cycle_path.size() - 1; ++i) {
                cycle_weight += adj[cycle_path[i]][cycle_path[i+1]];
            }

            QString pathStr = joinPath(nodeNames, cycle_path);
            QString message = QString("Ciclo negativo: %1, peso %2")
                                  .arg(pathStr)
                                  .arg(cycle_weight, 0, 'f', 2);
            
            ui->txtOutput->setText(message);
            QMessageBox::warning(this, "Ciclo negativo", "Se detectó un ciclo de peso negativo. El camino mínimo no está definido.");
            graph->highlightPath(cycle_path, adj, INF, directed);
            graph->update();
            return;
        } else {
             QMessageBox::warning(this, "Ciclo negativo", "Se detectó un ciclo negativo, pero no se pudo reconstruir. El camino mínimo no está definido.");
             graph->highlightPath({});
             graph->update();
             return;
        }
    }

    if (dist[dest] >= INF / 2) {
        ui->txtOutput->setText("No existe camino entre los nodos seleccionados.");
        graph->highlightPath({});
        graph->update();
        return;
    }

    const QVector<int> path = buildPathFromPrev(prev, dest);
    ui->txtOutput->setText(
            QString("Camino encontrado. Distancia = %1\nCamino: %2")
            .arg(dist[dest], 0, 'f', 2)
            .arg(joinPath(nodeNames, path))
    );
    graph->highlightPath(path, adj, INF, directed);
    graph->update();
}

void MainWindow::runFloydWarshall(int src, int dest) {
    const int n = nodeNames.size();
    QVector<QVector<double>> dist(n, QVector<double>(n, INF));
    QVector<QVector<int>> next(n, QVector<int>(n, -1));

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            dist[i][j] = adj[i][j];
            if (adj[i][j] < INF / 2) next[i][j] = j;
        }
        dist[i][i] = qMin(0.0, dist[i][i]);
        next[i][i] = i;
    }

    for (int k = 0; k < n; ++k) {
        for (int i = 0; i < n; ++i) {
            if (dist[i][k] >= INF / 2) continue;
            for (int j = 0; j < n; ++j) {
                if (dist[k][j] >= INF / 2) continue;
                double candidate = dist[i][k] + dist[k][j];
                if (candidate < dist[i][j]) {
                    dist[i][j] = candidate;
                    next[i][j] = next[i][k];
                }
            }
        }
    }

    for (int i = 0; i < n; ++i) {
        if (dist[i][i] < 0) {
            QVector<int> cycle_path;
            int start_node = i;
            int u = i;
            do {
                cycle_path.append(u);
                u = next[u][start_node];
                if (u == -1 || cycle_path.size() > n) {
                    cycle_path.clear();
                    break;
                }
            } while (u != start_node);

            if (!cycle_path.isEmpty()) {
                cycle_path.append(start_node);

                double cycle_weight = 0;
                for(size_t j = 0; j < cycle_path.size() - 1; ++j) {
                    cycle_weight += adj[cycle_path[j]][cycle_path[j+1]];
                }

                QString pathStr = joinPath(nodeNames, cycle_path);
                QString message = QString("Ciclo negativo: %1, peso %2")
                                      .arg(pathStr)
                                      .arg(cycle_weight, 0, 'f', 2);
                
                ui->txtOutput->setText(message);
                QMessageBox::warning(this, "Ciclo negativo", "Se detectó un ciclo de peso negativo. El camino mínimo no está definido.");
                graph->highlightPath(cycle_path, adj, INF, directed);
                graph->update();
                return;
            } else {
                QMessageBox::warning(this, "Ciclo negativo", "Se detectó un ciclo de peso negativo, pero no se pudo reconstruir. El camino mínimo no está definido.");
                graph->highlightPath({});
                graph->update();
                return;
            }
        }
    }

    if (next[src][dest] == -1) {
        ui->txtOutput->setText("No existe camino entre los nodos seleccionados.");
        graph->highlightPath({});
        graph->update();
        return;
    }

    QVector<int> path;
    int u = src;
    path.append(u);
    while (u != dest) {
        u = next[u][dest];
        if (u == -1) break;
        path.append(u);
    }

    ui->txtOutput->setText(
            QString("Camino encontrado. Distancia = %1\nCamino: %2")
            .arg(dist[src][dest], 0, 'f', 2)
            .arg(joinPath(nodeNames, path))
    );
    graph->highlightPath(path, adj, INF, directed);
    graph->update();
}