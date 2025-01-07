#include "vp8_config_window.hpp"
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

VP8ConfigWindow::VP8ConfigWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();
    createConnections();
    
    // 设置默认配置
    VP8ParamTest::TestConfig defaultConfig;
    updateUIFromConfig(defaultConfig);
}

VP8ConfigWindow::~VP8ConfigWindow() {
    shouldStop_ = true;
    if (encodingThread_.joinable()) {
        encodingThread_.join();
    }
}

void VP8ConfigWindow::setupUI()
{
    auto* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    auto* mainLayout = new QVBoxLayout(centralWidget);
    auto* topLayout = new QHBoxLayout();
    mainLayout->addLayout(topLayout);
    
    auto* leftLayout = new QVBoxLayout();
    auto* rightLayout = new QVBoxLayout();
    topLayout->addLayout(leftLayout, 3);
    topLayout->addLayout(rightLayout, 4);
    
    // 设置布局的边距和间距
    mainLayout->setContentsMargins(6, 6, 6, 6);
    mainLayout->setSpacing(6);
    topLayout->setSpacing(6);
    
    // 左侧参数设置区域
    // 基本参数组
    auto* basicGroup = new QGroupBox(tr("基本参数"), this);
    auto* basicLayout = new QGridLayout(basicGroup);
    
    presetCombo_ = new QComboBox(this);
    presetCombo_->addItems({
        tr("最佳质量"),
        tr("平衡"),
        tr("实时")
    });
    
    speedCombo_ = new QComboBox(this);
    for (int i = 0; i <= 16; ++i) {
        speedCombo_->addItem(QString::number(i));
    }
    
    threadsSpinBox_ = new QSpinBox(this);
    threadsSpinBox_->setRange(1, QThread::idealThreadCount());
    threadsSpinBox_->setValue(QThread::idealThreadCount());
    
    basicLayout->addWidget(new QLabel(tr("预设:")), 0, 0);
    basicLayout->addWidget(presetCombo_, 0, 1);
    basicLayout->addWidget(new QLabel(tr("速度:")), 1, 0);
    basicLayout->addWidget(speedCombo_, 1, 1);
    basicLayout->addWidget(new QLabel(tr("线程数:")), 2, 0);
    basicLayout->addWidget(threadsSpinBox_, 2, 1);
    
    // 分辨率和帧数组
    auto* videoGroup = new QGroupBox(tr("视频参数"), this);
    auto* videoLayout = new QGridLayout(videoGroup);
    
    widthSpinBox_ = new QSpinBox(this);
    widthSpinBox_->setRange(16, 7680);
    widthSpinBox_->setValue(1920);
    heightSpinBox_ = new QSpinBox(this);
    heightSpinBox_->setRange(16, 4320);
    heightSpinBox_->setValue(1080);
    frameCountSpinBox_ = new QSpinBox(this);
    frameCountSpinBox_->setRange(1, 1000000);
    frameCountSpinBox_->setValue(300);
    
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
        tr("固定质量(CQ)"),
        tr("固定码率(CBR)"),
        tr("可变码率(VBR)")
    });
    
    rateValueSpinBox_ = new QSpinBox(this);
    rateValueSpinBox_->setRange(0, 63);
    
    rateLayout->addWidget(new QLabel(tr("控制模式:")), 0, 0);
    rateLayout->addWidget(rateControlCombo_, 0, 1);
    rateLayout->addWidget(new QLabel(tr("参数值:")), 1, 0);
    rateLayout->addWidget(rateValueSpinBox_, 1, 1);
    
    // 量化参数组
    auto* qpGroup = new QGroupBox(tr("量化参数"), this);
    auto* qpLayout = new QGridLayout(qpGroup);
    
    qminSpinBox_ = new QSpinBox(this);
    qminSpinBox_->setRange(0, 63);
    qmaxSpinBox_ = new QSpinBox(this);
    qmaxSpinBox_->setRange(0, 63);
    qmaxSpinBox_->setValue(63);
    keyintSpinBox_ = new QSpinBox(this);
    keyintSpinBox_->setRange(1, 1000);
    keyintSpinBox_->setValue(250);
    
    qpLayout->addWidget(new QLabel(tr("最小QP:")), 0, 0);
    qpLayout->addWidget(qminSpinBox_, 0, 1);
    qpLayout->addWidget(new QLabel(tr("最大QP:")), 1, 0);
    qpLayout->addWidget(qmaxSpinBox_, 1, 1);
    qpLayout->addWidget(new QLabel(tr("关键帧间隔:")), 2, 0);
    qpLayout->addWidget(keyintSpinBox_, 2, 1);
    
    // 添加所有组到左侧布局
    leftLayout->addWidget(basicGroup);
    leftLayout->addWidget(videoGroup);
    leftLayout->addWidget(rateGroup);
    leftLayout->addWidget(qpGroup);
    
    // 右侧编码控制区域
    // 场景选择
    sceneConfigCombo_ = new QComboBox(this);
    sceneConfigCombo_->addItems({
        tr("默认配置"),
        tr("视频会议"),
        tr("游戏直播"),
        tr("视频点播"),
        tr("屏幕录制")
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
    historyTable_->setColumnCount(10);
    historyTable_->setHorizontalHeaderLabels({
        tr("时间"),
        tr("分辨率"),
        tr("预设"),
        tr("速度"),
        tr("码率控制"),
        tr("码率/CQ"),
        tr("编码时间"),
        tr("编码速度"),
        tr("PSNR"),
        tr("SSIM")
    });
    
    // 设置表格属性
    historyTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    historyTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    historyTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    
    // 创建历史记录控制按钮
    auto* historyButtonLayout = new QHBoxLayout();
    clearHistoryButton_ = new QPushButton(tr("清空历史"), this);
    exportHistoryButton_ = new QPushButton(tr("导出历史"), this);
    historyButtonLayout->addWidget(clearHistoryButton_);
    historyButtonLayout->addWidget(exportHistoryButton_);
    
    historyLayout->addWidget(historyTable_);
    historyLayout->addLayout(historyButtonLayout);
    
    mainLayout->addWidget(historyGroup);
    
    setWindowTitle(tr("VP8编码器配置"));
    resize(1280, 960);
}

void VP8ConfigWindow::createConnections()
{
    // 编码控制
    connect(startButton_, &QPushButton::clicked, this, &VP8ConfigWindow::onStartEncoding);
    connect(stopButton_, &QPushButton::clicked, this, &VP8ConfigWindow::onStopEncoding);
    connect(generateButton_, &QPushButton::clicked, this, &VP8ConfigWindow::onGenerateFrames);
    
    // 参数设置
    connect(rateControlCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &VP8ConfigWindow::onRateControlChanged);
    connect(presetCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &VP8ConfigWindow::onPresetConfigSelected);
    connect(sceneConfigCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &VP8ConfigWindow::onSceneConfigChanged);
    
    // 视频播放
    connect(playButton_, &QPushButton::clicked, this, &VP8ConfigWindow::onPlayVideo);
    connect(mediaPlayer_, &QMediaPlayer::positionChanged, this, &VP8ConfigWindow::onMediaPositionChanged);
    connect(mediaPlayer_, &QMediaPlayer::durationChanged, this, &VP8ConfigWindow::onMediaDurationChanged);
    connect(videoSlider_, &QSlider::sliderMoved, this, &VP8ConfigWindow::onSliderMoved);
    
    // 历史记录
    connect(clearHistoryButton_, &QPushButton::clicked, this, &VP8ConfigWindow::clearEncodingHistory);
    connect(exportHistoryButton_, &QPushButton::clicked, this, &VP8ConfigWindow::exportEncodingHistory);
    
    // 加载历史记录
    QFile file(QDir::homePath() + "/.vp8_encoding_history.json");
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();
        
        if (doc.isArray()) {
            QJsonArray array = doc.array();
            for (const auto& value : array) {
                QJsonObject obj = value.toObject();
                VP8EncodingRecord record;
                record.timestamp = QDateTime::fromString(obj["timestamp"].toString(), Qt::ISODate);
                record.width = obj["width"].toInt();
                record.height = obj["height"].toInt();
                record.preset = obj["preset"].toString();
                record.speed = obj["speed"].toInt();
                record.rateControl = obj["rateControl"].toString();
                record.rateValue = obj["rateValue"].toInt();
                record.keyint = obj["keyint"].toInt();
                record.qmin = obj["qmin"].toInt();
                record.qmax = obj["qmax"].toInt();
                record.encodingTime = obj["encodingTime"].toDouble();
                record.fps = obj["fps"].toDouble();
                record.bitrate = obj["bitrate"].toDouble();
                record.psnr = obj["psnr"].toDouble();
                record.ssim = obj["ssim"].toDouble();
                encodingHistory_.push_back(record);
            }
            updateHistoryTable();
        }
    }
}

