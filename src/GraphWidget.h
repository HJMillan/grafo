#ifndef GRAPHWIDGET_H
#define GRAPHWIDGET_H

#include <QFrame>
#include <QPainter>
#include <QMouseEvent>
#include <QVector>

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

    static constexpr int nodeRadius = 22;

    void addNodeVisual(const QString &name, const QPoint &pos);
    void removeLastNode();
    void clearAll();
    void setAdjacency(const QVector<QVector<double>> &adj, int nodeCount, double inf, bool directed);
    void highlightPath(const QVector<int> &path, const QVector<QVector<double>> &adj, double inf, bool directed);
    void highlightPath(const QVector<int> &path); // limpia resaltado si path vacío

signals:
    void nodeClicked(const QPoint &pos);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    QVector<Node> nodes;
    QVector<Edge> edges;
    QVector<QPair<int,int>> highlightedPairs;
    bool directedEdges = false;
};

#endif // GRAPHWIDGET_H
