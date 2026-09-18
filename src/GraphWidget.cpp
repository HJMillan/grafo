#include "GraphWidget.h"
#include "GraphConstants.h"
#include "GraphFormat.h"

#include <QEvent>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QRadialGradient>
#include <QResizeEvent>
#include <QtGlobal>
#include <QtMath>
#include <cmath>

GraphWidget::GraphWidget(QWidget *parent) : QFrame(parent) {
    setFrameShape(QFrame::NoFrame);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);
    setFocusPolicy(Qt::StrongFocus);
}

void GraphWidget::clampIndex(int &index) const {
    if (index >= nodes.size()) index = -1;
}

void GraphWidget::setNodes(const QVector<QString> &names, const QVector<QPoint> &positions) {
    const int count = qMin(names.size(), positions.size());
    nodes.resize(count);
    for (int i = 0; i < count; ++i) nodes[i] = {names[i], positions[i]};

    clampIndex(selectedNode);
    clampIndex(hoverNode);
    clampIndex(originNode);
    clampIndex(destNode);
    clampIndex(draggedNode);
    update();
}

void GraphWidget::clearAll() {
    nodes.clear();
    edges.clear();
    highlightedPairs.clear();
    highlightedNodes.clear();
    draggedNode = -1;
    dragDidMove = false;
    selectedNode = -1;
    hoverNode = -1;
    originNode = -1;
    destNode = -1;
    update();
}

void GraphWidget::clearSelection() {
    if (selectedNode < 0) return;
    selectedNode = -1;
    update();
}

void GraphWidget::setQueryEndpoints(int origin, int dest) {
    const int nextOrigin = (origin >= 0 && origin < nodes.size()) ? origin : -1;
    const int nextDest = (dest >= 0 && dest < nodes.size()) ? dest : -1;
    if (nextOrigin == originNode && nextDest == destNode) return;
    originNode = nextOrigin;
    destNode = nextDest;
    update();
}

QVector<QPoint> GraphWidget::nodePositions() const {
    QVector<QPoint> out;
    out.reserve(nodes.size());
    for (const Node &n : nodes) out.append(n.pos);
    return out;
}

void GraphWidget::restoreNodes(const QVector<QString> &names, const QVector<QPoint> &positions) {
    edges.clear();
    highlightedPairs.clear();
    highlightedNodes.clear();
    draggedNode = -1;
    dragDidMove = false;
    selectedNode = -1;
    hoverNode = -1;
    originNode = -1;
    destNode = -1;
    setNodes(names, positions);
}

QPoint GraphWidget::placeWithoutOverlap(QPoint p) const {
    p = clampToCanvas(p);
    const int minDist = nodeSeparation;
    const int minSq = minDist * minDist;

    auto overlaps = [&](const QPoint &candidate) {
        for (const Node &n : nodes) {
            const QPoint d = candidate - n.pos;
            if (d.x() * d.x() + d.y() * d.y() < minSq) return true;
        }
        return false;
    };

    if (!overlaps(p)) return p;

    for (int attempt = 1; attempt <= 48; ++attempt) {
        const double angle = attempt * 0.6180339887 * 2.0 * M_PI;
        const int radius = minDist + (attempt / 6) * 10;
        const QPoint next = clampToCanvas(QPoint(
                p.x() + static_cast<int>(radius * std::cos(angle)),
                p.y() + static_cast<int>(radius * std::sin(angle))));
        if (!overlaps(next)) return next;
    }
    return p;
}

