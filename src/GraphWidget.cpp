#include "GraphWidget.h"
#include <QPen>
#include <QFont>
#include <QFontMetrics>
#include <QPainterPath>
#include <QtMath>
#include <QtGlobal>

GraphWidget::GraphWidget(QWidget *parent) : QFrame(parent) {
    setStyleSheet("background-color: blue; border: 1px solid #1E4370;");
    setFrameShape(QFrame::Box);
    setAttribute(Qt::WA_OpaquePaintEvent);
}

void GraphWidget::addNodeVisual(const QString &name, const QPoint &pos) {
    nodes.append({name, pos});
    update();
}

void GraphWidget::removeLastNode() {
    if (!nodes.isEmpty()) nodes.removeLast();
    edges.clear();
    highlightedPairs.clear();
    update();
}

void GraphWidget::clearAll() {
    nodes.clear();
    edges.clear();
    highlightedPairs.clear();
    update();
}

void GraphWidget::setAdjacency(const QVector<QVector<double>> &adj, int nodeCount, double inf, bool directed) {
    edges.clear();
    directedEdges = directed;
    if (nodeCount <= 0) { update(); return; }

    QVector<QVector<bool>> used(nodeCount, QVector<bool>(nodeCount, false));

    // reconstruir aristas; si es dirigido y existe ida/vuelta, combinamos en una sola con ambos pesos
    for (int i = 0; i < nodeCount; ++i) {
        for (int j = 0; j < nodeCount; ++j) {
            if (adj[i][j] >= inf / 2) continue;
            if (used[i][j]) continue;
            if (!directed) {
                if (j <= i) continue; // evitar duplicar en no dirigido
                edges.append({i, j, adj[i][j], false, false, 0.0});
            } else {
                if (i == j) {
                    edges.append({i, j, adj[i][j], true, false, 0.0});
                    used[i][j] = true;
                    continue;
                }
                bool hasRev = (j < nodeCount && adj[j][i] < inf / 2);
                double revW = hasRev ? adj[j][i] : 0.0;
                edges.append({i, j, adj[i][j], true, hasRev, revW});
                used[i][j] = true;
                if (hasRev) used[j][i] = true;
            }
        }
    }
    update();
}

void GraphWidget::highlightPath(const QVector<int> &path) {
    highlightedPairs.clear();
    if (path.size() < 2) { update(); return; }
    for (int i = 0; i + 1 < path.size(); ++i) {
        highlightedPairs.append({path[i], path[i+1]});
    }
    update();
}

void GraphWidget::highlightPath(const QVector<int> &path, const QVector<QVector<double>> &adj, double inf, bool directed) {
    Q_UNUSED(adj);
    Q_UNUSED(inf);
    directedEdges = directed;
    highlightPath(path);
}

void GraphWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter g(this);
    g.setRenderHint(QPainter::Antialiasing, true);
    g.setRenderHint(QPainter::TextAntialiasing, true);
    g.setRenderHint(QPainter::SmoothPixmapTransform, true);

    g.fillRect(rect(), QColor("#0b0f14"));

    const QColor edgeColor(225,230,238);
    const QColor edgeHiColor("#00B2A9");
    const int edgeW = 2;
    const int edgeHiW = 4;

    QFont wFont = g.font();
    wFont.setPointSizeF(12.0);
    QFontMetrics wfm(wFont);

    // Aristas
    for (const Edge &e : edges) {
        if (e.from >= nodes.size() || e.to >= nodes.size()) continue;
        const Node &a = nodes[e.from];
        const Node &b = nodes[e.to];

        const bool hi = highlightedPairs.contains({e.from, e.to}) ||
                        (!e.directed && highlightedPairs.contains({e.to, e.from})) ||
                        (e.hasReverse && highlightedPairs.contains({e.to, e.from}));

        const bool selfLoop = (e.from == e.to);

        if (hi) {
            QPen glow(edgeHiColor);
            glow.setWidth(edgeHiW + 6);
            QColor glowC = edgeHiColor; glowC.setAlpha(60);
            glow.setColor(glowC);
            glow.setCapStyle(Qt::RoundCap);
            glow.setJoinStyle(Qt::RoundJoin);
            g.setPen(glow);
            if (selfLoop) {
                QRect loopRect(a.pos.x() - nodeRadius - 12, a.pos.y() - nodeRadius - 22,
                               (nodeRadius + 12) * 2, (nodeRadius + 12) * 2);
                g.drawArc(loopRect, 45 * 16, 270 * 16);
            } else {
                g.drawLine(a.pos, b.pos);
            }
        }

        QPen pen(hi ? edgeHiColor : edgeColor, hi ? edgeHiW : edgeW,
                 Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        g.setPen(pen);
        g.setBrush(Qt::NoBrush);
        if (selfLoop) {
            QRect loopRect(a.pos.x() - nodeRadius - 12, a.pos.y() - nodeRadius - 22,
                           (nodeRadius + 12) * 2, (nodeRadius + 12) * 2);
            g.drawArc(loopRect, 45 * 16, 270 * 16);
        } else {
            g.drawLine(a.pos, b.pos);
        }

        auto drawArrow = [&](const QPointF &fromP, const QPointF &toP) {
            QLineF line(fromP, toP);
            if (line.length() <= 1e-3) return;
            const double arrowSize = 10.0;
            line.setLength(line.length() - nodeRadius + 2);
            QPointF end = line.p2();
            line.setLength(line.length() - arrowSize);
            QPointF back = line.p2();
            QLineF normal = line.normalVector();
            normal.setLength(arrowSize / 2.0);
            QPointF p1 = back + normal.p2() - normal.p1();
            QPointF p2 = back - (normal.p2() - normal.p1());

            QPainterPath path;
            path.moveTo(end);
            path.lineTo(p1);
            path.lineTo(p2);
            path.closeSubpath();
            g.setBrush(hi ? edgeHiColor : edgeColor);
            g.drawPath(path);
        };

        if (e.directed && !selfLoop) {
            drawArrow(a.pos, b.pos);
            if (e.hasReverse) drawArrow(b.pos, a.pos);
        }

        QLineF line(a.pos, b.pos);
        const qreal len = line.length();
        const QPointF mid = line.pointAt(0.5);
        QPointF n(0,0.5);
        if (len > 0.0001) n = QPointF(-line.dy()/len, line.dx()/len);

        QString txt;
        if (selfLoop) {
            txt = QString("%1->%1=%2")
                    .arg(nodes[e.from].name)
                    .arg(e.weight, 0, 'f', 2);
        } else if (e.directed && e.hasReverse) {
            txt = QString("%1->%2=%3 / %2->%1=%4")
                    .arg(nodes[e.from].name)
                    .arg(nodes[e.to].name)
                    .arg(e.weight, 0, 'f', 2)
                    .arg(e.reverseWeight, 0, 'f', 2);
        } else {
            txt = QString::number(e.weight, 'f', 2);
        }
        const QSize ts  = wfm.size(Qt::TextSingleLine, txt);
        const int pad = 7;
        const int radius = 10;

        QRect pill(0, 0, ts.width() + pad*2, ts.height() + pad);
        if (selfLoop) {
            pill.moveCenter(QPoint(a.pos.x(), a.pos.y() - nodeRadius - 28));
        } else {
            pill.moveCenter((mid + n * 16.0).toPoint());
        }

        QPainterPath shadowPath;
        shadowPath.addRoundedRect(pill.adjusted(1, 1, 1, 1), radius, radius);
        g.setPen(Qt::NoPen);
        g.setBrush(QColor(0, 0, 0, 90));
        g.drawPath(shadowPath);

        QPainterPath pillPath;
        pillPath.addRoundedRect(pill, radius, radius);
        g.setBrush(QColor(10, 10, 10, 150));
        g.drawPath(pillPath);

        QColor border = hi ? edgeHiColor : QColor(255,255,255,40);
        QPen pillPen(border, 1);
        g.setPen(pillPen);
        g.setBrush(Qt::NoBrush);
        g.drawPath(pillPath);

        g.setPen(QColor(245,247,255));
        g.setFont(wFont);
        g.drawText(pill, Qt::AlignCenter, txt);
    }

    // Nodos
    QFont nFont = g.font();
    nFont.setBold(true);
    nFont.setPointSizeF(11.0);
    g.setFont(nFont);

    for (const Node &n : nodes) {
        QRectF circle(n.pos.x() - nodeRadius, n.pos.y() - nodeRadius,
                      nodeRadius * 2, nodeRadius * 2);

        QRadialGradient radial(n.pos, nodeRadius);
        radial.setColorAt(0.0, QColor("#f0f2f6"));
        radial.setColorAt(0.7, QColor("#e4e7ed"));
        radial.setColorAt(1.0, QColor("#d8dbe2"));

        g.setPen(QPen(QColor("#3a4250"), 2));
        g.setBrush(radial);
        g.drawEllipse(circle);

        g.setPen(QColor("#12161c"));
        g.drawText(circle, Qt::AlignCenter, n.name);
    }
}

void GraphWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        emit nodeClicked(event->pos());
    }
    QFrame::mousePressEvent(event);
}
