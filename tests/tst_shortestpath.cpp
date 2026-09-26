#include <QRandomGenerator>
#include <QTest>

#include "Graph.h"
#include "ShortestPath.h"
#include "Weight.h"
#include "Layout.h"
#include <QLineF>

namespace {
// Crea un grafo con nodos A, B, C... y las aristas indicadas como "AB:2.5".
Graph makeGraph(int nodes, bool directed, const QStringList &edges) {
    Graph g;
    g.setDirected(directed);
    for (int i = 0; i < nodes; ++i) g.addNode(QString(QChar('A' + i)), {}, false);
    for (const QString &e : edges) {
        const int from = e[0].unicode() - 'A';
        const int to = e[1].unicode() - 'A';
        g.setEdge(from, to, e.mid(3).toDouble());
    }
    return g;
}

int n(char c) { return c - 'A'; }

void addAllAlgorithms() {
    QTest::addColumn<int>("algo");
    QTest::newRow("dijkstra") << 0;
    QTest::newRow("bellman-ford") << 1;
    QTest::newRow("floyd-warshall") << 2;
}

void addNegativeCapable() {
    QTest::addColumn<int>("algo");
    QTest::newRow("bellman-ford") << 1;
    QTest::newRow("floyd-warshall") << 2;
}

PathResult run(int algo, const Graph &g, int src, int dst) {
    switch (algo) {
    case 0: return dijkstra(g, src, dst);
    case 1: return bellmanFord(g, src, dst);
    default: return floydWarshall(g, src, dst);
    }
}
}

class TestShortestPath : public QObject {
    Q_OBJECT

private slots:
    // ---------- Graph ----------
    void graph_undirectedSetsBothDirections() {
        Graph g = makeGraph(2, false, {"AB:3"});
        QCOMPARE(g.edge(n('A'), n('B')), 3.0);
        QCOMPARE(g.edge(n('B'), n('A')), 3.0);
        QCOMPARE(g.edgeList().size(), 1);
    }

    void graph_undirectedEdgeListIncludesSelfLoops() {
        Graph g = makeGraph(2, false, {"AA:1", "AB:2"});
        QCOMPARE(g.edgeList().size(), 2);
    }

    void graph_removeNodeReindexes() {
        Graph g = makeGraph(3, true, {"AB:1", "BC:2", "AC:5"});
        g.removeNode(n('B'));
        QCOMPARE(g.nodeCount(), 2);
        QCOMPARE(g.node(1).name, QString("C"));
        QCOMPARE(g.edge(0, 1), 5.0);
        QCOMPARE(g.arcs().size(), 1);
    }

    void graph_asymmetricArcs() {
        Graph g = makeGraph(3, true, {"AB:1", "BA:1", "BC:2", "CB:3"});
        QVERIFY(!g.isSymmetric());
        QCOMPARE(g.asymmetricArcs().size(), 1); // B->C y C->B difieren: una pareja
        g.removeEdge(n('C'), n('B'));
        g.setEdge(n('C'), n('B'), 2);
        QVERIFY(g.isSymmetric());
    }

    // ---------- Casos comunes a los tres algoritmos ----------
    void all_simplePath_data() { addAllAlgorithms(); }
    void all_simplePath() {
        QFETCH(int, algo);
        Graph g = makeGraph(4, true, {"AB:1", "BC:2", "AC:5", "CD:1"});
        const PathResult r = run(algo, g, n('A'), n('D'));
        QCOMPARE(r.status, PathResult::Ok);
        QCOMPARE(r.distance, 4.0);
        QCOMPARE(r.path, (QVector<int>{0, 1, 2, 3}));
    }

    void all_noPath_data() { addAllAlgorithms(); }
    void all_noPath() {
        QFETCH(int, algo);
        Graph g = makeGraph(3, true, {"AB:1", "CB:1"});
        QCOMPARE(run(algo, g, n('A'), n('C')).status, PathResult::NoPath);
    }

    void all_sameNode_data() { addAllAlgorithms(); }
    void all_sameNode() {
        QFETCH(int, algo);
        Graph g = makeGraph(2, true, {"AA:3", "AB:1"});
        const PathResult r = run(algo, g, n('A'), n('A'));
        QCOMPARE(r.status, PathResult::Ok);
        QCOMPARE(r.distance, 0.0);
        QCOMPARE(r.path, QVector<int>{0});
    }

    void all_undirected_data() { addAllAlgorithms(); }
    void all_undirected() {
        QFETCH(int, algo);
        Graph g = makeGraph(3, false, {"AB:4", "CB:1", "AC:2"});
        const PathResult r = run(algo, g, n('B'), n('A'));
        QCOMPARE(r.status, PathResult::Ok);
        QCOMPARE(r.distance, 3.0);
        QCOMPARE(r.path, (QVector<int>{1, 2, 0}));
    }

