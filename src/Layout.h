#ifndef LAYOUT_H
#define LAYOUT_H

#include <QPointF>
#include <QSizeF>

class Graph;

// Conversión entre posiciones normalizadas (0..1, como las guarda Graph) y
// píxeles del lienzo. El margen deja el círculo del nodo entero a la vista.
struct CanvasGeometry {
    QSizeF size;
    double margin = 0.0;

    QPointF toPixel(const QPointF &normalized) const;
    QPointF toNormalized(const QPointF &pixel) const; // limitado a 0..1
};

// Reparte en una elipse los nodos colocados por la app (autoPlaced), empezando
// arriba y en sentido horario. Los nodos colocados por el usuario no se mueven:
// si una posición queda a menos de minDistance píxeles de uno de ellos (o de
// otro nodo ya repartido), se desplaza por la elipse hasta encontrar hueco.
void layoutAutoPlaced(Graph &g, const CanvasGeometry &geo, double minDistance);

// Índice del nodo más cercano a pixel a menos de maxDistance píxeles, o -1.
int nodeNear(const Graph &g, const CanvasGeometry &geo, const QPointF &pixel, double maxDistance);

#endif // LAYOUT_H
