#ifndef MESSAGES_H
#define MESSAGES_H

#include <QString>
#include <QVector>
#include "Graph.h"

// Catálogo de todos los textos que ve el usuario.
// Regla: qué pasó -> por qué -> cómo arreglarlo, tratando de «usted»
// y nombrando nodos y valores concretos.
namespace Msg {

struct Message {
    QString title;
    QString text;
};

// ---------- Formato ----------
QString number(double value);                       // 2,5 · -3 · 1000000
QString path(const Graph &g, const QVector<int> &nodes); // A → B → C
QString edge(const Graph &g, const Graph::Edge &e); // A→B (dirigido) o A–B
QString edgeSample(const Graph &g, const QVector<Graph::Edge> &edges, int max = 3);

// ---------- Campo «Peso» ----------
QString weightEmpty();
QString weightNotNumber(const QString &text);
QString weightNotFinite();
QString weightOutOfRange(double limit);
QString weightNegativeUndirected(const QString &from, const QString &to, double weight);
QString weightNegativeDijkstra();

// ---------- Campo «Nombre» del nodo ----------
QString nodeNameInvalid();
QString nodeNameDuplicate(const QString &name, const QString &alternative);
QString nodeLimit(int max);
QString nodeNoFreeName(bool uppercase);
QString nodeOverlap();

// ---------- Aristas ----------
QString edgeMissing(const QString &from, const QString &to);
QString addEdgeText(bool exists);   // «Agregar arista» o «Cambiar peso»

// ---------- Menú del nodo ----------
QString removeNodeAction(const QString &name);

// ---------- Botones desactivados (tooltips) ----------
QString needsNodesForEdge();
QString needsNodesForCalculate();

// ---------- Aviso del grupo «Camino más corto» ----------
QString dijkstraWithNegatives(const Graph &g, const QVector<Graph::Edge> &negatives);

// ---------- Banner de resultado ----------
Message initialHint();
Message graphChanged();
Message pathFound(const Graph &g, const QVector<int> &nodes, double distance, const QString &algorithm);
Message noPath(const Graph &g, int src, int dest);
Message negativeCycle(const Graph &g, int src, int dest, const QVector<int> &cycle, double weight);

// ---------- Confirmaciones (ventanas modales) ----------
Message confirmToUndirected(const Graph &g, const QVector<Graph::Edge> &asymmetric,
                            const QVector<Graph::Edge> &negatives);
QString confirmToUndirectedAccept();
Message confirmClearAll(int nodes, int edges);
QString confirmClearAllAccept();
Message confirmRemoveNode(const QString &name, int edges);
QString confirmRemoveNodeAccept();

} // namespace Msg

#endif // MESSAGES_H
