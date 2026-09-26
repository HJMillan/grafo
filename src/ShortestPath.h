#ifndef SHORTESTPATH_H
#define SHORTESTPATH_H

#include "Graph.h"
#include <QVector>

struct PathResult {
    enum Status {
        Ok,            // path = camino de origen a destino, distance = su peso
        NoPath,        // el destino no se alcanza desde el origen
        NegativeCycle  // path = ciclo (primer nodo repetido al final), distance = peso
                       // del ciclo; path vacío si no se pudo reconstruir
    };
    Status status = NoPath;
    double distance = 0.0;
    QVector<int> path;
};

PathResult dijkstra(const Graph &g, int src, int dest);
PathResult bellmanFord(const Graph &g, int src, int dest);
PathResult floydWarshall(const Graph &g, int src, int dest);

#endif // SHORTESTPATH_H
