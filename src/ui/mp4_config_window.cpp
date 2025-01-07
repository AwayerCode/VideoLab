#include "mp4_config_window.hpp"
#include "../mp4/mp4_parser.hpp"

MP4ConfigWindow::MP4ConfigWindow(QWidget* parent)
    : QWidget(parent)
    , mainLayout_(new QVBoxLayout(this))
    , fileSelectLayout_(new QHBoxLayout())
    , filePathEdit_(new QLineEdit(this))
    , selectFileBtn_(new QPushButton("选择文件", this))
    , analyzeBtn_(new QPushButton("解析MP4", this))
    , resultDisplay_(new QTextEdit(this))
{
    setupUI();
    setupConnections();
}

MP4ConfigWindow::~MP4ConfigWindow() = default;

void MP4ConfigWindow::setupUI()
{
    // 设置文件选择区域
    fileSelectLayout_->addWidget(new QLabel("MP4文件：", this));
    fileSelectLayout_->addWidget(filePathEdit_);
    fileSelectLayout_->addWidget(selectFileBtn_);
    
    // 设置主布局
    mainLayout_->addLayout(fileSelectLayout_);
    mainLayout_->addWidget(analyzeBtn_);
    mainLayout_->addWidget(resultDisplay_);
    
    // 设置结果显示区域属性
    resultDisplay_->setReadOnly(true);
    resultDisplay_->setPlaceholderText("MP4文件解析结果将在这里显示...");
    
    // 设置按钮状态
    analyzeBtn_->setEnabled(false);
}

void MP4ConfigWindow::setupConnections()
{
    connect(selectFileBtn_, &QPushButton::clicked, this, &MP4ConfigWindow::onSelectFile);
    connect(analyzeBtn_, &QPushButton::clicked, this, &MP4ConfigWindow::onAnalyzeMP4);
    connect(filePathEdit_, &QLineEdit::textChanged, [this](const QString& text) {
        analyzeBtn_->setEnabled(!text.isEmpty());
    });
}

void MP4ConfigWindow::onSelectFile()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "选择MP4文件",
        QString(),
        "MP4文件 (*.mp4);;所有文件 (*.*)"
    );
    
    if (!filePath.isEmpty()) {
        filePathEdit_->setText(filePath);
    }
}

void MP4ConfigWindow::onAnalyzeMP4()
{
    QString filePath = filePathEdit_->text();
    if (filePath.isEmpty()) {
        return;
    }

    resultDisplay_->clear();
    resultDisplay_->append("开始解析MP4文件...\n");
    
    try {
        MP4Parser parser;
        if (!parser.open(filePath.toStdString())) {
            resultDisplay_->append("无法打开MP4文件！");
            return;
        }

        // 获取视频信息
        auto videoInfo = parser.getVideoInfo();
        resultDisplay_->append(QString("视频信息：\n"
                                     "分辨率: %1x%2\n"
                                     "时长: %.2f秒\n"
                                     "比特率: %.2f kbps\n"
                                     "帧率: %.2f fps\n"
                                     "总帧数: %3\n"
                                     "关键帧数: %4\n"
                                     "编码器: %5\n"
                                     "格式: %6\n")
                                     .arg(videoInfo.width)
                                     .arg(videoInfo.height)
                                     .arg(videoInfo.duration)
                                     .arg(videoInfo.bitrate / 1000.0)
                                     .arg(videoInfo.fps)
                                     .arg(videoInfo.total_frames)
                                     .arg(videoInfo.keyframe_count)
                                     .arg(QString::fromStdString(videoInfo.codec_name))
                                     .arg(QString::fromStdString(videoInfo.format_name)));

        // 获取音频信息
        auto audioInfo = parser.getAudioInfo();
        if (audioInfo.channels > 0) {  // 如果有音频流
            resultDisplay_->append(QString("\n音频信息：\n"
                                         "声道数: %1\n"
                                         "采样率: %2 Hz\n"
                                         "时长: %.2f秒\n"
                                         "比特率: %.2f kbps\n"
                                         "编码器: %3\n")
                                         .arg(audioInfo.channels)
                                         .arg(audioInfo.sample_rate)
                                         .arg(audioInfo.duration)
                                         .arg(audioInfo.bitrate / 1000.0)
                                         .arg(QString::fromStdString(audioInfo.codec_name)));
        }

        // 获取元数据
        auto metadata = parser.getMetadata();
        if (!metadata.empty()) {
            resultDisplay_->append("\n元数据：");
            for (const auto& [key, value] : metadata) {
                resultDisplay_->append(QString("%1: %2")
                    .arg(QString::fromStdString(key))
                    .arg(QString::fromStdString(value)));
            }
        }

        // 获取关键帧信息
        auto keyFrames = parser.getKeyFrameInfo();
        if (!keyFrames.empty()) {
            resultDisplay_->append(QString("\n关键帧分布（前5个）："));
            int count = 0;
            for (const auto& kf : keyFrames) {
                if (count >= 5) break;
                resultDisplay_->append(QString("时间戳: %.2f秒")
                    .arg(kf.timestamp));
                count++;
            }
        }

        parser.close();
    } catch (const std::exception& e) {
        resultDisplay_->append("解析出错: " + QString(e.what()));
    }
} 