#include "x264_config_window.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QApplication>
#include <thread>
#include <chrono>

X264ConfigWindow::X264ConfigWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();
    createConnections();
    
    // 设置默认配置
    X264ParamTest::TestConfig defaultConfig;
    updateUIFromConfig(defaultConfig);
}

X264ConfigWindow::~X264ConfigWindow() = default;

void X264ConfigWindow::setupUI()
{
    auto* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    auto* mainLayout = new QHBoxLayout(centralWidget);
    auto* leftLayout = new QVBoxLayout();
    auto* rightLayout = new QVBoxLayout();
    mainLayout->addLayout(leftLayout, 2);
    mainLayout->addLayout(rightLayout, 1);
    
    // 左侧参数设置区域
    // 基本参数组
    auto* basicGroup = new QGroupBox(tr("基本参数"), this);
    auto* basicLayout = new QGridLayout(basicGroup);
    
    presetCombo_ = new QComboBox(this);
    presetCombo_->addItems({
        "UltraFast", "SuperFast", "VeryFast", "Faster",
        "Fast", "Medium", "Slow", "Slower", "VerySlow", "Placebo"
    });
    
    tuneCombo_ = new QComboBox(this);
    tuneCombo_->addItems({
        "None", "Film", "Animation", "Grain", "StillImage",
        "PSNR", "SSIM", "FastDecode", "ZeroLatency"
    });
    
    threadsSpinBox_ = new QSpinBox(this);
    threadsSpinBox_->setRange(1, 128);
    
    basicLayout->addWidget(new QLabel(tr("预设:")), 0, 0);
    basicLayout->addWidget(presetCombo_, 0, 1);
    basicLayout->addWidget(new QLabel(tr("调优:")), 1, 0);
    basicLayout->addWidget(tuneCombo_, 1, 1);
    basicLayout->addWidget(new QLabel(tr("线程数:")), 2, 0);
    basicLayout->addWidget(threadsSpinBox_, 2, 1);
    
    // 分辨率和帧数组
    auto* videoGroup = new QGroupBox(tr("视频参数"), this);
    auto* videoLayout = new QGridLayout(videoGroup);
    
    widthSpinBox_ = new QSpinBox(this);
    widthSpinBox_->setRange(16, 7680);
    heightSpinBox_ = new QSpinBox(this);
    heightSpinBox_->setRange(16, 4320);
    frameCountSpinBox_ = new QSpinBox(this);
    frameCountSpinBox_->setRange(1, 1000000);
    
    videoLayout->addWidget(new QLabel(tr("宽度:")), 0, 0);
    videoLayout->addWidget(widthSpinBox_, 0, 1);
    videoLayout->addWidget(new QLabel(tr("高度:")), 1, 0);
    videoLayout->addWidget(heightSpinBox_, 1, 1);
    videoLayout->addWidget(new QLabel(tr("帧数:")), 2, 0);
    videoLayout->addWidget(frameCountSpinBox_, 2, 1);
    
    // 码率控制组
    auto* rateGroup = new QGroupBox(tr("码率控制"), this);
    auto* rateLayout = new QGridLayout(rateGroup);
    
    rateControlCombo_ = new QComboBox(this);
    rateControlCombo_->addItems({
        tr("CRF (恒定质量)"),
        tr("CQP (恒定量化)"),
        tr("ABR (平均码率)"),
        tr("CBR (恒定码率)")
    });
    
    rateValueSpinBox_ = new QSpinBox(this);
    rateValueSpinBox_->setRange(0, 51);
    
    rateLayout->addWidget(new QLabel(tr("控制模式:")), 0, 0);
    rateLayout->addWidget(rateControlCombo_, 0, 1);
    rateLayout->addWidget(new QLabel(tr("参数值:")), 1, 0);
    rateLayout->addWidget(rateValueSpinBox_, 1, 1);
    
    // GOP参数组
    auto* gopGroup = new QGroupBox(tr("GOP参数"), this);
    auto* gopLayout = new QGridLayout(gopGroup);
    
    keyintMaxSpinBox_ = new QSpinBox(this);
    keyintMaxSpinBox_->setRange(1, 1000);
    bframesSpinBox_ = new QSpinBox(this);
    bframesSpinBox_->setRange(0, 16);
    refsSpinBox_ = new QSpinBox(this);
    refsSpinBox_->setRange(1, 16);
    
    gopLayout->addWidget(new QLabel(tr("关键帧间隔:")), 0, 0);
    gopLayout->addWidget(keyintMaxSpinBox_, 0, 1);
    gopLayout->addWidget(new QLabel(tr("B帧数量:")), 1, 0);
    gopLayout->addWidget(bframesSpinBox_, 1, 1);
    gopLayout->addWidget(new QLabel(tr("参考帧数:")), 2, 0);
    gopLayout->addWidget(refsSpinBox_, 2, 1);
    
    // 质量参数组
    auto* qualityGroup = new QGroupBox(tr("质量参数"), this);
    auto* qualityLayout = new QGridLayout(qualityGroup);
    
    fastFirstPassCheckBox_ = new QCheckBox(tr("快速首遍编码"), this);
    meRangeSpinBox_ = new QSpinBox(this);
    meRangeSpinBox_->setRange(4, 64);
    weightedPredCheckBox_ = new QCheckBox(tr("加权预测"), this);
    cabacCheckBox_ = new QCheckBox(tr("CABAC熵编码"), this);
    
    qualityLayout->addWidget(fastFirstPassCheckBox_, 0, 0, 1, 2);
    qualityLayout->addWidget(new QLabel(tr("运动估计范围:")), 1, 0);
    qualityLayout->addWidget(meRangeSpinBox_, 1, 1);
    qualityLayout->addWidget(weightedPredCheckBox_, 2, 0, 1, 2);
    qualityLayout->addWidget(cabacCheckBox_, 3, 0, 1, 2);
    
    // 添加所有组到左侧布局
    leftLayout->addWidget(basicGroup);
    leftLayout->addWidget(videoGroup);
    leftLayout->addWidget(rateGroup);
    leftLayout->addWidget(gopGroup);
    leftLayout->addWidget(qualityGroup);
    
    // 右侧控制和显示区域
    sceneConfigCombo_ = new QComboBox(this);
    sceneConfigCombo_->addItems({
        tr("默认配置"),
        tr("直播场景"),
        tr("点播场景"),
        tr("存档场景")
    });
    
    auto* buttonLayout = new QHBoxLayout();
    startButton_ = new QPushButton(tr("开始编码"), this);
    stopButton_ = new QPushButton(tr("停止编码"), this);
    stopButton_->setEnabled(false);
    buttonLayout->addWidget(startButton_);
    buttonLayout->addWidget(stopButton_);
    
    progressBar_ = new QProgressBar(this);
    logTextEdit_ = new QTextEdit(this);
    logTextEdit_->setReadOnly(true);
    
    rightLayout->addWidget(new QLabel(tr("预设场景:")));
    rightLayout->addWidget(sceneConfigCombo_);
    rightLayout->addLayout(buttonLayout);
    rightLayout->addWidget(progressBar_);
    rightLayout->addWidget(logTextEdit_);
    
    // 设置窗口属性
    setWindowTitle(tr("X264编码器配置"));
    setFixedSize(800, 600);  // 固定窗口大小
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);  // 禁用最大化按钮
}

