#include "main_window.hpp"
#include <QApplication>
#include <QScreen>
#include <QStyle>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setupUI();
    
    // Set window title
    setWindowTitle(tr("VideoLab"));
    
    // Set window size to 60% of screen size
    QScreen* screen = QApplication::primaryScreen();
    QSize screenSize = screen->availableSize();
    resize(screenSize.width() * 0.6, screenSize.height() * 0.7);
    
    // Set minimum window size
    setMinimumSize(800, 600);
    
    // Move window to screen center
    move((screenSize.width() - width()) / 2,
         (screenSize.height() - height()) / 2);
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUI() {
    // Create central widget
    centralWidget_ = new QWidget(this);
    setCentralWidget(centralWidget_);
    
    // Set minimum window size
    this->setMinimumSize(800, 600);
    
    // Create main layout
    mainLayout_ = new QHBoxLayout(centralWidget_);
    mainLayout_->setSpacing(0);
    mainLayout_->setContentsMargins(0, 0, 0, 0);
    
    // Create and setup tabs
    createTabs();
    
    // Create stacked widget
    stackedWidget_ = new QStackedWidget(this);
    
    // Create configuration windows
    x264Window_ = new X264ConfigWindow(this);
    mp4Window_ = new MP4ConfigWindow(this);  // Use new MP4ConfigWindow
    x265Window_ = new QWidget(this); // Temporary empty window
    h264Window_ = new QWidget(this); // Temporary empty window
    
    // Add windows to stacked widget
    stackedWidget_->addWidget(x264Window_);
    stackedWidget_->addWidget(mp4Window_);
    stackedWidget_->addWidget(x265Window_);
    stackedWidget_->addWidget(h264Window_);
    
    // Add tab widget and stacked widget to main layout
    mainLayout_->addWidget(tabWidget_);
    mainLayout_->addWidget(stackedWidget_);
    
    // Set layout stretch factors
    mainLayout_->setStretch(0, 0);  // Tab widget doesn't stretch
    mainLayout_->setStretch(1, 1);  // Stacked widget takes all remaining space
}

void MainWindow::createTabs() {
    tabWidget_ = new QTabWidget(this);
    tabWidget_->setTabPosition(QTabWidget::West);  // Tabs on the left side
    
    // Add tab pages
    QWidget* x264Tab = new QWidget();
    QWidget* mp4Tab = new QWidget();
    QWidget* x265Tab = new QWidget();
    QWidget* h264Tab = new QWidget();
    
    // Set tab widget style
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
    
    // Add tabs and set text
    QTabBar* tabBar = tabWidget_->tabBar();
    
    tabWidget_->addTab(x264Tab, "");  // Add tab with empty text
    tabWidget_->addTab(mp4Tab, "");
    tabWidget_->addTab(x265Tab, "");
    tabWidget_->addTab(h264Tab, "");
    
    // Set tab labels
    QStringList tabTexts = {"x264 Encoder", "MP4 Parser", "x265 Encoder", "H264 Parser"};
    for(int i = 0; i < tabBar->count(); i++) {
        QLabel* label = new QLabel(tabTexts[i]);
        label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        label->setStyleSheet("background: transparent; border: none; padding-left: 16px;");
        tabBar->setTabButton(i, QTabBar::LeftSide, label);
        tabBar->setTabToolTip(i, tabTexts[i]);
    }
    
    // Adjust tab width
    tabWidget_->setFixedWidth(150);
    
    // Connect tab change signal
    connect(tabWidget_, &QTabWidget::currentChanged, this, [this](int index) {
        // Switch to corresponding stacked widget page
        stackedWidget_->setCurrentIndex(index);
    });
} 