void VP8ConfigWindow::onStartEncoding()
{
    if (currentOutputFile_.isEmpty()) {
        currentOutputFile_ = QFileDialog::getSaveFileName(
            this,
            tr("选择输出文件"),
            QDir::homePath(),
            tr("WebM文件 (*.webm)")
        );
        if (currentOutputFile_.isEmpty()) {
            return;
        }
    }

    shouldStop_ = false;
    startButton_->setEnabled(false);
    stopButton_->setEnabled(true);
    playButton_->setEnabled(false);

    // 获取配置
    auto config = getConfigFromUI();

    // 清空日志
    logTextEdit_->clear();
    appendLog(tr("开始VP8编码..."));

    // 重置进度条
    progressBar_->setValue(0);
    progressBar_->setMaximum(config.frames);

    // 创建编码记录
    VP8EncodingRecord record;
    record.timestamp = QDateTime::currentDateTime();
    record.width = config.width;
    record.height = config.height;
    record.frameCount = config.frames;
    record.preset = presetCombo_->currentText();
    record.speed = speedCombo_->currentText().toInt();
    record.threads = config.threads;
    record.rateControl = rateControlCombo_->currentText();
    record.rateValue = rateValueSpinBox_->value();
    record.keyint = config.keyint;
    record.qmin = config.qmin;
    record.qmax = config.qmax;

    // 运行编码测试
    encodingThread_ = std::thread([this, config, record]() mutable {
        auto result = VP8ParamTest::runTest(config,
            [this](int frame, const VP8ParamTest::TestResult& result) {
                if (shouldStop_) return false;
                QMetaObject::invokeMethod(this, "updateProgress",
                    Qt::QueuedConnection,
                    Q_ARG(int, frame),
                    Q_ARG(double, result.encoding_time),
                    Q_ARG(double, result.fps),
                    Q_ARG(double, result.bitrate),
                    Q_ARG(double, result.psnr),
                    Q_ARG(double, result.ssim));
                return true;
            });
            
        record.encodingTime = result.encoding_time;
        record.fps = result.fps;
        record.bitrate = result.bitrate;
        record.psnr = result.psnr;
        record.ssim = result.ssim;
        
        QMetaObject::invokeMethod(this, "addEncodingRecord",
            Qt::QueuedConnection,
            Q_ARG(VP8EncodingRecord, record));
            
        QMetaObject::invokeMethod(this, "onEncodingFinished",
            Qt::QueuedConnection);
    });
    encodingThread_.detach();
}

