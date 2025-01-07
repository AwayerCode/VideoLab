#include "h264_config_window.hpp"
#include "h264_matrix_view.hpp"
#include <QMessageBox>

H264ConfigWindow::H264ConfigWindow(QWidget* parent)
    : QWidget(parent)
    , mainLayout_(new QVBoxLayout(this))
    , fileSelectLayout_(new QHBoxLayout())
    , contentLayout_(new QHBoxLayout())
    , filePathEdit_(new QLineEdit(this))
    , selectFileBtn_(new QPushButton("Select File", this))
    , analyzeBtn_(new QPushButton("Analyze H264", this))
    , leftPanel_(new QWidget(this))
    , leftLayout_(new QVBoxLayout(leftPanel_))
    , resultDisplay_(new QTextEdit(this))
    , nalTable_(new QTableWidget(this))
    , matrixScrollArea_(new QScrollArea(this))
    , matrixView_(new H264MatrixView(this))
{
    setupUI();
    setupConnections();
}

void H264ConfigWindow::setupUI()
{
    // 设置文件选择区域
    fileSelectLayout_->addWidget(new QLabel("H264 File:", this));
    fileSelectLayout_->addWidget(filePathEdit_);
    fileSelectLayout_->addWidget(selectFileBtn_);
    
    // 创建水平分割布局
    leftPanel_->setMaximumWidth(400);
    
    // 设置结果显示区域
    resultDisplay_->setReadOnly(true);
    resultDisplay_->setPlaceholderText("H264 file analysis results will be displayed here...");
    
    // 设置NAL单元表格
    nalTable_->setColumnCount(5);
    nalTable_->setHorizontalHeaderLabels({
        "Type",
        "Size",
        "Offset",
        "Timestamp",
        "Keyframe"
    });
    nalTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    
    // 添加到左侧布局
    leftLayout_->addWidget(resultDisplay_);
    leftLayout_->addWidget(nalTable_);
    
    // 设置矩阵视图区域
    matrixScrollArea_->setWidget(matrixView_);
    matrixScrollArea_->setWidgetResizable(true);
    matrixScrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    matrixScrollArea_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    // 添加到内容布局
    contentLayout_->addWidget(leftPanel_);
    contentLayout_->addWidget(matrixScrollArea_);
    
    // 设置主布局
    mainLayout_->addLayout(fileSelectLayout_);
    mainLayout_->addWidget(analyzeBtn_);
    mainLayout_->addLayout(contentLayout_);
    
    // 设置按钮属性
    analyzeBtn_->setEnabled(false);
}

void H264ConfigWindow::setupConnections()
{
    connect(selectFileBtn_, &QPushButton::clicked, this, &H264ConfigWindow::onSelectFile);
    connect(analyzeBtn_, &QPushButton::clicked, this, &H264ConfigWindow::onAnalyzeH264);
    connect(filePathEdit_, &QLineEdit::textChanged, [this](const QString& text) {
        analyzeBtn_->setEnabled(!text.isEmpty());
    });
}

void H264ConfigWindow::onSelectFile()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Select H264 File",
        QString(),
        "H264 Files (*.h264 *.264);;All Files (*.*)"
    );
    
    if (!filePath.isEmpty()) {
        filePathEdit_->setText(filePath);
    }
}

void H264ConfigWindow::onAnalyzeH264()
{
    QString filePath = filePathEdit_->text();
    if (filePath.isEmpty()) {
        return;
    }

    resultDisplay_->clear();
    nalTable_->setRowCount(0);
    matrixView_->clear();
    
    resultDisplay_->append("Starting H264 file analysis...\n");
    
    try {
        if (!parser_.open(filePath.toStdString())) {
            resultDisplay_->append("Failed to open H264 file!");
            return;
        }

        // 获取视频信息
        auto videoInfo = parser_.getVideoInfo();
        resultDisplay_->append(QString("Video Information:\n"
                                     "Resolution: %1x%2\n"
                                     "Duration: %3 sec\n"
                                     "Bitrate: %4 kbps\n"
                                     "FPS: %5\n"
                                     "Total Frames: %6\n"
                                     "Keyframes: %7\n"
                                     "Profile: %8\n"
                                     "Level: %9")
                                     .arg(videoInfo.width)
                                     .arg(videoInfo.height)
                                     .arg(videoInfo.duration, 0, 'f', 2)
                                     .arg(videoInfo.bitrate / 1000.0, 0, 'f', 2)
                                     .arg(videoInfo.fps, 0, 'f', 2)
                                     .arg(videoInfo.total_frames)
                                     .arg(videoInfo.keyframe_count)
                                     .arg(QString::fromStdString(videoInfo.profile))
                                     .arg(videoInfo.level));

        // 获取NAL单元信息
        auto nalUnits = parser_.getNALUnits();
        displayNALUnits(nalUnits);
        updateMatrixView(nalUnits);

        // 获取SPS信息
        auto spsInfo = parser_.getSPSInfo();
        resultDisplay_->append(QString("\nSPS Information:\n"
                                     "Profile IDC: %1\n"
                                     "Level IDC: %2\n"
                                     "Max Reference Frames: %3")
                                     .arg(spsInfo.profile_idc)
                                     .arg(spsInfo.level_idc)
                                     .arg(spsInfo.max_num_ref_frames));

        // 获取PPS信息
        auto ppsInfo = parser_.getPPSInfo();
        resultDisplay_->append(QString("\nPPS Information:\n"
                                     "Entropy Coding Mode: %1\n"
                                     "Weighted Prediction: %2")
                                     .arg(ppsInfo.entropy_coding_mode_flag ? "CABAC" : "CAVLC")
                                     .arg(ppsInfo.weighted_pred_flag ? "Yes" : "No"));

        parser_.close();
    } catch (const std::exception& e) {
        resultDisplay_->append("Error during analysis: " + QString(e.what()));
    }
}

void H264ConfigWindow::displayNALUnits(const std::vector<H264Parser::NALUnitInfo>& nalUnits)
{
    nalTable_->setRowCount(nalUnits.size());
    
    int row = 0;
    for (const auto& nal : nalUnits) {
        nalTable_->setItem(row, 0, new QTableWidgetItem(getNALUnitTypeString(nal.type)));
        nalTable_->setItem(row, 1, new QTableWidgetItem(QString::number(nal.size)));
        nalTable_->setItem(row, 2, new QTableWidgetItem(QString("0x%1").arg(nal.offset, 0, 16)));
        nalTable_->setItem(row, 3, new QTableWidgetItem(QString::number(nal.timestamp, 'f', 3)));
        nalTable_->setItem(row, 4, new QTableWidgetItem(nal.is_keyframe ? "Yes" : "No"));
        row++;
    }
}

void H264ConfigWindow::updateMatrixView(const std::vector<H264Parser::NALUnitInfo>& nalUnits)
{
    matrixView_->setNALUnits(nalUnits);
}

QString H264ConfigWindow::getNALUnitTypeString(H264Parser::NALUnitType type)
{
    switch (type) {
        case H264Parser::NALUnitType::CODED_SLICE_NON_IDR:
            return "Non-IDR Slice";
        case H264Parser::NALUnitType::CODED_SLICE_IDR:
            return "IDR Slice";
        case H264Parser::NALUnitType::SPS:
            return "SPS";
        case H264Parser::NALUnitType::PPS:
            return "PPS";
        case H264Parser::NALUnitType::SEI:
            return "SEI";
        default:
            return QString("Type %1").arg(static_cast<int>(type));
    }
} 