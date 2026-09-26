#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QTest>
#include <QTimer>
#include <QValidator>

#include "Graph.h"
#include "GraphWidget.h"
#include <QContextMenuEvent>
#include <QMenu>
#include "MainWindow.h"
#include "ResultBanner.h"

namespace {
// Responde a la próxima ventana modal pulsando el botón con ese texto.
// Guarda en *title el título de la ventana para poder comprobarlo.
void answerNextDialog(const QString &buttonText, QString *title = nullptr) {
    QTimer::singleShot(0, [=] {
        auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
        QVERIFY2(box, "se esperaba una ventana de confirmación");
        if (title) *title = box->text();
        for (QAbstractButton *b : box->buttons()) {
            if (b->text() == buttonText) {
                b->click();
                return;
            }
        }
        QFAIL(qPrintable("no hay botón " + buttonText));
    });
}
}

class TestMainWindow : public QObject {
    Q_OBJECT

    MainWindow *w = nullptr;

    template <typename T> T *get(const char *name) {
        T *child = w->findChild<T *>(name);
        if (!child) qFatal("falta el widget %s", name);
        return child;
    }
    QPushButton *button(const char *name) { return get<QPushButton>(name); }
    ResultBanner *banner() { return get<ResultBanner>("resultBanner"); }

    void addNodes(int count) {
        for (int i = 0; i < count; ++i) button("btnAddNode")->click();
    }
    void addEdge(const QString &from, const QString &to, const QString &weight) {
        get<QComboBox>("cbEdgeFrom")->setCurrentText(from);
        get<QComboBox>("cbEdgeTo")->setCurrentText(to);
        get<QLineEdit>("txtWeight")->setText(weight);
        QVERIFY2(button("btnAddEdge")->isEnabled(), qPrintable(get<QLabel>("lblWeightError")->text()));
        button("btnAddEdge")->click();
    }
    void calculate(const QString &from, const QString &to) {
        get<QComboBox>("cbOrigin")->setCurrentText(from);
        get<QComboBox>("cbDestination")->setCurrentText(to);
        QVERIFY(button("btnCalculate")->isEnabled());
        button("btnCalculate")->click();
    }

private slots:
    void init() {
        w = new MainWindow;
        w->resize(1100, 700);
        w->show();
    }
    void cleanup() {
        delete w;
        w = nullptr;
    }

    void startup_buttonsExplainWhyDisabled() {
        QVERIFY(!button("btnCalculate")->isEnabled());
        QVERIFY(button("btnCalculate")->toolTip().contains("Agregue al menos un nodo"));
        QVERIFY(!button("btnAddEdge")->isEnabled());
        QVERIFY(!button("btnAddEdge")->toolTip().isEmpty());
        QVERIFY(get<QLabel>("lblWeightError")->isHidden());
        QCOMPARE(banner()->kind(), ResultBanner::Info);
    }

    void nodes_duplicateNameSuggestsOtherCase() {
        addNodes(1); // A
        get<QLineEdit>("txtNodeName")->setText("A");
        QLabel *err = get<QLabel>("lblNodeError");
        QVERIFY(!err->isHidden());
        QVERIFY(err->text().contains("«a»"));
        QVERIFY(get<QLineEdit>("txtNodeName")->property("error").toBool());
        QVERIFY(!button("btnAddNode")->isEnabled());
        get<QLineEdit>("txtNodeName")->setText("a");
        QVERIFY(err->isHidden());
        QVERIFY(button("btnAddNode")->isEnabled());
    }

    void weight_acceptsDecimalComma() {
        addNodes(2);
        addEdge("A", "B", "2,5");
        calculate("A", "B");
        QCOMPARE(banner()->kind(), ResultBanner::Success);
        QCOMPARE(banner()->title(), QString("Distancia de A a B: 2,5"));
        QVERIFY(banner()->text().contains("A → B"));
    }

    void weight_validatorBlocksLetters() {
        addNodes(1);
        QLineEdit *field = get<QLineEdit>("txtWeight");
        field->clear();
        QTest::keyClicks(field, "inf");
        QCOMPARE(field->text(), QString());
        QVERIFY(!get<QLabel>("lblWeightError")->isHidden()); // «Escriba un peso»
    }

    void weight_outOfRange() {
        addNodes(1);
        get<QLineEdit>("txtWeight")->setText("2000000000");
        QVERIFY(get<QLabel>("lblWeightError")->text().contains("entre"));
        QVERIFY(!button("btnAddEdge")->isEnabled());
    }

    void weight_negativeInUndirectedIsExplained() {
        addNodes(2);
        get<QComboBox>("cbEdgeFrom")->setCurrentText("A");
        get<QComboBox>("cbEdgeTo")->setCurrentText("B");
        get<QLineEdit>("txtWeight")->setText("-2");
        const QString err = get<QLabel>("lblWeightError")->text();
        QVERIFY(err.contains("A→B→A"));
        QVERIFY(err.contains("Grafo dirigido"));
        QVERIFY(!button("btnAddEdge")->isEnabled());
        QCOMPARE(button("btnAddEdge")->toolTip(), err);
    }