void VP8ConfigWindow::onStopEncoding()
{
    shouldStop_ = true;
    appendLog(tr("\n编码已停止"));
}

void VP8ConfigWindow::onRateControlChanged(int index)
{
    bool isCQ = index == 0;
    if (isCQ) {
        rateValueSpinBox_->setRange(0, 63);
        rateValueSpinBox_->setValue(10);
        rateValueSpinBox_->setSuffix("");
    } else {
        rateValueSpinBox_->setRange(100, 50000);
        rateValueSpinBox_->setValue(2000);
        rateValueSpinBox_->setSuffix(" kbps");
    }
}

void VP8ConfigWindow::onPresetConfigSelected(int index)
{
    // 根据预设更新速度设置
    switch (index) {
        case 0: // 最佳质量
            speedCombo_->setCurrentIndex(0);
            break;
        case 1: // 平衡
            speedCombo_->setCurrentIndex(4);
            break;
        case 2: // 实时
            speedCombo_->setCurrentIndex(8);
            break;
    }
}

void VP8ConfigWindow::onSceneConfigChanged(int index)
{
    if (index == 0) return;  // 默认配置，不做改变

    VP8ParamTest::TestConfig config;
    
    switch (index) {
        case 1:  // 视频会议
            config.preset = VP8ParamTest::Preset::REALTIME;
            config.speed = static_cast<VP8ParamTest::Speed>(8);
            config.rate_control = VP8ParamTest::RateControl::CBR;
            config.bitrate = 1000000;  // 1Mbps
            config.keyint = 30;
            config.width = 1280;
            config.height = 720;
            config.fps = 30;
            break;
            
        case 2:  // 游戏直播
            config.preset = VP8ParamTest::Preset::GOOD;
            config.speed = static_cast<VP8ParamTest::Speed>(4);
            config.rate_control = VP8ParamTest::RateControl::VBR;
            config.bitrate = 4000000;  // 4Mbps
            config.keyint = 60;
            config.width = 1920;
            config.height = 1080;
            config.fps = 60;
            break;
            
        case 3:  // 视频点播
            config.preset = VP8ParamTest::Preset::BEST;
            config.speed = static_cast<VP8ParamTest::Speed>(1);
            config.rate_control = VP8ParamTest::RateControl::CQ;
            config.cq_level = 20;
            config.keyint = 250;
            config.width = 1920;
            config.height = 1080;
            config.fps = 30;
            break;
            
        case 4:  // 屏幕录制
            config.preset = VP8ParamTest::Preset::GOOD;
            config.speed = static_cast<VP8ParamTest::Speed>(6);
            config.rate_control = VP8ParamTest::RateControl::VBR;
            config.bitrate = 2000000;  // 2Mbps
            config.keyint = 120;
            config.width = 1920;
            config.height = 1080;
            config.fps = 30;
            break;
    }
    
    updateUIFromConfig(config);
}

