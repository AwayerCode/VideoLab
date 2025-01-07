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
#include <QTableWidget>
#include <QHeaderView>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include "encode/vp8_param_test.hpp"
#include <thread>

// 历史记录结构体
struct VP8EncodingRecord {
    // 输入参数
    int width;
    int height;
    int frameCount;
    QString preset;     // best/good/realtime
    int speed;         // 0-16
    int threads;
    QString rateControl;
    int rateValue;     // bitrate或CQ值
    int keyint;        // 关键帧间隔
    int qmin;          // 最小量化参数
    int qmax;          // 最大量化参数
    
    // 编码结果
    double encodingTime;
    double fps;
    double bitrate;
    double psnr;
    double ssim;
    QString outputFile;
    
    // 时间戳
    QDateTime timestamp;
};

class VP8ConfigWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit VP8ConfigWindow(QWidget *parent = nullptr);
    ~VP8ConfigWindow() override;

public Q_SLOTS:
    void onStartEncoding();
    void onStopEncoding();
    void onRateControlChanged(int index);
    void onPresetConfigSelected(int index);
    void onSceneConfigChanged(int index);
    void onPlayVideo();
    void onMediaPositionChanged(qint64 position);
    void onMediaDurationChanged(qint64 duration);
    void onSliderMoved(int position);
    void updateProgress(int frame, double time, double fps, double bitrate, double psnr, double ssim);
    void appendLog(const QString& text);
    void onEncodingFinished();
    void onGenerateFrames();
    void addEncodingRecord(const VP8EncodingRecord& record);
    void clearEncodingHistory();
    void exportEncodingHistory();

private:
    void setupUI();
    void createConnections();
    void updateUIFromConfig(const VP8ParamTest::TestConfig& config);
    VP8ParamTest::TestConfig getConfigFromUI() const;
    void updateHistoryTable();
    void saveHistoryToFile();

    // 基本参数控件
    QComboBox* presetCombo_{};      // best/good/realtime
    QComboBox* speedCombo_{};       // 0-16
    QSpinBox* threadsSpinBox_{};
    QSpinBox* widthSpinBox_{};
    QSpinBox* heightSpinBox_{};
    QSpinBox* frameCountSpinBox_{};
    QComboBox* rateControlCombo_{};
    QSpinBox* rateValueSpinBox_{};  // bitrate或CQ值
    QSpinBox* keyintSpinBox_{};
    QSpinBox* qminSpinBox_{};
    QSpinBox* qmaxSpinBox_{};
    QComboBox* sceneConfigCombo_{};

    // 编码控制控件
    QPushButton* startButton_{};
    QPushButton* stopButton_{};
    QProgressBar* progressBar_{};
    QTextEdit* logTextEdit_{};
    bool shouldStop_{false};
    std::thread encodingThread_{};

    // 帧生成控件
    QLabel* frameGenStatusLabel_{};
    QProgressBar* frameGenProgressBar_{};
    QPushButton* generateButton_{};

    // 视频播放控件
    QVideoWidget* videoWidget_{};
    QMediaPlayer* mediaPlayer_{};
    QPushButton* playButton_{};
    QSlider* videoSlider_{};
    QLabel* timeLabel_{};
    QString currentOutputFile_{};

    // 历史记录控件
    QTableWidget* historyTable_{};
    QPushButton* clearHistoryButton_{};
    QPushButton* exportHistoryButton_{};
    std::vector<VP8EncodingRecord> encodingHistory_;
}; 