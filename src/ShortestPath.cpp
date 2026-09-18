#include "ShortestPath.h"
#include "GraphConstants.h"

#include <algorithm>
#include <cmath>
#include <queue>
#include <utility>
#include <vector>

namespace ShortestPath {
namespace {

using GraphConstants::isMissing;
using GraphConstants::kEps;
using GraphConstants::sameWeight;

struct Edge {
    int u;
    int v;
    double w;
};

std::vector<Edge> collectEdges(const std::vector<std::vector<double>> &adj, int n, double inf) {
    std::vector<Edge> edges;
    edges.reserve(static_cast<size_t>(n * n));
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            if (!isMissing(adj[static_cast<size_t>(i)][static_cast<size_t>(j)], inf)) {
                edges.push_back({i, j, adj[static_cast<size_t>(i)][static_cast<size_t>(j)]});
            }
        }
    }
    return edges;
}

std::vector<int> buildPathFromPrev(const std::vector<int> &prev, int dest) {
    std::vector<int> path;
    if (dest < 0 || dest >= static_cast<int>(prev.size())) return path;

    std::vector<bool> seen(prev.size(), false);
    for (int v = dest; v != -1; v = prev[static_cast<size_t>(v)]) {
        if (v < 0 || v >= static_cast<int>(prev.size()) || seen[static_cast<size_t>(v)]) {
            path.clear();
            return path;
        }
        seen[static_cast<size_t>(v)] = true;
        path.insert(path.begin(), v);
    }
    return path;
}

double closedWalkWeight(const std::vector<std::vector<double>> &adj, const std::vector<int> &cycle,
                        double inf) {
    if (cycle.size() < 2) return inf;
    double weight = 0.0;
    for (size_t i = 0; i + 1 < cycle.size(); ++i) {
        const int u = cycle[i];
        const int v = cycle[i + 1];
        if (u < 0 || v < 0 || u >= static_cast<int>(adj.size())
            || v >= static_cast<int>(adj[static_cast<size_t>(u)].size())) {
            return inf;
        }
        const double w = adj[static_cast<size_t>(u)][static_cast<size_t>(v)];
        if (isMissing(w, inf)) return inf;
        weight += w;
    }
    return weight;
}

std::vector<int> walkNegativeCycle(const std::vector<int> &prev, int start,
                                   const std::vector<std::vector<double>> &adj, double inf) {
    const int n = static_cast<int>(prev.size());
    if (start < 0 || start >= n) return {};

    int inCycle = start;
    for (int i = 0; i < n; ++i) {
        if (inCycle < 0 || inCycle >= n) return {};
        inCycle = prev[static_cast<size_t>(inCycle)];
    }
    if (inCycle < 0 || inCycle >= n) return {};

    std::vector<int> cycle;
    int v = inCycle;
    for (int i = 0; i <= n; ++i) {
        cycle.push_back(v);
        v = prev[static_cast<size_t>(v)];
        if (v < 0 || v >= n) return {};
        if (v == inCycle) {
            cycle.push_back(inCycle);
            std::reverse(cycle.begin(), cycle.end());
            if (closedWalkWeight(adj, cycle, inf) < 0) return cycle;
            return {};
        }
    }
    return {};
}

std::vector<int> reconstructCycle(const std::vector<int> &prev, const std::vector<int> &starts,
                                  const std::vector<std::vector<double>> &adj, double inf) {
    for (int start : starts) {
        std::vector<int> cycle = walkNegativeCycle(prev, start, adj, inf);
        if (!cycle.empty()) return cycle;
    }
    return {};
}

std::vector<int> extraPassRelaxed(const std::vector<Edge> &edges, std::vector<double> &dist,
                                  std::vector<int> &prev, double inf) {
    std::vector<int> relaxed;
    std::vector<char> seen(dist.size(), 0);
    for (const auto &e : edges) {
        if (isMissing(dist[static_cast<size_t>(e.u)], inf)) continue;
        if (dist[static_cast<size_t>(e.u)] + e.w < dist[static_cast<size_t>(e.v)]) {
            dist[static_cast<size_t>(e.v)] = dist[static_cast<size_t>(e.u)] + e.w;
            prev[static_cast<size_t>(e.v)] = e.u;
            if (!seen[static_cast<size_t>(e.v)]) {
                seen[static_cast<size_t>(e.v)] = 1;
                relaxed.push_back(e.v);
            }
        }
    }
    return relaxed;
}

void bellmanFordRelax(const std::vector<Edge> &edges, int n, double inf, std::vector<double> &dist,
                      std::vector<int> &prev) {
    for (int it = 0; it < n - 1; ++it) {
        bool changed = false;
        for (const auto &e : edges) {
            if (isMissing(dist[static_cast<size_t>(e.u)], inf)) continue;
            if (dist[static_cast<size_t>(e.u)] + e.w < dist[static_cast<size_t>(e.v)]) {
                dist[static_cast<size_t>(e.v)] = dist[static_cast<size_t>(e.u)] + e.w;
                prev[static_cast<size_t>(e.v)] = e.u;
                changed = true;
            }
        }
        if (!changed) break;
    }
}

