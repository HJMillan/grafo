#include "GraphWidget.h"
#include "Graph.h"
#include "Messages.h"
#include "Theme.h"
#include <QContextMenuEvent>
#include <QPainter>
#include <QPen>
#include <QFont>
#include <QFontMetrics>
#include <QPainterPath>
#include <QtMath>
#include <QtGlobal>

namespace {
// Arista tal como se dibuja: en modo dirigido, ida y vuelta se combinan
// en una sola línea con ambos pesos.
struct DrawEdge {
    int from;
    int to;
    double weight;
    bool directed;
    bool hasReverse;
    double reverseWeight;
};

// Círculo del lazo, desplazado 10 px hacia arriba (o hacia abajo) del nodo.
QRectF selfLoopRect(const QPointF &center, bool below) {
    const double r = GraphWidget::nodeRadius + 12;
    const double cy = center.y() + (below ? 10 : -10);
    return QRectF(center.x() - r, cy - r, 2 * r, 2 * r);
}

QVector<DrawEdge> buildDrawEdges(const Graph &g) {
    QVector<DrawEdge> out;
    if (!g.isDirected()) {
        for (const auto &e : g.edgeList())
            out.append({e.from, e.to, e.weight, false, false, 0.0});
        return out;
    }
    for (const auto &e : g.arcs()) {
        const auto rev = g.edge(e.to, e.from);
        if (e.from != e.to && rev && e.to < e.from) continue; // ya combinada
        const bool hasRev = e.from != e.to && rev.has_value();
        out.append({e.from, e.to, e.weight, true, hasRev, hasRev ? *rev : 0.0});
    }
    return out;
}
}

GraphWidget::GraphWidget(QWidget *parent) : QFrame(parent) {
    setFrameShape(QFrame::NoFrame);
    setMouseTracking(true); // para mostrar la mano al pasar sobre un nodo
}

CanvasGeometry GraphWidget::canvasGeometry() const {
    return {QSizeF(size()), nodeRadius + 4.0};
}

void GraphWidget::setGraph(const Graph *graph) {
    m_graph = graph;
    update();
}

void GraphWidget::highlightPath(const QVector<int> &path, Highlight kind) {
    m_highlightKind = kind;
    m_highlighted.clear();
    for (int i = 0; i + 1 < path.size(); ++i) m_highlighted.append({path[i], path[i + 1]});
    m_highlightedNodes = path;
    update();
}

void GraphWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter g(this);
    g.setRenderHint(QPainter::Antialiasing, true);
    g.setRenderHint(QPainter::TextAntialiasing, true);

    // Lienzo con esquinas redondeadas; fuera de ellas se ve el fondo de la ventana.
    QPainterPath clip;
    clip.addRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5),
                        Theme::CanvasRadius, Theme::CanvasRadius);
    g.setClipPath(clip);
    g.fillRect(rect(), Theme::canvasBackground());

    if (!m_graph || m_graph->nodeCount() == 0) {
        drawEmptyState(g);
        return;
    }
    drawEdges(g);
    drawNodes(g);
}

void GraphWidget::drawEmptyState(QPainter &g) const {
    QFont titleFont = Theme::headline();
    titleFont.setPointSizeF(15.0);
    const QFont captionFont = Theme::caption();
    const QString title = QStringLiteral("El lienzo está vacío");
    const QString caption = QStringLiteral("Haga clic aquí para crear un nodo, o use «Agregar nodo»");

    const QFontMetrics titleFm(titleFont);
    const QFontMetrics captionFm(captionFont);
    const int blockHeight = titleFm.height() + 6 + captionFm.height();
    const int top = rect().center().y() - blockHeight / 2;

    g.setFont(titleFont);
    g.setPen(Theme::emptyStateLabel());
    g.drawText(QRect(20, top, width() - 40, titleFm.height()), Qt::AlignCenter, title);
    g.setFont(captionFont);
    g.setPen(Theme::emptyStateCaption());
    g.drawText(QRect(20, top + titleFm.height() + 6, width() - 40, captionFm.height()),
               Qt::AlignCenter, caption);
}