void VP8ConfigWindow::onPlayVideo()
{
    if (!QFile::exists(currentOutputFile_)) {
        QMessageBox::warning(this, tr("错误"), tr("输出文件不存在"));
        return;
    }

    if (mediaPlayer_->playbackState() != QMediaPlayer::PlayingState) {
        mediaPlayer_->setSource(QUrl::fromLocalFile(currentOutputFile_));
        mediaPlayer_->play();
        playButton_->setText(tr("停止"));
    } else {
        mediaPlayer_->stop();
        playButton_->setText(tr("播放"));
    }
}

void VP8ConfigWindow::onMediaPositionChanged(qint64 position)
{
    videoSlider_->setValue(position);
    
    QTime currentTime(0, 0);
    currentTime = currentTime.addMSecs(position);
    
    QTime totalTime(0, 0);
    totalTime = totalTime.addMSecs(mediaPlayer_->duration());
    
    timeLabel_->setText(QString("%1 / %2")
        .arg(currentTime.toString("mm:ss"))
        .arg(totalTime.toString("mm:ss")));
}

void VP8ConfigWindow::onMediaDurationChanged(qint64 duration)
{
    videoSlider_->setRange(0, duration);
}

void VP8ConfigWindow::onSliderMoved(int position)
{
    mediaPlayer_->setPosition(position);
}

