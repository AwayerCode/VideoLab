#include "vp8_config_window.hpp"
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <thread>

VP8ConfigWindow::VP8ConfigWindow(QWidget *parent)
    : QMainWindow(parent)
    , centralWidget_(new QWidget(this))
    , mainLayout_(new QVBoxLayout(centralWidget_))
{
    setCentralWidget(centralWidget_);
    setupUI();
    createConnections();

    // 设置默认配置
    VP8ParamTest::TestConfig defaultConfig;
    updateUIFromConfig(defaultConfig);

    // 自动生成初始帧
    QTimer::singleShot(0, this, &VP8ConfigWindow::onGenerateFrames);
}

VP8ConfigWindow::~VP8ConfigWindow() {
    if (isEncoding_) {
        onStopEncoding();
    }
}

void VP8ConfigWindow::setupUI()
{
    // 基本参数设置组
    basicParamsGroup_ = new QGroupBox(tr("基本参数"), this);
    auto basicParamsLayout = new QGridLayout(basicParamsGroup_);

    // 预设
    presetCombo_ = new QComboBox(this);
    presetCombo_->addItems({"最佳质量", "平衡", "实时"});
    basicParamsLayout->addWidget(new QLabel(tr("预设:")), 0, 0);
    basicParamsLayout->addWidget(presetCombo_, 0, 1);

    // 速度
    speedCombo_ = new QComboBox(this);
    for (int i = 0; i <= 16; ++i) {
        speedCombo_->addItem(QString::number(i));
    }
    basicParamsLayout->addWidget(new QLabel(tr("速度:")), 0, 2);
    basicParamsLayout->addWidget(speedCombo_, 0, 3);

    // 码率控制
    rateControlCombo_ = new QComboBox(this);
    rateControlCombo_->addItems({"固定质量(CQ)", "固定码率(CBR)", "可变码率(VBR)"});
    basicParamsLayout->addWidget(new QLabel(tr("码率控制:")), 1, 0);
    basicParamsLayout->addWidget(rateControlCombo_, 1, 1);

    // 码率
    bitrateSpinBox_ = new QSpinBox(this);
    bitrateSpinBox_->setRange(100, 50000);
    bitrateSpinBox_->setValue(2000);
    bitrateSpinBox_->setSuffix(" kbps");
    basicParamsLayout->addWidget(new QLabel(tr("码率:")), 1, 2);
    basicParamsLayout->addWidget(bitrateSpinBox_, 1, 3);

    // CQ级别
    cqLevelSpinBox_ = new QSpinBox(this);
    cqLevelSpinBox_->setRange(0, 63);
    cqLevelSpinBox_->setValue(10);
    basicParamsLayout->addWidget(new QLabel(tr("CQ级别:")), 2, 0);
    basicParamsLayout->addWidget(cqLevelSpinBox_, 2, 1);

    // 量化参数
    qminSpinBox_ = new QSpinBox(this);
    qminSpinBox_->setRange(0, 63);
    qmaxSpinBox_ = new QSpinBox(this);
    qmaxSpinBox_->setRange(0, 63);
    qmaxSpinBox_->setValue(63);
    basicParamsLayout->addWidget(new QLabel(tr("最小QP:")), 2, 2);
    basicParamsLayout->addWidget(qminSpinBox_, 2, 3);
    basicParamsLayout->addWidget(new QLabel(tr("最大QP:")), 3, 0);
    basicParamsLayout->addWidget(qmaxSpinBox_, 3, 1);

    // 关键帧间隔
    keyintSpinBox_ = new QSpinBox(this);
    keyintSpinBox_->setRange(1, 1000);
    keyintSpinBox_->setValue(250);
    basicParamsLayout->addWidget(new QLabel(tr("关键帧间隔:")), 3, 2);
    basicParamsLayout->addWidget(keyintSpinBox_, 3, 3);

    // 线程数
    threadsSpinBox_ = new QSpinBox(this);
    threadsSpinBox_->setRange(1, std::thread::hardware_concurrency());
    threadsSpinBox_->setValue(std::thread::hardware_concurrency());
    basicParamsLayout->addWidget(new QLabel(tr("线程数:")), 4, 0);
    basicParamsLayout->addWidget(threadsSpinBox_, 4, 1);

    // 视频参数设置组
    videoParamsGroup_ = new QGroupBox(tr("视频参数"), this);
    auto videoParamsLayout = new QGridLayout(videoParamsGroup_);

    // 分辨率
    widthSpinBox_ = new QSpinBox(this);
    widthSpinBox_->setRange(16, 7680);
    widthSpinBox_->setValue(1920);
    widthSpinBox_->setSingleStep(2);
    heightSpinBox_ = new QSpinBox(this);
    heightSpinBox_->setRange(16, 4320);
    heightSpinBox_->setValue(1080);
    heightSpinBox_->setSingleStep(2);
    videoParamsLayout->addWidget(new QLabel(tr("宽度:")), 0, 0);
    videoParamsLayout->addWidget(widthSpinBox_, 0, 1);
    videoParamsLayout->addWidget(new QLabel(tr("高度:")), 0, 2);
    videoParamsLayout->addWidget(heightSpinBox_, 0, 3);

    // 帧率和帧数
    fpsSpinBox_ = new QSpinBox(this);
    fpsSpinBox_->setRange(1, 240);
    fpsSpinBox_->setValue(30);
    framesSpinBox_ = new QSpinBox(this);
    framesSpinBox_->setRange(1, 10000);
    framesSpinBox_->setValue(300);
    videoParamsLayout->addWidget(new QLabel(tr("帧率:")), 1, 0);
    videoParamsLayout->addWidget(fpsSpinBox_, 1, 1);
    videoParamsLayout->addWidget(new QLabel(tr("总帧数:")), 1, 2);
    videoParamsLayout->addWidget(framesSpinBox_, 1, 3);

    // 输出设置组
    outputGroup_ = new QGroupBox(tr("输出设置"), this);
    auto outputLayout = new QHBoxLayout(outputGroup_);
    outputPathEdit_ = new QLineEdit(this);
    browseButton_ = new QPushButton(tr("浏览"), this);
    outputLayout->addWidget(new QLabel(tr("输出文件:")));
    outputLayout->addWidget(outputPathEdit_);
    outputLayout->addWidget(browseButton_);

    // 控制按钮
    controlLayout_ = new QHBoxLayout();
    generateButton_ = new QPushButton(tr("生成帧"), this);
    startButton_ = new QPushButton(tr("开始编码"), this);
    stopButton_ = new QPushButton(tr("停止编码"), this);
    playButton_ = new QPushButton(tr("播放"), this);
    stopButton_->setEnabled(false);
    playButton_->setEnabled(false);
    controlLayout_->addWidget(generateButton_);
    controlLayout_->addWidget(startButton_);
    controlLayout_->addWidget(stopButton_);
    controlLayout_->addWidget(playButton_);

    // 进度显示组
    progressGroup_ = new QGroupBox(tr("编码进度"), this);
    auto progressLayout = new QGridLayout(progressGroup_);
    progressBar_ = new QProgressBar(this);
    timeLabel_ = new QLabel(tr("时间: 0s"), this);
    fpsLabel_ = new QLabel(tr("速度: 0 fps"), this);
    bitrateLabel_ = new QLabel(tr("码率: 0 kbps"), this);
    psnrLabel_ = new QLabel(tr("PSNR: 0 dB"), this);
    ssimLabel_ = new QLabel(tr("SSIM: 0"), this);
    progressLayout->addWidget(progressBar_, 0, 0, 1, 2);
    progressLayout->addWidget(timeLabel_, 1, 0);
    progressLayout->addWidget(fpsLabel_, 1, 1);
    progressLayout->addWidget(bitrateLabel_, 2, 0);
    progressLayout->addWidget(psnrLabel_, 2, 1);
    progressLayout->addWidget(ssimLabel_, 3, 0);

    // 日志显示组
    logGroup_ = new QGroupBox(tr("编码日志"), this);
    auto logLayout = new QVBoxLayout(logGroup_);
    logEdit_ = new QTextEdit(this);
    logEdit_->setReadOnly(true);
    logLayout->addWidget(logEdit_);

    // 视频预览组
    previewGroup_ = new QGroupBox(tr("视频预览"), this);
    auto previewLayout = new QVBoxLayout(previewGroup_);
    videoWidget_ = new QVideoWidget(this);
    videoSlider_ = new QSlider(Qt::Horizontal, this);
    mediaPlayer_ = new QMediaPlayer(this);
    mediaPlayer_->setVideoOutput(videoWidget_);
    previewLayout->addWidget(videoWidget_);
    previewLayout->addWidget(videoSlider_);

    // 添加所有组件到主布局
    mainLayout_->addWidget(basicParamsGroup_);
    mainLayout_->addWidget(videoParamsGroup_);
    mainLayout_->addWidget(outputGroup_);
    mainLayout_->addLayout(controlLayout_);
    mainLayout_->addWidget(progressGroup_);
    mainLayout_->addWidget(logGroup_);
    mainLayout_->addWidget(previewGroup_);

    setWindowTitle(tr("VP8编码器配置"));
}

