#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QScrollArea>
#include <QFileDialog>
#include <QTableWidget>
#include <QHeaderView>
#include "../format/h264_parser.hpp"

// 前向声明
class H264MatrixView;

class H264ConfigWindow : public QWidget {
    Q_OBJECT

public:
    explicit H264ConfigWindow(QWidget* parent = nullptr);
    ~H264ConfigWindow() override = default;

private slots:
    void onSelectFile();
    void onAnalyzeH264();

private:
    void setupUI();
    void setupConnections();
    void displayNALUnits(const std::vector<H264Parser::NALUnitInfo>& nalUnits);
    void updateMatrixView(const std::vector<H264Parser::NALUnitInfo>& nalUnits);
    QString getNALUnitTypeString(H264Parser::NALUnitType type);

    // 布局
    QVBoxLayout* mainLayout_{nullptr};
    QHBoxLayout* fileSelectLayout_{nullptr};
    QHBoxLayout* contentLayout_{nullptr};

    // 文件选择控件
    QLineEdit* filePathEdit_{nullptr};
    QPushButton* selectFileBtn_{nullptr};
    QPushButton* analyzeBtn_{nullptr};

    // 显示区域
    QWidget* leftPanel_{nullptr};
    QVBoxLayout* leftLayout_{nullptr};
    QTextEdit* resultDisplay_{nullptr};
    QTableWidget* nalTable_{nullptr};

    // NAL单元矩阵视图
    QScrollArea* matrixScrollArea_{nullptr};
    H264MatrixView* matrixView_{nullptr};

    // 解析器
    H264Parser parser_;
}; 