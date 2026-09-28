#include "Graph.h"

#include <QtGlobal>

const Graph::Node &Graph::node(int i) const {
    Q_ASSERT(isValid(i));
    return m_nodes[i];
}

int Graph::indexOf(const QString &name) const {
    for (int i = 0; i < nodeCount(); ++i) {
        if (m_nodes[i].name == name) return i;
    }
    return -1;
}

QVector<QString> Graph::names() const {
    QVector<QString> out;
    out.reserve(m_nodes.size());
    for (const Node &n : m_nodes) out.append(n.name);
    return out;
}

int Graph::addNode(const QString &name, const QPointF &pos, bool autoPlaced) {
    m_nodes.append({name, pos, autoPlaced});
    for (auto &row : m_adj) row.append(std::nullopt);
    m_adj.append(QVector<std::optional<double>>(m_nodes.size(), std::nullopt));
    return nodeCount() - 1;
}

void Graph::removeNode(int index) {
    if (!isValid(index)) return;
    m_nodes.removeAt(index);
    m_adj.removeAt(index);
    for (auto &row : m_adj) row.removeAt(index);
}

void Graph::setNodePos(int index, const QPointF &pos) {
    if (isValid(index)) m_nodes[index].pos = pos;
}

void Graph::setNodeAutoPlaced(int index, bool autoPlaced) {
    if (isValid(index)) m_nodes[index].autoPlaced = autoPlaced;
}

void Graph::clear() {
    m_nodes.clear();
    m_adj.clear();
}

std::optional<double> Graph::edge(int from, int to) const {
    Q_ASSERT(isValid(from) && isValid(to));
    return m_adj[from][to];
}

bool Graph::hasEdge(int from, int to) const {
    Q_ASSERT(isValid(from) && isValid(to));
    return m_adj[from][to].has_value();
}

void Graph::setEdge(int from, int to, double weight) {
    Q_ASSERT(isValid(from) && isValid(to));
    m_adj[from][to] = weight;
    if (!m_directed) m_adj[to][from] = weight;
}

void Graph::removeEdge(int from, int to) {
    Q_ASSERT(isValid(from) && isValid(to));
    m_adj[from][to].reset();
    if (!m_directed) m_adj[to][from].reset();
}

void Graph::clearEdges() {
    for (auto &row : m_adj)
        for (auto &cell : row) cell.reset();
}

QVector<Graph::Edge> Graph::arcs() const {
    QVector<Edge> out;
    for (int i = 0; i < nodeCount(); ++i)
        for (int j = 0; j < nodeCount(); ++j)
            if (m_adj[i][j]) out.append({i, j, *m_adj[i][j]});
    return out;
}

QVector<Graph::Edge> Graph::edgeList() const {
    if (m_directed) return arcs();
    QVector<Edge> out;
    for (int i = 0; i < nodeCount(); ++i)
        for (int j = i; j < nodeCount(); ++j)
            if (m_adj[i][j]) out.append({i, j, *m_adj[i][j]});
    return out;
}

bool Graph::isSymmetric() const {
    return asymmetricArcs().isEmpty();
}

QVector<Graph::Edge> Graph::negativeEdges() const {
    QVector<Edge> out;
    for (const Edge &e : edgeList())
        if (e.weight < 0) out.append(e);
    return out;
}

QVector<Graph::Edge> Graph::asymmetricArcs() const {
    QVector<Edge> out;
    for (const Edge &e : arcs()) {
        const auto &rev = m_adj[e.to][e.from];
        // Si existe la vuelta con otro peso, la pareja se cuenta una sola vez.
        if (!rev || (e.from < e.to && *rev != e.weight)) out.append(e);
    }
    return out;
}
