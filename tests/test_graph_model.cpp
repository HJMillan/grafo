#include "GraphModel.h"
#include "GraphConstants.h"
#include "test_support.h"

#include <QPoint>
#include <QString>
#include <string>

void runGraphModelTests() {
    std::cout << "GraphModel tests\n";

    {
        GraphModel m;
        std::string errors;
        check(m.addNode(QStringLiteral("A"), QPoint(10, 20)), errors, "add A");
        check(m.addNode(QStringLiteral("B"), QPoint(30, 40)), errors, "add B");
        check(m.nodeCount() == 2, errors, "node count");
        check(m.positions().size() == 2, errors, "position count");
        check(m.positions()[0] == QPoint(10, 20), errors, "A position");
        check(m.positions()[1] == QPoint(30, 40), errors, "B position");
        finishCase("M1", errors);
    }

    {
        GraphModel m;
        m.addNode(QStringLiteral("A"), QPoint(0, 0));
        m.addNode(QStringLiteral("B"), QPoint(1, 0));
        std::string errors;
        check(!m.addEdge(1, 0, 3.0), errors, "new edge should return false");
        check(m.userEdges().size() == 1, errors, "one undirected edge");
        check(m.userEdges()[0].from == 0 && m.userEdges()[0].to == 1, errors, "canonical A–B");
        check(GraphConstants::sameWeight(m.userEdges()[0].weight, 3.0), errors, "weight");
        check(GraphConstants::sameWeight(m.adjacency()[0][1], 3.0), errors, "adj AB");
        check(GraphConstants::sameWeight(m.adjacency()[1][0], 3.0), errors, "adj BA mirrored");
        finishCase("M2", errors);
    }

    {
        GraphModel m;
        m.addNode(QStringLiteral("A"), QPoint(0, 0));
        m.addNode(QStringLiteral("B"), QPoint(1, 0));
        m.addEdge(0, 1, 3.0);
        double old = 0.0;
        std::string errors;
        check(m.addEdge(1, 0, 5.0, &old), errors, "update should return true");
        check(GraphConstants::sameWeight(old, 3.0), errors, "old weight");
        check(m.userEdges().size() == 1, errors, "still one edge");
        check(GraphConstants::sameWeight(m.userEdges()[0].weight, 5.0), errors, "new weight");
        finishCase("M3", errors);
    }

    {
        GraphModel m;
        std::string errors;
        for (int i = 0; i < 26; ++i) {
            const QString name(QChar('A' + i));
            if (!m.addNode(name, QPoint(i, 0))) errors += "failed " + name.toStdString() + "; ";
        }
        QString reason;
        check(!m.canAddNode(QStringLiteral("a"), reason), errors, "27th node should fail");
        check(m.nodeCount() == 26, errors, "still 26");
        finishCase("M4", errors);
    }

    {
        GraphModel m;
        m.addNode(QStringLiteral("A"), QPoint(0, 0));
        m.addNode(QStringLiteral("B"), QPoint(1, 0));
        m.addEdge(0, 1, 3.0);
        std::string errors;
        const QString notice = m.expandUndirectedEdges();
        m.setDirected(true);
        check(!notice.isEmpty(), errors, "expand notice");
        check(m.userEdges().size() == 2, errors, "two directed edges");
        check(m.userEdges()[0].from == 0 && m.userEdges()[0].to == 1, errors, "A→B");
        check(m.userEdges()[1].from == 1 && m.userEdges()[1].to == 0, errors, "B→A");
        finishCase("M5", errors);
    }

    {
        GraphModel m;
        m.setDirected(true);
        m.addNode(QStringLiteral("A"), QPoint(0, 0));
        m.addNode(QStringLiteral("B"), QPoint(1, 0));
        m.addEdge(0, 1, 3.0);
        m.addEdge(1, 0, 5.0);
        std::string errors;
        const QString notice = m.collapseDirectedEdges();
        m.setDirected(false);
        check(m.userEdges().size() == 1, errors, "one undirected edge");
        check(m.userEdges()[0].from == 0 && m.userEdges()[0].to == 1, errors, "stored A–B");
        check(GraphConstants::sameWeight(m.userEdges()[0].weight, 5.0), errors, "winner is last index");
        check(notice.contains(QStringLiteral("Queda 5")), errors, "conflict copy");
        finishCase("M6", errors);
    }

    {
        GraphModel m;
        m.addNode(QStringLiteral("A"), QPoint(4, 8));
        m.addNode(QStringLiteral("B"), QPoint(9, 1));
        m.addEdge(0, 1, 2.0);
        m.pushUndo();
        m.addNode(QStringLiteral("C"), QPoint(0, 0));
        m.addEdge(1, 2, 4.0);
        GraphSnapshot s;
        std::string errors;
        check(m.popUndo(s), errors, "pop undo");
        m.apply(s);
        check(m.nodeCount() == 2, errors, "restored 2 nodes");
        check(m.userEdges().size() == 1, errors, "restored 1 edge");
        check(m.positions()[0] == QPoint(4, 8), errors, "restored A pos");
        check(!m.popUndo(s), errors, "stack empty");
        finishCase("M7", errors);
    }

    {
        GraphModel m;
        m.addNode(QStringLiteral("A"), QPoint(0, 0));
        m.addNode(QStringLiteral("B"), QPoint(1, 0));
        m.addNode(QStringLiteral("C"), QPoint(2, 0));
        m.addEdge(0, 2, 1.0);
        m.addEdge(1, 2, 1.0);
        m.removeLastNode();
        std::string errors;
        check(m.nodeCount() == 2, errors, "two nodes left");
        check(m.positions().size() == 2, errors, "two positions");
        check(m.userEdges().isEmpty(), errors, "incident edges dropped");
        finishCase("M8", errors);
    }

    {
        GraphModel m;
        m.addNode(QStringLiteral("A"), QPoint(0, 0));
        std::string errors;
        check(!m.addEdge(0, 0, 1.5), errors, "self-loop is new");
        check(m.userEdges().size() == 1, errors, "loop stored");
        check(m.userEdges()[0].from == 0 && m.userEdges()[0].to == 0, errors, "A→A");
        check(!GraphConstants::isMissing(m.adjacency()[0][0]), errors, "adj loop");
        finishCase("M9", errors);
    }
}
