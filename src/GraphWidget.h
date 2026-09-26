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
    enum class Highlight { Path, NegativeCycle };

    // Resalta las aristas consecutivas de path; lo limpia si path está vacío.
    void highlightPath(const QVector<int> &path, Highlight kind = Highlight::Path);

signals:
    void canvasClicked(const QPoint &pos);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    const Graph *m_graph = nullptr;
    QVector<QPair<int, int>> m_highlighted;
    Highlight m_highlightKind = Highlight::Path;
};

#endif // GRAPHWIDGET_H
