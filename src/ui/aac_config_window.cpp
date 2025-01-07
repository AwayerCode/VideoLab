#include "aac_config_window.hpp"
#include "aac_frame_view.hpp"
#include <QMessageBox>

AACConfigWindow::AACConfigWindow(QWidget* parent)
    : QWidget(parent)
    , mainLayout_(new QVBoxLayout(this))
    , fileSelectLayout_(new QHBoxLayout())
    , buttonLayout_(new QHBoxLayout())
    , contentLayout_(new QHBoxLayout())
    , filePathEdit_(new QLineEdit(this))
    , selectFileBtn_(new QPushButton("Select File", this))
    , analyzeBtn_(new QPushButton("Analyze AAC", this))
    , leftPanel_(new QWidget(this))
    , leftLayout_(new QVBoxLayout(leftPanel_))
    , resultDisplay_(new QTextEdit(this))
    , frameScrollArea_(new QScrollArea(this))
    , frameView_(new AACFrameView(this))
{
    setupUI();
    setupConnections();
}

void AACConfigWindow::setupUI()
{
    // 设置文件选择区域
    fileSelectLayout_->addWidget(new QLabel("AAC File:", this));
    fileSelectLayout_->addWidget(filePathEdit_);
    fileSelectLayout_->addWidget(selectFileBtn_);
    
    // 设置按钮区域
    buttonLayout_->addWidget(analyzeBtn_);
    
    // 创建水平分割布局
    leftPanel_->setMaximumWidth(400);
    
    // 设置结果显示区域
    resultDisplay_->setReadOnly(true);
    resultDisplay_->setPlaceholderText("AAC file analysis results will be displayed here...");
    
    // 添加到左侧布局
    leftLayout_->addWidget(resultDisplay_);
    
    // 设置帧视图区域
    frameScrollArea_->setWidget(frameView_);
    frameScrollArea_->setWidgetResizable(true);
    frameScrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    frameScrollArea_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    // 添加到内容布局
    contentLayout_->addWidget(leftPanel_);
    contentLayout_->addWidget(frameScrollArea_);
    
    // 设置主布局
    mainLayout_->addLayout(fileSelectLayout_);
    mainLayout_->addLayout(buttonLayout_);
    mainLayout_->addLayout(contentLayout_);
    
    // 设置按钮属性
    analyzeBtn_->setEnabled(false);
}

void AACConfigWindow::setupConnections()
{
    connect(selectFileBtn_, &QPushButton::clicked, this, &AACConfigWindow::onSelectFile);
    connect(analyzeBtn_, &QPushButton::clicked, this, &AACConfigWindow::onAnalyzeAAC);
    connect(filePathEdit_, &QLineEdit::textChanged, [this](const QString& text) {
        analyzeBtn_->setEnabled(!text.isEmpty());
    });
}

void AACConfigWindow::onSelectFile()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Select AAC File",
        QString(),
        "AAC Files (*.aac);;All Files (*.*)"
    );
    
    if (!filePath.isEmpty()) {
        filePathEdit_->setText(filePath);
    }
}

void AACConfigWindow::onAnalyzeAAC()
{
    QString filePath = filePathEdit_->text();
    if (filePath.isEmpty()) {
        return;
    }

    resultDisplay_->clear();
    frameView_->clear();
    
    resultDisplay_->append("Starting AAC file analysis...\n");
    
    try {
        if (!parser_.open(filePath.toStdString())) {
            resultDisplay_->append("Failed to open AAC file!");
            return;
        }

        // 显示音频信息
        auto audioInfo = parser_.getAudioInfo();
        displayAudioInfo(audioInfo);

        // 获取并显示帧信息
        auto frames = parser_.getFrames();
        displayFrames(frames);
        updateFrameView(frames);

        parser_.close();
    } catch (const std::exception& e) {
        resultDisplay_->append("Error during analysis: " + QString(e.what()));
    }
}

void AACConfigWindow::displayAudioInfo(const AACParser::AudioInfo& info)
{
    resultDisplay_->append(QString("Audio Information:\n"
                                 "Format: %1\n"
                                 "Sample Rate: %2 Hz\n"
                                 "Channels: %3\n"
                                 "Profile: %4\n"
                                 "Duration: %5 sec\n"
                                 "Bitrate: %6 kbps\n"
                                 "Total Frames: %7")
                                 .arg(QString::fromStdString(info.format))
                                 .arg(info.sample_rate)
                                 .arg(info.channels)
                                 .arg(info.profile)
                                 .arg(info.duration, 0, 'f', 2)
                                 .arg(info.bitrate / 1000.0, 0, 'f', 2)
                                 .arg(info.total_frames));
}

void AACConfigWindow::displayFrames(const std::vector<AACParser::FrameInfo>& frames)
{
    resultDisplay_->append(QString("\nFrame Analysis:\n"
                                 "Total Frames: %1\n"
                                 "First 5 frames:")
                                 .arg(frames.size()));

    for (size_t i = 0; i < std::min(size_t(5), frames.size()); ++i) {
        const auto& frame = frames[i];
        resultDisplay_->append(QString("Frame %1:\n"
                                     "  Time: %2s\n"
                                     "  Size: %3 bytes\n"
                                     "  Sample Rate: %4 Hz\n"
                                     "  Channels: %5")
                                     .arg(i)
                                     .arg(frame.timestamp, 0, 'f', 3)
                                     .arg(frame.size)
                                     .arg(frame.sample_rate)
                                     .arg(frame.channels));
    }
}

void AACConfigWindow::updateFrameView(const std::vector<AACParser::FrameInfo>& frames)
{
    frameView_->setFrames(frames);
} 