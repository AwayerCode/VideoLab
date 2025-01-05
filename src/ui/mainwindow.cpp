#include "mainwindow.hpp"
#include "../ffmpeg/x264_param_test.hpp"
#include "../ffmpeg/ffmpeg.hpp"
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QMimeDatabase>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , centralWidget_(new QWidget(this))
    , mainLayout_(new QVBoxLayout(centralWidget_))
{
    setCentralWidget(centralWidget_);
    setupUI();
    setWindowTitle("视频编码工具");
    resize(800, 600);
}

void MainWindow::setupUI() {
    // 创建标签页
    tabWidget_ = new QTabWidget(centralWidget_);
    mainLayout_->addWidget(tabWidget_);

    // 创建编码测试页面
    createEncoderTestTab();

    // 创建格式解析页面
    createFormatAnalysisTab();

    // 创建播放器页面
    createPlayerTab();

    // 创建滤镜测试页面
    createFilterTab();
}

void MainWindow::createEncoderTestTab() {
    encoderTestTab_ = new QWidget();
    encoderTestLayout_ = new QVBoxLayout(encoderTestTab_);
    
    createEncoderGroup();
    createParameterGroup();
    createTestGroup();
    createResultGroup();

    // 连接信号和槽
    connect(startButton_, &QPushButton::clicked, this, &MainWindow::onStartTest);
    connect(sceneCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onSceneChanged);

    tabWidget_->addTab(encoderTestTab_, "编码测试");
}

void MainWindow::createFormatAnalysisTab() {
    formatAnalysisTab_ = new QWidget();
    formatAnalysisLayout_ = new QVBoxLayout(formatAnalysisTab_);
    formatAnalysisLayout_->setSpacing(6);  // 减小间距
    formatAnalysisLayout_->setContentsMargins(6, 6, 6, 6);  // 减小边距

    createFileGroup();
    
    // 连接信号和槽
    connect(selectFileButton_, &QPushButton::clicked, this, &MainWindow::onSelectFile);
    connect(analyzeButton_, &QPushButton::clicked, this, &MainWindow::onAnalyzeFile);

    // 创建分割器
    formatAnalysisSplitter_ = new QSplitter(Qt::Horizontal, formatAnalysisTab_);
    
    // 创建树形视图
    formatTree_ = new QTreeWidget(formatAnalysisSplitter_);
    formatTree_->setHeaderLabels({"属性", "值"});
    formatTree_->setColumnWidth(0, 200);
    formatTree_->setMinimumHeight(400);  // 设置最小高度
    
    // 创建文本编辑器
    resultText_ = new QTextEdit(formatAnalysisSplitter_);
    resultText_->setReadOnly(true);
    
    // 设置分割器的初始大小比例
    formatAnalysisSplitter_->setStretchFactor(0, 2);  // 树形视图占比更大
    formatAnalysisSplitter_->setStretchFactor(1, 1);
    
    formatAnalysisLayout_->addWidget(formatAnalysisSplitter_);
    tabWidget_->addTab(formatAnalysisTab_, "格式解析");
}

void MainWindow::createFileGroup() {
    fileGroup_ = new QGroupBox("视频文件", formatAnalysisTab_);
    auto* layout = new QHBoxLayout(fileGroup_);
    layout->setContentsMargins(6, 6, 6, 6);  // 减小边距

    filePathEdit_ = new QLineEdit(fileGroup_);
    filePathEdit_->setObjectName("filePathEdit_");
    filePathEdit_->setReadOnly(true);
    filePathEdit_->setPlaceholderText("请选择视频文件...");
    filePathEdit_->setMaximumHeight(25);  // 限制高度

    selectFileButton_ = new QPushButton("选择文件", fileGroup_);
    selectFileButton_->setObjectName("selectFileButton_");
    selectFileButton_->setMaximumWidth(80);  // 限制按钮宽度
    selectFileButton_->setMaximumHeight(25);  // 限制按钮高度

    analyzeButton_ = new QPushButton("解析文件", fileGroup_);
    analyzeButton_->setObjectName("analyzeButton_");
    analyzeButton_->setEnabled(false);
    analyzeButton_->setMaximumWidth(80);  // 限制按钮宽度
    analyzeButton_->setMaximumHeight(25);  // 限制按钮高度

    layout->addWidget(filePathEdit_);
    layout->addWidget(selectFileButton_);
    layout->addWidget(analyzeButton_);

    formatAnalysisLayout_->addWidget(fileGroup_);
}

