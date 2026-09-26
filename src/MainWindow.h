#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPoint>
#include "Graph.h"
#include "GraphWidget.h"
#include "ShortestPath.h"

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
    Ui::MainWindow *ui;
    GraphWidget *graphView = nullptr;
    Graph graph;
    bool uppercaseDefault = true;

    void initGraphWidget();
    QString nextSuggestedName() const;
    QPoint suggestedPosition(int idx) const;
    bool canAddNode(const QString &name, QString &reason) const;
    bool addNodeInternal(const QString &name, const QPoint &pos, bool autoPlaced);
    void removeLastNode();
    void clearGraph();
    void refreshNodeSelectors();
    void graphChanged();
    bool parseWeight(double &w) const;
    void addEdge(int from, int to, double weight);
    void clearHighlights();

    void runSelectedAlgorithm();
    void showResult(const PathResult &result);
};

#endif // MAINWINDOW_H
