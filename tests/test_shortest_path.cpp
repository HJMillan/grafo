#include "ShortestPath.h"
#include "GraphConstants.h"
#include "test_support.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

void runGraphModelTests();

namespace {

std::vector<std::vector<double>> makeAdj(int n) {
    return std::vector<std::vector<double>>(static_cast<size_t>(n),
                                            std::vector<double>(static_cast<size_t>(n), GraphConstants::INF));
}

void addEdge(std::vector<std::vector<double>> &adj, int u, int v, double w, bool bidirectional) {
    adj[static_cast<size_t>(u)][static_cast<size_t>(v)] = w;
    if (bidirectional) adj[static_cast<size_t>(v)][static_cast<size_t>(u)] = w;
}

std::string pathStr(const std::vector<int> &p) {
    std::string s = "[";
    for (size_t i = 0; i < p.size(); ++i) {
        if (i) s += ",";
        s += std::to_string(p[i]);
    }
    s += "]";
    return s;
}

bool cycleContains(const std::vector<int> &cycle, const std::vector<int> &need) {
    std::vector<int> nodes = cycle;
    if (nodes.size() >= 2 && nodes.front() == nodes.back()) nodes.pop_back();
    for (int x : need) {
        if (std::find(nodes.begin(), nodes.end(), x) == nodes.end()) return false;
    }
    return true;
}

std::vector<std::vector<double>> pathABC() {
    auto adj = makeAdj(3);
    addEdge(adj, 0, 1, 1.0, true);
    addEdge(adj, 1, 2, 1.0, true);
    return adj;
}

std::vector<std::vector<double>> cycleCornerAndPath() {
    auto adj = makeAdj(4);
    addEdge(adj, 0, 1, 1.0, false);
    addEdge(adj, 1, 2, -1.0, false);
    addEdge(adj, 2, 1, -1.0, false);
    addEdge(adj, 0, 3, 4.0, false);
    return adj;
}

} // namespace

