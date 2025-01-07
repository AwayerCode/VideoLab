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
#include "../format/aac_parser.hpp"

// 前向声明
class AACFrameView;

class AACConfigWindow : public QWidget {
    Q_OBJECT

public:
    explicit AACConfigWindow(QWidget* parent = nullptr);
    ~AACConfigWindow() override = default;

private slots:
    void onSelectFile();
    void onAnalyzeAAC();

private:
    void setupUI();
    void setupConnections();
    void displayFrames(const std::vector<AACParser::FrameInfo>& frames);
    void updateFrameView(const std::vector<AACParser::FrameInfo>& frames);
    void displayAudioInfo(const AACParser::AudioInfo& info);

    // 布局
    QVBoxLayout* mainLayout_{nullptr};
    QHBoxLayout* fileSelectLayout_{nullptr};
    QHBoxLayout* buttonLayout_{nullptr};
    QHBoxLayout* contentLayout_{nullptr};

    // 文件选择控件
    QLineEdit* filePathEdit_{nullptr};
    QPushButton* selectFileBtn_{nullptr};
    QPushButton* analyzeBtn_{nullptr};

    // 显示区域
    QWidget* leftPanel_{nullptr};
    QVBoxLayout* leftLayout_{nullptr};
    QTextEdit* resultDisplay_{nullptr};

    // 帧视图
    QScrollArea* frameScrollArea_{nullptr};
    AACFrameView* frameView_{nullptr};

    // 解析器
    AACParser parser_;
}; 