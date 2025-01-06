#include "x264_config_window.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QApplication>
#include <QTime>
#include <QThread>
#include <QScrollBar>
#include <thread>
#include <chrono>
#include <QVideoWidget>
#include <QMediaPlayer>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFileDialog>
#include <QTextStream>

X264ConfigWindow::X264ConfigWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();
    createConnections();
    setupHistoryUI();
    
    // 设置默认配置
    X264ParamTest::TestConfig defaultConfig;
    updateUIFromConfig(defaultConfig);
}

X264ConfigWindow::~X264ConfigWindow() {
    shouldStop_ = true;
    if (encodingThread_.joinable()) {
        encodingThread_.join();
    }
}

void X264ConfigWindow::setupUI()
{
    auto* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    auto* mainLayout = new QVBoxLayout(centralWidget);
    auto* topLayout = new QHBoxLayout();
    mainLayout->addLayout(topLayout);
    
    auto* leftLayout = new QVBoxLayout();
    auto* rightLayout = new QVBoxLayout();
    topLayout->addLayout(leftLayout, 1);
    topLayout->addLayout(rightLayout, 1);
    
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
    
    // 右侧编码控制区域
    // 场景选择
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
    
    // 底部历史记录区域
    auto* historyGroup = new QGroupBox(tr("编码历史记录"), this);
    auto* historyLayout = new QVBoxLayout(historyGroup);
    
    // 创建历史记录表格
    historyTable_ = new QTableWidget(this);
    historyTable_->setColumnCount(15);  // 增加列数
    historyTable_->setHorizontalHeaderLabels({
        tr("时间"),
        tr("分辨率"),
        tr("帧数"),
        tr("预设"),
        tr("调优"),
        tr("线程数"),
        tr("码率控制"),
        tr("码率/QP值"),
        tr("关键帧间隔"),
        tr("B帧数"),
        tr("参考帧数"),
        tr("编码时间(s)"),
        tr("速度(fps)"),
        tr("码率(kbps)"),
        tr("PSNR/SSIM")
    });
    
    // 设置表格属性
    historyTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    historyTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    historyTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    historyTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    historyTable_->verticalHeader()->setVisible(false);
    historyTable_->setAlternatingRowColors(true);
    historyTable_->setMinimumHeight(200);
    
    // 设置表格的工具提示
    historyTable_->horizontalHeaderItem(3)->setToolTip(tr("编码速度预设"));
    historyTable_->horizontalHeaderItem(4)->setToolTip(tr("场景优化调优"));
    historyTable_->horizontalHeaderItem(6)->setToolTip(tr("码率控制模式"));
    historyTable_->horizontalHeaderItem(7)->setToolTip(tr("CRF/CQP: 0-51, ABR/CBR: 比特率"));
    historyTable_->horizontalHeaderItem(8)->setToolTip(tr("最大关键帧间隔"));
    historyTable_->horizontalHeaderItem(9)->setToolTip(tr("B帧数量"));
    historyTable_->horizontalHeaderItem(10)->setToolTip(tr("参考帧数量"));
    historyTable_->horizontalHeaderItem(14)->setToolTip(tr("PSNR: 峰值信噪比, SSIM: 结构相似度"));
    
    // 添加历史记录控制按钮
    auto* historyButtonLayout = new QHBoxLayout();
    clearHistoryButton_ = new QPushButton(tr("清除历史"), this);
    exportHistoryButton_ = new QPushButton(tr("导出历史"), this);
    historyButtonLayout->addWidget(clearHistoryButton_);
    historyButtonLayout->addWidget(exportHistoryButton_);
    historyButtonLayout->addStretch();
    
    historyLayout->addWidget(historyTable_);
    historyLayout->addLayout(historyButtonLayout);
    
    mainLayout->addWidget(historyGroup);
    
    // 设置窗口属性
    setWindowTitle(tr("X264编码器配置"));
    resize(1400, 900);
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
    if (encodingThread_.joinable()) {
        encodingThread_.join();
    }
    encodingThread_ = std::thread([this, config]() {
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
        QMetaObject::invokeMethod(this, [this, result, config]() {
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

                // 添加到历史记录
                EncodingRecord record;
                record.timestamp = QDateTime::currentDateTime();
                record.width = config.width;
                record.height = config.height;
                record.frameCount = config.frameCount;
                record.preset = presetCombo_->currentText();
                record.tune = tuneCombo_->currentText();
                record.threads = config.threads;
                record.rateControl = rateControlCombo_->currentText();
                record.rateValue = rateValueSpinBox_->value();
                record.keyintMax = config.keyintMax;
                record.bframes = config.bframes;
                record.refs = config.refs;
                record.fastFirstPass = config.fastFirstPass;
                record.meRange = config.meRange;
                record.weightedPred = config.weightedPred;
                record.cabac = config.cabac;
                record.encodingTime = result.encodingTime;
                record.fps = result.fps;
                record.bitrate = result.bitrate;
                record.psnr = result.psnr;
                record.ssim = result.ssim;
                record.outputFile = currentOutputFile_;
                
                addEncodingRecord(record);
            } else {
                appendLog(tr("\n编码失败：%1\n").arg(QString::fromStdString(result.errorMessage)));
            }
            onEncodingFinished();
        }, Qt::QueuedConnection);
    });
}