void GraphWidget::drawEdges(QPainter &g) const {
    const auto &nodes = m_graph->nodes();
    const CanvasGeometry geo = canvasGeometry();
    const QColor edgeColor = Theme::edge();
    const QColor edgeHiColor = m_highlightKind == Highlight::NegativeCycle ? Theme::cycleHighlight()
                                                                           : Theme::pathHighlight();
    const int edgeW = 2;
    const int edgeHiW = 4;
    const QFont wFont = Theme::caption();
    const QFontMetrics wfm(wFont);

    for (const DrawEdge &e : buildDrawEdges(*m_graph)) {
        const QPointF ap = geo.toPixel(nodes[e.from].pos);
        const QPointF bp = geo.toPixel(nodes[e.to].pos);

        const bool hi = m_highlighted.contains({e.from, e.to}) ||
                        (!e.directed && m_highlighted.contains({e.to, e.from})) ||
                        (e.hasReverse && m_highlighted.contains({e.to, e.from}));

        const bool selfLoop = (e.from == e.to);
        // Cerca del borde superior el lazo y su etiqueta se dibujan debajo del nodo.
        const bool loopBelow = ap.y() - nodeRadius - 45 < 0;

        const auto strokeEdge = [&] {
            if (selfLoop)
                g.drawArc(selfLoopRect(ap, loopBelow), 45 * 16, 270 * 16);
            else
                g.drawLine(ap, bp);
        };

        if (hi) {
            QColor glowC = edgeHiColor;
            glowC.setAlpha(60);
            g.setPen(QPen(glowC, edgeHiW + 6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            strokeEdge();
        }
        g.setPen(QPen(hi ? edgeHiColor : edgeColor, hi ? edgeHiW : edgeW,
                      Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        g.setBrush(Qt::NoBrush);
        strokeEdge();

        const auto drawArrow = [&](const QPointF &fromP, const QPointF &toP) {
            QLineF line(fromP, toP);
            if (line.length() <= 1e-3) return;
            const double arrowSize = 10.0;
            line.setLength(line.length() - nodeRadius + 2);
            const QPointF end = line.p2();
            line.setLength(line.length() - arrowSize);
            const QPointF back = line.p2();
            QLineF normal = line.normalVector();
            normal.setLength(arrowSize / 2.0);
            const QPointF p1 = back + normal.p2() - normal.p1();
            const QPointF p2 = back - (normal.p2() - normal.p1());

            QPainterPath path;
            path.moveTo(end);
            path.lineTo(p1);
            path.lineTo(p2);
            path.closeSubpath();
            g.setBrush(hi ? edgeHiColor : edgeColor);
            g.drawPath(path);
        };
        if (e.directed && !selfLoop) {
            drawArrow(ap, bp);
            if (e.hasReverse) drawArrow(bp, ap);
        }

        // Etiqueta del peso
        const QLineF line(ap, bp);
        const qreal len = line.length();
        const QPointF mid = line.pointAt(0.5);
        QPointF n(0, 0.5);
        if (len > 0.0001) n = QPointF(-line.dy() / len, line.dx() / len);

        QString txt;
        if (selfLoop) {
            txt = QString("%1→%1  %2").arg(nodes[e.from].name, Msg::number(e.weight));
        } else if (e.directed && e.hasReverse) {
            txt = QString("%1→%2  %3   %2→%1  %4")
                    .arg(nodes[e.from].name, nodes[e.to].name,
                         Msg::number(e.weight), Msg::number(e.reverseWeight));
        } else {
            txt = Msg::number(e.weight);
        }
        const QSize ts = wfm.size(Qt::TextSingleLine, txt);
        const int pad = 7;
        QRect pill(0, 0, ts.width() + pad * 2, ts.height() + pad);
        if (selfLoop) {
            const double dy = nodeRadius + 28;
            pill.moveCenter(QPointF(ap.x(), loopBelow ? ap.y() + dy : ap.y() - dy).toPoint());
        } else {
            pill.moveCenter((mid + n * 16.0).toPoint());
        }

        QPainterPath pillPath;
        pillPath.addRoundedRect(pill, 10, 10);
        g.setPen(Qt::NoPen);
        g.setBrush(Theme::pillFill());
        g.drawPath(pillPath);
        g.setPen(QPen(hi ? edgeHiColor : Theme::pillStroke(), 1));
        g.setBrush(Qt::NoBrush);
        g.drawPath(pillPath);
        g.setPen(Theme::pillLabel());
        g.setFont(wFont);
        g.drawText(pill, Qt::AlignCenter, txt);
    }
}

void GraphWidget::drawNodes(QPainter &g) const {
    const CanvasGeometry geo = canvasGeometry();
    const QColor pathColor = m_highlightKind == Highlight::NegativeCycle ? Theme::cycleHighlight()
                                                                         : Theme::pathHighlight();
    g.setFont(Theme::headline());
    for (int i = 0; i < m_graph->nodeCount(); ++i) {
        const Graph::Node &n = m_graph->node(i);
        const QPointF c = geo.toPixel(n.pos);
        const QRectF circle(c.x() - nodeRadius, c.y() - nodeRadius, nodeRadius * 2, nodeRadius * 2);

        const QColor base = Theme::nodeFill();
        QRadialGradient radial(c, nodeRadius);
        radial.setColorAt(0.0, base.lighter(105));
        radial.setColorAt(0.7, base);
        radial.setColorAt(1.0, base.darker(107));

        // Borde: arrastrando o bajo el cursor (acento), en el camino/ciclo, o normal.
        QColor stroke = Theme::nodeStroke();
        int strokeW = 2;
        if (m_dragging && i == m_pressedNode) {
            stroke = Theme::accent();
            strokeW = 3;
        } else if (m_highlightedNodes.contains(i)) {
            stroke = pathColor;
            strokeW = 3;
        } else if (i == m_hoverNode) {
            stroke = Theme::accent();
        }

        g.setPen(QPen(stroke, strokeW));
        g.setBrush(radial);
        g.drawEllipse(circle);
        g.setPen(Theme::nodeLabel());
        g.drawText(circle, Qt::AlignCenter, n.name);
    }
}

void GraphWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && m_graph) {
        const QPointF p = event->position();
        const CanvasGeometry geo = canvasGeometry();
        const int onNode = nodeNear(*m_graph, geo, p, nodeRadius);
        if (onNode >= 0) {
            // Sobre un nodo: puede empezar un arrastre; nunca crea otro encima.
            m_pressedNode = onNode;
            m_pressPos = p;
        } else if (nodeNear(*m_graph, geo, p, minNodeDistance) >= 0) {
            emit placementBlocked();
        } else {
            emit canvasClicked(geo.toNormalized(p));
        }
    }
    QFrame::mousePressEvent(event);
}

void GraphWidget::mouseMoveEvent(QMouseEvent *event) {
    if (!m_graph) return;
    const QPointF p = event->position();
    if (m_pressedNode >= 0 && (event->buttons() & Qt::LeftButton)) {
        // Un umbral pequeño distingue un clic de un arrastre.
        if (!m_dragging && (p - m_pressPos).manhattanLength() > dragThreshold) {
            m_dragging = true;
            setCursor(Qt::ClosedHandCursor);
            emit nodeDragStarted(m_pressedNode);
        }
        if (m_dragging) emit nodeDragged(m_pressedNode, canvasGeometry().toNormalized(p));
        return;
    }
    const int hover = nodeNear(*m_graph, canvasGeometry(), p, nodeRadius);
    setCursor(hover >= 0 ? Qt::OpenHandCursor : Qt::ArrowCursor);
    if (hover != m_hoverNode) {
        m_hoverNode = hover;
        update();
    }
}

void GraphWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_pressedNode = -1;
        if (m_dragging) {
            m_dragging = false;
            setCursor(Qt::OpenHandCursor);
        }
    }
    QFrame::mouseReleaseEvent(event);
}

void GraphWidget::leaveEvent(QEvent *event) {
    m_hoverNode = -1;
    update();
    QFrame::leaveEvent(event);
}

void GraphWidget::contextMenuEvent(QContextMenuEvent *event) {
    if (!m_graph) return;
    const int node = nodeNear(*m_graph, canvasGeometry(), event->pos(), nodeRadius);
    if (node >= 0) emit nodeMenuRequested(node, event->globalPos());
}
