#include "mp4_config_window.hpp"
#include "../mp4/mp4_parser.hpp"

MP4ConfigWindow::MP4ConfigWindow(QWidget* parent)
    : QWidget(parent)
    , mainLayout_(new QVBoxLayout(this))
    , fileSelectLayout_(new QHBoxLayout())
    , filePathEdit_(new QLineEdit(this))
    , selectFileBtn_(new QPushButton("Select File", this))
    , analyzeBtn_(new QPushButton("Analyze MP4", this))
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
    // Set file selection area
    fileSelectLayout_->addWidget(new QLabel("MP4 File:", this));
    fileSelectLayout_->addWidget(filePathEdit_);
    fileSelectLayout_->addWidget(selectFileBtn_);
    
    // Create horizontal split layout
    auto* horizontalLayout = new QHBoxLayout();
    
    // Left text display area
    auto* leftWidget = new QWidget(this);
    auto* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->addWidget(resultDisplay_);
    leftWidget->setMaximumWidth(400);  // Limit left side width
    
    // Right box view area
    boxScrollArea_->setWidget(boxView_);
    boxScrollArea_->setWidgetResizable(true);
    boxScrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    boxScrollArea_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    // Add to horizontal layout
    horizontalLayout->addWidget(leftWidget);
    horizontalLayout->addWidget(boxScrollArea_);
    
    // Set main layout
    mainLayout_->addLayout(fileSelectLayout_);
    mainLayout_->addWidget(analyzeBtn_);
    mainLayout_->addLayout(horizontalLayout);
    
    // Set result display properties
    resultDisplay_->setReadOnly(true);
    resultDisplay_->setPlaceholderText("MP4 file analysis results will be displayed here...");
    
    // Set button properties
    selectFileBtn_->setText("Select File");
    analyzeBtn_->setText("Analyze MP4");
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
        "Select MP4 File",
        QString(),
        "MP4 Files (*.mp4);;All Files (*.*)"
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
    resultDisplay_->append("Starting MP4 file analysis...\n");
    boxView_->clear();
    
    try {
        MP4Parser parser;
        if (!parser.open(filePath.toStdString())) {
            resultDisplay_->append("Failed to open MP4 file!");
            return;
        }

        // Get video information
        auto videoInfo = parser.getVideoInfo();
        resultDisplay_->append(QString("Video Information:\n"
                                     "Resolution: %1x%2\n"
                                     "Duration: %3 sec\n"
                                     "Bitrate: %4 kbps\n"
                                     "Frame Rate: %5 fps\n"
                                     "Total Frames: %6\n"
                                     "Keyframes: %7\n"
                                     "Codec: %8\n"
                                     "Format: %9")
                                     .arg(videoInfo.width)
                                     .arg(videoInfo.height)
                                     .arg(videoInfo.duration, 0, 'f', 2)
                                     .arg(videoInfo.bitrate / 1000.0, 0, 'f', 2)
                                     .arg(videoInfo.fps, 0, 'f', 2)
                                     .arg(videoInfo.total_frames)
                                     .arg(videoInfo.keyframe_count)
                                     .arg(QString::fromStdString(videoInfo.codec_name))
                                     .arg(QString::fromStdString(videoInfo.format_name)));

        // Get audio information
        auto audioInfo = parser.getAudioInfo();
        if (audioInfo.channels > 0) {  // If has audio stream
            resultDisplay_->append(QString("\nAudio Information:\n"
                                         "Channels: %1\n"
                                         "Sample Rate: %2 Hz\n"
                                         "Duration: %3 sec\n"
                                         "Bitrate: %4 kbps\n"
                                         "Codec: %5")
                                         .arg(audioInfo.channels)
                                         .arg(audioInfo.sample_rate)
                                         .arg(audioInfo.duration, 0, 'f', 2)
                                         .arg(audioInfo.bitrate / 1000.0, 0, 'f', 2)
                                         .arg(QString::fromStdString(audioInfo.codec_name)));
        }

        // Get metadata
        auto metadata = parser.getMetadata();
        if (!metadata.empty()) {
            resultDisplay_->append("\nMetadata:");
            for (const auto& [key, value] : metadata) {
                resultDisplay_->append(QString("%1: %2")
                    .arg(QString::fromStdString(key))
                    .arg(QString::fromStdString(value)));
            }
        }

        // Get keyframe information
        auto keyFrames = parser.getKeyFrameInfo();
        if (!keyFrames.empty()) {
            resultDisplay_->append(QString("\nKeyframe Distribution (First 5):"));
            int count = 0;
            for (const auto& kf : keyFrames) {
                if (count >= 5) break;
                resultDisplay_->append(QString("Timestamp: %1 sec (Position: %2)")
                    .arg(kf.timestamp, 0, 'f', 2)
                    .arg(kf.pos));
                count++;
            }
        }

        // Get and display box information
        auto boxes = parser.getBoxes();
        
        // Add box list text output
        if (!boxes.empty()) {
            resultDisplay_->append("\nMP4 Box List:");
            for (const auto& box : boxes) {
                QString indent = QString("  ").repeated(box.level);
                resultDisplay_->append(QString("%1%2 (Size: %3 bytes, Offset: 0x%4)")
                    .arg(indent)
                    .arg(QString::fromStdString(box.type))
                    .arg(box.size)
                    .arg(QString::number(box.offset, 16).toUpper()));
            }
        }
        
        // Convert BoxInfo type
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
        resultDisplay_->append("Error during analysis: " + QString(e.what()));
    }
} 