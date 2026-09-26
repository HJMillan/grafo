#include "Layout.h"
#include "Graph.h"

#include <QLineF>
#include <QVector>
#include <QtGlobal>
#include <cmath>
#include <optional>

QPointF CanvasGeometry::toPixel(const QPointF &n) const {
    const double w = qMax(0.0, size.width() - 2 * margin);
    const double h = qMax(0.0, size.height() - 2 * margin);
    return {margin + n.x() * w, margin + n.y() * h};
}

QPointF CanvasGeometry::toNormalized(const QPointF &p) const {
    const double w = size.width() - 2 * margin;
    const double h = size.height() - 2 * margin;
    const double x = w > 0 ? (p.x() - margin) / w : 0.5;
    const double y = h > 0 ? (p.y() - margin) / h : 0.5;
    return {qBound(0.0, x, 1.0), qBound(0.0, y, 1.0)};
}

void layoutAutoPlaced(Graph &g, const CanvasGeometry &geo, double minDistance) {
    QVector<int> autoNodes;
    QVector<QPointF> occupied; // píxeles de los nodos fijos y de los ya repartidos
    for (int i = 0; i < g.nodeCount(); ++i) {
        if (g.node(i).autoPlaced)
            autoNodes.append(i);
        else
            occupied.append(geo.toPixel(g.node(i).pos));
    }
    const int m = static_cast<int>(autoNodes.size());
    if (m == 0) return;

    constexpr double PI = 3.14159265358979323846;
    const auto pointAt = [](double radius, double angle) {
        return QPointF(0.5 + radius * std::cos(angle), 0.5 + radius * std::sin(angle));
    };
    const auto isFree = [&](const QPointF &normalized) {
        const QPointF px = geo.toPixel(normalized);
        for (const QPointF &o : occupied)
            if (QLineF(px, o).length() < minDistance) return false;
        return true;
    };

    // Para cada nodo se prueba, en orden: su posición ideal en la elipse exterior,
    // desplazamientos alternos a cada lado (hasta media separación, luego la vuelta
    // entera) y, si el lienzo es pequeño y no hay hueco, elipses interiores.
    const double slot = 2 * PI / m;
    constexpr int steps = 48;
    const double radii[] = {0.45, 0.30, 0.15};
    const auto findSpot = [&](double ideal) -> std::optional<QPointF> {
        for (double radius : radii) {
            const double span = radius == radii[0] ? slot / 2 : PI;
            for (double limit : {span, PI}) {
                for (int s = 0; s <= steps; ++s) {
                    const double offset = (s + 1) / 2 * (limit / (steps / 2)) * (s % 2 ? 1 : -1);
                    const QPointF candidate = pointAt(radius, ideal + offset);
                    if (isFree(candidate)) return candidate;
                }
            }
        }
        return std::nullopt;
    };

    for (int k = 0; k < m; ++k) {
        const double ideal = -PI / 2 + k * slot;
        const QPointF chosen = findSpot(ideal).value_or(pointAt(radii[0], ideal));
        g.setNodePos(autoNodes[k], chosen);
        occupied.append(geo.toPixel(chosen));
    }
}

int nodeNear(const Graph &g, const CanvasGeometry &geo, const QPointF &pixel, double maxDistance) {
    int best = -1;
    double bestDist = maxDistance;
    for (int i = 0; i < g.nodeCount(); ++i) {
        const double d = QLineF(geo.toPixel(g.node(i).pos), pixel).length();
        if (d < bestDist) {
            bestDist = d;
            best = i;
        }
    }
    return best;
}
