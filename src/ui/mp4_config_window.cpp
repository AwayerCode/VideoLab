#include "mp4_config_window.hpp"
#include "mp4_box_view.hpp"
#include <QMessageBox>
#include <QDir>

MP4ConfigWindow::MP4ConfigWindow(QWidget* parent)
    : QWidget(parent)
    , mainLayout_(new QVBoxLayout(this))
    , fileSelectLayout_(new QHBoxLayout())
    , buttonLayout_(new QHBoxLayout())
    , contentLayout_(new QHBoxLayout())
    , filePathEdit_(new QLineEdit(this))
    , selectFileBtn_(new QPushButton("Select File", this))
    , analyzeBtn_(new QPushButton("Analyze MP4", this))
    , demuxBtn_(new QPushButton("Extract H264/AAC", this))
    , leftPanel_(new QWidget(this))
    , leftLayout_(new QVBoxLayout(leftPanel_))
    , resultDisplay_(new QTextEdit(this))
    , boxScrollArea_(new QScrollArea(this))
    , boxView_(new MP4BoxView(this))
{
    setupUI();
    setupConnections();
}

void MP4ConfigWindow::setupUI()
{
    // 设置文件选择区域
    fileSelectLayout_->addWidget(new QLabel("MP4 File:", this));
    fileSelectLayout_->addWidget(filePathEdit_);
    fileSelectLayout_->addWidget(selectFileBtn_);
    
    // 设置按钮区域
    buttonLayout_->addWidget(analyzeBtn_);
    buttonLayout_->addWidget(demuxBtn_);
    
    // 创建水平分割布局
    leftPanel_->setMaximumWidth(400);
    
    // 设置结果显示区域
    resultDisplay_->setReadOnly(true);
    resultDisplay_->setPlaceholderText("MP4 file analysis results will be displayed here...");
    
    // 添加到左侧布局
    leftLayout_->addWidget(resultDisplay_);
    
    // 设置Box视图区域
    boxScrollArea_->setWidget(boxView_);
    boxScrollArea_->setWidgetResizable(true);
    boxScrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    boxScrollArea_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    // 添加到内容布局
    contentLayout_->addWidget(leftPanel_);
    contentLayout_->addWidget(boxScrollArea_);
    
    // 设置主布局
    mainLayout_->addLayout(fileSelectLayout_);
    mainLayout_->addLayout(buttonLayout_);
    mainLayout_->addLayout(contentLayout_);
    
    // 设置按钮属性
    analyzeBtn_->setEnabled(false);
    demuxBtn_->setEnabled(false);
}

void MP4ConfigWindow::setupConnections()
{
    connect(selectFileBtn_, &QPushButton::clicked, this, &MP4ConfigWindow::onSelectFile);
    connect(analyzeBtn_, &QPushButton::clicked, this, &MP4ConfigWindow::onAnalyzeMP4);
    connect(demuxBtn_, &QPushButton::clicked, this, &MP4ConfigWindow::onDemuxMP4);
    connect(filePathEdit_, &QLineEdit::textChanged, [this](const QString& text) {
        bool hasFile = !text.isEmpty();
        analyzeBtn_->setEnabled(hasFile);
        demuxBtn_->setEnabled(hasFile);
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
    boxView_->clear();
    
    resultDisplay_->append("Starting MP4 file analysis...\n");
    
    try {
        if (!parser_.open(filePath.toStdString())) {
            resultDisplay_->append("Failed to open MP4 file!");
            return;
        }

        auto boxes = parser_.getBoxes();
        displayBoxes(boxes);
        updateBoxView(boxes);

        parser_.close();
    } catch (const std::exception& e) {
        resultDisplay_->append("Error during analysis: " + QString(e.what()));
    }
}

void MP4ConfigWindow::onDemuxMP4()
{
    QString filePath = filePathEdit_->text();
    if (filePath.isEmpty()) {
        return;
    }

    ensureDataDirectory();
    
    resultDisplay_->append("\nStarting MP4 demuxing...");
    
    try {
        if (!parser_.open(filePath.toStdString())) {
            resultDisplay_->append("Failed to open MP4 file!");
            return;
        }

        // 提取H264和AAC数据
        QString baseFileName = QFileInfo(filePath).baseName();
        QString h264Path = QString("datas/%1.h264").arg(baseFileName);
        QString aacPath = QString("datas/%1.aac").arg(baseFileName);

        bool h264Success = parser_.extractH264(h264Path.toStdString());
        bool aacSuccess = parser_.extractAAC(aacPath.toStdString());

        if (h264Success) {
            resultDisplay_->append(QString("H264 stream extracted to: %1").arg(h264Path));
        } else {
            resultDisplay_->append("No H264 stream found or extraction failed.");
        }

        if (aacSuccess) {
            resultDisplay_->append(QString("AAC stream extracted to: %1").arg(aacPath));
        } else {
            resultDisplay_->append("No AAC stream found or extraction failed.");
        }

        parser_.close();
        
        if (h264Success || aacSuccess) {
            QMessageBox::information(this, "Extraction Complete", 
                "Media streams have been successfully extracted to the 'datas' directory.");
        } else {
            QMessageBox::warning(this, "Extraction Failed", 
                "No supported media streams were found in the MP4 file.");
        }
    } catch (const std::exception& e) {
        resultDisplay_->append("Error during demuxing: " + QString(e.what()));
        QMessageBox::critical(this, "Error", 
            QString("Failed to extract media streams: %1").arg(e.what()));
    }
}

void MP4ConfigWindow::ensureDataDirectory()
{
    QDir dir;
    if (!dir.exists("datas")) {
        dir.mkdir("datas");
    }
}

void MP4ConfigWindow::displayBoxes(const std::vector<MP4Parser::BoxInfo>& boxes)
{
    resultDisplay_->clear();
    resultDisplay_->append("MP4 Box Structure:\n");
    
    for (const auto& box : boxes) {
        QString indent(box.level * 2, ' ');
        resultDisplay_->append(QString("%1%2 (size: %3 bytes)")
            .arg(indent)
            .arg(QString::fromStdString(box.type))
            .arg(box.size));
    }
}

void MP4ConfigWindow::updateBoxView(const std::vector<MP4Parser::BoxInfo>& boxes)
{
    // 转换 BoxInfo 类型
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
} 