void MainWindow::createEncoderGroup() {
    encoderGroup_ = new QGroupBox("编码器选择", encoderTestTab_);
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

    encoderTestLayout_->addWidget(encoderGroup_);
}

void MainWindow::createParameterGroup() {
    paramGroup_ = new QGroupBox("参数设置", encoderTestTab_);
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

    encoderTestLayout_->addWidget(paramGroup_);
}

void MainWindow::createTestGroup() {
    testGroup_ = new QGroupBox("测试控制", encoderTestTab_);
    auto* layout = new QVBoxLayout(testGroup_);

    startButton_ = new QPushButton("开始测试", testGroup_);
    startButton_->setObjectName("startButton_");
    progressBar_ = new QProgressBar(testGroup_);
    progressBar_->setObjectName("progressBar_");
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);

    layout->addWidget(startButton_);
    layout->addWidget(progressBar_);

    encoderTestLayout_->addWidget(testGroup_);
}

void MainWindow::createResultGroup() {
    resultGroup_ = new QGroupBox("测试结果", encoderTestTab_);
    auto* layout = new QVBoxLayout(resultGroup_);

    resultText_ = new QTextEdit(resultGroup_);
    resultText_->setObjectName("resultText_");
    resultText_->setReadOnly(true);

    layout->addWidget(resultText_);

    encoderTestLayout_->addWidget(resultGroup_);
}

void MainWindow::onSelectFile() {
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "选择视频文件",
        QString(),
        "视频文件 (*.mp4 *.avi *.mkv *.mov *.wmv);;所有文件 (*.*)"
    );

    if (!filePath.isEmpty()) {
        filePathEdit_->setText(filePath);
        analyzeButton_->setEnabled(true);
    }
}