void VP8ConfigWindow::createConnections()
{
    connect(startButton_, &QPushButton::clicked, this, &VP8ConfigWindow::onStartEncoding);
    connect(stopButton_, &QPushButton::clicked, this, &VP8ConfigWindow::onStopEncoding);
    connect(rateControlCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &VP8ConfigWindow::onRateControlChanged);
    connect(presetCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &VP8ConfigWindow::onPresetConfigSelected);
    connect(playButton_, &QPushButton::clicked, this, &VP8ConfigWindow::onPlayVideo);
    connect(browseButton_, &QPushButton::clicked, [this]() {
        QString filePath = QFileDialog::getSaveFileName(
            this,
            tr("选择输出文件"),
            QStandardPaths::writableLocation(QStandardPaths::MoviesLocation),
            tr("WebM文件 (*.webm)")
        );
        if (!filePath.isEmpty()) {
            outputPathEdit_->setText(filePath);
        }
    });

    connect(mediaPlayer_, &QMediaPlayer::positionChanged, this, &VP8ConfigWindow::onMediaPositionChanged);
    connect(mediaPlayer_, &QMediaPlayer::durationChanged, this, &VP8ConfigWindow::onMediaDurationChanged);
    connect(videoSlider_, &QSlider::sliderMoved, this, &VP8ConfigWindow::onSliderMoved);

    connect(generateButton_, &QPushButton::clicked, this, &VP8ConfigWindow::onGenerateFrames);
}