void VP8ConfigWindow::updateProgress(int frame, double time, double fps, double bitrate, double psnr, double ssim)
{
    progressBar_->setValue(frame + 1);
    appendLog(QString(tr("帧: %1, 时间: %2s, 速度: %3fps, 码率: %4kbps, PSNR: %5dB, SSIM: %6"))
        .arg(frame + 1)
        .arg(time, 0, 'f', 1)
        .arg(fps, 0, 'f', 1)
        .arg(bitrate / 1000.0, 0, 'f', 1)
        .arg(psnr, 0, 'f', 2)
        .arg(ssim, 0, 'f', 4));
}

void VP8ConfigWindow::appendLog(const QString& text)
{
    logTextEdit_->append(text);
    logTextEdit_->verticalScrollBar()->setValue(logTextEdit_->verticalScrollBar()->maximum());
}

void VP8ConfigWindow::onEncodingFinished()
{
    startButton_->setEnabled(true);
    stopButton_->setEnabled(false);
    playButton_->setEnabled(true);
    appendLog(tr("\n编码完成"));
}

void VP8ConfigWindow::onGenerateFrames()
{
    appendLog(tr("生成测试帧..."));
    auto config = getConfigFromUI();
    VP8ParamTest test;
    if (test.initFrameCache(config)) {
        appendLog(tr("测试帧生成完成"));
        frameGenProgressBar_->setValue(100);
    } else {
        appendLog(tr("测试帧生成失败"));
        frameGenProgressBar_->setValue(0);
    }
}

void VP8ConfigWindow::addEncodingRecord(const VP8EncodingRecord& record)
{
    encodingHistory_.push_back(record);
    updateHistoryTable();
    saveHistoryToFile();
}

void VP8ConfigWindow::updateHistoryTable()
{
    historyTable_->setRowCount(encodingHistory_.size());
    
    for (size_t i = 0; i < encodingHistory_.size(); ++i) {
        const auto& record = encodingHistory_[i];
        int col = 0;
        
        // 时间
        historyTable_->setItem(i, col++, new QTableWidgetItem(
            record.timestamp.toString("yyyy-MM-dd hh:mm:ss")));
        
        // 分辨率
        historyTable_->setItem(i, col++, new QTableWidgetItem(
            QString("%1x%2").arg(record.width).arg(record.height)));
        
        // 预设
        historyTable_->setItem(i, col++, new QTableWidgetItem(record.preset));
        
        // 速度
        historyTable_->setItem(i, col++, new QTableWidgetItem(
            QString::number(record.speed)));
        
        // 码率控制
        historyTable_->setItem(i, col++, new QTableWidgetItem(record.rateControl));
        
        // 码率/CQ
        QString rateStr;
        if (record.rateControl.contains("CQ")) {
            rateStr = QString("CQ %1").arg(record.rateValue);
        } else {
            rateStr = QString("%1 kbps").arg(record.rateValue);
        }
        historyTable_->setItem(i, col++, new QTableWidgetItem(rateStr));
        
        // 编码时间
        historyTable_->setItem(i, col++, new QTableWidgetItem(
            QString("%1 s").arg(record.encodingTime, 0, 'f', 1)));
        
        // 编码速度
        historyTable_->setItem(i, col++, new QTableWidgetItem(
            QString("%1 fps").arg(record.fps, 0, 'f', 1)));
        
        // PSNR
        historyTable_->setItem(i, col++, new QTableWidgetItem(
            QString("%1 dB").arg(record.psnr, 0, 'f', 2)));
        
        // SSIM
        historyTable_->setItem(i, col++, new QTableWidgetItem(
            QString("%1").arg(record.ssim, 0, 'f', 4)));
    }
    
    historyTable_->scrollToBottom();
}

void VP8ConfigWindow::clearEncodingHistory()
{
    encodingHistory_.clear();
    updateHistoryTable();
    saveHistoryToFile();
}

