#include "main_window.hpp"
#include <QApplication>
#include <QScreen>
#include <QStyle>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , tabWidget_(new QTabWidget(this))
    , x264ConfigWindow_(new X264ConfigWindow(this))
    , mp4ConfigWindow_(new MP4ConfigWindow(this))
    , aacConfigWindow_(new AACConfigWindow(this))
{
    setupUI();
    createTabs();
}

void MainWindow::setupUI()
{
    setWindowTitle("VideoLab");
    resize(1280, 800);
    setCentralWidget(tabWidget_);
}

void MainWindow::createTabs()
{
    // 添加标签页
    tabWidget_->addTab(x264ConfigWindow_, "X264");
    tabWidget_->addTab(mp4ConfigWindow_, "MP4");
    tabWidget_->addTab(aacConfigWindow_, "AAC");
} 