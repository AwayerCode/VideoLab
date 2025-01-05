#include "mainwindow.hpp"
#include "../ffmpeg/x264_param_test.hpp"
#include <QMessageBox>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , centralWidget_(new QWidget(this))
    , mainLayout_(new QVBoxLayout(centralWidget_))
{
    setCentralWidget(centralWidget_);
    setupUI();
    setWindowTitle("编码器性能测试");
    resize(800, 600);
}

void MainWindow::setupUI() {
    createEncoderGroup();
    createParameterGroup();
    createTestGroup();
    createResultGroup();

    // 连接信号和槽
    connect(startButton_, &QPushButton::clicked, this, &MainWindow::onStartTest);
    connect(sceneCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onSceneChanged);
}

void MainWindow::createEncoderGroup() {
    encoderGroup_ = new QGroupBox("编码器选择", centralWidget_);
    auto* layout = new QHBoxLayout(encoderGroup_);

    // 编码器选择
    auto* encoderLabel = new QLabel("编码器:", encoderGroup_);
    encoderCombo_ = new QComboBox(encoderGroup_);
    encoderCombo_->setObjectName("encoderCombo_");
    encoderCombo_->addItem("x264 (CPU)");
    encoderCombo_->addItem("NVENC (GPU)");

    // 场景选择
    auto* sceneLabel = new QLabel("预设场景:", encoderGroup_);
    sceneCombo_ = new QComboBox(encoderGroup_);
    sceneCombo_->setObjectName("sceneCombo_");
    sceneCombo_->addItem("自定义");
    sceneCombo_->addItem("直播场景");
    sceneCombo_->addItem("点播场景");
    sceneCombo_->addItem("存档场景");

    layout->addWidget(encoderLabel);
    layout->addWidget(encoderCombo_);
    layout->addSpacing(20);
    layout->addWidget(sceneLabel);
    layout->addWidget(sceneCombo_);
    layout->addStretch();

    mainLayout_->addWidget(encoderGroup_);
}

void MainWindow::createParameterGroup() {
    paramGroup_ = new QGroupBox("参数设置", centralWidget_);
    auto* layout = new QGridLayout(paramGroup_);

    // 分辨率设置
    auto* resLabel = new QLabel("分辨率:", paramGroup_);
    widthSpin_ = new QSpinBox(paramGroup_);
    widthSpin_->setObjectName("widthSpin_");
    widthSpin_->setRange(320, 3840);
    widthSpin_->setValue(1920);
    auto* xLabel = new QLabel("x", paramGroup_);
    heightSpin_ = new QSpinBox(paramGroup_);
    heightSpin_->setObjectName("heightSpin_");
    heightSpin_->setRange(240, 2160);
    heightSpin_->setValue(1080);

    // 帧数设置
    auto* frameLabel = new QLabel("测试帧数:", paramGroup_);
    frameCountSpin_ = new QSpinBox(paramGroup_);
    frameCountSpin_->setObjectName("frameCountSpin_");
    frameCountSpin_->setRange(30, 3000);
    frameCountSpin_->setValue(300);
    frameCountSpin_->setSingleStep(30);

    // 线程数设置
    auto* threadLabel = new QLabel("线程数:", paramGroup_);
    threadsSpin_ = new QSpinBox(paramGroup_);
    threadsSpin_->setObjectName("threadsSpin_");
    threadsSpin_->setRange(1, 32);
    threadsSpin_->setValue(20);

    // 硬件加速选项
    hwAccelCheck_ = new QCheckBox("启用硬件加速", paramGroup_);
    hwAccelCheck_->setObjectName("hwAccelCheck_");

    // 布局
    int row = 0;
    layout->addWidget(resLabel, row, 0);
    layout->addWidget(widthSpin_, row, 1);
    layout->addWidget(xLabel, row, 2);
    layout->addWidget(heightSpin_, row, 3);

    row++;
    layout->addWidget(frameLabel, row, 0);
    layout->addWidget(frameCountSpin_, row, 1);

    row++;
    layout->addWidget(threadLabel, row, 0);
    layout->addWidget(threadsSpin_, row, 1);

    row++;
    layout->addWidget(hwAccelCheck_, row, 0, 1, 2);

    mainLayout_->addWidget(paramGroup_);
}

