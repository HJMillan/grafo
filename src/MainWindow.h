#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPoint>
#include <QString>
#include <QVector>

#include "GraphModel.h"
#include "GraphWidget.h"
#include "ShortestPath.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QEvent;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void on_btnAddEdge_clicked();
    void on_btnRemoveEdge_clicked();
    void on_btnCalculate_clicked();
    void on_btnAddNode_clicked();
    void on_btnRemoveLast_clicked();
    void on_btnClearAll_clicked();
    void on_btnUndo_clicked();
    void on_chkDirected_toggled(bool checked);
    void handleCanvasClick(const QPoint &p);
    void handleNodeAboutToMove();
    void handleNodeMoved();
    void handlePositionsChanged();
    void handleNodesLinked(int from, int to);

private:
    Ui::MainWindow *ui;
    GraphWidget *graph = nullptr;
    GraphModel model;

    bool uppercaseDefault = true;
    bool applyingTheme = false;
    bool statusLocked = false;

    void applyTheme();
    void initGraphWidget();
    void fitToScreen();
    void updateActionStates();
    void updateGuidance();
    void showNotice(const QString &text);
    void refreshStatus();
    QString graphSummary() const;

    QString nextSuggestedName() const;
    QPoint suggestedPosition(int idx) const;
    bool addNodeInternal(const QString &name, const QPoint &pos);
    void removeLastNode();
    void clearGraph();
    void refreshNodeSelectors();
    void syncQueryEndpoints();
    void rebuildEdgesForView();
    void syncNodesFromModel();
    bool parseWeight(double &w) const;
    void addEdge(int from, int to, double weight);
    void removeEdge(int from, int to);
    void clearHighlights();
    void showResultNotice(const QString &text);

    void pushUndo();
    void applySnapshot(const GraphSnapshot &s);

    void runSelectedAlgorithm();
    void runDijkstra(int src, int dest);
    void runBellmanFord(int src, int dest);
    void runFloydWarshall(int src, int dest);
    void presentAlgorithmResult(const ShortestPath::PathResult &result);
    int countNegativeEdges() const;
    void showPathResult(const QVector<int> &path, double distance);
    void showNegativeCycle(const QVector<int> &cycle);
};

#endif
