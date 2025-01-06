#pragma once

#include <QMainWindow>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QProgressBar>
#include "../ffmpeg/x264_param_test.hpp"

class X264ConfigWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit X264ConfigWindow(QWidget *parent = nullptr);
    ~X264ConfigWindow();

private slots:
    void onStartEncoding();
    void onRateControlChanged(int index);
    void onPresetConfigSelected(int index);

private:
    // 基本参数控件
    QComboBox* presetCombo_;
    QComboBox* tuneCombo_;
    QSpinBox* threadsSpinBox_;
    
    // 分辨率和帧数
    QSpinBox* widthSpinBox_;
    QSpinBox* heightSpinBox_;
    QSpinBox* frameCountSpinBox_;
    
    // 码率控制
    QComboBox* rateControlCombo_;
    QSpinBox* rateValueSpinBox_;
    
    // 帧率控制
    QSpinBox* fpsSpinBox_;
    QCheckBox* vfrCheckBox_;
    
    // GOP参数
    QSpinBox* keyintMaxSpinBox_;
    QSpinBox* bframesSpinBox_;
    QSpinBox* refsSpinBox_;
    
    // 质量参数
    QCheckBox* fastFirstPassCheckBox_;
    QSpinBox* meRangeSpinBox_;
    QCheckBox* weightedPredCheckBox_;
    QCheckBox* cabacCheckBox_;
    
    // 预设场景配置
    QComboBox* sceneConfigCombo_;
    
    // 控制按钮
    QPushButton* startButton_;
    
    // 状态显示
    QProgressBar* progressBar_;
    QTextEdit* logTextEdit_;

    void setupUI();
    void createConnections();
    void updateUIFromConfig(const X264ParamTest::TestConfig& config);
    X264ParamTest::TestConfig getConfigFromUI() const;
}; 