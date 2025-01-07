#pragma once

#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QProgressBar>
#include <QTextEdit>
#include <QSlider>
#include <QGroupBox>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QTimer>

#include "encode/vp8_param_test.hpp"

class VP8ConfigWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit VP8ConfigWindow(QWidget *parent = nullptr);
    ~VP8ConfigWindow() override;

private slots:
    void onStartEncoding();
    void onStopEncoding();
    void onRateControlChanged(int index);
    void onPresetConfigSelected(int index);
    void onPlayVideo();
    void onMediaPositionChanged(qint64 position);
    void onMediaDurationChanged(qint64 duration);
    void onSliderMoved(int position);
    void onGenerateFrames();
    void updateProgress(int frame, double encodingTime, double fps, double bitrate, double psnr, double ssim);

private:
    void setupUI();
    void createConnections();
    void appendLog(const QString& text);
    void onEncodingFinished();
    void updateUIFromConfig(const VP8ParamTest::TestConfig& config);
    VP8ParamTest::TestConfig getConfigFromUI() const;

    // UI组件
    QWidget* centralWidget_{nullptr};
    QVBoxLayout* mainLayout_{nullptr};
    
    // 基本参数设置
    QGroupBox* basicParamsGroup_{nullptr};
    QComboBox* presetCombo_{nullptr};
    QComboBox* speedCombo_{nullptr};
    QComboBox* rateControlCombo_{nullptr};
    QSpinBox* bitrateSpinBox_{nullptr};
    QSpinBox* cqLevelSpinBox_{nullptr};
    QSpinBox* qminSpinBox_{nullptr};
    QSpinBox* qmaxSpinBox_{nullptr};
    QSpinBox* keyintSpinBox_{nullptr};
    QSpinBox* threadsSpinBox_{nullptr};

    // 分辨率和帧率设置
    QGroupBox* videoParamsGroup_{nullptr};
    QSpinBox* widthSpinBox_{nullptr};
    QSpinBox* heightSpinBox_{nullptr};
    QSpinBox* fpsSpinBox_{nullptr};
    QSpinBox* framesSpinBox_{nullptr};

    // 输出设置
    QGroupBox* outputGroup_{nullptr};
    QLineEdit* outputPathEdit_{nullptr};
    QPushButton* browseButton_{nullptr};

    // 控制按钮
    QHBoxLayout* controlLayout_{nullptr};
    QPushButton* generateButton_{nullptr};
    QPushButton* startButton_{nullptr};
    QPushButton* stopButton_{nullptr};
    QPushButton* playButton_{nullptr};

    // 进度显示
    QGroupBox* progressGroup_{nullptr};
    QProgressBar* progressBar_{nullptr};
    QLabel* timeLabel_{nullptr};
    QLabel* fpsLabel_{nullptr};
    QLabel* bitrateLabel_{nullptr};
    QLabel* psnrLabel_{nullptr};
    QLabel* ssimLabel_{nullptr};

    // 日志显示
    QGroupBox* logGroup_{nullptr};
    QTextEdit* logEdit_{nullptr};

    // 视频预览
    QGroupBox* previewGroup_{nullptr};
    QVideoWidget* videoWidget_{nullptr};
    QSlider* videoSlider_{nullptr};
    QMediaPlayer* mediaPlayer_{nullptr};

    // 状态变量
    bool isEncoding_{false};
    bool isPlaying_{false};
}; 