    void weight_negativeBlockedWhileDijkstraSelected() {
        get<QCheckBox>("chkDirected")->setChecked(true);
        addNodes(2);
        get<QLineEdit>("txtWeight")->setText("-2");
        QVERIFY(get<QLabel>("lblWeightError")->text().contains("Dijkstra no admite"));
        get<QRadioButton>("rbtnBellman")->setChecked(true);
        QVERIFY(get<QLabel>("lblWeightError")->isHidden());
        QVERIFY(button("btnAddEdge")->isEnabled());
    }

    void dijkstra_disabledWhileNegativeEdgesExist() {
        get<QCheckBox>("chkDirected")->setChecked(true);
        get<QRadioButton>("rbtnBellman")->setChecked(true);
        addNodes(3);
        addEdge("C", "B", "-1");
        get<QRadioButton>("rbtnDijkstra")->setChecked(true);
        QLabel *warn = get<QLabel>("lblAlgoWarning");
        QVERIFY(!warn->isHidden());
        QVERIFY(warn->text().contains("C→B"));
        QVERIFY(!button("btnCalculate")->isEnabled());
        QCOMPARE(button("btnCalculate")->toolTip(), warn->text());
        get<QRadioButton>("rbtnFloyd")->setChecked(true);
        QVERIFY(warn->isHidden());
        QVERIFY(button("btnCalculate")->isEnabled());
    }

    void negativeCycle_isShownAsError() {
        get<QCheckBox>("chkDirected")->setChecked(true);
        get<QRadioButton>("rbtnBellman")->setChecked(true);
        addNodes(3);
        addEdge("A", "B", "1");
        addEdge("B", "C", "-3");
        addEdge("C", "B", "1");
        calculate("A", "C");
        QCOMPARE(banner()->kind(), ResultBanner::Error);
        QCOMPARE(banner()->title(), QString("No hay camino mínimo de A a C"));
        QVERIFY(banner()->text().contains("peso -2"));
    }

    void result_isInvalidatedWhenGraphChanges() {
        addNodes(2);
        addEdge("A", "B", "1");
        calculate("A", "B");
        QCOMPARE(banner()->kind(), ResultBanner::Success);
        addEdge("A", "B", "3");
        QCOMPARE(banner()->title(), QString("El grafo cambió"));
    }

    void modeSwitch_cancelKeepsDirectedAndEdges() {
        get<QCheckBox>("chkDirected")->setChecked(true);
        addNodes(2);
        addEdge("B", "A", "1");
        QString title;
        answerNextDialog("Cancelar", &title);
        get<QCheckBox>("chkDirected")->setChecked(false);
        QCOMPARE(title, QString("¿Cambiar a grafo no dirigido?"));
        QVERIFY(get<QCheckBox>("chkDirected")->isChecked());
        calculate("B", "A");
        QCOMPARE(banner()->kind(), ResultBanner::Success);
    }

    void modeSwitch_acceptClearsEdges() {
        get<QCheckBox>("chkDirected")->setChecked(true);
        addNodes(2);
        addEdge("B", "A", "1");
        answerNextDialog("Borrar aristas y cambiar");
        get<QCheckBox>("chkDirected")->setChecked(false);
        QVERIFY(!get<QCheckBox>("chkDirected")->isChecked());
        calculate("B", "A");
        QCOMPARE(banner()->title(), QString("No hay camino de B a A"));
    }

    void modeSwitch_symmetricNeedsNoConfirmation() {
        addNodes(2);
        addEdge("A", "B", "1");                            // no dirigido: A⇄B
        get<QCheckBox>("chkDirected")->setChecked(true);   // sin ventana
        get<QCheckBox>("chkDirected")->setChecked(false);  // sigue simétrico: sin ventana
        QVERIFY(!get<QCheckBox>("chkDirected")->isChecked());
        calculate("B", "A");
        QCOMPARE(banner()->kind(), ResultBanner::Success);
    }

