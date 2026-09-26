#include <QApplication>
#include <QStyleFactory>

#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Caminos más cortos");

    // Fusion aplica la hoja de estilos de forma predecible y respeta la
    // apariencia clara u oscura del sistema, de donde salen los tokens del tema.
    if (QStyleFactory::keys().contains("Fusion")) app.setStyle(QStyleFactory::create("Fusion"));

    MainWindow w;
    w.show();
    return app.exec();
}