void VP8ConfigWindow::onStartEncoding()
{
    if (outputPathEdit_->text().isEmpty()) {
        QMessageBox::warning(this, tr("错误"), tr("请选择输出文件路径"));
        return;
    }

    isEncoding_ = true;
    startButton_->setEnabled(false);
    stopButton_->setEnabled(true);
    playButton_->setEnabled(false);

    // 获取配置
    auto config = getConfigFromUI();

    // 清空日志
    logEdit_->clear();
    appendLog(tr("开始VP8编码..."));

    // 重置进度条
    progressBar_->setValue(0);
    progressBar_->setMaximum(config.frames);

    // 运行编码测试
    auto result = VP8ParamTest::runTest(config,
        [this](int frame, const VP8ParamTest::TestResult& result) {
            QMetaObject::invokeMethod(this, "updateProgress",
                Qt::QueuedConnection,
                Q_ARG(int, frame),
                Q_ARG(double, result.encoding_time),
                Q_ARG(double, result.fps),
                Q_ARG(double, result.bitrate),
                Q_ARG(double, result.psnr),
                Q_ARG(double, result.ssim));
        });

    // 编码完成
    QMetaObject::invokeMethod(this, "onEncodingFinished",
        Qt::QueuedConnection);
}

void VP8ConfigWindow::updateProgress(int frame, double encodingTime, double fps, double bitrate, double psnr, double ssim)
{
    progressBar_->setValue(frame + 1);
    timeLabel_->setText(tr("时间: %1s").arg(QString::number(encodingTime, 'f', 1)));
    fpsLabel_->setText(tr("速度: %1 fps").arg(QString::number(fps, 'f', 1)));
    bitrateLabel_->setText(tr("码率: %1 kbps").arg(QString::number(bitrate / 1000.0, 'f', 1)));
    psnrLabel_->setText(tr("PSNR: %1 dB").arg(QString::number(psnr, 'f', 2)));
    ssimLabel_->setText(tr("SSIM: %1").arg(QString::number(ssim, 'f', 3)));
}

void VP8ConfigWindow::appendLog(const QString& text)
{
    logEdit_->append(text);
}

void VP8ConfigWindow::onEncodingFinished()
{
    isEncoding_ = false;
    startButton_->setEnabled(true);
    stopButton_->setEnabled(false);
    playButton_->setEnabled(true);
    appendLog(tr("\n编码完成"));
}

