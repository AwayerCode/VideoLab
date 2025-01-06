#include "x264_config_window.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QApplication>
#include <QTime>
#include <thread>
#include <chrono>
#include <QVideoWidget>
#include <QMediaPlayer>
#include <QTimer>

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
    
    rightLayout->addWidget(new QLabel(tr("预设场景:")));
    rightLayout->addWidget(sceneConfigCombo_);

    // 帧生成状态
    auto* frameGenLayout = new QHBoxLayout();
    frameGenStatusLabel_ = new QLabel(tr("帧生成状态:"), this);
    frameGenProgressBar_ = new QProgressBar(this);
    frameGenProgressBar_->setRange(0, 100);
    frameGenProgressBar_->setValue(0);
    generateButton_ = new QPushButton(tr("重新生成"), this);
    generateButton_->setEnabled(true);
    
    frameGenLayout->addWidget(frameGenStatusLabel_);
    frameGenLayout->addWidget(frameGenProgressBar_, 1);
    frameGenLayout->addWidget(generateButton_);
    
    rightLayout->addLayout(frameGenLayout);
    
    // 编码控制按钮
    auto* buttonLayout = new QHBoxLayout();
    startButton_ = new QPushButton(tr("开始编码"), this);
    stopButton_ = new QPushButton(tr("停止编码"), this);
    stopButton_->setEnabled(false);
    buttonLayout->addWidget(startButton_);
    buttonLayout->addWidget(stopButton_);
    
    rightLayout->addLayout(buttonLayout);
    
    progressBar_ = new QProgressBar(this);
    logTextEdit_ = new QTextEdit(this);
    logTextEdit_->setReadOnly(true);
    
    rightLayout->addWidget(progressBar_);
    rightLayout->addWidget(logTextEdit_);
    
    // 视频播放区域
    videoWidget_ = new QVideoWidget(this);
    videoWidget_->setMinimumSize(480, 270);  // 16:9 比例
    rightLayout->addWidget(videoWidget_);
    
    mediaPlayer_ = new QMediaPlayer(this);
    mediaPlayer_->setVideoOutput(videoWidget_);
    
    // 播放控制
    auto* playControlLayout = new QVBoxLayout();
    
    // 进度条和时间标签
    auto* sliderLayout = new QHBoxLayout();
    videoSlider_ = new QSlider(Qt::Horizontal, this);
    videoSlider_->setEnabled(false);
    timeLabel_ = new QLabel("00:00 / 00:00", this);
    sliderLayout->addWidget(videoSlider_);
    sliderLayout->addWidget(timeLabel_);
    playControlLayout->addLayout(sliderLayout);
    
    // 播放按钮
    auto* playButtonLayout = new QHBoxLayout();
    playButton_ = new QPushButton(tr("播放"), this);
    playButton_->setEnabled(false);
    playButtonLayout->addWidget(playButton_);
    playControlLayout->addLayout(playButtonLayout);
    
    rightLayout->addLayout(playControlLayout);
    
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
    connect(playButton_, &QPushButton::clicked, this, &X264ConfigWindow::onPlayVideo);
    
    // 添加媒体播放相关的连接
    connect(mediaPlayer_, &QMediaPlayer::positionChanged, this, &X264ConfigWindow::onMediaPositionChanged);
    connect(mediaPlayer_, &QMediaPlayer::durationChanged, this, &X264ConfigWindow::onMediaDurationChanged);
    connect(videoSlider_, &QSlider::sliderMoved, this, &X264ConfigWindow::onSliderMoved);
    
    // 添加生成按钮的连接
    connect(generateButton_, &QPushButton::clicked, this, &X264ConfigWindow::onGenerateFrames);
    
    // 在窗口显示后自动开始生成帧
    QTimer::singleShot(0, this, &X264ConfigWindow::onGenerateFrames);
}

