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
    setWindowTitle("编码器性能测试");
    resize(800, 600);
}

void MainWindow::setupUI() {
    createFileGroup();
    createEncoderGroup();
    createParameterGroup();
    createTestGroup();
    createResultGroup();

    // 连接信号和槽
    connect(startButton_, &QPushButton::clicked, this, &MainWindow::onStartTest);
    connect(sceneCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onSceneChanged);
    connect(selectFileButton_, &QPushButton::clicked, this, &MainWindow::onSelectFile);
    connect(analyzeButton_, &QPushButton::clicked, this, &MainWindow::onAnalyzeFile);
}

void MainWindow::createFileGroup() {
    fileGroup_ = new QGroupBox("视频文件", centralWidget_);
    auto* layout = new QHBoxLayout(fileGroup_);

    filePathEdit_ = new QLineEdit(fileGroup_);
    filePathEdit_->setObjectName("filePathEdit_");
    filePathEdit_->setReadOnly(true);
    filePathEdit_->setPlaceholderText("请选择视频文件...");

    selectFileButton_ = new QPushButton("选择文件", fileGroup_);
    selectFileButton_->setObjectName("selectFileButton_");

    analyzeButton_ = new QPushButton("解析文件", fileGroup_);
    analyzeButton_->setObjectName("analyzeButton_");
    analyzeButton_->setEnabled(false);

    layout->addWidget(filePathEdit_);
    layout->addWidget(selectFileButton_);
    layout->addWidget(analyzeButton_);

    mainLayout_->addWidget(fileGroup_);
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

    QFileInfo fileInfo(filePath);
    QMimeDatabase db;
    QString mimeType = db.mimeTypeForFile(filePath).name();

    QString resultStr;
    resultStr += QString("文件信息:\n");
    resultStr += QString("文件名: %1\n").arg(fileInfo.fileName());
    resultStr += QString("文件大小: %1 MB\n").arg(fileInfo.size() / 1024.0 / 1024.0, 0, 'f', 2);
    resultStr += QString("MIME类型: %1\n").arg(mimeType);
    resultStr += QString("创建时间: %1\n").arg(fileInfo.birthTime().toString("yyyy-MM-dd hh:mm:ss"));
    resultStr += QString("修改时间: %1\n").arg(fileInfo.lastModified().toString("yyyy-MM-dd hh:mm:ss"));
    resultStr += QString("\n正在分析视频格式...\n");

    // 使用FFmpeg分析视频文件
    FFmpeg ffmpeg;
    if (ffmpeg.openFile(filePath.toStdString())) {
        AVFormatContext* formatContext = ffmpeg.getFormatContext();
        if (formatContext) {
            resultStr += QString("\n视频格式信息:\n");
            resultStr += QString("格式: %1\n").arg(formatContext->iformat->long_name);
            resultStr += QString("时长: %1 秒\n").arg(formatContext->duration / 1000000.0, 0, 'f', 2);
            resultStr += QString("比特率: %1 Kbps\n").arg(formatContext->bit_rate / 1000);
            resultStr += QString("流数量: %1\n").arg(formatContext->nb_streams);

            // 分析每个流
            for (unsigned int i = 0; i < formatContext->nb_streams; i++) {
                AVStream* stream = formatContext->streams[i];
                AVCodecParameters* codecParams = stream->codecpar;
                const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);

                resultStr += QString("\n流 #%1\n").arg(i);
                switch (codecParams->codec_type) {
                    case AVMEDIA_TYPE_VIDEO:
                        resultStr += QString("类型: 视频\n");
                        if (codec) {
                            resultStr += QString("编码器: %1\n").arg(codec->long_name);
                        }
                        resultStr += QString("分辨率: %1x%2\n").arg(codecParams->width).arg(codecParams->height);
                        if (stream->avg_frame_rate.num && stream->avg_frame_rate.den) {
                            resultStr += QString("帧率: %1 fps\n")
                                .arg(av_q2d(stream->avg_frame_rate), 0, 'f', 2);
                        }
                        break;

                    case AVMEDIA_TYPE_AUDIO:
                        resultStr += QString("类型: 音频\n");
                        if (codec) {
                            resultStr += QString("编码器: %1\n").arg(codec->long_name);
                        }
                        resultStr += QString("采样率: %1 Hz\n").arg(codecParams->sample_rate);
                        resultStr += QString("声道数: %1\n").arg(codecParams->ch_layout.nb_channels);
                        break;

                    case AVMEDIA_TYPE_SUBTITLE:
                        resultStr += QString("类型: 字幕\n");
                        if (codec) {
                            resultStr += QString("编码器: %1\n").arg(codec->long_name);
                        }
                        break;

                    default:
                        resultStr += QString("类型: 其他\n");
                        break;
                }
            }
        }
        ffmpeg.closeFile();
    } else {
        resultStr += QString("\n无法打开文件进行分析，请确保文件格式正确且未被损坏。");
    }

    resultText_->setText(resultStr);
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