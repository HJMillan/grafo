#include "ShortestPath.h"

#include <limits>
#include <queue>
#include <vector>

namespace {
constexpr double INF = std::numeric_limits<double>::infinity();

QVector<int> buildPathFromPrev(const QVector<int> &prev, int dest) {
    QVector<int> path;
    for (int v = dest; v != -1; v = prev[v]) path.prepend(v);
    return path;
}

double cycleWeight(const Graph &g, const QVector<int> &cycle) {
    double w = 0.0;
    for (qsizetype i = 0; i + 1 < cycle.size(); ++i) w += *g.edge(cycle[i], cycle[i + 1]);
    return w;
}

// Nodos alcanzables desde start siguiendo las aristas hacia delante
// (forward = true) o hacia atrás (forward = false).
QVector<bool> reachable(const Graph &g, int start, bool forward) {
    const int n = g.nodeCount();
    QVector<bool> seen(n, false);
    QVector<int> stack{start};
    seen[start] = true;
    while (!stack.isEmpty()) {
        const int u = stack.takeLast();
        for (int v = 0; v < n; ++v) {
            const bool arc = forward ? g.hasEdge(u, v) : g.hasEdge(v, u);
            if (arc && !seen[v]) {
                seen[v] = true;
                stack.append(v);
            }
        }
    }
    return seen;
}

// Busca un ciclo negativo formado solo por nodos que se alcanzan desde src
// y desde los que se llega a dest, es decir, un ciclo que afecta al camino.
// Devuelve el ciclo con el primer nodo repetido al final, o vacío si no hay.
QVector<int> findNegativeCycleAffecting(const Graph &g, int src, int dest) {
    const int n = g.nodeCount();
    const QVector<bool> fromSrc = reachable(g, src, true);
    const QVector<bool> toDest = reachable(g, dest, false);
    QVector<Graph::Edge> arcs;
    for (const auto &e : g.arcs())
        if (fromSrc[e.from] && toDest[e.from] && fromSrc[e.to] && toDest[e.to]) arcs.append(e);

    // Bellman-Ford con fuente virtual: todas las distancias parten de 0.
    QVector<double> dist(n, 0.0);
    QVector<int> prev(n, -1);
    int last = -1;
    for (int it = 0; it < n; ++it) {
        last = -1;
        for (const auto &e : arcs) {
            if (dist[e.from] + e.weight < dist[e.to]) {
                dist[e.to] = dist[e.from] + e.weight;
                prev[e.to] = e.from;
                last = e.to;
            }
        }
        if (last == -1) return {};
    }

    // Retroceder n pasos garantiza caer dentro del ciclo.
    int x = last;
    for (int i = 0; i < n && x != -1; ++i) x = prev[x];
    if (x == -1) return {};

    QVector<int> cycle;
    for (int v = x;; v = prev[v]) {
        if (v == -1 || cycle.size() > n) return {};
        cycle.prepend(v);
        if (v == x && cycle.size() > 1) break;
    }
    return cycle;
}

PathResult negativeCycleResult(const Graph &g, int src, int dest) {
    PathResult r;
    r.status = PathResult::NegativeCycle;
    r.path = findNegativeCycleAffecting(g, src, dest);
    if (!r.path.isEmpty()) r.distance = cycleWeight(g, r.path);
    return r;
}
}

PathResult dijkstra(const Graph &g, int src, int dest) {
    const int n = g.nodeCount();
    QVector<double> dist(n, INF);
    QVector<int> prev(n, -1);

    using P = std::pair<double, int>;
    std::priority_queue<P, std::vector<P>, std::greater<P>> pq;

    dist[src] = 0.0;
    pq.push({0.0, src});

    while (!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();
        if (d != dist[u]) continue;
        if (u == dest) break;
        for (int v = 0; v < n; ++v) {
            const auto w = g.edge(u, v);
            if (!w) continue;
            if (dist[u] + *w < dist[v]) {
                dist[v] = dist[u] + *w;
                prev[v] = u;
                pq.push({dist[v], v});
            }
        }
    }

    PathResult r;
    if (dist[dest] == INF) return r;
    r.status = PathResult::Ok;
    r.distance = dist[dest];
    r.path = buildPathFromPrev(prev, dest);
    return r;
}

PathResult bellmanFord(const Graph &g, int src, int dest) {
    const int n = g.nodeCount();
    const QVector<Graph::Edge> arcs = g.arcs();
    QVector<double> dist(n, INF);
    QVector<int> prev(n, -1);

    dist[src] = 0.0;
    for (int it = 0; it < n - 1; ++it) {
        bool changed = false;
        for (const auto &e : arcs) {
            if (dist[e.from] == INF) continue;
            if (dist[e.from] + e.weight < dist[e.to]) {
                dist[e.to] = dist[e.from] + e.weight;
                prev[e.to] = e.from;
                changed = true;
            }
        }
        if (!changed) break;
    }

    // Propagar «-infinito» desde las aristas que aún se relajan: esos nodos
    // tienen distancia no acotada. Si llega al destino, el ciclo afecta al camino.
    QVector<bool> unbounded(n, false);
    for (int it = 0; it < n; ++it) {
        for (const auto &e : arcs) {
            if (dist[e.from] == INF) continue;
            if (unbounded[e.from] || dist[e.from] + e.weight < dist[e.to]) {
                if (!unbounded[e.from]) dist[e.to] = dist[e.from] + e.weight;
                unbounded[e.to] = true;
            }
        }
    }
    if (unbounded[dest]) return negativeCycleResult(g, src, dest);

    PathResult r;
    if (dist[dest] == INF) return r;
    r.status = PathResult::Ok;
    r.distance = dist[dest];
    r.path = buildPathFromPrev(prev, dest);
    return r;
}

PathResult floydWarshall(const Graph &g, int src, int dest) {
    const int n = g.nodeCount();
    QVector<QVector<double>> dist(n, QVector<double>(n, INF));
    QVector<QVector<int>> next(n, QVector<int>(n, -1));

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            if (const auto w = g.edge(i, j)) {
                dist[i][j] = *w;
                next[i][j] = j;
            }
        }
        dist[i][i] = qMin(0.0, dist[i][i]);
        next[i][i] = i;
    }

    for (int k = 0; k < n; ++k)
        for (int i = 0; i < n; ++i) {
            if (dist[i][k] == INF) continue;
            for (int j = 0; j < n; ++j) {
                if (dist[k][j] == INF) continue;
                const double candidate = dist[i][k] + dist[k][j];
                if (candidate < dist[i][j]) {
                    dist[i][j] = candidate;
                    next[i][j] = next[i][k];
                }
            }
        }

    // Hay un ciclo negativo que afecta al camino si algún nodo k de un ciclo
    // negativo se alcanza desde el origen y desde él se llega al destino.
    for (int k = 0; k < n; ++k) {
        if (dist[k][k] < 0 && dist[src][k] != INF && dist[k][dest] != INF)
            return negativeCycleResult(g, src, dest);
    }

    PathResult r;
    if (next[src][dest] == -1) return r;
    r.status = PathResult::Ok;
    r.distance = dist[src][dest];
    int u = src;
    r.path.append(u);
    while (u != dest) {
        u = next[u][dest];
        if (u == -1) break;
        r.path.append(u);
    }
    return r;
}