void X264ConfigWindow::onStartEncoding()
{
    if (shouldStop_) {
        return;
    }

    // 检查帧是否已生成
    if (frameGenProgressBar_->value() != 100) {
        QMessageBox::warning(this, tr("警告"), tr("请等待帧生成完成后再开始编码"));
        return;
    }

    auto config = getConfigFromUI();
    startButton_->setEnabled(false);
    stopButton_->setEnabled(true);
    playButton_->setEnabled(false);
    progressBar_->setValue(0);
    logTextEdit_->clear();
    
    shouldStop_ = false;
    
    appendLog(tr("开始编码...\n"));
    appendLog(tr("编码配置:\n"));
    appendLog(tr("分辨率: %1x%2\n").arg(config.width).arg(config.height));
    appendLog(tr("帧数: %1\n").arg(config.frameCount));
    appendLog(tr("线程数: %1\n").arg(config.threads));
    appendLog(tr("预设: %1\n").arg(presetCombo_->currentText()));
    appendLog(tr("调优: %1\n").arg(tuneCombo_->currentText()));
    appendLog(tr("码率控制: %1\n").arg(rateControlCombo_->currentText()));
    appendLog(tr("正在启动编码线程...\n"));
    
    // 在新线程中运行编码测试
    std::thread encodingThread([this, config]() {
        appendLog(tr("正在初始化编码器...\n"));
        
        auto result = X264ParamTest::runTest(config,
            [this](int frame, const X264ParamTest::TestResult& result) {
                if (shouldStop_) {
                    appendLog(tr("\n编码已停止\n"));
                    return false;
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
                
                return !shouldStop_; // 返回false表示停止编码
            });
        
        // 编码完成后，在主线程中更新UI
        QMetaObject::invokeMethod(this, [this, result]() {
            if (result.success) {
                currentOutputFile_ = QString::fromStdString(result.outputFile);
                QString summary = QString(
                    "\n编码完成！\n"
                    "总编码时间: %1 秒\n"
                    "平均编码速度: %2 fps\n"
                    "平均码率: %3 kbps\n"
                    "PSNR: %4\n"
                    "SSIM: %5\n"
                    "输出文件：%6\n")
                    .arg(result.encodingTime, 0, 'f', 2)
                    .arg(result.fps, 0, 'f', 2)
                    .arg(result.bitrate / 1000.0, 0, 'f', 2)
                    .arg(result.psnr, 0, 'f', 2)
                    .arg(result.ssim, 0, 'f', 3)
                    .arg(currentOutputFile_);
                appendLog(summary);
            } else {
                appendLog(tr("\n编码失败：%1\n").arg(QString::fromStdString(result.errorMessage)));
            }
            onEncodingFinished();
        }, Qt::QueuedConnection);
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
        
        // 处理Qt事件，保持UI响应
        QApplication::processEvents();
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
    playButton_->setEnabled(!currentOutputFile_.isEmpty());
    shouldStop_ = false;
    
    // 处理Qt事件，确保UI更新
    QApplication::processEvents();
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

void X264ConfigWindow::onPlayVideo()
{
    if (currentOutputFile_.isEmpty()) {
        QMessageBox::warning(this, tr("错误"), tr("没有可播放的视频文件"));
        return;
    }
    
    if (mediaPlayer_->playbackState() == QMediaPlayer::PlayingState) {
        mediaPlayer_->pause();
        playButton_->setText(tr("播放"));
    } else {
        mediaPlayer_->setSource(QUrl::fromLocalFile(currentOutputFile_));
        mediaPlayer_->play();
        playButton_->setText(tr("暂停"));
    }
}

void X264ConfigWindow::onMediaPositionChanged(qint64 position)
{
    if (!videoSlider_->isSliderDown()) {
        videoSlider_->setValue(position);
    }
    
    // 更新时间标签
    qint64 duration = mediaPlayer_->duration();
    QString currentTime = QTime(0, 0).addMSecs(position).toString("mm:ss");
    QString totalTime = QTime(0, 0).addMSecs(duration).toString("mm:ss");
    timeLabel_->setText(QString("%1 / %2").arg(currentTime, totalTime));
}

void X264ConfigWindow::onMediaDurationChanged(qint64 duration)
{
    videoSlider_->setRange(0, duration);
    videoSlider_->setEnabled(true);
}

void X264ConfigWindow::onSliderMoved(int position)
{
    mediaPlayer_->setPosition(position);
}

void X264ConfigWindow::onGenerateFrames()
{
    generateButton_->setEnabled(false);
    frameGenStatusLabel_->setText(tr("正在生成帧..."));
    frameGenProgressBar_->setValue(0);
    logTextEdit_->clear();
    appendLog(tr("开始生成视频帧...\n"));

    // 获取当前配置
    auto config = getConfigFromUI();
    appendLog(tr("配置信息:\n"));
    appendLog(tr("分辨率: %1x%2\n").arg(config.width).arg(config.height));
    appendLog(tr("帧数: %1\n").arg(config.frameCount));
    appendLog(tr("线程数: %1\n").arg(config.threads));
    
    // 创建后台线程进行帧生成
    std::thread([this, config]() {
        X264ParamTest test;
        test.gen_status_.progress_callback = [this](float progress) {
            // 使用Qt的信号槽机制更新UI
            QMetaObject::invokeMethod(this, [this, progress]() {
                int percent = static_cast<int>(progress * 100);
                frameGenProgressBar_->setValue(percent);
                appendLog(tr("\r生成进度: %1%").arg(percent));
                
                // 每10%更新一次进度
                if (percent % 10 == 0) {
                    appendLog(tr("\n已生成 %1% 的帧\n").arg(percent));
                }
            }, Qt::QueuedConnection);
        };

        auto start_time = std::chrono::steady_clock::now();

        if (test.initFrameCache(config)) {
            auto end_time = std::chrono::steady_clock::now();
            double duration = std::chrono::duration<double>(end_time - start_time).count();
            
            QMetaObject::invokeMethod(this, [this, duration]() {
                frameGenStatusLabel_->setText(tr("帧生成完成"));
                generateButton_->setEnabled(true);
                appendLog(tr("\n帧生成完成!\n"));
                appendLog(tr("总耗时: %1 秒\n").arg(duration, 0, 'f', 2));
                appendLog(tr("平均速度: %1 帧/秒\n").arg(frameCountSpinBox_->value() / duration, 0, 'f', 1));
            }, Qt::QueuedConnection);
        } else {
            QMetaObject::invokeMethod(this, [this]() {
                frameGenStatusLabel_->setText(tr("帧生成失败"));
                generateButton_->setEnabled(true);
                appendLog(tr("\n帧生成失败!\n"));
            }, Qt::QueuedConnection);
        }
    }).detach();
} 