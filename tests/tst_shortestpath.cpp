#include <QRandomGenerator>
#include <QTest>

#include "Graph.h"
#include "ShortestPath.h"

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
        QCOMPARE(g.asymmetricArcs().size(), 2); // B->C y C->B difieren
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
};

QTEST_APPLESS_MAIN(TestShortestPath)
#include "tst_shortestpath.moc"
