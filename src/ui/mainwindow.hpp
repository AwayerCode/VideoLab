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
    void createFileGroup();
    void createEncoderGroup();
    void createParameterGroup();
    void createTestGroup();
    void createResultGroup();

    // UI 组件
    QWidget* centralWidget_;
    QVBoxLayout* mainLayout_;

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
    QTextEdit* resultText_;
}; 