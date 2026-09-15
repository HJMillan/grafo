#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPoint>
#include <QHash>
#include <QVector>
#include "GraphWidget.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btnAddEdge_clicked();
    void on_btnCalculate_clicked();
    void on_btnAddNode_clicked();
    void on_btnRemoveLast_clicked();
    void on_btnClearAll_clicked();
    void handleCanvasClick(const QPoint &p);
    void on_chkDirected_toggled(bool checked);

private:
    static constexpr int MAX_NODES = 26;
    static constexpr double INF = 1e15;

    Ui::MainWindow *ui;
    GraphWidget *graph = nullptr;

    QVector<QString> nodeNames;
    QHash<QString, int> nameToIndex;
    QVector<QVector<double>> adj; // matriz de adyacencia
    bool directed = false;
    bool uppercaseDefault = true;

    // Lógica
    void initGraphWidget();
    QString nextSuggestedName() const;
    QPoint suggestedPosition(int idx) const;
    bool canAddNode(const QString &name, QString &reason) const;
    bool addNodeInternal(const QString &name, const QPoint &pos);
    void removeLastNode();
    void clearGraph();
    void refreshNodeSelectors();
    void rebuildEdgesForView();
    bool parseWeight(double &w) const;
    void addEdge(int from, int to, double weight);
    void clearHighlights();

    // Algoritmos
    void runSelectedAlgorithm();
    void runDijkstra(int src, int dest);
    void runBellmanFord(int src, int dest);
    void runFloydWarshall(int src, int dest);
    QVector<int> buildPathFromPrev(const QVector<int> &prev, int dest) const;
    int countNegativeEdges() const;
};

#endif // MAINWINDOW_H
