#ifndef GRAPHWIDGET_H
#define GRAPHWIDGET_H

#include <QFrame>
#include <QMouseEvent>
#include <QPair>
#include <QVector>

class Graph;

// Lienzo que dibuja un Graph. No guarda copia de nodos ni aristas:
// siempre pinta el estado actual del modelo.
class GraphWidget : public QFrame {
    Q_OBJECT
public:
    explicit GraphWidget(QWidget *parent = nullptr);

    static constexpr int nodeRadius = 22;

    void setGraph(const Graph *graph);
    void highlightPath(const QVector<int> &path); // limpia el resaltado si path está vacío

signals:
    void canvasClicked(const QPoint &pos);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    const Graph *m_graph = nullptr;
    QVector<QPair<int, int>> m_highlighted;
};

#endif // GRAPHWIDGET_H
