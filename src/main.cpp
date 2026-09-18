#include <QApplication>
#include <QStyleFactory>

#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Caminos más cortos");

    // Fusion aplica el stylesheet de forma predecible y respeta la apariencia
    // clara u oscura del sistema, que es de donde se resuelven los tokens.
    if (QStyleFactory::keys().contains("Fusion")) {
        app.setStyle(QStyleFactory::create("Fusion"));
    }

    MainWindow w;
    w.show();
    return app.exec();
}
