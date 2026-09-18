#include "GraphModel.h"
#include "GraphFormat.h"

#include <QStringList>

GraphModel::GraphModel() {
    adj_ = QVector<QVector<double>>(MAX_NODES, QVector<double>(MAX_NODES, INF));
}

void GraphModel::rebuildNameIndex() {
    nameToIndex_.clear();
    for (int i = 0; i < nodeNames_.size(); ++i) nameToIndex_.insert(nodeNames_[i], i);
}

void GraphModel::rebuildAdjacency() {
    for (int i = 0; i < MAX_NODES; ++i) {
        for (int j = 0; j < MAX_NODES; ++j) {
            adj_[i][j] = INF;
        }
    }

    for (const UserEdge &e : userEdges_) {
        if (e.from < 0 || e.to < 0) continue;
        if (e.from >= nodeNames_.size() || e.to >= nodeNames_.size()) continue;
        adj_[e.from][e.to] = e.weight;
        if (!directed_) adj_[e.to][e.from] = e.weight;
    }
}

std::vector<std::vector<double>> GraphModel::adjacencyStd() const {
    const int n = nodeCount();
    std::vector<std::vector<double>> out(static_cast<size_t>(n),
                                         std::vector<double>(static_cast<size_t>(n), INF));
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            out[static_cast<size_t>(i)][static_cast<size_t>(j)] = adj_[i][j];
        }
    }
    return out;
}

QString GraphModel::nextSuggestedName(bool uppercase) const {
    const QString alphabet = uppercase ? QStringLiteral("ABCDEFGHIJKLMNOPQRSTUVWXYZ")
                                       : QStringLiteral("abcdefghijklmnopqrstuvwxyz");
    for (int i = 0; i < alphabet.size(); ++i) {
        const QString candidate(alphabet[i]);
        if (!nameToIndex_.contains(candidate)) return candidate;
    }
    return QString();
}

bool GraphModel::canAddNode(const QString &name, QString &reason) const {
    if (nodeNames_.size() >= MAX_NODES) {
        reason = QStringLiteral("Límite de 26 nodos alcanzado.");
        return false;
    }

    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty()) {
        reason = QStringLiteral("El nombre no puede estar vacío.");
        return false;
    }

    if (trimmed.length() != 1 || !trimmed.at(0).isLetter()) {
        reason = QStringLiteral("El nombre del nodo debe ser una única letra (A–Z, a–z).");
        return false;
    }

    if (nameToIndex_.contains(trimmed)) {
        reason = QStringLiteral("Ya existe un nodo con ese nombre (se distinguen mayúsculas).");
        return false;
    }
    return true;
}

bool GraphModel::addNode(const QString &name, const QPoint &pos) {
    QString reason;
    if (!canAddNode(name, reason)) return false;
    const QString trimmed = name.trimmed();
    nodeNames_.append(trimmed);
    positions_.append(pos);
    nameToIndex_.insert(trimmed, nodeNames_.size() - 1);
    return true;
}

void GraphModel::removeLastNode() {
    if (nodeNames_.isEmpty()) return;
    const int idx = nodeNames_.size() - 1;
    const QString name = nodeNames_.last();
    nodeNames_.removeLast();
    if (!positions_.isEmpty()) positions_.removeLast();
    nameToIndex_.remove(name);
    for (int i = userEdges_.size() - 1; i >= 0; --i) {
        if (userEdges_[i].from == idx || userEdges_[i].to == idx) userEdges_.removeAt(i);
    }
    rebuildAdjacency();
}

void GraphModel::clear() {
    nodeNames_.clear();
    positions_.clear();
    nameToIndex_.clear();
    userEdges_.clear();
    rebuildAdjacency();
}

void GraphModel::setPositions(const QVector<QPoint> &positions) {
    if (positions.size() != nodeNames_.size()) return;
    positions_ = positions;
}

int GraphModel::findUserEdge(int from, int to) const {
    for (int i = 0; i < userEdges_.size(); ++i) {
        const UserEdge &e = userEdges_[i];
        if (e.from == from && e.to == to) return i;
        if (!directed_ && e.from == to && e.to == from) return i;
    }
    return -1;
}

QString GraphModel::edgePairLabel(int from, int to) const {
    if (from < 0 || to < 0 || from >= nodeNames_.size() || to >= nodeNames_.size()) return {};
    if (directed_) {
        return QStringLiteral("%1→%2").arg(nodeNames_[from], nodeNames_[to]);
    }
    const int a = qMin(from, to);
    const int b = qMax(from, to);
    return QStringLiteral("%1–%2").arg(nodeNames_[a], nodeNames_[b]);
}

