#ifndef GRAPHWIDGET_H
#define GRAPHWIDGET_H

#include <QFrame>
#include <QPoint>
#include <QString>
#include <QVector>

#include "Theme.h"

class QMouseEvent;
class QPaintEvent;
class QPainter;
class QKeyEvent;
class QResizeEvent;

class GraphWidget : public QFrame {
    Q_OBJECT
public:
    explicit GraphWidget(QWidget *parent = nullptr);

    struct Node {
        QString name;
        QPoint pos;
    };
    struct Edge {
        int from;
        int to;
        double weight;
        bool directed;
        bool hasReverse;
        double reverseWeight;
    };

    static constexpr int nodeRadius = Theme::NodeRadius;
    static constexpr int nodeSeparation = 2 * Theme::NodeRadius + Theme::SpacingBezeled;

    void setNodes(const QVector<QString> &names, const QVector<QPoint> &positions);
    void clearAll();
    void setAdjacency(const QVector<QVector<double>> &adj, int nodeCount, double inf, bool directed);
    void highlightPath(const QVector<int> &path);

    QVector<QPoint> nodePositions() const;
    void restoreNodes(const QVector<QString> &names, const QVector<QPoint> &positions);
    QPoint placeWithoutOverlap(QPoint p) const;
    void clearSelection();
    int selectedIndex() const { return selectedNode; }
    void setQueryEndpoints(int origin, int dest);

signals:
    void canvasClicked(const QPoint &pos);
    void nodeAboutToMove();
    void nodeMoved();
    void positionsChanged();
    void nodesLinked(int from, int to);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    int nodeAt(const QPoint &p) const;
    QPoint clampToCanvas(const QPoint &p) const;
    void clampIndex(int &index) const;
    void drawEmptyState(QPainter &g) const;
    void drawEdges(QPainter &g) const;
    void drawLinkPreview(QPainter &g) const;
    void drawNode(QPainter &g, int index) const;
    void drawNodes(QPainter &g) const;
    void drawQueryBadge(QPainter &g, int index, const QString &text) const;
    void drawQueryBadges(QPainter &g) const;

    QVector<Node> nodes;
    QVector<Edge> edges;
    QVector<QPair<int, int>> highlightedPairs;
    QVector<int> highlightedNodes;
    bool directedEdges = false;

    int draggedNode = -1;
    QPoint dragGrabOffset;
    QPoint dragPressPos;
    bool dragDidMove = false;

    int selectedNode = -1;
    int hoverNode = -1;
    int originNode = -1;
    int destNode = -1;
    QPoint cursorPos;
};

#endif // GRAPHWIDGET_H