void MainWindow::onAnalyzeFile() {
    QString filePath = filePathEdit_->text();
    if (filePath.isEmpty()) {
        QMessageBox::warning(this, "错误", "请先选择视频文件");
        return;
    }

    // 清空之前的结果
    formatTree_->clear();
    resultText_->clear();

    QFileInfo fileInfo(filePath);
    
    // 创建基本信息节点
    auto* basicInfoItem = new QTreeWidgetItem(formatTree_, {"基本信息"});
    new QTreeWidgetItem(basicInfoItem, {"文件名", fileInfo.fileName()});
    new QTreeWidgetItem(basicInfoItem, {"文件大小", QString("%1 MB").arg(fileInfo.size() / 1024.0 / 1024.0, 0, 'f', 2)});
    new QTreeWidgetItem(basicInfoItem, {"创建时间", fileInfo.birthTime().toString("yyyy-MM-dd hh:mm:ss")});
    new QTreeWidgetItem(basicInfoItem, {"修改时间", fileInfo.lastModified().toString("yyyy-MM-dd hh:mm:ss")});

    // 使用FFmpeg分析视频文件
    FFmpeg ffmpeg;
    if (ffmpeg.openFile(filePath.toStdString())) {
        AVFormatContext* formatContext = ffmpeg.getFormatContext();
        if (formatContext) {
            // 创建容器格式节点
            auto* formatInfoItem = new QTreeWidgetItem(formatTree_, {"容器格式"});
            new QTreeWidgetItem(formatInfoItem, {"格式", formatContext->iformat->long_name});
            new QTreeWidgetItem(formatInfoItem, {"时长", QString("%1 秒").arg(formatContext->duration / 1000000.0, 0, 'f', 2)});
            new QTreeWidgetItem(formatInfoItem, {"比特率", QString("%1 Kbps").arg(formatContext->bit_rate / 1000)});
            new QTreeWidgetItem(formatInfoItem, {"流数量", QString::number(formatContext->nb_streams)});

            // 分析每个流
            for (unsigned int i = 0; i < formatContext->nb_streams; i++) {
                AVStream* stream = formatContext->streams[i];
                AVCodecParameters* codecParams = stream->codecpar;
                const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);

                QString streamType;
                auto* streamItem = new QTreeWidgetItem(formatTree_);

                switch (codecParams->codec_type) {
                    case AVMEDIA_TYPE_VIDEO: {
                        streamType = "视频流";
                        streamItem->setText(0, QString("视频流 #%1").arg(i));
                        if (codec) {
                            new QTreeWidgetItem(streamItem, {"编码器", codec->long_name});
                        }
                        new QTreeWidgetItem(streamItem, {"分辨率", QString("%1x%2").arg(codecParams->width).arg(codecParams->height)});
                        if (stream->avg_frame_rate.num && stream->avg_frame_rate.den) {
                            new QTreeWidgetItem(streamItem, {"帧率", QString("%1 fps").arg(av_q2d(stream->avg_frame_rate), 0, 'f', 2)});
                        }
                        
                        // 如果是H.264编码，添加详细信息
                        if (codecParams->codec_id == AV_CODEC_ID_H264) {
                            updateH264Details(codecParams, streamItem);
                        }
                        break;
                    }
                    case AVMEDIA_TYPE_AUDIO: {
                        streamType = "音频流";
                        streamItem->setText(0, QString("音频流 #%1").arg(i));
                        if (codec) {
                            new QTreeWidgetItem(streamItem, {"编码器", codec->long_name});
                        }
                        new QTreeWidgetItem(streamItem, {"采样率", QString("%1 Hz").arg(codecParams->sample_rate)});
                        new QTreeWidgetItem(streamItem, {"声道数", QString::number(codecParams->ch_layout.nb_channels)});
                        new QTreeWidgetItem(streamItem, {"采样格式", QString::number(codecParams->format)});
                        break;
                    }
                    case AVMEDIA_TYPE_SUBTITLE: {
                        streamType = "字幕流";
                        streamItem->setText(0, QString("字幕流 #%1").arg(i));
                        if (codec) {
                            new QTreeWidgetItem(streamItem, {"编码器", codec->long_name});
                        }
                        break;
                    }
                    default:
                        streamType = "其他流";
                        streamItem->setText(0, QString("其他流 #%1").arg(i));
                        break;
                }

                // 添加流的通用信息
                new QTreeWidgetItem(streamItem, {"索引", QString::number(i)});
                new QTreeWidgetItem(streamItem, {"编码器ID", QString::number(codecParams->codec_id)});
                if (stream->duration > 0) {
                    new QTreeWidgetItem(streamItem, {"流时长", QString("%1 秒").arg(stream->duration * av_q2d(stream->time_base), 0, 'f', 2)});
                }
            }
        }
        ffmpeg.closeFile();
    } else {
        QMessageBox::warning(this, "错误", "无法打开文件进行分析，请确保文件格式正确且未被损坏。");
    }

    // 展开所有节点
    formatTree_->expandAll();
}