    void all_agreeOnRandomNonNegativeGraphs() {
        auto *rng = QRandomGenerator::global();
        for (int iter = 0; iter < 200; ++iter) {
            const int count = 2 + rng->bounded(8);
            Graph g;
            g.setDirected(rng->bounded(2) == 0);
            for (int i = 0; i < count; ++i) g.addNode(QString(QChar('A' + i)), {}, false);
            for (int i = 0; i < count; ++i)
                for (int j = 0; j < count; ++j)
                    if (rng->bounded(3) == 0) g.setEdge(i, j, rng->bounded(20));
            const int src = rng->bounded(count);
            const int dst = rng->bounded(count);
            const PathResult d = dijkstra(g, src, dst);
            const PathResult b = bellmanFord(g, src, dst);
            const PathResult f = floydWarshall(g, src, dst);
            QCOMPARE(b.status, d.status);
            QCOMPARE(f.status, d.status);
            if (d.status == PathResult::Ok) {
                QCOMPARE(b.distance, d.distance);
                QCOMPARE(f.distance, d.distance);
            }
        }
    }

    // ---------- Pesos negativos ----------
    // Contraejemplo que justifica bloquear Dijkstra con pesos negativos:
    // el resultado correcto es A->C->B = 1, pero Dijkstra cierra B con 2.
    void dijkstra_isWrongWithNegativeWeights() {
        Graph g = makeGraph(3, true, {"AB:2", "AC:3", "CB:-2"});
        QCOMPARE(dijkstra(g, n('A'), n('B')).distance, 2.0);
        QCOMPARE(bellmanFord(g, n('A'), n('B')).distance, 1.0);
        QCOMPARE(floydWarshall(g, n('A'), n('B')).distance, 1.0);
    }

    void negativeWeights_withoutCycle_data() { addNegativeCapable(); }
    void negativeWeights_withoutCycle() {
        QFETCH(int, algo);
        Graph g = makeGraph(4, true, {"AB:4", "AC:2", "CB:-3", "BD:1"});
        const PathResult r = run(algo, g, n('A'), n('D'));
        QCOMPARE(r.status, PathResult::Ok);
        QCOMPARE(r.distance, 0.0);
        QCOMPARE(r.path, (QVector<int>{0, 2, 1, 3}));
    }

    void negativeCycle_onPath_data() { addNegativeCapable(); }
    void negativeCycle_onPath() {
        QFETCH(int, algo);
        Graph g = makeGraph(4, true, {"AB:1", "BC:-2", "CB:1", "CD:1"});
        const PathResult r = run(algo, g, n('A'), n('D'));
        QCOMPARE(r.status, PathResult::NegativeCycle);
        QCOMPARE(r.distance, -1.0);
        QCOMPARE(r.path.size(), 3);
        QCOMPARE(r.path.first(), r.path.last());
    }

    void negativeCycle_selfLoop_data() { addNegativeCapable(); }
    void negativeCycle_selfLoop() {
        QFETCH(int, algo);
        Graph g = makeGraph(2, true, {"AB:1", "BB:-1"});
        const PathResult r = run(algo, g, n('A'), n('B'));
        QCOMPARE(r.status, PathResult::NegativeCycle);
        QCOMPARE(r.path, (QVector<int>{1, 1}));
        QCOMPARE(r.distance, -1.0);
    }

    // Criterio común: solo hay error si el ciclo afecta al camino origen→destino.
    void negativeCycle_notReachableFromSource_data() { addNegativeCapable(); }
    void negativeCycle_notReachableFromSource() {
        QFETCH(int, algo);
        // Ciclo C⇄D negativo, pero no se alcanza desde A.
        Graph g = makeGraph(4, true, {"AB:2", "CD:-3", "DC:1"});
        const PathResult r = run(algo, g, n('A'), n('B'));
        QCOMPARE(r.status, PathResult::Ok);
        QCOMPARE(r.distance, 2.0);
    }

    void negativeCycle_doesNotReachDestination_data() { addNegativeCapable(); }
    void negativeCycle_doesNotReachDestination() {
        QFETCH(int, algo);
        // Desde A se alcanza el ciclo B⇄C, pero desde el ciclo no se llega a D.
        Graph g = makeGraph(4, true, {"AB:1", "BC:-2", "CB:1", "AD:3"});
        const PathResult r = run(algo, g, n('A'), n('D'));
        QCOMPARE(r.status, PathResult::Ok);
        QCOMPARE(r.distance, 3.0);
        QCOMPARE(r.path, (QVector<int>{0, 3}));
    }

    void negativeCycle_reportsTheAffectingCycle_data() { addNegativeCapable(); }
    void negativeCycle_reportsTheAffectingCycle() {
        QFETCH(int, algo);
        // Dos ciclos negativos alcanzables: B⇄C no llega a F; D⇄E sí.
        Graph g = makeGraph(6, true, {"AB:1", "BC:-2", "CB:1", "AD:1", "DE:-2", "ED:1", "EF:1"});
        const PathResult r = run(algo, g, n('A'), n('F'));
        QCOMPARE(r.status, PathResult::NegativeCycle);
        for (int v : r.path) QVERIFY2(v == n('D') || v == n('E'), "el ciclo debe ser D⇄E");
        QCOMPARE(r.distance, -1.0);
    }

