#pragma once

#include <QMainWindow>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QProgressBar>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QSlider>
#include "../ffmpeg/x264_param_test.hpp"
#include <thread>

class X264ConfigWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit X264ConfigWindow(QWidget *parent = nullptr);
    ~X264ConfigWindow() override;

private slots:
    void onStartEncoding();
    void onStopEncoding();
    void onRateControlChanged(int index);
    void onPresetConfigSelected(int index);
    void onPlayVideo();
    void onMediaPositionChanged(qint64 position);
    void onMediaDurationChanged(qint64 duration);
    void onSliderMoved(int position);
    void updateProgress(int frame, double time, double fps, double bitrate, double psnr, double ssim);
    void appendLog(const QString& text);
    void onEncodingFinished();
    void onGenerateFrames();

private:
    void setupUI();
    void createConnections();
    void updateUIFromConfig(const X264ParamTest::TestConfig& config);
    X264ParamTest::TestConfig getConfigFromUI() const;

    // 基本参数控件
    QComboBox* presetCombo_{};
    QComboBox* tuneCombo_{};
    QSpinBox* threadsSpinBox_{};
    
    // 视频参数控件
    QSpinBox* widthSpinBox_{};
    QSpinBox* heightSpinBox_{};
    QSpinBox* frameCountSpinBox_{};
    
    // 码率控制控件
    QComboBox* rateControlCombo_{};
    QSpinBox* rateValueSpinBox_{};
    
    // GOP参数控件
    QSpinBox* keyintMaxSpinBox_{};
    QSpinBox* bframesSpinBox_{};
    QSpinBox* refsSpinBox_{};
    
    // 质量参数控件
    QCheckBox* fastFirstPassCheckBox_{};
    QSpinBox* meRangeSpinBox_{};
    QCheckBox* weightedPredCheckBox_{};
    QCheckBox* cabacCheckBox_{};
    
    // 场景配置控件
    QComboBox* sceneConfigCombo_{};
    
    // 编码控制控件
    QPushButton* startButton_{};
    QPushButton* stopButton_{};
    QProgressBar* progressBar_{};
    QTextEdit* logTextEdit_{};

    // 视频播放控件
    QVideoWidget* videoWidget_{};
    QMediaPlayer* mediaPlayer_{};
    QPushButton* playButton_{};
    QSlider* videoSlider_{};
    QLabel* timeLabel_{};

    // 编码控制
    bool shouldStop_{false};

    // 当前输出文件
    QString currentOutputFile_;

    // 帧生成控件
    QProgressBar* frameGenProgressBar_{};
    QPushButton* generateButton_{};
    QLabel* frameGenStatusLabel_{};

    // 编码线程
    std::thread encodingThread_;
}; 