void MainWindow::updateH264Details(AVCodecParameters* codecParams, QTreeWidgetItem* parent) {
    auto* h264Item = new QTreeWidgetItem(parent, {"H.264详细信息"});
    
    // 基本编码信息
    QString profileName;
    switch (codecParams->profile) {
        case FF_PROFILE_H264_BASELINE: profileName = "Baseline"; break;
        case FF_PROFILE_H264_MAIN: profileName = "Main"; break;
        case FF_PROFILE_H264_EXTENDED: profileName = "Extended"; break;
        case FF_PROFILE_H264_HIGH: profileName = "High"; break;
        case FF_PROFILE_H264_HIGH_10: profileName = "High 10"; break;
        case FF_PROFILE_H264_HIGH_422: profileName = "High 422"; break;
        case FF_PROFILE_H264_HIGH_444: profileName = "High 444"; break;
        default: profileName = QString::number(codecParams->profile); break;
    }
    new QTreeWidgetItem(h264Item, {"Profile", profileName});
    
    // Level信息
    double level = codecParams->level / 10.0;
    new QTreeWidgetItem(h264Item, {"Level", QString::number(level, 'f', 1)});
    
    // 编码参数
    if (codecParams->bit_rate > 0) {
        new QTreeWidgetItem(h264Item, {"编码器比特率", QString("%1 Kbps").arg(codecParams->bit_rate / 1000)});
    }

    // NAL单元分析
    if (codecParams->extradata && codecParams->extradata_size > 0) {
        auto* nalItem = new QTreeWidgetItem(h264Item, {"NAL单元分析"});
        const uint8_t* data = codecParams->extradata;
        int size = codecParams->extradata_size;
        
        int spsCount = 0;
        int ppsCount = 0;
        QMap<int, int> naluTypes;  // 统计各类型NAL单元数量

        for (int i = 0; i < size - 4; i++) {
            // 查找NAL单元起始码
            if ((data[i] == 0x00 && data[i+1] == 0x00 && data[i+2] == 0x01) ||
                (data[i] == 0x00 && data[i+1] == 0x00 && data[i+2] == 0x00 && data[i+3] == 0x01)) {
                
                int startCodeLen = (data[i+2] == 0x01) ? 3 : 4;
                int offset = i + startCodeLen;
                uint8_t nalType = data[offset] & 0x1F;
                naluTypes[nalType]++;

                // 分析特定类型的NAL单元
                switch (nalType) {
                    case 7: {  // SPS
                        spsCount++;
                        auto* spsItem = new QTreeWidgetItem(nalItem, {QString("SPS #%1").arg(spsCount)});
                        // SPS详细信息解析
                        uint8_t profileIdc = data[offset + 1];
                        uint8_t levelIdc = data[offset + 3];
                        new QTreeWidgetItem(spsItem, {"Profile IDC", QString::number(profileIdc)});
                        new QTreeWidgetItem(spsItem, {"Level IDC", QString::number(levelIdc)});
                        break;
                    }
                    case 8: {  // PPS
                        ppsCount++;
                        auto* ppsItem = new QTreeWidgetItem(nalItem, {QString("PPS #%1").arg(ppsCount)});
                        // PPS基本信息
                        new QTreeWidgetItem(ppsItem, {"偏移位置", QString("0x%1").arg(offset, 0, 16)});
                        break;
                    }
                }
                
                // 跳过已处理的NAL单元
                i = offset;
            }
        }

        // 添加NAL单元统计信息
        auto* nalStatsItem = new QTreeWidgetItem(nalItem, {"NAL单元统计"});
        new QTreeWidgetItem(nalStatsItem, {"SPS数量", QString::number(spsCount)});
        new QTreeWidgetItem(nalStatsItem, {"PPS数量", QString::number(ppsCount)});
        
        // 添加各类型NAL单元统计
        for (auto it = naluTypes.constBegin(); it != naluTypes.constEnd(); ++it) {
            QString nalTypeName;
            switch (it.key()) {
                case 1: nalTypeName = "非IDR图像片段"; break;
                case 5: nalTypeName = "IDR图像片段"; break;
                case 6: nalTypeName = "SEI"; break;
                case 7: nalTypeName = "SPS"; break;
                case 8: nalTypeName = "PPS"; break;
                case 9: nalTypeName = "分隔符"; break;
                default: nalTypeName = QString("类型%1").arg(it.key()); break;
            }
            new QTreeWidgetItem(nalStatsItem, {
                nalTypeName,
                QString("%1个").arg(it.value())
            });
        }
    }

    // 编码参数组
    auto* paramItem = new QTreeWidgetItem(h264Item, {"编码参数"});
    new QTreeWidgetItem(paramItem, {"色彩空间", getPixFmtName(codecParams->format)});
    new QTreeWidgetItem(paramItem, {"比特深度", QString("%1 bits").arg(av_get_bits_per_pixel(av_pix_fmt_desc_get((AVPixelFormat)codecParams->format)))});
    new QTreeWidgetItem(paramItem, {"色度子采样", getChromaSamplingName(codecParams->format)});
}