void VP8ConfigWindow::exportEncodingHistory()
{
    QString filePath = QFileDialog::getSaveFileName(
        this,
        tr("导出编码历史"),
        QDir::homePath(),
        tr("CSV文件 (*.csv)")
    );
    
    if (filePath.isEmpty()) return;
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("错误"), tr("无法打开文件进行写入"));
        return;
    }
    
    QTextStream out(&file);
    
    // 写入表头
    out << "时间,分辨率,预设,速度,码率控制,码率/CQ,编码时间,编码速度,PSNR,SSIM\n";
    
    // 写入数据
    for (const auto& record : encodingHistory_) {
        out << record.timestamp.toString("yyyy-MM-dd hh:mm:ss") << ",";
        out << record.width << "x" << record.height << ",";
        out << record.preset << ",";
        out << record.speed << ",";
        out << record.rateControl << ",";
        if (record.rateControl.contains("CQ")) {
            out << "CQ " << record.rateValue;
        } else {
            out << record.rateValue << " kbps";
        }
        out << ",";
        out << QString::number(record.encodingTime, 'f', 1) << ",";
        out << QString::number(record.fps, 'f', 1) << ",";
        out << QString::number(record.psnr, 'f', 2) << ",";
        out << QString::number(record.ssim, 'f', 4) << "\n";
    }
    
    file.close();
    QMessageBox::information(this, tr("成功"), tr("编码历史已成功导出"));
}

void VP8ConfigWindow::saveHistoryToFile()
{
    QFile file(QDir::homePath() + "/.vp8_encoding_history.json");
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "无法保存编码历史到文件";
        return;
    }
    
    QJsonArray array;
    for (const auto& record : encodingHistory_) {
        QJsonObject obj;
        obj["timestamp"] = record.timestamp.toString(Qt::ISODate);
        obj["width"] = record.width;
        obj["height"] = record.height;
        obj["preset"] = record.preset;
        obj["speed"] = record.speed;
        obj["rateControl"] = record.rateControl;
        obj["rateValue"] = record.rateValue;
        obj["keyint"] = record.keyint;
        obj["qmin"] = record.qmin;
        obj["qmax"] = record.qmax;
        obj["encodingTime"] = record.encodingTime;
        obj["fps"] = record.fps;
        obj["bitrate"] = record.bitrate;
        obj["psnr"] = record.psnr;
        obj["ssim"] = record.ssim;
        array.append(obj);
    }
    
    QJsonDocument doc(array);
    file.write(doc.toJson());
    file.close();
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
    if (config.rate_control == VP8ParamTest::RateControl::CQ) {
        rateValueSpinBox_->setValue(config.cq_level);
    } else {
        rateValueSpinBox_->setValue(config.bitrate / 1000); // 转换为kbps
    }
    
    qminSpinBox_->setValue(config.qmin);
    qmaxSpinBox_->setValue(config.qmax);
    keyintSpinBox_->setValue(config.keyint);
    threadsSpinBox_->setValue(config.threads);
    
    // 更新视频参数
    widthSpinBox_->setValue(config.width);
    heightSpinBox_->setValue(config.height);
    frameCountSpinBox_->setValue(config.frames);
    
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
    
    // 码率控制参数
    if (config.rate_control == VP8ParamTest::RateControl::CQ) {
        config.cq_level = rateValueSpinBox_->value();
        config.bitrate = 0;
    } else {
        config.cq_level = 0;
        config.bitrate = rateValueSpinBox_->value() * 1000; // 转换为bps
    }
    
    // 其他参数
    config.qmin = qminSpinBox_->value();
    config.qmax = qmaxSpinBox_->value();
    config.keyint = keyintSpinBox_->value();
    config.threads = threadsSpinBox_->value();
    
    // 视频参数
    config.width = widthSpinBox_->value();
    config.height = heightSpinBox_->value();
    config.frames = frameCountSpinBox_->value();
    
    // 输出文件
    config.output_file = currentOutputFile_.toStdString();
    
    return config;
}

// ... 其他函数的实现 ... 