void X264ConfigWindow::updateProgress(int frame, double encodingTime, double fps, double bitrate, double psnr, double ssim)
{
    static int lastUpdateFrame = 0;
    static auto lastUpdateTime = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    
    // 限制更新频率，每200ms更新一次
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdateTime).count() < 200) {
        return;
    }
    
    lastUpdateTime = now;
    lastUpdateFrame = frame;
    
    // 更新进度条
    int progress = static_cast<int>(frame * 100.0 / frameCountSpinBox_->value());
    progressBar_->setValue(progress);
    
    // 只在整数百分比变化时更新状态
    static int lastProgress = -1;
    if (progress != lastProgress) {
        lastProgress = progress;
        
        QString status = QString("帧: %1/%2 | FPS: %3 | 码率: %4 kbps")
            .arg(frame)
            .arg(frameCountSpinBox_->value())
            .arg(fps, 0, 'f', 1)
            .arg(bitrate / 1000.0, 0, 'f', 0);
            
        if (psnr > 0) {
            status += QString(" | PSNR: %1").arg(psnr, 0, 'f', 2);
        }
        if (ssim > 0) {
            status += QString(" | SSIM: %1").arg(ssim, 0, 'f', 3);
        }
        
        // 只在整5%的进度时更新日志
        if (progress % 5 == 0) {
            appendLog(status);
        }
    }
}