// 辅助函数：获取像素格式名称
QString MainWindow::getPixFmtName(int format) {
    switch (format) {
        case AV_PIX_FMT_YUV420P: return "YUV420P";
        case AV_PIX_FMT_YUV422P: return "YUV422P";
        case AV_PIX_FMT_YUV444P: return "YUV444P";
        case AV_PIX_FMT_NV12: return "NV12";
        case AV_PIX_FMT_NV21: return "NV21";
        default: return QString("格式%1").arg(format);
    }
}

// 辅助函数：获取色度子采样名称
QString MainWindow::getChromaSamplingName(int format) {
    switch (format) {
        case AV_PIX_FMT_YUV420P: return "4:2:0";
        case AV_PIX_FMT_YUV422P: return "4:2:2";
        case AV_PIX_FMT_YUV444P: return "4:4:4";
        case AV_PIX_FMT_NV12: return "4:2:0";
        case AV_PIX_FMT_NV21: return "4:2:0";
        default: return "未知";
    }
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

void MainWindow::createPlayerTab() {
    playerTab_ = new QWidget();
    playerLayout_ = new QVBoxLayout(playerTab_);
    playerLayout_->setSpacing(6);
    playerLayout_->setContentsMargins(6, 6, 6, 6);

    // 创建视频播放控件
    videoWidget_ = new QVideoWidget(playerTab_);
    videoWidget_->setMinimumHeight(400);
    playerLayout_->addWidget(videoWidget_);

    // 创建媒体播放器
    mediaPlayer_ = new QMediaPlayer(this);
    mediaPlayer_->setVideoOutput(videoWidget_);

    // 创建控制栏
    auto* controlLayout = new QHBoxLayout();
    controlLayout->setSpacing(6);

    // 选择文件按钮
    selectVideoButton_ = new QPushButton("选择视频", playerTab_);
    selectVideoButton_->setMaximumWidth(80);

    // 播放/暂停按钮
    playPauseButton_ = new QPushButton("播放", playerTab_);
    playPauseButton_->setMaximumWidth(80);
    playPauseButton_->setEnabled(false);

    // 时间标签
    timeLabel_ = new QLabel("00:00:00", playerTab_);
    timeLabel_->setMinimumWidth(60);
    durationLabel_ = new QLabel("00:00:00", playerTab_);
    durationLabel_->setMinimumWidth(60);

    // 进度滑块
    positionSlider_ = new QSlider(Qt::Horizontal, playerTab_);
    positionSlider_->setEnabled(false);

    // 添加到控制栏布局
    controlLayout->addWidget(selectVideoButton_);
    controlLayout->addWidget(playPauseButton_);
    controlLayout->addWidget(timeLabel_);
    controlLayout->addWidget(positionSlider_);
    controlLayout->addWidget(durationLabel_);

    playerLayout_->addLayout(controlLayout);

    // 连接信号和槽
    connect(selectVideoButton_, &QPushButton::clicked, this, &MainWindow::onSelectVideoFile);
    connect(playPauseButton_, &QPushButton::clicked, this, &MainWindow::onPlayPause);
    connect(mediaPlayer_, &QMediaPlayer::positionChanged, this, &MainWindow::onPositionChanged);
    connect(mediaPlayer_, &QMediaPlayer::durationChanged, this, &MainWindow::onDurationChanged);
    connect(positionSlider_, &QSlider::sliderMoved, this, &MainWindow::onSliderMoved);
    connect(mediaPlayer_, &QMediaPlayer::playbackStateChanged, this, &MainWindow::onPlayerStateChanged);

    tabWidget_->addTab(playerTab_, "视频播放");
}

void MainWindow::onSelectVideoFile() {
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "选择视频文件",
        QString(),
        "视频文件 (*.mp4 *.avi *.mkv *.mov *.wmv);;所有文件 (*.*)"
    );

    if (!filePath.isEmpty()) {
        mediaPlayer_->setSource(QUrl::fromLocalFile(filePath));
        playPauseButton_->setEnabled(true);
        playPauseButton_->setText("播放");
    }
}

void MainWindow::onPlayPause() {
    if (mediaPlayer_->playbackState() == QMediaPlayer::PlayingState) {
        mediaPlayer_->pause();
    } else {
        mediaPlayer_->play();
    }
}