void GraphWidget::setAdjacency(const QVector<QVector<double>> &adj, int nodeCount, double inf, bool directed) {
    edges.clear();
    directedEdges = directed;
    if (nodeCount <= 0) { update(); return; }

    QVector<QVector<bool>> used(nodeCount, QVector<bool>(nodeCount, false));

    for (int i = 0; i < nodeCount; ++i) {
        for (int j = 0; j < nodeCount; ++j) {
            if (GraphConstants::isMissing(adj[i][j], inf)) continue;
            if (used[i][j]) continue;
            if (!directed) {
                if (j < i) continue;
                edges.append({i, j, adj[i][j], false, false, 0.0});
            } else {
                if (i == j) {
                    edges.append({i, j, adj[i][j], true, false, 0.0});
                    used[i][j] = true;
                    continue;
                }
                const bool hasRev = (j < nodeCount && !GraphConstants::isMissing(adj[j][i], inf));
                const double revW = hasRev ? adj[j][i] : 0.0;
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
    highlightedNodes = path;
    if (path.size() < 2) { update(); return; }
    for (int i = 0; i + 1 < path.size(); ++i) {
        highlightedPairs.append({path[i], path[i + 1]});
    }
    update();
}

int GraphWidget::nodeAt(const QPoint &p) const {
    for (int i = nodes.size() - 1; i >= 0; --i) {
        const QPoint d = p - nodes[i].pos;
        if (d.x() * d.x() + d.y() * d.y() <= nodeRadius * nodeRadius) return i;
    }
    return -1;
}

QPoint GraphWidget::clampToCanvas(const QPoint &p) const {
    const QRect r = rect().adjusted(nodeRadius + 8, nodeRadius + 8, -nodeRadius - 8, -nodeRadius - 8);
    if (r.width() <= 0 || r.height() <= 0) return p;
    return QPoint(qBound(r.left(), p.x(), r.right()), qBound(r.top(), p.y(), r.bottom()));
}

void GraphWidget::drawEmptyState(QPainter &g) const {
    QFont titleFont = Theme::headline();
    titleFont.setPointSizeF(15.0);
    QFont captionFont = Theme::caption();

    const QString title = QStringLiteral("El lienzo está vacío");
    const QString caption = QStringLiteral("Clic para crear un nodo · clic en dos nodos para unirlos");

    QFontMetrics titleFm(titleFont);
    QFontMetrics captionFm(captionFont);
    const int blockHeight = titleFm.height() + 6 + captionFm.height();
    const int top = rect().center().y() - blockHeight / 2;

    g.setFont(titleFont);
    g.setPen(Theme::emptyStateLabel());
    g.drawText(QRect(20, top, width() - 40, titleFm.height()),
               Qt::AlignHCenter | Qt::AlignVCenter, title);

    g.setFont(captionFont);
    g.setPen(Theme::emptyStateCaption());
    g.drawText(QRect(20, top + titleFm.height() + 6, width() - 40, captionFm.height()),
               Qt::AlignHCenter | Qt::AlignVCenter, caption);
}

void GraphWidget::drawNode(QPainter &g, int index) const {
    const Node &n = nodes[index];
    QRectF circle(n.pos.x() - nodeRadius, n.pos.y() - nodeRadius,
                  nodeRadius * 2, nodeRadius * 2);

    const bool onPath = highlightedNodes.contains(index);
    const bool dragging = (index == draggedNode);
    const bool selected = (index == selectedNode);
    const bool hovered = (index == hoverNode);
    const bool isQuery = (index == originNode || index == destNode);

    QColor base = Theme::nodeFill();
    if (isQuery) {
        const QColor accent = Theme::accent();
        base = QColor((accent.red() + base.red() * 2) / 3,
                      (accent.green() + base.green() * 2) / 3,
                      (accent.blue() + base.blue() * 2) / 3);
    }
    QRadialGradient radial(n.pos, nodeRadius);
    radial.setColorAt(0.0, base.lighter(105));
    radial.setColorAt(0.7, base);
    radial.setColorAt(1.0, base.darker(107));

    QColor stroke = Theme::nodeStroke();
    int strokeW = 2;
    if (dragging || selected) {
        stroke = Theme::accent();
        strokeW = 3;
    } else if (onPath) {
        stroke = Theme::edgeHighlight();
        strokeW = 3;
    } else if (hovered) {
        stroke = Theme::accent();
        strokeW = 2;
    }

    g.setPen(QPen(stroke, strokeW));
    g.setBrush(radial);
    g.drawEllipse(circle);

    g.setPen(Theme::nodeLabel());
    g.setFont(Theme::headline());
    g.drawText(circle, Qt::AlignCenter, n.name);
}

void GraphWidget::drawQueryBadge(QPainter &g, int index, const QString &text) const {
    if (index < 0 || index >= nodes.size()) return;

    QFont font = Theme::caption();
    QFontMetrics fm(font);
    const QSize ts = fm.size(Qt::TextSingleLine, text);
    const int pad = 6;
    QRect pill(0, 0, ts.width() + pad * 2, ts.height() + 4);
    pill.moveCenter(QPoint(nodes[index].pos.x(), nodes[index].pos.y() + nodeRadius + 14));

    QPainterPath pillPath;
    pillPath.addRoundedRect(pill, 8, 8);
    g.setPen(Qt::NoPen);
    g.setBrush(Theme::pillFill());
    g.drawPath(pillPath);

    g.setPen(QPen(Theme::accent(), 1));
    g.setBrush(Qt::NoBrush);
    g.drawPath(pillPath);

    g.setPen(Theme::pillLabel());
    g.setFont(font);
    g.drawText(pill, Qt::AlignCenter, text);
}

void GraphWidget::drawQueryBadges(QPainter &g) const {
    if (originNode >= 0 && originNode == destNode) {
        drawQueryBadge(g, originNode, QStringLiteral("Origen y destino"));
        return;
    }
    if (originNode >= 0) drawQueryBadge(g, originNode, QStringLiteral("Origen"));
    if (destNode >= 0) drawQueryBadge(g, destNode, QStringLiteral("Destino"));
}

void GraphWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter g(this);
    g.setRenderHint(QPainter::Antialiasing, true);
    g.setRenderHint(QPainter::TextAntialiasing, true);

    QPainterPath clip;
    clip.addRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 16, 16);
    g.setClipPath(clip);
    g.fillRect(rect(), Theme::canvasBackground());

    if (nodes.isEmpty()) {
        drawEmptyState(g);
        return;
    }

    drawEdges(g);
    drawLinkPreview(g);
    drawNodes(g);
    drawQueryBadges(g);
}

void GraphWidget::drawEdges(QPainter &g) const {
    const QColor edgeColor = Theme::edge();
    const QColor edgeHiColor = Theme::edgeHighlight();
    const int edgeW = 2;
    const int edgeHiW = 4;

    QFont wFont = Theme::caption();
    QFontMetrics wfm(wFont);

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
            QColor glowC = edgeHiColor;
            glowC.setAlpha(60);
            glow.setColor(glowC);
            glow.setCapStyle(Qt::RoundCap);
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
        QPointF n(0, 0.5);
        if (len > 0.0001) n = QPointF(-line.dy() / len, line.dx() / len);

        QString txt;
        if (selfLoop) {
            txt = QString("%1→%1  %2")
                    .arg(nodes[e.from].name)
                    .arg(formatWeight(e.weight));
        } else if (e.directed && e.hasReverse) {
            txt = QString("%1→%2  %3   %2→%1  %4")
                    .arg(nodes[e.from].name)
                    .arg(nodes[e.to].name)
                    .arg(formatWeight(e.weight), formatWeight(e.reverseWeight));
        } else {
            txt = formatWeight(e.weight);
        }
        const QSize ts = wfm.size(Qt::TextSingleLine, txt);
        const int pad = 7;
        const int radius = 10;

        QRect pill(0, 0, ts.width() + pad * 2, ts.height() + pad);
        if (selfLoop) {
            pill.moveCenter(QPoint(a.pos.x(), a.pos.y() - nodeRadius - 28));
        } else {
            pill.moveCenter((mid + n * 16.0).toPoint());
        }

        QPainterPath pillPath;
        pillPath.addRoundedRect(pill, radius, radius);
        g.setPen(Qt::NoPen);
        g.setBrush(Theme::pillFill());
        g.drawPath(pillPath);

        QPen pillPen(hi ? edgeHiColor : Theme::pillStroke(), 1);
        g.setPen(pillPen);
        g.setBrush(Qt::NoBrush);
        g.drawPath(pillPath);

        g.setPen(Theme::pillLabel());
        g.setFont(wFont);
        g.drawText(pill, Qt::AlignCenter, txt);
    }
}