void VP8ConfigWindow::onStopEncoding()
{
    if (isEncoding_) {
        isEncoding_ = false;
        appendLog(tr("\n编码已停止"));
    }
}

void VP8ConfigWindow::onRateControlChanged(int index)
{
    bool isCQ = index == 0;
    cqLevelSpinBox_->setEnabled(isCQ);
    bitrateSpinBox_->setEnabled(!isCQ);
}

void VP8ConfigWindow::onPresetConfigSelected(int index)
{
    // 根据预设更新UI
    switch (index) {
        case 0: // 最佳质量
            speedCombo_->setCurrentIndex(0);
            break;
        case 1: // 平衡
            speedCombo_->setCurrentIndex(1);
            break;
        case 2: // 实时
            speedCombo_->setCurrentIndex(4);
            break;
    }
}

void VP8ConfigWindow::onPlayVideo()
{
    if (!isPlaying_) {
        mediaPlayer_->setSource(QUrl::fromLocalFile(outputPathEdit_->text()));
        mediaPlayer_->play();
        playButton_->setText(tr("停止"));
        isPlaying_ = true;
    } else {
        mediaPlayer_->stop();
        playButton_->setText(tr("播放"));
        isPlaying_ = false;
    }
}

void VP8ConfigWindow::onMediaPositionChanged(qint64 position)
{
    videoSlider_->setValue(position);
}

void VP8ConfigWindow::onMediaDurationChanged(qint64 duration)
{
    videoSlider_->setRange(0, duration);
}

void VP8ConfigWindow::onSliderMoved(int position)
{
    mediaPlayer_->setPosition(position);
}

void VP8ConfigWindow::onGenerateFrames()
{
    appendLog(tr("生成测试帧..."));
    auto config = getConfigFromUI();
    VP8ParamTest test;
    if (test.initFrameCache(config)) {
        appendLog(tr("测试帧生成完成"));
    } else {
        appendLog(tr("测试帧生成失败"));
    }
}

void VP8ConfigWindow::updateUIFromConfig(const VP8ParamTest::TestConfig& config)
{
    // 更新预设
    presetCombo_->setCurrentIndex(static_cast<int>(config.preset));
    
    // 更新速度
    speedCombo_->setCurrentIndex(static_cast<int>(config.speed));
    
    // 更新码率控制
    rateControlCombo_->setCurrentIndex(static_cast<int>(config.rate_control));
    
    // 更新其他参数
    bitrateSpinBox_->setValue(config.bitrate / 1000); // 转换为kbps
    cqLevelSpinBox_->setValue(config.cq_level);
    qminSpinBox_->setValue(config.qmin);
    qmaxSpinBox_->setValue(config.qmax);
    keyintSpinBox_->setValue(config.keyint);
    threadsSpinBox_->setValue(config.threads);
    
    // 更新视频参数
    widthSpinBox_->setValue(config.width);
    heightSpinBox_->setValue(config.height);
    fpsSpinBox_->setValue(config.fps);
    framesSpinBox_->setValue(config.frames);
    
    // 更新UI状态
    onRateControlChanged(rateControlCombo_->currentIndex());
}

VP8ParamTest::TestConfig VP8ConfigWindow::getConfigFromUI() const
{
    VP8ParamTest::TestConfig config;
    
    // 基本参数
    config.preset = static_cast<VP8ParamTest::Preset>(presetCombo_->currentIndex());
    config.speed = static_cast<VP8ParamTest::Speed>(speedCombo_->currentIndex());
    config.rate_control = static_cast<VP8ParamTest::RateControl>(rateControlCombo_->currentIndex());
    config.bitrate = bitrateSpinBox_->value() * 1000; // 转换为bps
    config.cq_level = cqLevelSpinBox_->value();
    config.qmin = qminSpinBox_->value();
    config.qmax = qmaxSpinBox_->value();
    config.keyint = keyintSpinBox_->value();
    config.threads = threadsSpinBox_->value();
    
    // 视频参数
    config.width = widthSpinBox_->value();
    config.height = heightSpinBox_->value();
    config.fps = fpsSpinBox_->value();
    config.frames = framesSpinBox_->value();
    
    // 输出文件
    config.output_file = outputPathEdit_->text().toStdString();
    
    return config;
} 