std::vector<int> cycleVertices(const std::vector<int> &cycle) {
    if (cycle.empty()) return {};
    std::vector<int> nodes = cycle;
    if (nodes.size() >= 2 && nodes.front() == nodes.back()) nodes.pop_back();
    return nodes;
}

bool reachesFrom(const std::vector<std::vector<double>> &adj, int n, double inf,
                 const std::vector<int> &starts, int dest) {
    if (starts.empty()) return false;

    std::vector<char> vis(static_cast<size_t>(n), 0);
    std::queue<int> q;
    for (int s : starts) {
        if (s < 0 || s >= n || vis[static_cast<size_t>(s)]) continue;
        vis[static_cast<size_t>(s)] = 1;
        q.push(s);
        if (s == dest) return true;
    }

    while (!q.empty()) {
        const int u = q.front();
        q.pop();
        for (int v = 0; v < n; ++v) {
            if (vis[static_cast<size_t>(v)]) continue;
            if (isMissing(adj[static_cast<size_t>(u)][static_cast<size_t>(v)], inf)) continue;
            if (v == dest) return true;
            vis[static_cast<size_t>(v)] = 1;
            q.push(v);
        }
    }
    return false;
}

PathResult negativeCycleResult(const std::vector<int> &cycle) {
    PathResult result;
    result.ok = false;
    result.negativeCycle = true;
    result.cycle = cycle;
    result.reconstructed = !cycle.empty();
    return result;
}

std::vector<int> findNegativeCycleFrom(const std::vector<std::vector<double>> &adj, int n,
                                       double inf, int src) {
    if (n <= 0 || src < 0 || src >= n) return {};
    const std::vector<Edge> edges = collectEdges(adj, n, inf);
    std::vector<double> dist(static_cast<size_t>(n), inf);
    std::vector<int> prev(static_cast<size_t>(n), -1);
    dist[static_cast<size_t>(src)] = 0.0;
    bellmanFordRelax(edges, n, inf, dist, prev);
    const std::vector<int> relaxed = extraPassRelaxed(edges, dist, prev, inf);
    return reconstructCycle(prev, relaxed, adj, inf);
}

std::vector<int> findAnyNegativeCycle(const std::vector<std::vector<double>> &adj, int n,
                                      double inf) {
    if (n <= 0) return {};
    const std::vector<Edge> edges = collectEdges(adj, n, inf);
    std::vector<double> dist(static_cast<size_t>(n), 0.0);
    std::vector<int> prev(static_cast<size_t>(n), -1);
    bellmanFordRelax(edges, n, inf, dist, prev);
    const std::vector<int> relaxed = extraPassRelaxed(edges, dist, prev, inf);
    return reconstructCycle(prev, relaxed, adj, inf);
}

} // namespace

int countNegativeEdges(const std::vector<std::vector<double>> &adj, int n, double inf) {
    int neg = 0;
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            const double w = adj[static_cast<size_t>(i)][static_cast<size_t>(j)];
            if (isMissing(w, inf)) continue;
            if (w < 0) {
                if (j < i && !isMissing(adj[static_cast<size_t>(j)][static_cast<size_t>(i)], inf)
                    && sameWeight(w, adj[static_cast<size_t>(j)][static_cast<size_t>(i)])) {
                    continue;
                }
                ++neg;
            }
        }
    }
    return neg;
}

PathResult dijkstra(const std::vector<std::vector<double>> &adj, int n, double inf,
                    int src, int dest) {
    PathResult result;
    std::vector<double> dist(static_cast<size_t>(n), inf);
    std::vector<int> prev(static_cast<size_t>(n), -1);

    using P = std::pair<double, int>;
    auto cmp = [](const P &a, const P &b) { return a.first > b.first; };
    std::priority_queue<P, std::vector<P>, decltype(cmp)> pq(cmp);

    dist[static_cast<size_t>(src)] = 0.0;
    pq.push({0.0, src});

    while (!pq.empty()) {
        const auto [d, u] = pq.top();
        pq.pop();
        if (d > dist[static_cast<size_t>(u)] + kEps) continue;
        if (u == dest) break;
        for (int v = 0; v < n; ++v) {
            const double w = adj[static_cast<size_t>(u)][static_cast<size_t>(v)];
            if (isMissing(w, inf)) continue;
            if (dist[static_cast<size_t>(u)] + w < dist[static_cast<size_t>(v)]) {
                dist[static_cast<size_t>(v)] = dist[static_cast<size_t>(u)] + w;
                prev[static_cast<size_t>(v)] = u;
                pq.push({dist[static_cast<size_t>(v)], v});
            }
        }
    }

    if (isMissing(dist[static_cast<size_t>(dest)], inf)) {
        result.ok = false;
        return result;
    }

    result.ok = true;
    result.path = buildPathFromPrev(prev, dest);
    result.distance = dist[static_cast<size_t>(dest)];
    result.reconstructed = !result.path.empty();
    return result;
}