    void negativeCycle_bothAlgorithmsAgree() {
        auto *rng = QRandomGenerator::global();
        for (int iter = 0; iter < 300; ++iter) {
            const int count = 2 + rng->bounded(7);
            Graph g;
            g.setDirected(true);
            for (int i = 0; i < count; ++i) g.addNode(QString(QChar('A' + i)), {}, false);
            for (int i = 0; i < count; ++i)
                for (int j = 0; j < count; ++j)
                    if (rng->bounded(4) == 0) g.setEdge(i, j, rng->bounded(12) - 3);
            const int src = rng->bounded(count);
            const int dst = rng->bounded(count);
            const PathResult b = bellmanFord(g, src, dst);
            const PathResult f = floydWarshall(g, src, dst);
            QCOMPARE(f.status, b.status);
            if (b.status == PathResult::Ok) QCOMPARE(f.distance, b.distance);
            if (b.status == PathResult::NegativeCycle) {
                QVERIFY(!b.path.isEmpty());
                QVERIFY(b.distance < 0);
                QCOMPARE(f.path, b.path); // misma reconstrucción
            }
        }
    }

    // ---------- Campo «Peso» ----------
    void weight_parse_data() {
        QTest::addColumn<QString>("text");
        QTest::addColumn<int>("error");
        QTest::addColumn<double>("value");
        QTest::newRow("entero") << "3" << int(WeightParse::None) << 3.0;
        QTest::newRow("coma") << "2,5" << int(WeightParse::None) << 2.5;
        QTest::newRow("punto") << "2.5" << int(WeightParse::None) << 2.5;
        QTest::newRow("negativo") << " -1,25 " << int(WeightParse::None) << -1.25;
        QTest::newRow("vacío") << "  " << int(WeightParse::Empty) << 0.0;
        QTest::newRow("solo signo") << "-" << int(WeightParse::NotNumber) << 0.0;
        QTest::newRow("dos separadores") << "1,2.3" << int(WeightParse::NotNumber) << 0.0;
        QTest::newRow("texto") << "abc" << int(WeightParse::NotNumber) << 0.0;
        QTest::newRow("inf") << "inf" << int(WeightParse::NotFinite) << 0.0;
        QTest::newRow("nan") << "nan" << int(WeightParse::NotFinite) << 0.0;
        QTest::newRow("límite") << "1000000000" << int(WeightParse::None) << 1e9;
        QTest::newRow("fuera de rango") << "1000000001" << int(WeightParse::OutOfRange) << 0.0;
    }
    void weight_parse() {
        QFETCH(QString, text);
        QFETCH(int, error);
        QFETCH(double, value);
        const WeightParse p = parseWeight(text);
        QCOMPARE(int(p.error), error);
        if (p.ok()) QCOMPARE(p.value, value);
    }
    // ---------- Reparto de nodos en el lienzo ----------
    void layout_geometryRoundTripAndClamp() {
        const CanvasGeometry geo{QSizeF(800, 600), 26};
        const QPointF p = geo.toPixel({0.25, 0.75});
        QCOMPARE(geo.toNormalized(p), QPointF(0.25, 0.75));
        QCOMPARE(geo.toNormalized({-50, 9999}), QPointF(0.0, 1.0)); // siempre dentro del lienzo
    }

    void layout_autoNodesNeverOverlap_data() {
        QTest::addColumn<QSizeF>("canvas");
        QTest::newRow("ventana normal") << QSizeF(1000, 700);
        QTest::newRow("ventana mínima") << QSizeF(480, 380);
    }
    void layout_autoNodesNeverOverlap() {
        QFETCH(QSizeF, canvas);
        const CanvasGeometry geo{canvas, 26};
        Graph g;
        for (int k = 0; k < Graph::MaxNodes; ++k) {
            g.addNode(QString(QChar('A' + k)), {0.5, 0.5}, true);
            layoutAutoPlaced(g, geo, 50);
            for (int i = 0; i < g.nodeCount(); ++i)
                for (int j = i + 1; j < g.nodeCount(); ++j) {
                    const double d = QLineF(geo.toPixel(g.node(i).pos), geo.toPixel(g.node(j).pos)).length();
                    QVERIFY2(d >= 44, qPrintable(QString("%1 nodos: %2 y %3 a %4 px")
                                                     .arg(g.nodeCount()).arg(i).arg(j).arg(d)));
                }
        }
    }

    void layout_userNodesStayAndAreAvoided() {
        const CanvasGeometry geo{QSizeF(1000, 700), 26};
        Graph g;
        g.addNode("X", {0.5, 0.05}, false); // justo donde iría el primer nodo automático
        for (int k = 0; k < 6; ++k) g.addNode(QString(QChar('A' + k)), {0.5, 0.5}, true);
        layoutAutoPlaced(g, geo, 50);
        QCOMPARE(g.node(0).pos, QPointF(0.5, 0.05));
        const QPointF fixed = geo.toPixel(g.node(0).pos);
        for (int i = 1; i < g.nodeCount(); ++i)
            QVERIFY(QLineF(geo.toPixel(g.node(i).pos), fixed).length() >= 50);
    }
};

QTEST_APPLESS_MAIN(TestShortestPath)
#include "tst_shortestpath.moc"
