// ============================================================================
// main.cpp —— rt_reader GUI 入口
// ============================================================================
#include <QApplication>

#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    MainWindow w;
    w.show();
    return app.exec();
}