bool GraphModel::addEdge(int from, int to, double weight, double *oldWeight) {
    if (from < 0 || to < 0 || from >= nodeNames_.size() || to >= nodeNames_.size()) return false;

    int storeFrom = from;
    int storeTo = to;
    if (!directed_) {
        storeFrom = qMin(from, to);
        storeTo = qMax(from, to);
    }

    const int existing = findUserEdge(from, to);
    if (existing >= 0) {
        if (oldWeight) *oldWeight = userEdges_[existing].weight;
        userEdges_[existing] = {storeFrom, storeTo, weight};
    } else {
        if (oldWeight) *oldWeight = weight;
        userEdges_.append({storeFrom, storeTo, weight});
    }
    rebuildAdjacency();
    return existing >= 0;
}

bool GraphModel::removeEdge(int from, int to) {
    const int idx = findUserEdge(from, to);
    if (idx < 0) return false;
    userEdges_.removeAt(idx);
    rebuildAdjacency();
    return true;
}

void GraphModel::setDirected(bool directed) {
    directed_ = directed;
    rebuildAdjacency();
}

QString GraphModel::expandUndirectedEdges() {
    bool expanded = false;
    QVector<UserEdge> next;
    next.reserve(userEdges_.size() * 2);
    for (const UserEdge &e : userEdges_) {
        if (e.from == e.to) {
            next.append(e);
            continue;
        }
        next.append(e);
        next.append({e.to, e.from, e.weight});
        expanded = true;
    }
    userEdges_ = next;
    if (!expanded) return {};
    return QStringLiteral("Ahora el grafo es dirigido. Cada arista pasó a ser A→B y B→A con el mismo peso.");
}

QString GraphModel::collapseDirectedEdges() {
    QVector<UserEdge> next;
    QStringList conflicts;
    QVector<bool> used(userEdges_.size(), false);

    for (int i = 0; i < userEdges_.size(); ++i) {
        if (used[i]) continue;
        const UserEdge &e = userEdges_[i];
        if (e.from == e.to) {
            used[i] = true;
            next.append(e);
            continue;
        }

        const int a = qMin(e.from, e.to);
        const int b = qMax(e.from, e.to);
        int winner = i;
        int idxAB = -1;
        int idxBA = -1;

        for (int j = i; j < userEdges_.size(); ++j) {
            const UserEdge &f = userEdges_[j];
            if (f.from == f.to) continue;
            if (qMin(f.from, f.to) != a || qMax(f.from, f.to) != b) continue;
            used[j] = true;
            if (f.from == a && f.to == b) idxAB = j;
            if (f.from == b && f.to == a) idxBA = j;
            winner = j;
        }

        next.append({a, b, userEdges_[winner].weight});

        if (idxAB >= 0 && idxBA >= 0
            && !GraphConstants::sameWeight(userEdges_[idxAB].weight, userEdges_[idxBA].weight)) {
            const QString nA = nodeNames_[a];
            const QString nB = nodeNames_[b];
            conflicts.append(
                    QStringLiteral("%1–%2 tenía pesos distintos (%1→%2 %3 y %2→%1 %4). "
                                   "Queda %5 en ambos sentidos.")
                            .arg(nA, nB, formatWeight(userEdges_[idxAB].weight))
                            .arg(formatWeight(userEdges_[idxBA].weight),
                                 formatWeight(userEdges_[winner].weight)));
        }
    }

    userEdges_ = next;
    return conflicts.join(QLatin1Char('\n'));
}

GraphSnapshot GraphModel::capture() const {
    GraphSnapshot s;
    s.nodeNames = nodeNames_;
    s.positions = positions_;
    s.userEdges = userEdges_;
    s.directed = directed_;
    return s;
}

void GraphModel::apply(const GraphSnapshot &s) {
    nodeNames_ = s.nodeNames;
    userEdges_ = s.userEdges;
    directed_ = s.directed;
    positions_ = s.positions;
    if (positions_.size() != nodeNames_.size()) positions_.resize(nodeNames_.size());
    rebuildNameIndex();
    rebuildAdjacency();
}

void GraphModel::pushUndo() {
    undoStack_.append(capture());
    if (undoStack_.size() > MAX_UNDO) undoStack_.removeFirst();
}

bool GraphModel::popUndo(GraphSnapshot &out) {
    if (undoStack_.isEmpty()) return false;
    out = undoStack_.takeLast();
    return true;
}
