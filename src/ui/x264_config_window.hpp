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
#include "../ffmpeg/x264_param_test.hpp"
#include <thread>

// 历史记录结构体
struct EncodingRecord {
    // 输入参数
    int width;
    int height;
    int frameCount;
    QString preset;
    QString tune;
    int threads;
    QString rateControl;
    int rateValue;
    int keyintMax;      // 关键帧间隔
    int bframes;        // B帧数量
    int refs;          // 参考帧数
    bool fastFirstPass; // 快速首遍编码
    int meRange;       // 运动估计范围
    bool weightedPred; // 加权预测
    bool cabac;        // CABAC熵编码
    
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

class X264ConfigWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit X264ConfigWindow(QWidget *parent = nullptr);
    ~X264ConfigWindow() override;

public Q_SLOTS:
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
    void addEncodingRecord(const EncodingRecord& record);
    void clearEncodingHistory();
    void exportEncodingHistory();
    void loadEncodingHistory();

private:
    void setupUI();
    void createConnections();
    void updateUIFromConfig(const X264ParamTest::TestConfig& config);
    X264ParamTest::TestConfig getConfigFromUI() const;
    void setupHistoryUI();
    void updateHistoryTable();
    void saveHistoryToFile();

    // 基本参数控件
    QComboBox* presetCombo_{};
    QComboBox* tuneCombo_{};
    QSpinBox* threadsSpinBox_{};
    QSpinBox* widthSpinBox_{};
    QSpinBox* heightSpinBox_{};
    QSpinBox* frameCountSpinBox_{};
    QComboBox* rateControlCombo_{};
    QSpinBox* rateValueSpinBox_{};
    QSpinBox* keyintMaxSpinBox_{};
    QSpinBox* bframesSpinBox_{};
    QSpinBox* refsSpinBox_{};
    QCheckBox* fastFirstPassCheckBox_{};
    QSpinBox* meRangeSpinBox_{};
    QCheckBox* weightedPredCheckBox_{};
    QCheckBox* cabacCheckBox_{};
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
    std::vector<EncodingRecord> encodingHistory_;
}; 