void X264ConfigWindow::createConnections()
{
    connect(startButton_, &QPushButton::clicked, this, &X264ConfigWindow::onStartEncoding);
    connect(stopButton_, &QPushButton::clicked, this, &X264ConfigWindow::onStopEncoding);
    connect(rateControlCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &X264ConfigWindow::onRateControlChanged);
    connect(sceneConfigCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &X264ConfigWindow::onPresetConfigSelected);
}

void X264ConfigWindow::onStartEncoding()
{
    if (shouldStop_) {
        return;
    }

    auto config = getConfigFromUI();
    startButton_->setEnabled(false);
    stopButton_->setEnabled(true);
    progressBar_->setValue(0);
    logTextEdit_->clear();
    
    shouldStop_ = false;
    
    // 在新线程中运行编码测试
    std::thread encodingThread([this, config]() {
        X264ParamTest::runTest(config,
            [this](int frame, const X264ParamTest::TestResult& result) {
                if (shouldStop_) {
                    return false; // 返回false表示停止编码
                }

                // 通过信号槽更新UI
                QMetaObject::invokeMethod(this, "updateProgress",
                    Qt::QueuedConnection,
                    Q_ARG(int, frame),
                    Q_ARG(double, result.encodingTime),
                    Q_ARG(double, result.fps),
                    Q_ARG(double, result.bitrate),
                    Q_ARG(double, result.psnr),
                    Q_ARG(double, result.ssim));
                
                return true; // 返回true继续编码
            });
        
        // 编码完成后，在主线程中更新UI
        QMetaObject::invokeMethod(this, "onEncodingFinished", Qt::QueuedConnection);
    });
    encodingThread.detach();
}

void X264ConfigWindow::updateProgress(int frame, double time, double fps, double bitrate, double psnr, double ssim)
{
    // 更新进度
    int progress = (frame * 100) / frameCountSpinBox_->value();
    progressBar_->setValue(progress);
    
    // 每秒最多更新5次日志
    static auto lastUpdate = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdate).count() >= 200) {
        // 更新日志
        QString log = QString("Frame: %1\n编码时间: %2秒\n编码速度: %3 fps\n"
                            "码率: %4 kbps\nPSNR: %5\nSSIM: %6\n")
                        .arg(frame)
                        .arg(time, 0, 'f', 2)
                        .arg(fps, 0, 'f', 2)
                        .arg(bitrate / 1000.0, 0, 'f', 2)
                        .arg(psnr, 0, 'f', 2)
                        .arg(ssim, 0, 'f', 3);
        logTextEdit_->append(log);
        lastUpdate = now;
    }
}

