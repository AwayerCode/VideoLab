#include <QApplication>
#include "ui/x264_config_window.hpp"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // 创建并显示x264配置窗口
    X264ConfigWindow window;
    window.show();
    
    return app.exec();
}   