void X264ConfigWindow::appendLog(const QString& text)
{
    // 如果不在主线程，使用信号槽机制
    if (QThread::currentThread() != QApplication::instance()->thread()) {
        QMetaObject::invokeMethod(this, "appendLog",
            Qt::QueuedConnection,
            Q_ARG(QString, text));
        return;
    }
    
    // 在主线程中直接更新
    logTextEdit_->append(text);
    
    // 滚动到底部
    QScrollBar* scrollBar = logTextEdit_->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
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
            static auto lastUpdateTime = std::chrono::steady_clock::now();
            auto now = std::chrono::steady_clock::now();
            static int lastPercent = -1;
            
            // 限制更新频率为每200ms更新一次
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdateTime).count() < 200) {
                return;
            }
            lastUpdateTime = now;
            
            // 使用Qt的信号槽机制更新UI
            QMetaObject::invokeMethod(this, [this, progress]() {
                int percent = static_cast<int>(std::round(progress * 100));
                
                // 确保进度不会后退
                static int lastPercent = -1;
                if (percent < lastPercent && lastPercent < 100) {
                    return;
                }
                lastPercent = percent;
                
                frameGenProgressBar_->setValue(percent);
                
                // 只在整10%时更新日志，或在100%时
                if (percent % 10 == 0 || percent == 100) {
                    appendLog(tr("\n已生成 %1% 的帧\n").arg(percent));
                }

                // 如果是100%，更新状态
                if (percent == 100) {
                    frameGenStatusLabel_->setText(tr("帧生成完成"));
                    generateButton_->setEnabled(true);
                }
            }, Qt::QueuedConnection);
        };

        auto start_time = std::chrono::steady_clock::now();

        if (test.initFrameCache(config)) {
            auto end_time = std::chrono::steady_clock::now();
            double duration = std::chrono::duration<double>(end_time - start_time).count();
            
            QMetaObject::invokeMethod(this, [this, duration]() {
                appendLog(tr("\n帧生成完成!\n"));
                appendLog(tr("总耗时: %1 秒\n").arg(duration, 0, 'f', 2));
                appendLog(tr("平均速度: %1 帧/秒\n").arg(frameCountSpinBox_->value() / duration, 0, 'f', 1));
                
                // 确保最终状态正确
                frameGenProgressBar_->setValue(100);
                frameGenStatusLabel_->setText(tr("帧生成完成"));
                generateButton_->setEnabled(true);
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

void X264ConfigWindow::setupHistoryUI()
{
    // 连接历史记录相关的信号槽
    connect(clearHistoryButton_, &QPushButton::clicked, this, &X264ConfigWindow::clearEncodingHistory);
    connect(exportHistoryButton_, &QPushButton::clicked, this, &X264ConfigWindow::exportEncodingHistory);
    
    // 加载历史记录
    loadEncodingHistory();
}

void X264ConfigWindow::addEncodingRecord(const EncodingRecord& record)
{
    // 添加到历史记录向量
    encodingHistory_.push_back(record);
    
    // 添加到表格
    int row = historyTable_->rowCount();
    historyTable_->insertRow(row);
    
    // 设置单元格内容
    historyTable_->setItem(row, 0, new QTableWidgetItem(record.timestamp.toString("yyyy-MM-dd HH:mm:ss")));
    historyTable_->setItem(row, 1, new QTableWidgetItem(QString("%1x%2").arg(record.width).arg(record.height)));
    historyTable_->setItem(row, 2, new QTableWidgetItem(QString::number(record.frameCount)));
    historyTable_->setItem(row, 3, new QTableWidgetItem(record.preset));
    historyTable_->setItem(row, 4, new QTableWidgetItem(record.tune));
    historyTable_->setItem(row, 5, new QTableWidgetItem(QString::number(record.threads)));
    historyTable_->setItem(row, 6, new QTableWidgetItem(record.rateControl));
    historyTable_->setItem(row, 7, new QTableWidgetItem(QString::number(record.rateValue)));
    historyTable_->setItem(row, 8, new QTableWidgetItem(QString::number(record.keyintMax)));
    historyTable_->setItem(row, 9, new QTableWidgetItem(QString::number(record.bframes)));
    historyTable_->setItem(row, 10, new QTableWidgetItem(QString::number(record.refs)));
    historyTable_->setItem(row, 11, new QTableWidgetItem(QString::number(record.encodingTime, 'f', 2)));
    historyTable_->setItem(row, 12, new QTableWidgetItem(QString::number(record.fps, 'f', 1)));
    historyTable_->setItem(row, 13, new QTableWidgetItem(QString::number(record.bitrate / 1000.0, 'f', 0)));
    historyTable_->setItem(row, 14, new QTableWidgetItem(QString("%1/%2").arg(record.psnr, 0, 'f', 2).arg(record.ssim, 0, 'f', 3)));
    
    // 保存到文件
    saveHistoryToFile();
}

void X264ConfigWindow::clearEncodingHistory()
{
    if (QMessageBox::question(this, tr("确认"), tr("确定要清除所有历史记录吗？")) == QMessageBox::Yes) {
        encodingHistory_.clear();
        historyTable_->setRowCount(0);
        saveHistoryToFile();
    }
}

void X264ConfigWindow::exportEncodingHistory()
{
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("导出历史记录"), QDir::homePath(), tr("CSV文件 (*.csv)"));
        
    if (fileName.isEmpty()) {
        return;
    }
    
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("错误"), tr("无法创建文件"));
        return;
    }
    
    QTextStream out(&file);
    
    // 写入表头
    out << "时间,分辨率,帧数,预设,调优,线程数,码率控制,码率/QP值,关键帧间隔,B帧数,参考帧数,编码时间(s),速度(fps),码率(kbps),PSNR,SSIM\n";
    
    // 写入数据
    for (const auto& record : encodingHistory_) {
        out << record.timestamp.toString("yyyy-MM-dd HH:mm:ss") << ","
            << record.width << "x" << record.height << ","
            << record.frameCount << ","
            << record.preset << ","
            << record.tune << ","
            << record.threads << ","
            << record.rateControl << ","
            << record.rateValue << ","
            << record.keyintMax << ","
            << record.bframes << ","
            << record.refs << ","
            << QString::number(record.encodingTime, 'f', 2) << ","
            << QString::number(record.fps, 'f', 1) << ","
            << QString::number(record.bitrate / 1000.0, 'f', 0) << ","
            << QString::number(record.psnr, 'f', 2) << ","
            << QString::number(record.ssim, 'f', 3) << "\n";
    }
    
    QMessageBox::information(this, tr("成功"), tr("历史记录已导出"));
}