void MainWindow::onPositionChanged(qint64 position) {
    if (!positionSlider_->isSliderDown()) {
        positionSlider_->setValue(position);
    }
    timeLabel_->setText(formatTime(position));
}

void MainWindow::onDurationChanged(qint64 duration) {
    positionSlider_->setRange(0, duration);
    positionSlider_->setEnabled(true);
    durationLabel_->setText(formatTime(duration));
}

void MainWindow::onSliderMoved(int position) {
    mediaPlayer_->setPosition(position);
}

void MainWindow::onPlayerStateChanged(QMediaPlayer::PlaybackState state) {
    switch (state) {
        case QMediaPlayer::PlayingState:
            playPauseButton_->setText("暂停");
            break;
        case QMediaPlayer::PausedState:
        case QMediaPlayer::StoppedState:
            playPauseButton_->setText("播放");
            break;
    }
}

QString MainWindow::formatTime(qint64 ms) {
    qint64 seconds = ms / 1000;
    qint64 minutes = seconds / 60;
    qint64 hours = minutes / 60;

    return QString("%1:%2:%3")
        .arg(hours, 2, 10, QChar('0'))
        .arg(minutes % 60, 2, 10, QChar('0'))
        .arg(seconds % 60, 2, 10, QChar('0'));
}

void MainWindow::createFilterTab() {
    filterTab_ = new QWidget();
    filterLayout_ = new QHBoxLayout(filterTab_);
    filterLayout_->setSpacing(6);
    filterLayout_->setContentsMargins(6, 6, 6, 6);

    // 创建源视频播放器
    sourceVideoWidget_ = new QVideoWidget(filterTab_);
    sourceVideoWidget_->setMinimumWidth(400);
    sourceVideoWidget_->setMinimumHeight(400);
    filterLayout_->addWidget(sourceVideoWidget_);

    sourcePlayer_ = new QMediaPlayer(this);
    sourcePlayer_->setVideoOutput(sourceVideoWidget_);

    // 创建滤镜控制区域
    filterControlWidget_ = new QWidget(filterTab_);
    filterControlWidget_->setMaximumWidth(200);
    filterControlLayout_ = new QVBoxLayout(filterControlWidget_);
    filterControlLayout_->setSpacing(6);
    filterControlLayout_->setContentsMargins(6, 6, 6, 6);

    // 文件选择按钮
    selectFilterFileButton_ = new QPushButton("选择视频", filterControlWidget_);
    filterControlLayout_->addWidget(selectFilterFileButton_);

    // 播放控制
    filterPlayPauseButton_ = new QPushButton("播放", filterControlWidget_);
    filterPlayPauseButton_->setEnabled(false);
    filterControlLayout_->addWidget(filterPlayPauseButton_);

    // 滤镜选择
    filterCombo_ = new QComboBox(filterControlWidget_);
    filterCombo_->addItem("亮度/对比度");
    filterCombo_->addItem("HSV调整");
    filterCombo_->addItem("锐化/模糊");
    filterControlLayout_->addWidget(filterCombo_);

    // 滤镜参数区域
    filterParamsWidget_ = new QStackedWidget(filterControlWidget_);
    
    // 亮度/对比度参数
    brightnessContrastWidget_ = new QWidget();
    auto* brightnessLayout = new QVBoxLayout(brightnessContrastWidget_);
    brightnessSpin_ = new QSpinBox(brightnessContrastWidget_);
    brightnessSpin_->setRange(-100, 100);
    brightnessSpin_->setValue(0);
    brightnessSpin_->setSuffix(" %");
    brightnessLayout->addWidget(new QLabel("亮度:"));
    brightnessLayout->addWidget(brightnessSpin_);

    contrastSpin_ = new QSpinBox(brightnessContrastWidget_);
    contrastSpin_->setRange(-100, 100);
    contrastSpin_->setValue(0);
    contrastSpin_->setSuffix(" %");
    brightnessLayout->addWidget(new QLabel("对比度:"));
    brightnessLayout->addWidget(contrastSpin_);
    brightnessLayout->addStretch();
    filterParamsWidget_->addWidget(brightnessContrastWidget_);

    // HSV调整参数
    hsvAdjustWidget_ = new QWidget();
    auto* hsvLayout = new QVBoxLayout(hsvAdjustWidget_);
    hueSpin_ = new QSpinBox(hsvAdjustWidget_);
    hueSpin_->setRange(-180, 180);
    hueSpin_->setValue(0);
    hueSpin_->setSuffix("°");
    hsvLayout->addWidget(new QLabel("色相:"));
    hsvLayout->addWidget(hueSpin_);

    saturationSpin_ = new QSpinBox(hsvAdjustWidget_);
    saturationSpin_->setRange(-100, 100);
    saturationSpin_->setValue(0);
    saturationSpin_->setSuffix(" %");
    hsvLayout->addWidget(new QLabel("饱和度:"));
    hsvLayout->addWidget(saturationSpin_);

    valueSpin_ = new QSpinBox(hsvAdjustWidget_);
    valueSpin_->setRange(-100, 100);
    valueSpin_->setValue(0);
    valueSpin_->setSuffix(" %");
    hsvLayout->addWidget(new QLabel("明度:"));
    hsvLayout->addWidget(valueSpin_);
    hsvLayout->addStretch();
    filterParamsWidget_->addWidget(hsvAdjustWidget_);

    // 锐化/模糊参数
    sharpenBlurWidget_ = new QWidget();
    auto* sharpenLayout = new QVBoxLayout(sharpenBlurWidget_);
    sharpenSpin_ = new QDoubleSpinBox(sharpenBlurWidget_);
    sharpenSpin_->setRange(0, 10);
    sharpenSpin_->setValue(0);
    sharpenSpin_->setSingleStep(0.1);
    sharpenLayout->addWidget(new QLabel("锐化:"));
    sharpenLayout->addWidget(sharpenSpin_);

    blurSpin_ = new QDoubleSpinBox(sharpenBlurWidget_);
    blurSpin_->setRange(0, 10);
    blurSpin_->setValue(0);
    blurSpin_->setSingleStep(0.1);
    sharpenLayout->addWidget(new QLabel("模糊:"));
    sharpenLayout->addWidget(blurSpin_);
    sharpenLayout->addStretch();
    filterParamsWidget_->addWidget(sharpenBlurWidget_);

    filterControlLayout_->addWidget(filterParamsWidget_);

    // 进度控制
    filterPositionSlider_ = new QSlider(Qt::Horizontal, filterControlWidget_);
    filterPositionSlider_->setEnabled(false);
    filterControlLayout_->addWidget(filterPositionSlider_);

    auto* timeLayout = new QHBoxLayout();
    filterTimeLabel_ = new QLabel("00:00:00", filterControlWidget_);
    filterDurationLabel_ = new QLabel("00:00:00", filterControlWidget_);
    timeLayout->addWidget(filterTimeLabel_);
    timeLayout->addStretch();
    timeLayout->addWidget(filterDurationLabel_);
    filterControlLayout_->addLayout(timeLayout);

    filterLayout_->addWidget(filterControlWidget_);

    // 创建处理后的视频播放器
    filteredVideoWidget_ = new QVideoWidget(filterTab_);
    filteredVideoWidget_->setMinimumWidth(400);
    filteredVideoWidget_->setMinimumHeight(400);
    filterLayout_->addWidget(filteredVideoWidget_);

    filteredPlayer_ = new QMediaPlayer(this);
    filteredPlayer_->setVideoOutput(filteredVideoWidget_);

    // 连接信号和槽
    connect(selectFilterFileButton_, &QPushButton::clicked, this, &MainWindow::onSelectFilterFile);
    connect(filterPlayPauseButton_, &QPushButton::clicked, this, &MainWindow::onFilterPlayPause);
    connect(filterCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onFilterChanged);
    connect(brightnessSpin_, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::onFilterParamChanged);
    connect(contrastSpin_, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::onFilterParamChanged);
    connect(hueSpin_, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::onFilterParamChanged);
    connect(saturationSpin_, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::onFilterParamChanged);
    connect(valueSpin_, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::onFilterParamChanged);
    connect(sharpenSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onFilterParamChanged);
    connect(blurSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onFilterParamChanged);
    connect(sourcePlayer_, &QMediaPlayer::positionChanged,
            filterPositionSlider_, &QSlider::setValue);
    connect(sourcePlayer_, &QMediaPlayer::durationChanged, this,
            [this](qint64 duration) {
                filterPositionSlider_->setRange(0, duration);
                filterPositionSlider_->setEnabled(true);
                filterDurationLabel_->setText(formatTime(duration));
            });
    connect(filterPositionSlider_, &QSlider::sliderMoved,
            [this](int position) {
                sourcePlayer_->setPosition(position);
                filteredPlayer_->setPosition(position);
            });
    connect(sourcePlayer_, &QMediaPlayer::positionChanged,
            [this](qint64 position) {
                if (!filterPositionSlider_->isSliderDown()) {
                    filterPositionSlider_->setValue(position);
                }
                filterTimeLabel_->setText(formatTime(position));
            });

    tabWidget_->addTab(filterTab_, "滤镜测试");
}