int main() {
    std::cout << "ShortestPath tests\n";

    {
        const auto adj = pathABC();
        const auto d = ShortestPath::dijkstra(adj, 3, GraphConstants::INF, 0, 2);
        std::string errors;
        check(d.ok, errors, "expected a path");
        check(std::abs(d.distance - 2.0) < 1e-9, errors, "distance");
        check(d.path == std::vector<int>({0, 1, 2}), errors, "path " + pathStr(d.path));
        finishCase("D1", errors);
    }

    {
        auto adj = makeAdj(2);
        const auto d = ShortestPath::dijkstra(adj, 2, GraphConstants::INF, 0, 1);
        std::string errors;
        check(!d.ok && !d.negativeCycle, errors, "expected unreachable");
        finishCase("D2", errors);
    }

    {
        auto adj = makeAdj(2);
        addEdge(adj, 0, 1, -1.0, true);
        std::string errors;
        const int neg = ShortestPath::countNegativeEdges(adj, 2, GraphConstants::INF);
        check(neg == 1, errors, "negative edge count got " + std::to_string(neg));
        finishCase("D3", errors);
    }

    {
        const auto adj = pathABC();
        const auto r = ShortestPath::bellmanFord(adj, 3, GraphConstants::INF, 0, 2);
        std::string errors;
        check(r.ok, errors, "expected a path");
        check(std::abs(r.distance - 2.0) < 1e-9, errors, "distance");
        check(r.path == std::vector<int>({0, 1, 2}), errors, "path " + pathStr(r.path));
        finishCase("BF1", errors);
    }

    {
        auto adj = makeAdj(2);
        addEdge(adj, 0, 1, 1.0, false);
        addEdge(adj, 1, 0, -2.0, false);
        const auto r = ShortestPath::bellmanFord(adj, 2, GraphConstants::INF, 0, 1);
        std::string errors;
        check(r.negativeCycle, errors, "expected a negative cycle");
        finishCase("BF2", errors);
    }

    {
        const auto adj = pathABC();
        const auto r = ShortestPath::floydWarshall(adj, 3, GraphConstants::INF, 0, 2);
        std::string errors;
        check(r.ok, errors, "expected a path");
        check(std::abs(r.distance - 2.0) < 1e-9, errors, "distance");
        check(r.path == std::vector<int>({0, 1, 2}), errors, "path " + pathStr(r.path));
        finishCase("FW1", errors);
    }

    {
        const auto adj = pathABC();
        const auto r = ShortestPath::floydWarshall(adj, 3, GraphConstants::INF, 1, 1);
        std::string errors;
        check(r.ok, errors, "expected a trivial path");
        check(std::abs(r.distance - 0.0) < 1e-9, errors, "distance");
        check(r.path == std::vector<int>({1}), errors, "path " + pathStr(r.path));
        finishCase("FW2", errors);
    }

    {
        auto adj = makeAdj(11);
        for (int i = 0; i < 10; ++i) addEdge(adj, i, i + 1, 0.1, false);
        const auto d = ShortestPath::dijkstra(adj, 11, GraphConstants::INF, 0, 10);
        std::string errors;
        check(d.ok, errors, "expected a path");
        check(std::abs(d.distance - 1.0) < 1e-9, errors, "distance");
        finishCase("D-float", errors);
    }

    {
        auto adj = makeAdj(4);
        addEdge(adj, 0, 1, 1.0, false);
        addEdge(adj, 1, 2, 1.0, false);
        addEdge(adj, 2, 0, -3.0, false);
        addEdge(adj, 0, 3, 8.0, false);
        const auto r = ShortestPath::bellmanFord(adj, 4, GraphConstants::INF, 0, 0);
        std::string errors;
        check(r.negativeCycle, errors, "expected a negative cycle");
        check(r.reconstructed, errors, "cycle should reconstruct");
        check(cycleContains(r.cycle, {0, 1, 2}), errors, "cycle nodes " + pathStr(r.cycle));
        finishCase("BF-walk", errors);
    }

    {
        const auto adj = cycleCornerAndPath();
        const auto r = ShortestPath::bellmanFord(adj, 4, GraphConstants::INF, 0, 3);
        std::string errors;
        check(r.ok, errors, "path to dest should survive");
        check(!r.negativeCycle, errors, "cycle does not reach dest");
        check(std::abs(r.distance - 4.0) < 1e-9, errors, "distance");
        check(r.path == std::vector<int>({0, 3}), errors, "path " + pathStr(r.path));
        finishCase("BF3", errors);
    }

    {
        const auto adj = cycleCornerAndPath();
        const auto r = ShortestPath::bellmanFord(adj, 4, GraphConstants::INF, 0, 1);
        std::string errors;
        check(r.negativeCycle, errors, "dest reachable from a negative cycle");
        finishCase("BF3-hit", errors);
    }

    {
        const auto adj = cycleCornerAndPath();
        const auto r = ShortestPath::floydWarshall(adj, 4, GraphConstants::INF, 0, 3);
        std::string errors;
        check(r.ok, errors, "path to dest should survive");
        check(!r.negativeCycle, errors, "cycle does not reach query");
        check(std::abs(r.distance - 4.0) < 1e-9, errors, "distance");
        check(r.path == std::vector<int>({0, 3}), errors, "path " + pathStr(r.path));
        finishCase("FW3", errors);
    }

    {
        const auto adj = cycleCornerAndPath();
        const auto r = ShortestPath::floydWarshall(adj, 4, GraphConstants::INF, 0, 1);
        std::string errors;
        check(r.negativeCycle, errors, "query reaches a negative cycle");
        finishCase("FW3-hit", errors);
    }

    runGraphModelTests();

    std::cout << "\n" << g_passed << " passed, " << g_failed << " failed\n";
    return g_failed == 0 ? 0 : 1;
}
