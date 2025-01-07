#include "main_window.hpp"
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , tabWidget_(new QTabWidget(this))
    , x264ConfigWindow_(new X264ConfigWindow(this))
    , vp8ConfigWindow_(new VP8ConfigWindow(this))
    , aacConfigWindow_(new AACConfigWindow(this))
    , mp4ConfigWindow_(new MP4ConfigWindow(this))
{
    setupUI();
}

void MainWindow::setupUI()
{
    // 设置中心部件
    auto centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // 创建主布局
    auto mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // 添加标签页
    tabWidget_->addTab(x264ConfigWindow_, "X264");
    tabWidget_->addTab(vp8ConfigWindow_, "VP8");
    tabWidget_->addTab(aacConfigWindow_, "AAC");
    tabWidget_->addTab(mp4ConfigWindow_, "MP4");

    mainLayout->addWidget(tabWidget_);

    // 设置窗口标题和大小
    setWindowTitle(tr("编码器测试工具"));
    resize(1024, 768);
} 