PathResult bellmanFord(const std::vector<std::vector<double>> &adj, int n, double inf,
                       int src, int dest) {
    PathResult result;
    std::vector<double> dist(static_cast<size_t>(n), inf);
    std::vector<int> prev(static_cast<size_t>(n), -1);
    const std::vector<Edge> edges = collectEdges(adj, n, inf);

    dist[static_cast<size_t>(src)] = 0.0;
    bellmanFordRelax(edges, n, inf, dist, prev);

    std::vector<int> prevCycle = prev;
    std::vector<double> distCycle = dist;
    const std::vector<int> relaxed = extraPassRelaxed(edges, distCycle, prevCycle, inf);

    if (!relaxed.empty()) {
        const std::vector<int> cycle = reconstructCycle(prevCycle, relaxed, adj, inf);
        if (cycle.empty()) {
            return negativeCycleResult({});
        }
        if (reachesFrom(adj, n, inf, cycleVertices(cycle), dest)) {
            return negativeCycleResult(cycle);
        }
    }

    if (isMissing(dist[static_cast<size_t>(dest)], inf)) {
        result.ok = false;
        return result;
    }

    result.ok = true;
    result.path = buildPathFromPrev(prev, dest);
    result.distance = dist[static_cast<size_t>(dest)];
    result.reconstructed = !result.path.empty();
    return result;
}

PathResult floydWarshall(const std::vector<std::vector<double>> &adj, int n, double inf,
                         int src, int dest) {
    PathResult result;
    std::vector<std::vector<double>> dist(static_cast<size_t>(n),
                                          std::vector<double>(static_cast<size_t>(n), inf));
    std::vector<std::vector<int>> next(static_cast<size_t>(n),
                                       std::vector<int>(static_cast<size_t>(n), -1));

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            dist[static_cast<size_t>(i)][static_cast<size_t>(j)] =
                    adj[static_cast<size_t>(i)][static_cast<size_t>(j)];
            if (!isMissing(adj[static_cast<size_t>(i)][static_cast<size_t>(j)], inf)) {
                next[static_cast<size_t>(i)][static_cast<size_t>(j)] = j;
            }
        }
        dist[static_cast<size_t>(i)][static_cast<size_t>(i)] =
                std::min(0.0, dist[static_cast<size_t>(i)][static_cast<size_t>(i)]);
        next[static_cast<size_t>(i)][static_cast<size_t>(i)] = i;
    }

    for (int k = 0; k < n; ++k) {
        for (int i = 0; i < n; ++i) {
            if (isMissing(dist[static_cast<size_t>(i)][static_cast<size_t>(k)], inf)) continue;
            for (int j = 0; j < n; ++j) {
                if (isMissing(dist[static_cast<size_t>(k)][static_cast<size_t>(j)], inf)) continue;
                const double candidate = dist[static_cast<size_t>(i)][static_cast<size_t>(k)]
                        + dist[static_cast<size_t>(k)][static_cast<size_t>(j)];
                if (candidate < dist[static_cast<size_t>(i)][static_cast<size_t>(j)]) {
                    dist[static_cast<size_t>(i)][static_cast<size_t>(j)] = candidate;
                    next[static_cast<size_t>(i)][static_cast<size_t>(j)] =
                            next[static_cast<size_t>(i)][static_cast<size_t>(k)];
                }
            }
        }
    }

    int affecting = -1;
    for (int k = 0; k < n; ++k) {
        if (dist[static_cast<size_t>(k)][static_cast<size_t>(k)] >= 0) continue;
        if (isMissing(dist[static_cast<size_t>(src)][static_cast<size_t>(k)], inf)) continue;
        if (isMissing(dist[static_cast<size_t>(k)][static_cast<size_t>(dest)], inf)) continue;
        affecting = k;
        break;
    }

    if (affecting >= 0) {
        std::vector<int> cycle = findNegativeCycleFrom(adj, n, inf, affecting);
        if (cycle.empty()) cycle = findAnyNegativeCycle(adj, n, inf);
        return negativeCycleResult(cycle);
    }

    if (next[static_cast<size_t>(src)][static_cast<size_t>(dest)] == -1) {
        result.ok = false;
        return result;
    }

    std::vector<int> path;
    std::vector<bool> seen(static_cast<size_t>(n), false);
    int u = src;
    path.push_back(u);
    seen[static_cast<size_t>(u)] = true;
    while (u != dest) {
        u = next[static_cast<size_t>(u)][static_cast<size_t>(dest)];
        if (u < 0 || seen[static_cast<size_t>(u)]) {
            result.ok = false;
            result.reconstructed = false;
            return result;
        }
        seen[static_cast<size_t>(u)] = true;
        path.push_back(u);
    }

    result.ok = true;
    result.path = std::move(path);
    result.distance = dist[static_cast<size_t>(src)][static_cast<size_t>(dest)];
    return result;
}

} // namespace ShortestPath
