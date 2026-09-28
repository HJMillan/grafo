#ifndef GRAPHWIDGET_H
#define GRAPHWIDGET_H

#include <QFrame>
#include <QMouseEvent>
#include <QPair>
#include <QVector>
#include "Layout.h"

class Graph;
class QPainter;

// Lienzo que dibuja un Graph. No guarda copia de nodos ni aristas:
// siempre pinta el estado actual del modelo.
class GraphWidget : public QFrame {
    Q_OBJECT
public:
    explicit GraphWidget(QWidget *parent = nullptr);

    static constexpr int nodeRadius = 22;
    // Distancia mínima entre centros para que dos nodos no se toquen.
    static constexpr double minNodeDistance = 2.0 * nodeRadius + 6.0;

    CanvasGeometry canvasGeometry() const;

    void setGraph(const Graph *graph);
    const Graph *graph() const { return m_graph; }
    enum class Highlight { Path, NegativeCycle };

    // Resalta las aristas consecutivas de path; lo limpia si path está vacío.
    void highlightPath(const QVector<int> &path, Highlight kind = Highlight::Path);

signals:
    // Clic en un espacio libre: posición normalizada (0..1) para un nodo nuevo.
    void canvasClicked(const QPointF &normalizedPos);
    // Clic demasiado cerca de un nodo existente: no cabe otro ahí.
    void placementBlocked();
    // Empieza el arrastre de un nodo (una vez por arrastre).
    void nodeDragStarted(int index);
    // Arrastre de un nodo a una nueva posición normalizada.
    void nodeDragged(int index, const QPointF &normalizedPos);
    // Clic derecho sobre un nodo.
    void nodeMenuRequested(int index, const QPoint &globalPos);
    // El lienzo cambió de tamaño: los nodos automáticos pueden necesitar recolocarse.
    void canvasResized();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    void drawEmptyState(QPainter &g) const;
    void drawEdges(QPainter &g) const;
    void drawNodes(QPainter &g) const;

    const Graph *m_graph = nullptr;
    QVector<QPair<int, int>> m_highlighted;
    Highlight m_highlightKind = Highlight::Path;
    QVector<int> m_highlightedNodes;
    int m_hoverNode = -1;

    static constexpr int dragThreshold = 4;
    int m_pressedNode = -1;
    QPointF m_pressPos;
    bool m_dragging = false;
};

#endif // GRAPHWIDGET_H