void X264ConfigWindow::appendLog(const QString& text)
{
    logTextEdit_->append(text);
}

void X264ConfigWindow::onEncodingFinished()
{
    startButton_->setEnabled(true);
    stopButton_->setEnabled(false);
    shouldStop_ = false;
}

void X264ConfigWindow::onStopEncoding()
{
    shouldStop_ = true;
    QMetaObject::invokeMethod(this, "appendLog",
        Qt::QueuedConnection,
        Q_ARG(QString, tr("\n已请求停止编码...\n")));
}

void X264ConfigWindow::onRateControlChanged(int index)
{
    switch (static_cast<X264ParamTest::RateControl>(index)) {
        case X264ParamTest::RateControl::CRF:
        case X264ParamTest::RateControl::CQP:
            rateValueSpinBox_->setRange(0, 51);
            rateValueSpinBox_->setValue(23);
            break;
        case X264ParamTest::RateControl::ABR:
        case X264ParamTest::RateControl::CBR:
            rateValueSpinBox_->setRange(1, 100000000);
            rateValueSpinBox_->setValue(4000000);
            break;
    }
}

void X264ConfigWindow::onPresetConfigSelected(int index)
{
    X264ParamTest::TestConfig config;
    switch (index) {
        case 1: // 直播场景
            config = X264ParamTest::getLiveStreamConfig().config;
            break;
        case 2: // 点播场景
            config = X264ParamTest::getVODConfig().config;
            break;
        case 3: // 存档场景
            config = X264ParamTest::getArchiveConfig().config;
            break;
        default: // 默认配置
            break;
    }
    updateUIFromConfig(config);
}

void X264ConfigWindow::updateUIFromConfig(const X264ParamTest::TestConfig& config)
{
    presetCombo_->setCurrentIndex(static_cast<int>(config.preset));
    tuneCombo_->setCurrentIndex(static_cast<int>(config.tune));
    threadsSpinBox_->setValue(config.threads);
    
    widthSpinBox_->setValue(config.width);
    heightSpinBox_->setValue(config.height);
    frameCountSpinBox_->setValue(config.frameCount);
    
    rateControlCombo_->setCurrentIndex(static_cast<int>(config.rateControl));
    switch (config.rateControl) {
        case X264ParamTest::RateControl::CRF:
            rateValueSpinBox_->setValue(config.crf);
            break;
        case X264ParamTest::RateControl::CQP:
            rateValueSpinBox_->setValue(config.qp);
            break;
        case X264ParamTest::RateControl::ABR:
        case X264ParamTest::RateControl::CBR:
            rateValueSpinBox_->setValue(config.bitrate);
            break;
    }
    
    keyintMaxSpinBox_->setValue(config.keyintMax);
    bframesSpinBox_->setValue(config.bframes);
    refsSpinBox_->setValue(config.refs);
    
    fastFirstPassCheckBox_->setChecked(config.fastFirstPass);
    meRangeSpinBox_->setValue(config.meRange);
    weightedPredCheckBox_->setChecked(config.weightedPred);
    cabacCheckBox_->setChecked(config.cabac);
}

X264ParamTest::TestConfig X264ConfigWindow::getConfigFromUI() const
{
    X264ParamTest::TestConfig config;
    
    config.preset = static_cast<X264ParamTest::Preset>(presetCombo_->currentIndex());
    config.tune = static_cast<X264ParamTest::Tune>(tuneCombo_->currentIndex());
    config.threads = threadsSpinBox_->value();
    
    config.width = widthSpinBox_->value();
    config.height = heightSpinBox_->value();
    config.frameCount = frameCountSpinBox_->value();
    
    config.rateControl = static_cast<X264ParamTest::RateControl>(rateControlCombo_->currentIndex());
    switch (config.rateControl) {
        case X264ParamTest::RateControl::CRF:
            config.crf = rateValueSpinBox_->value();
            break;
        case X264ParamTest::RateControl::CQP:
            config.qp = rateValueSpinBox_->value();
            break;
        case X264ParamTest::RateControl::ABR:
        case X264ParamTest::RateControl::CBR:
            config.bitrate = rateValueSpinBox_->value();
            break;
    }
    
    config.keyintMax = keyintMaxSpinBox_->value();
    config.bframes = bframesSpinBox_->value();
    config.refs = refsSpinBox_->value();
    
    config.fastFirstPass = fastFirstPassCheckBox_->isChecked();
    config.meRange = meRangeSpinBox_->value();
    config.weightedPred = weightedPredCheckBox_->isChecked();
    config.cabac = cabacCheckBox_->isChecked();
    
    return config;
} 