void X264ConfigWindow::loadEncodingHistory()
{
    QString filePath = QDir::homePath() + "/.x264_encoding_history.json";
    QFile file(filePath);
    
    if (!file.open(QIODevice::ReadOnly)) {
        return; // 文件不存在或无法打开，直接返回
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isArray()) {
        return;
    }
    
    encodingHistory_.clear();
    QJsonArray historyArray = doc.array();
    
    for (const auto& value : historyArray) {
        QJsonObject obj = value.toObject();
        EncodingRecord record;
        
        record.timestamp = QDateTime::fromString(obj["timestamp"].toString(), Qt::ISODate);
        record.width = obj["width"].toInt();
        record.height = obj["height"].toInt();
        record.frameCount = obj["frameCount"].toInt();
        record.preset = obj["preset"].toString();
        record.tune = obj["tune"].toString();
        record.threads = obj["threads"].toInt();
        record.rateControl = obj["rateControl"].toString();
        record.rateValue = obj["rateValue"].toInt();
        record.keyintMax = obj["keyintMax"].toInt();
        record.bframes = obj["bframes"].toInt();
        record.refs = obj["refs"].toInt();
        record.fastFirstPass = obj["fastFirstPass"].toBool();
        record.meRange = obj["meRange"].toInt();
        record.weightedPred = obj["weightedPred"].toBool();
        record.cabac = obj["cabac"].toBool();
        record.encodingTime = obj["encodingTime"].toDouble();
        record.fps = obj["fps"].toDouble();
        record.bitrate = obj["bitrate"].toDouble();
        record.psnr = obj["psnr"].toDouble();
        record.ssim = obj["ssim"].toDouble();
        record.outputFile = obj["outputFile"].toString();
        
        encodingHistory_.push_back(record);
    }
    
    updateHistoryTable();
}

void X264ConfigWindow::updateHistoryTable()
{
    historyTable_->setRowCount(0);
    for (const auto& record : encodingHistory_) {
        addEncodingRecord(record);
    }
}

void X264ConfigWindow::saveHistoryToFile()
{
    QString filePath = QDir::homePath() + "/.x264_encoding_history.json";
    QFile file(filePath);
    
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "无法保存编码历史到文件:" << filePath;
        return;
    }
    
    QJsonArray historyArray;
    for (const auto& record : encodingHistory_) {
        QJsonObject recordObj;
        recordObj["timestamp"] = record.timestamp.toString(Qt::ISODate);
        recordObj["width"] = record.width;
        recordObj["height"] = record.height;
        recordObj["frameCount"] = record.frameCount;
        recordObj["preset"] = record.preset;
        recordObj["tune"] = record.tune;
        recordObj["threads"] = record.threads;
        recordObj["rateControl"] = record.rateControl;
        recordObj["rateValue"] = record.rateValue;
        recordObj["keyintMax"] = record.keyintMax;
        recordObj["bframes"] = record.bframes;
        recordObj["refs"] = record.refs;
        recordObj["fastFirstPass"] = record.fastFirstPass;
        recordObj["meRange"] = record.meRange;
        recordObj["weightedPred"] = record.weightedPred;
        recordObj["cabac"] = record.cabac;
        recordObj["encodingTime"] = record.encodingTime;
        recordObj["fps"] = record.fps;
        recordObj["bitrate"] = record.bitrate;
        recordObj["psnr"] = record.psnr;
        recordObj["ssim"] = record.ssim;
        recordObj["outputFile"] = record.outputFile;
        historyArray.append(recordObj);
    }
    
    QJsonDocument doc(historyArray);
    file.write(doc.toJson());
} 