#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPoint>
#include <optional>
#include "Graph.h"
#include "ShortestPath.h"

class QAbstractButton;
class QLabel;
class QLineEdit;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onAddNode();
    void onRemoveLast();
    void onClearAll();
    void onCanvasClicked(const QPoint &p);
    void onAddEdge();
    void onCalculate();
    void onDirectedToggled(bool checked);

private:
    Ui::MainWindow *ui;
    Graph graph;
    bool resultShown = false; // hay un resultado en el banner que el grafo actual puede invalidar

    void setupConnections();

    // Nodos
    QString nextSuggestedName() const;
    QPoint suggestedPosition(int idx) const;
    QString nodeNameError(const QString &name) const;
    bool addNode(const QPoint &pos, bool autoPlaced);
    void refreshNodeSelectors();

    // Aristas
    struct WeightCheck {
        std::optional<double> value;
        QString error;
    };
    WeightCheck checkWeight() const;

    // Estado de la interfaz
    void graphChanged();
    void updateControls();
    static void setFieldError(QLineEdit *field, QLabel *label, const QString &error);
    static void setButtonEnabled(QAbstractButton *button, bool enabled, const QString &reason = {});

    // Resultado
    QString selectedAlgorithmName() const;
    void showResult(const PathResult &result, int src, int dest);
};

#endif // MAINWINDOW_H
