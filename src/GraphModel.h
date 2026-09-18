#ifndef GRAPHMODEL_H
#define GRAPHMODEL_H

#include "GraphConstants.h"

#include <QHash>
#include <QPoint>
#include <QString>
#include <QVector>

#include <vector>

struct UserEdge {
    int from = -1;
    int to = -1;
    double weight = 0.0;
};

struct GraphSnapshot {
    QVector<QString> nodeNames;
    QVector<QPoint> positions;
    QVector<UserEdge> userEdges;
    bool directed = false;
};

class GraphModel {
public:
    static constexpr int MAX_NODES = 26;
    static constexpr double INF = GraphConstants::INF;
    static constexpr int MAX_UNDO = 64;

    GraphModel();

    int nodeCount() const { return nodeNames_.size(); }
    bool isEmpty() const { return nodeNames_.isEmpty() && userEdges_.isEmpty(); }
    bool isDirected() const { return directed_; }
    const QVector<QString> &nodeNames() const { return nodeNames_; }
    const QVector<QPoint> &positions() const { return positions_; }
    const QVector<UserEdge> &userEdges() const { return userEdges_; }
    const QVector<QVector<double>> &adjacency() const { return adj_; }
    std::vector<std::vector<double>> adjacencyStd() const;

    QString nextSuggestedName(bool uppercase) const;
    bool canAddNode(const QString &name, QString &reason) const;
    bool addNode(const QString &name, const QPoint &pos);
    void removeLastNode();
    void clear();
    void setPositions(const QVector<QPoint> &positions);

    int findUserEdge(int from, int to) const;
    bool addEdge(int from, int to, double weight, double *oldWeight = nullptr);
    bool removeEdge(int from, int to);
    QString edgePairLabel(int from, int to) const;

    void setDirected(bool directed);
    QString expandUndirectedEdges();
    QString collapseDirectedEdges();

    void pushUndo();
    bool canUndo() const { return !undoStack_.isEmpty(); }
    bool popUndo(GraphSnapshot &out);

    GraphSnapshot capture() const;
    void apply(const GraphSnapshot &s);

private:
    void rebuildNameIndex();
    void rebuildAdjacency();

    QVector<QString> nodeNames_;
    QVector<QPoint> positions_;
    QHash<QString, int> nameToIndex_;
    QVector<UserEdge> userEdges_;
    QVector<QVector<double>> adj_;
    bool directed_ = false;
    QVector<GraphSnapshot> undoStack_;
};

#endif
