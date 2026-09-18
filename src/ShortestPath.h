#ifndef SHORTESTPATH_H
#define SHORTESTPATH_H

#include <vector>

namespace ShortestPath {

struct PathResult {
    bool ok = false;
    bool negativeCycle = false;
    bool reconstructed = true;
    std::vector<int> path;
    std::vector<int> cycle;
    double distance = 0.0;
};

PathResult dijkstra(const std::vector<std::vector<double>> &adj, int n, double inf,
                    int src, int dest);
PathResult bellmanFord(const std::vector<std::vector<double>> &adj, int n, double inf,
                       int src, int dest);
PathResult floydWarshall(const std::vector<std::vector<double>> &adj, int n, double inf,
                         int src, int dest);
int countNegativeEdges(const std::vector<std::vector<double>> &adj, int n, double inf);

} // namespace ShortestPath

#endif