void GraphWidget::drawLinkPreview(QPainter &g) const {
    if (selectedNode < 0 || selectedNode >= nodes.size() || draggedNode >= 0) return;

    QPoint end = cursorPos;
    if (hoverNode >= 0 && hoverNode != selectedNode) end = nodes[hoverNode].pos;
    QPen preview(Theme::accent());
    preview.setWidth(2);
    preview.setStyle(Qt::DashLine);
    preview.setCapStyle(Qt::RoundCap);
    g.setPen(preview);
    g.drawLine(nodes[selectedNode].pos, end);
}

void GraphWidget::drawNodes(QPainter &g) const {
    for (int i = 0; i < nodes.size(); ++i) drawNode(g, i);
}

void GraphWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        setFocus(Qt::MouseFocusReason);
        cursorPos = event->pos();
        const int hit = nodeAt(event->pos());
        if (hit >= 0) {
            draggedNode = hit;
            dragDidMove = false;
            dragPressPos = event->pos();
            dragGrabOffset = nodes[hit].pos - event->pos();
            hoverNode = hit;
            setCursor(Qt::ClosedHandCursor);
            update();
        } else if (selectedNode >= 0) {
            clearSelection();
        } else {
            emit canvasClicked(event->pos());
        }
    }
    QFrame::mousePressEvent(event);
}

