#include <QApplication>
#include "ui/main_window.hpp"

int main(int argc, char *argv[])
{
    // 强制使用 X11 显示服务器
    qputenv("QT_QPA_PLATFORM", "xcb");

    QApplication app(argc, argv);
    MainWindow window;
    window.show();
    return app.exec();
}   