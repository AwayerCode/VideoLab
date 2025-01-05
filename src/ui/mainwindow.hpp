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

private:
    void setupUI();
    void createEncoderTestTab();
    void createFormatAnalysisTab();
    void createFileGroup();
    void createEncoderGroup();
    void createParameterGroup();
    void createTestGroup();
    void createResultGroup();
    void updateH264Details(AVCodecParameters* codecParams, QTreeWidgetItem* parent);
    QString getPixFmtName(int format);
    QString getChromaSamplingName(int format);

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
}; 