void MainWindow::createTestGroup() {
    testGroup_ = new QGroupBox("测试控制", centralWidget_);
    auto* layout = new QVBoxLayout(testGroup_);

    startButton_ = new QPushButton("开始测试", testGroup_);
    startButton_->setObjectName("startButton_");
    progressBar_ = new QProgressBar(testGroup_);
    progressBar_->setObjectName("progressBar_");
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);

    layout->addWidget(startButton_);
    layout->addWidget(progressBar_);

    mainLayout_->addWidget(testGroup_);
}

void MainWindow::createResultGroup() {
    resultGroup_ = new QGroupBox("测试结果", centralWidget_);
    auto* layout = new QVBoxLayout(resultGroup_);

    resultText_ = new QTextEdit(resultGroup_);
    resultText_->setObjectName("resultText_");
    resultText_->setReadOnly(true);

    layout->addWidget(resultText_);

    mainLayout_->addWidget(resultGroup_);
}

void MainWindow::onStartTest() {
    // 禁用开始按钮
    startButton_->setEnabled(false);
    progressBar_->setValue(0);
    resultText_->clear();

    // 获取测试参数
    X264ParamTest::TestConfig config;
    config.width = widthSpin_->value();
    config.height = heightSpin_->value();
    config.frameCount = frameCountSpin_->value();
    config.threads = threadsSpin_->value();

    // 根据场景设置其他参数
    switch (sceneCombo_->currentIndex()) {
        case 1: {  // 直播场景
            auto scene = X264ParamTest::getLiveStreamConfig();
            config = scene.config;
            break;
        }
        case 2: {  // 点播场景
            auto scene = X264ParamTest::getVODConfig();
            config = scene.config;
            break;
        }
        case 3: {  // 存档场景
            auto scene = X264ParamTest::getArchiveConfig();
            config = scene.config;
            break;
        }
        default:
            break;
    }

    // 更新分辨率和帧数
    config.width = widthSpin_->value();
    config.height = heightSpin_->value();
    config.frameCount = frameCountSpin_->value();

    // 运行测试
    auto result = X264ParamTest::runTest(config,
        [this](int progress, const X264ParamTest::TestResult& current) {
            onUpdateProgress(progress, current.fps, current.bitrate);
        });

    // 显示结果
    if (result.success) {
        QString resultStr;
        resultStr += QString("编码完成\n");
        resultStr += QString("总编码时间: %1 秒\n").arg(result.encodingTime, 0, 'f', 2);
        resultStr += QString("平均编码速度: %1 fps\n").arg(result.fps, 0, 'f', 2);
        resultStr += QString("平均码率: %1 Kbps\n").arg(result.bitrate / 1000.0, 0, 'f', 2);
        if (result.psnr > 0) {
            resultStr += QString("PSNR: %1 dB\n").arg(result.psnr, 0, 'f', 2);
        }
        if (result.ssim > 0) {
            resultStr += QString("SSIM: %1\n").arg(result.ssim, 0, 'f', 4);
        }
        resultText_->setText(resultStr);
    } else {
        QMessageBox::warning(this, "错误", "测试失败: " + QString::fromStdString(result.errorMessage));
    }

    // 恢复开始按钮
    startButton_->setEnabled(true);
}

void MainWindow::onSceneChanged(int index) {
    // 根据选择的场景更新参数
    switch (index) {
        case 1: {  // 直播场景
            auto scene = X264ParamTest::getLiveStreamConfig();
            threadsSpin_->setValue(8);
            break;
        }
        case 2: {  // 点播场景
            auto scene = X264ParamTest::getVODConfig();
            threadsSpin_->setValue(16);
            break;
        }
        case 3: {  // 存档场景
            auto scene = X264ParamTest::getArchiveConfig();
            threadsSpin_->setValue(32);
            break;
        }
        default: {  // 自定义场景
            threadsSpin_->setValue(20);
            break;
        }
    }
}

void MainWindow::onUpdateProgress(int progress, double fps, double bitrate) {
    progressBar_->setValue(progress);
    QString status = QString("进度: %1% | 速度: %2 fps | 码率: %3 Kbps")
                        .arg(progress)
                        .arg(fps, 0, 'f', 2)
                        .arg(bitrate / 1000.0, 0, 'f', 2);
    statusBar()->showMessage(status);
    emit progressUpdated(progress, fps, bitrate);
} 