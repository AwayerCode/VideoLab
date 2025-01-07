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
#include "../format/mp4_parser.hpp"

// 前向声明
class MP4BoxView;

class MP4ConfigWindow : public QWidget {
    Q_OBJECT

public:
    explicit MP4ConfigWindow(QWidget* parent = nullptr);
    ~MP4ConfigWindow() override = default;

private slots:
    void onSelectFile();
    void onAnalyzeMP4();
    void onDemuxMP4();  // 新增：解封装功能

private:
    void setupUI();
    void setupConnections();
    void displayBoxes(const std::vector<MP4Parser::BoxInfo>& boxes);
    void updateBoxView(const std::vector<MP4Parser::BoxInfo>& boxes);
    void ensureDataDirectory();  // 新增：确保数据目录存在

    // 布局
    QVBoxLayout* mainLayout_{nullptr};
    QHBoxLayout* fileSelectLayout_{nullptr};
    QHBoxLayout* buttonLayout_{nullptr};  // 新增：按钮布局
    QHBoxLayout* contentLayout_{nullptr};

    // 文件选择控件
    QLineEdit* filePathEdit_{nullptr};
    QPushButton* selectFileBtn_{nullptr};
    QPushButton* analyzeBtn_{nullptr};
    QPushButton* demuxBtn_{nullptr};  // 新增：解封装按钮

    // 显示区域
    QWidget* leftPanel_{nullptr};
    QVBoxLayout* leftLayout_{nullptr};
    QTextEdit* resultDisplay_{nullptr};

    // Box视图
    QScrollArea* boxScrollArea_{nullptr};
    MP4BoxView* boxView_{nullptr};

    // 解析器
    MP4Parser parser_;
}; 