#include "mainwindow.h"
#include <QApplication>
#include <QFile>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName("DiffyInAJiffy");
    app.setApplicationName("DiffyInAJiffy");
    app.setApplicationVersion("1.0.0");
    QFile styleFile(":/styles/app.qss");
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        app.setStyleSheet(QString::fromUtf8(styleFile.readAll()));
    }
    
    MainWindow window;
    window.show();
    
    return app.exec();
}