void GraphWidget::mouseMoveEvent(QMouseEvent *event) {
    cursorPos = event->pos();

    if (draggedNode >= 0 && draggedNode < nodes.size()) {
        const int dist = (event->pos() - dragPressPos).manhattanLength();
        if (dist > 6) {
            const QPoint next = clampToCanvas(event->pos() + dragGrabOffset);
            if (next != nodes[draggedNode].pos) {
                if (!dragDidMove) {
                    dragDidMove = true;
                    emit nodeAboutToMove();
                }
                nodes[draggedNode].pos = next;
            }
        }
        update();
    } else {
        hoverNode = nodeAt(event->pos());
        if (selectedNode >= 0) {
            setCursor(hoverNode >= 0 && hoverNode != selectedNode ? Qt::PointingHandCursor : Qt::CrossCursor);
            update();
        } else {
            setCursor(hoverNode >= 0 ? Qt::OpenHandCursor : Qt::CrossCursor);
        }
    }
    QFrame::mouseMoveEvent(event);
}

void GraphWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (draggedNode >= 0) {
        const bool moved = dragDidMove;
        const int hit = nodeAt(event->pos());
        if (!dragDidMove && hit >= 0) {
            if (selectedNode >= 0 && selectedNode != hit) {
                emit nodesLinked(selectedNode, hit);
                selectedNode = hit;
            } else if (selectedNode == hit) {
                selectedNode = -1;
            } else {
                selectedNode = hit;
            }
        }
        draggedNode = -1;
        dragDidMove = false;
        hoverNode = nodeAt(event->pos());
        setCursor(hoverNode >= 0 ? Qt::OpenHandCursor : Qt::CrossCursor);
        if (moved) emit nodeMoved();
        update();
    }
    QFrame::mouseReleaseEvent(event);
}

void GraphWidget::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        clearSelection();
        return;
    }
    QFrame::keyPressEvent(event);
}

void GraphWidget::leaveEvent(QEvent *event) {
    hoverNode = -1;
    update();
    QFrame::leaveEvent(event);
}

void GraphWidget::resizeEvent(QResizeEvent *event) {
    QFrame::resizeEvent(event);
    bool changed = false;
    for (Node &n : nodes) {
        const QPoint clamped = clampToCanvas(n.pos);
        if (clamped != n.pos) {
            n.pos = clamped;
            changed = true;
        }
    }
    if (changed) emit positionsChanged();
    update();
}
