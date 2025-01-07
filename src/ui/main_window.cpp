#include "main_window.hpp"
#include <QApplication>
#include <QScreen>
#include <QStyle>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setupUI();
    
    // 设置窗口标题
    setWindowTitle(tr("跨平台视频编解码工具"));
    
    // 设置窗口大小为屏幕大小的60%
    QScreen* screen = QApplication::primaryScreen();
    QSize screenSize = screen->availableSize();
    resize(screenSize.width() * 0.6, screenSize.height() * 0.7);
    
    // 设置最小窗口大小
    setMinimumSize(800, 600);
    
    // 将窗口移动到屏幕中央
    move((screenSize.width() - width()) / 2,
         (screenSize.height() - height()) / 2);
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUI() {
    // 创建中央部件
    centralWidget_ = new QWidget(this);
    setCentralWidget(centralWidget_);
    
    // 设置窗口最小尺寸
    this->setMinimumSize(800, 600);
    
    // 创建主布局
    mainLayout_ = new QHBoxLayout(centralWidget_);
    mainLayout_->setSpacing(0);
    mainLayout_->setContentsMargins(0, 0, 0, 0);
    
    // 创建并设置标签页
    createTabs();
    
    // 创建x264配置窗口
    x264Window_ = new X264ConfigWindow(this);
    
    // 将标签页和x264窗口添加到主布局
    mainLayout_->addWidget(tabWidget_);
    mainLayout_->addWidget(x264Window_);
    
    // 设置布局比例
    mainLayout_->setStretch(0, 0);  // 标签页不参与拉伸
    mainLayout_->setStretch(1, 1);  // x264窗口占用所有剩余空间
}

void MainWindow::createTabs() {
    tabWidget_ = new QTabWidget(this);
    tabWidget_->setTabPosition(QTabWidget::West);  // 标签页位于左侧
    
    // 添加标签页
    QWidget* x264Tab = new QWidget();
    QWidget* mp4Tab = new QWidget();
    QWidget* x265Tab = new QWidget();
    QWidget* h264Tab = new QWidget();
    
    // 设置标签页样式
    tabWidget_->setStyleSheet(R"(
        QTabWidget::pane {
            border: none;
            background: white;
        }
        QTabWidget::tab-bar {
            alignment: left;
            border: none;
        }
        QTabBar::tab {
            padding: 8px 16px;
            min-height: 40px;
            max-height: 40px;
            min-width: 150px;
            max-width: 150px;
            margin: 0;
            border: none;
            background: white;
            color: #000000;
            text-align: left;
            font-size: 13px;
        }
        QTabBar::tab:selected {
            background: #e5e5e5;
            border-left: 2px solid #0078d4;
        }
        QTabBar::tab:!selected:hover {
            background: #f0f0f0;
        }
    )");
    
    // 添加标签页并设置文字
    QTabBar* tabBar = tabWidget_->tabBar();
    
    tabWidget_->addTab(x264Tab, "");  // 添加空文本的标签
    tabWidget_->addTab(mp4Tab, "");
    tabWidget_->addTab(x265Tab, "");
    tabWidget_->addTab(h264Tab, "");
    
    // 设置标签页文字
    QStringList tabTexts = {"x264", "MP4", "x265", "H264"};
    for(int i = 0; i < tabBar->count(); i++) {
        QLabel* label = new QLabel(tabTexts[i]);
        label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        label->setStyleSheet("background: transparent; border: none; padding-left: 16px;");
        tabBar->setTabButton(i, QTabBar::LeftSide, label);
        tabBar->setTabToolTip(i, tabTexts[i]);
    }
    
    // 调整标签页宽度
    tabWidget_->setFixedWidth(150);
    
    // 连接标签页切换信号
    connect(tabWidget_, &QTabWidget::currentChanged, this, [this](int index) {
        // 根据选中的标签页显示相应的内容
        x264Window_->setVisible(index == 0);  // 只在第一个标签页显示x264窗口
        // TODO: 处理其他标签页的内容
    });
} 