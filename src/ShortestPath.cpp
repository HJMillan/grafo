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

PathResult negativeCycleResult(const Graph &g, QVector<int> cycle) {
    PathResult r;
    r.status = PathResult::NegativeCycle;
    if (!cycle.isEmpty()) r.distance = cycleWeight(g, cycle);
    r.path = std::move(cycle);
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

    int cycleNode = -1;
    for (const auto &e : arcs) {
        if (dist[e.from] != INF && dist[e.from] + e.weight < dist[e.to]) {
            prev[e.to] = e.from;
            cycleNode = e.to;
            break;
        }
    }

    if (cycleNode != -1) {
        // Retroceder n pasos garantiza caer dentro del ciclo.
        for (int i = 0; i < n && cycleNode != -1; ++i) cycleNode = prev[cycleNode];

        QVector<int> cycle;
        if (cycleNode != -1) {
            for (int v = cycleNode;; v = prev[v]) {
                cycle.prepend(v);
                if (v == cycleNode && cycle.size() > 1) break;
                if (prev[v] == -1 || cycle.size() > n + 1) {
                    cycle.clear();
                    break;
                }
            }
        }
        return negativeCycleResult(g, cycle);
    }

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

    for (int i = 0; i < n; ++i) {
        if (dist[i][i] >= 0) continue;
        QVector<int> cycle;
        int u = i;
        do {
            cycle.append(u);
            u = next[u][i];
            if (u == -1 || cycle.size() > n) {
                cycle.clear();
                break;
            }
        } while (u != i);
        if (!cycle.isEmpty()) cycle.append(i);
        return negativeCycleResult(g, cycle);
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
