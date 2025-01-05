#pragma once

#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QProgressBar>
#include <QGroupBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QStatusBar>
#include <QGridLayout>
#include <QLineEdit>
#include <QTabWidget>
#include <QTreeWidget>
#include <QSplitter>
#include <QtMultimediaWidgets/QVideoWidget>
#include <QtMultimedia/QMediaPlayer>
#include <QSlider>
#include <QStackedWidget>
#include "../ffmpeg/videofilter.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

signals:
    void progressUpdated(int progress, double fps, double bitrate);

private slots:
    void onStartTest();
    void onSceneChanged(int index);
    void onUpdateProgress(int progress, double fps, double bitrate);
    void onSelectFile();
    void onAnalyzeFile();
    void onSelectVideoFile();
    void onPlayPause();
    void onPositionChanged(qint64 position);
    void onDurationChanged(qint64 duration);
    void onSliderMoved(int position);
    void onPlayerStateChanged(QMediaPlayer::PlaybackState state);
    void onSelectFilterFile();
    void onFilterChanged(int index);
    void onFilterParamChanged();
    void onFilterPlayPause();

private:
    void setupUI();
    void createEncoderTestTab();
    void createFormatAnalysisTab();
    void createPlayerTab();
    void createFilterTab();
    void createFileGroup();
    void createEncoderGroup();
    void createParameterGroup();
    void createTestGroup();
    void createResultGroup();
    void updateH264Details(AVCodecParameters* codecParams, QTreeWidgetItem* parent);
    QString getPixFmtName(int format);
    QString getChromaSamplingName(int format);
    QString formatTime(qint64 ms);

    // UI 组件
    QWidget* centralWidget_;
    QVBoxLayout* mainLayout_;
    QTabWidget* tabWidget_;

    // 编码测试页面
    QWidget* encoderTestTab_;
    QVBoxLayout* encoderTestLayout_;

    // 格式解析页面
    QWidget* formatAnalysisTab_;
    QVBoxLayout* formatAnalysisLayout_;
    QSplitter* formatAnalysisSplitter_;
    QTreeWidget* formatTree_;
    QTextEdit* resultText_;

    // 播放器页面
    QWidget* playerTab_;
    QVBoxLayout* playerLayout_;
    QVideoWidget* videoWidget_;
    QMediaPlayer* mediaPlayer_;
    QPushButton* selectVideoButton_;
    QPushButton* playPauseButton_;
    QSlider* positionSlider_;
    QLabel* timeLabel_;
    QLabel* durationLabel_;

    // 文件选择组
    QGroupBox* fileGroup_;
    QLineEdit* filePathEdit_;
    QPushButton* selectFileButton_;
    QPushButton* analyzeButton_;

    // 编码器选择组
    QGroupBox* encoderGroup_;
    QComboBox* encoderCombo_;
    QComboBox* sceneCombo_;

    // 参数设置组
    QGroupBox* paramGroup_;
    QSpinBox* widthSpin_;
    QSpinBox* heightSpin_;
    QSpinBox* frameCountSpin_;
    QSpinBox* threadsSpin_;
    QCheckBox* hwAccelCheck_;

    // 测试控制组
    QGroupBox* testGroup_;
    QPushButton* startButton_;
    QProgressBar* progressBar_;

    // 结果显示组
    QGroupBox* resultGroup_;

    // 滤镜测试页面
    QWidget* filterTab_;
    QHBoxLayout* filterLayout_;
    QVideoWidget* sourceVideoWidget_;
    QVideoWidget* filteredVideoWidget_;
    QMediaPlayer* sourcePlayer_;
    QMediaPlayer* filteredPlayer_;
    QWidget* filterControlWidget_;
    QVBoxLayout* filterControlLayout_;
    QPushButton* selectFilterFileButton_;
    QPushButton* filterPlayPauseButton_;
    QComboBox* filterCombo_;
    QStackedWidget* filterParamsWidget_;
    QSlider* filterPositionSlider_;
    QLabel* filterTimeLabel_;
    QLabel* filterDurationLabel_;

    // 滤镜参数组件
    QWidget* brightnessContrastWidget_;
    QSpinBox* brightnessSpin_;
    QSpinBox* contrastSpin_;
    QWidget* hsvAdjustWidget_;
    QSpinBox* hueSpin_;
    QSpinBox* saturationSpin_;
    QSpinBox* valueSpin_;
    QWidget* sharpenBlurWidget_;
    QDoubleSpinBox* sharpenSpin_;
    QDoubleSpinBox* blurSpin_;
}; 