void MainWindow::onSelectFilterFile() {
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "选择视频文件",
        QString(),
        "视频文件 (*.mp4 *.avi *.mkv *.mov *.wmv);;所有文件 (*.*)"
    );

    if (!filePath.isEmpty()) {
        sourcePlayer_->setSource(QUrl::fromLocalFile(filePath));
        filteredPlayer_->setSource(QUrl::fromLocalFile(filePath));  // 暂时使用相同文件
        filterPlayPauseButton_->setEnabled(true);
        filterPlayPauseButton_->setText("播放");
    }
}

void MainWindow::onFilterPlayPause() {
    if (sourcePlayer_->playbackState() == QMediaPlayer::PlayingState) {
        sourcePlayer_->pause();
        filteredPlayer_->pause();
        filterPlayPauseButton_->setText("播放");
    } else {
        sourcePlayer_->play();
        filteredPlayer_->play();
        filterPlayPauseButton_->setText("暂停");
    }
}

void MainWindow::onFilterChanged(int index) {
    filterParamsWidget_->setCurrentIndex(index);
    onFilterParamChanged();  // 更新滤镜效果
}

void MainWindow::onFilterParamChanged() {
    if (!sourcePlayer_->source().isValid()) {
        return;
    }

    QString inputPath = sourcePlayer_->source().toLocalFile();
    QString outputPath = inputPath + ".filtered.mp4";

    VideoFilter::FilterParams params;
    switch (filterCombo_->currentIndex()) {
        case 0:  // 亮度/对比度
            params.type = VideoFilter::FilterType::BRIGHTNESS_CONTRAST;
            params.bc.brightness = brightnessSpin_->value();
            params.bc.contrast = contrastSpin_->value();
            break;
        case 1:  // HSV调整
            params.type = VideoFilter::FilterType::HSV_ADJUST;
            params.hsv.hue = hueSpin_->value();
            params.hsv.saturation = saturationSpin_->value();
            params.hsv.value = valueSpin_->value();
            break;
        case 2:  // 锐化/模糊
            params.type = VideoFilter::FilterType::SHARPEN_BLUR;
            params.sb.sharpen = sharpenSpin_->value();
            params.sb.blur = blurSpin_->value();
            break;
    }

    VideoFilter filter;
    if (!filter.init(inputPath.toStdString(), outputPath.toStdString())) {
        QMessageBox::warning(this, "错误", "初始化滤镜失败");
        return;
    }

    if (!filter.updateFilter(params)) {
        QMessageBox::warning(this, "错误", "更新滤镜参数失败");
        return;
    }

    // 处理视频
    while (filter.processFrame()) {
        // TODO: 更新进度显示
    }

    filter.close();

    // 更新右侧视频播放器
    filteredPlayer_->setSource(QUrl::fromLocalFile(outputPath));
    if (sourcePlayer_->playbackState() == QMediaPlayer::PlayingState) {
        filteredPlayer_->play();
    }
} 