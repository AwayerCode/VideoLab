#include <QApplication>
#include "ui/main_window.hpp"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    MainWindow mainWindow;
    mainWindow.show();
    
    return app.exec();
}   