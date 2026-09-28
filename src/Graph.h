#ifndef GRAPH_H
#define GRAPH_H

#include <QPointF>
#include <QString>
#include <QVector>
#include <optional>

// Modelo del grafo: nodos con nombre y posición, y matriz de adyacencia.
// Solo depende de QtCore para poder probarse sin interfaz gráfica.
class Graph {
public:
    static constexpr int MaxNodes = 26;

    struct Node {
        QString name;
        QPointF pos;
        bool autoPlaced = false; // colocado por la app (true) o por el usuario (false)
    };

    struct Edge {
        int from;
        int to;
        double weight;
    };

    int nodeCount() const { return static_cast<int>(m_nodes.size()); }
    const Node &node(int i) const;
    const QVector<Node> &nodes() const { return m_nodes; }
    int indexOf(const QString &name) const;
    QVector<QString> names() const;

    bool isDirected() const { return m_directed; }
    void setDirected(bool directed) { m_directed = directed; }

    int addNode(const QString &name, const QPointF &pos, bool autoPlaced);
    void removeNode(int index);
    void setNodePos(int index, const QPointF &pos);
    void setNodeAutoPlaced(int index, bool autoPlaced);
    void clear();

    // En modo no dirigido, setEdge/removeEdge actúan sobre ambos sentidos.
    void setEdge(int from, int to, double weight);
    void removeEdge(int from, int to);
    void clearEdges();
    std::optional<double> edge(int from, int to) const;
    bool hasEdge(int from, int to) const;

    // Aristas para mostrar: en modo dirigido cada arco; en no dirigido
    // cada par una sola vez (from <= to), incluidos los lazos.
    QVector<Edge> edgeList() const;
    // Todos los arcos de la matriz, sin importar el modo.
    QVector<Edge> arcs() const;

    bool isSymmetric() const;
    QVector<Edge> negativeEdges() const;   // según el modo actual
    QVector<Edge> asymmetricArcs() const;  // arcos sin inverso, o con inverso de otro peso (una vez por pareja)

private:
    bool isValid(int i) const { return i >= 0 && i < nodeCount(); }

    QVector<Node> m_nodes;
    QVector<QVector<std::optional<double>>> m_adj;
    bool m_directed = false;
};

#endif // GRAPH_H
