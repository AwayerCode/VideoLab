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
    , boxScrollArea_(new QScrollArea(this))
    , boxView_(new MP4BoxView(this))
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
    
    // 创建水平分割布局
    auto* horizontalLayout = new QHBoxLayout();
    
    // 左侧文本显示区域
    auto* leftWidget = new QWidget(this);
    auto* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->addWidget(resultDisplay_);
    leftWidget->setMaximumWidth(400);  // 限制左侧宽度
    
    // 右侧box视图区域
    boxScrollArea_->setWidget(boxView_);
    boxScrollArea_->setWidgetResizable(true);
    boxScrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    boxScrollArea_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    // 添加到水平布局
    horizontalLayout->addWidget(leftWidget);
    horizontalLayout->addWidget(boxScrollArea_);
    
    // 设置主布局
    mainLayout_->addLayout(fileSelectLayout_);
    mainLayout_->addWidget(analyzeBtn_);
    mainLayout_->addLayout(horizontalLayout);
    
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
    boxView_->clear();
    
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
                                     "时长: %3 秒\n"
                                     "比特率: %4 kbps\n"
                                     "帧率: %5 fps\n"
                                     "总帧数: %6\n"
                                     "关键帧数: %7\n"
                                     "编码器: %8\n"
                                     "格式: %9")
                                     .arg(videoInfo.width)
                                     .arg(videoInfo.height)
                                     .arg(videoInfo.duration, 0, 'f', 2)
                                     .arg(videoInfo.bitrate / 1000.0, 0, 'f', 2)
                                     .arg(videoInfo.fps, 0, 'f', 2)
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
                                         "时长: %3 秒\n"
                                         "比特率: %4 kbps\n"
                                         "编码器: %5")
                                         .arg(audioInfo.channels)
                                         .arg(audioInfo.sample_rate)
                                         .arg(audioInfo.duration, 0, 'f', 2)
                                         .arg(audioInfo.bitrate / 1000.0, 0, 'f', 2)
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
                resultDisplay_->append(QString("时间戳: %1 秒 (位置: %2)")
                    .arg(kf.timestamp, 0, 'f', 2)
                    .arg(kf.pos));
                count++;
            }
        }

        // 获取并显示box信息
        auto boxes = parser.getBoxes();
        
        // 转换BoxInfo类型
        std::vector<MP4BoxView::BoxInfo> viewBoxes;
        viewBoxes.reserve(boxes.size());
        for (const auto& box : boxes) {
            MP4BoxView::BoxInfo viewBox;
            viewBox.type = box.type;
            viewBox.size = box.size;
            viewBox.offset = box.offset;
            viewBox.level = box.level;
            viewBoxes.push_back(viewBox);
        }
        
        boxView_->setBoxes(viewBoxes);

        parser.close();
    } catch (const std::exception& e) {
        resultDisplay_->append("解析出错: " + QString(e.what()));
    }
} 