    void clearAll_asksForConfirmation() {
        addNodes(2);
        answerNextDialog("Cancelar");
        button("btnClearAll")->click();
        QCOMPARE(get<QComboBox>("cbOrigin")->count(), 2);
        answerNextDialog("Borrar todo");
        button("btnClearAll")->click();
        QCOMPARE(get<QComboBox>("cbOrigin")->count(), 0);
    }
    void canvas_clickCreatesNodeWhereFree() {
        auto *canvas = get<GraphWidget>("graphView");
        QTest::mouseClick(canvas, Qt::LeftButton, {}, QPoint(100, 100));
        QCOMPARE(get<QComboBox>("cbOrigin")->count(), 1);
        // Sobre el nodo: no crea otro.
        QTest::mouseClick(canvas, Qt::LeftButton, {}, QPoint(105, 100));
        QCOMPARE(get<QComboBox>("cbOrigin")->count(), 1);
        // Pegado al nodo: no cabe y lo explica.
        QTest::mouseClick(canvas, Qt::LeftButton, {}, QPoint(135, 100));
        QCOMPARE(get<QComboBox>("cbOrigin")->count(), 1);
        QCOMPARE(banner()->kind(), ResultBanner::Warning);
        QCOMPARE(banner()->title(), QString("No se puede colocar el nodo"));
        // Lejos: se crea.
        QTest::mouseClick(canvas, Qt::LeftButton, {}, QPoint(300, 200));
        QCOMPARE(get<QComboBox>("cbOrigin")->count(), 2);
    }
    void nodes_nameOnlyAcceptsAsciiLetters() {
        const QValidator *v = get<QLineEdit>("txtNodeName")->validator();
        QVERIFY(v);
        for (QString bad : {QString("ñ"), QString("á"), QString("1"), QString("-")}) {
            int pos = 0;
            QCOMPARE(v->validate(bad, pos), QValidator::Invalid);
        }
        QString good("q");
        int pos = 0;
        QCOMPARE(v->validate(good, pos), QValidator::Acceptable);
    }
    void edges_removeAndUpdateWeight() {
        addNodes(2);
        get<QComboBox>("cbEdgeFrom")->setCurrentText("A");
        get<QComboBox>("cbEdgeTo")->setCurrentText("B");
        QVERIFY(!button("btnRemoveEdge")->isEnabled());
        QCOMPARE(button("btnRemoveEdge")->toolTip(), QString("No existe una arista de A a B."));
        QCOMPARE(button("btnAddEdge")->text(), QString("Agregar arista"));
        addEdge("A", "B", "4");
        QCOMPARE(button("btnAddEdge")->text(), QString("Cambiar peso"));
        QVERIFY(button("btnRemoveEdge")->isEnabled());
        button("btnRemoveEdge")->click();
        QVERIFY(!button("btnRemoveEdge")->isEnabled());
        calculate("A", "B");
        QCOMPARE(banner()->title(), QString("No hay camino de A a B"));
    }

    void drag_movesNodeAndFixesIt() {
        auto *canvas = get<GraphWidget>("graphView");
        addNodes(1);
        const Graph *g = canvas->graph();
        const QPoint start = canvas->canvasGeometry().toPixel(g->node(0).pos).toPoint();
        const QPoint end(200, 300);
        QTest::mousePress(canvas, Qt::LeftButton, {}, start);
        QMouseEvent move(QEvent::MouseMove, QPointF(end), canvas->mapToGlobal(QPointF(end)),
                         Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(canvas, &move);
        QTest::mouseRelease(canvas, Qt::LeftButton, {}, end);
        QVERIFY(!g->node(0).autoPlaced);
        const QPointF moved = g->node(0).pos;
        QVERIFY(QLineF(canvas->canvasGeometry().toPixel(moved), QPointF(end)).length() < 1.0);
        addNodes(3); // se reparten los nuevos; el arrastrado no se mueve
        QCOMPARE(g->node(0).pos, moved);
        QCOMPARE(get<QComboBox>("cbOrigin")->count(), 4); // soltar no creó nodos
    }

    void contextMenu_removesAnyNodeWithConfirmation() {
        auto *canvas = get<GraphWidget>("graphView");
        addNodes(3);
        addEdge("A", "B", "1");
        addEdge("B", "C", "1");
        const QPoint onB = canvas->canvasGeometry().toPixel(canvas->graph()->node(1).pos).toPoint();
        QString confirmTitle;
        QTimer::singleShot(0, [&] {
            auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
            QVERIFY2(menu, "se esperaba el menú del nodo");
            QCOMPARE(menu->actions().first()->text(), QString("Eliminar nodo «B»"));
            answerNextDialog("Eliminar nodo", &confirmTitle);
            menu->setActiveAction(menu->actions().first());
            QTest::keyClick(menu, Qt::Key_Return);
        });
        QContextMenuEvent ev(QContextMenuEvent::Mouse, onB, canvas->mapToGlobal(onB));
        QApplication::sendEvent(canvas, &ev);
        QCOMPARE(confirmTitle, QString("¿Eliminar el nodo «B»?"));
        auto *origin = get<QComboBox>("cbOrigin");
        QCOMPARE(origin->count(), 2);
        QCOMPARE(origin->itemText(0), QString("A"));
        QCOMPARE(origin->itemText(1), QString("C"));
    }
};

QTEST_MAIN(TestMainWindow)
#include "tst_mainwindow.moc"
