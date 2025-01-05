#include "mainwindow_test.hpp"
#include <QTest>
#include <QSignalSpy>

// 测试初始状态
TEST_F(MainWindowTest, InitialState) {
    // 测试窗口标题
    EXPECT_EQ(mainWindow->windowTitle(), QString("编码器性能测试"));
    
    // 测试窗口尺寸
    EXPECT_EQ(mainWindow->size(), QSize(800, 600));
    
    // 测试编码器下拉框初始状态
    auto encoderCombo = mainWindow->findChild<QComboBox*>("encoderCombo_");
    ASSERT_NE(encoderCombo, nullptr);
    EXPECT_EQ(encoderCombo->count(), 2);
    EXPECT_EQ(encoderCombo->currentText(), "x264 (CPU)");
}

// 测试场景选择
TEST_F(MainWindowTest, SceneSelection) {
    auto sceneCombo = mainWindow->findChild<QComboBox*>("sceneCombo_");
    ASSERT_NE(sceneCombo, nullptr);
    
    // 测试场景数量
    EXPECT_EQ(sceneCombo->count(), 4);
    
    // 测试默认场景
    EXPECT_EQ(sceneCombo->currentText(), QString("自定义"));
    
    // 测试场景切换
    sceneCombo->setCurrentText("直播场景");
    EXPECT_EQ(sceneCombo->currentText(), QString("直播场景"));
}

// 测试参数设置
TEST_F(MainWindowTest, ParameterSettings) {
    // 测试分辨率spinbox
    auto widthSpin = mainWindow->findChild<QSpinBox*>("widthSpin_");
    auto heightSpin = mainWindow->findChild<QSpinBox*>("heightSpin_");
    ASSERT_NE(widthSpin, nullptr);
    ASSERT_NE(heightSpin, nullptr);
    
    // 测试默认值
    EXPECT_EQ(widthSpin->value(), 1920);
    EXPECT_EQ(heightSpin->value(), 1080);
    
    // 测试范围
    EXPECT_EQ(widthSpin->minimum(), 320);
    EXPECT_EQ(widthSpin->maximum(), 3840);
    EXPECT_EQ(heightSpin->minimum(), 240);
    EXPECT_EQ(heightSpin->maximum(), 2160);
}

// 测试开始按钮
TEST_F(MainWindowTest, StartButton) {
    auto startButton = mainWindow->findChild<QPushButton*>("startButton_");
    ASSERT_NE(startButton, nullptr);
    
    // 测试按钮点击
    QTest::mouseClick(startButton, Qt::LeftButton);
}

// 测试进度条更新
TEST_F(MainWindowTest, ProgressUpdate) {
    auto progressBar = mainWindow->findChild<QProgressBar*>("progressBar_");
    ASSERT_NE(progressBar, nullptr);
    
    // 测试初始状态
    EXPECT_EQ(progressBar->value(), 0);
    EXPECT_EQ(progressBar->minimum(), 0);
    EXPECT_EQ(progressBar->maximum(), 100);
}

// 测试硬件加速选项
TEST_F(MainWindowTest, HardwareAcceleration) {
    auto hwAccelCheck = mainWindow->findChild<QCheckBox*>("hwAccelCheck_");
    ASSERT_NE(hwAccelCheck, nullptr);
    
    // 测试默认状态
    EXPECT_FALSE(hwAccelCheck->isChecked());
    
    // 测试切换状态
    hwAccelCheck->setChecked(true);
    EXPECT_TRUE(hwAccelCheck->isChecked());
}

// 测试场景切换时的参数联动
TEST_F(MainWindowTest, SceneParameterSync) {
    auto sceneCombo = mainWindow->findChild<QComboBox*>("sceneCombo_");
    auto threadsSpin = mainWindow->findChild<QSpinBox*>("threadsSpin_");
    ASSERT_NE(sceneCombo, nullptr);
    ASSERT_NE(threadsSpin, nullptr);

    // 记录初始值（自定义场景，20线程）
    int defaultThreads = threadsSpin->value();
    EXPECT_EQ(defaultThreads, 20);

    // 切换到直播场景（8线程）
    sceneCombo->setCurrentIndex(1);
    QTest::qWait(100); // 等待信号处理
    EXPECT_EQ(threadsSpin->value(), 8);

    // 切换到点播场景（16线程）
    sceneCombo->setCurrentIndex(2);
    QTest::qWait(100); // 等待信号处理
    EXPECT_EQ(threadsSpin->value(), 16);

    // 切换到存档场景（32线程）
    sceneCombo->setCurrentIndex(3);
    QTest::qWait(100); // 等待信号处理
    EXPECT_EQ(threadsSpin->value(), 32);

    // 切换回自定义场景（20线程）
    sceneCombo->setCurrentIndex(0);
    QTest::qWait(100); // 等待信号处理
    EXPECT_EQ(threadsSpin->value(), 20);
}

// 测试测试运行时的UI状态
TEST_F(MainWindowTest, TestRunningState) {
    auto startButton = mainWindow->findChild<QPushButton*>("startButton_");
    auto progressBar = mainWindow->findChild<QProgressBar*>("progressBar_");
    auto resultText = mainWindow->findChild<QTextEdit*>("resultText_");
    ASSERT_NE(startButton, nullptr);
    ASSERT_NE(progressBar, nullptr);
    ASSERT_NE(resultText, nullptr);

    // 记录初始状态
    EXPECT_TRUE(startButton->isEnabled());
    EXPECT_EQ(progressBar->value(), 0);
    EXPECT_TRUE(resultText->toPlainText().isEmpty());

    // 直接调用进度更新函数
    QMetaObject::invokeMethod(mainWindow, "onUpdateProgress", 
                             Qt::DirectConnection,
                             Q_ARG(int, 50),
                             Q_ARG(double, 30.0),
                             Q_ARG(double, 2000.0));

    // 验证进度条状态
    EXPECT_EQ(progressBar->value(), 50);
    
    // 验证状态栏文本
    QString expectedStatus = QString("进度: 50% | 速度: 30.00 fps | 码率: 2.00 Kbps");
    EXPECT_EQ(mainWindow->statusBar()->currentMessage(), expectedStatus);

    // 模拟测试运行
    QMetaObject::invokeMethod(mainWindow, "onStartTest", Qt::DirectConnection);
    
    // 等待测试完成
    QTest::qWait(2000);  // 等待2秒让测试完成

    // 验证测试完成后的状态
    EXPECT_TRUE(startButton->isEnabled());  // 按钮应该重新启用
    EXPECT_FALSE(resultText->toPlainText().isEmpty());  // 应该显示结果
}

// 测试状态栏更新
TEST_F(MainWindowTest, StatusBarUpdate) {
    // 模拟进度更新
    QMetaObject::invokeMethod(mainWindow, "onUpdateProgress", 
                             Qt::DirectConnection,
                             Q_ARG(int, 50),
                             Q_ARG(double, 30.0),
                             Q_ARG(double, 2000.0));

    // 验证状态栏文本
    QString expectedStatus = QString("进度: 50% | 速度: 30.00 fps | 码率: 2.00 Kbps");
    EXPECT_EQ(mainWindow->statusBar()->currentMessage(), expectedStatus);
}

// 测试结果显示
TEST_F(MainWindowTest, ResultDisplay) {
    auto resultText = mainWindow->findChild<QTextEdit*>("resultText_");
    ASSERT_NE(resultText, nullptr);

    // 直接调用进度更新函数
    QMetaObject::invokeMethod(mainWindow, "onUpdateProgress", 
                             Qt::DirectConnection,
                             Q_ARG(int, 50),
                             Q_ARG(double, 30.0),
                             Q_ARG(double, 2000.0));

    // 模拟一次成功的测试运行
    QMetaObject::invokeMethod(mainWindow, "onStartTest", Qt::DirectConnection);
    
    // 等待测试完成
    QTest::qWait(5000);  // 等待5秒让测试完成
    
    // 验证结果文本不为空
    EXPECT_FALSE(resultText->toPlainText().isEmpty());
    
    // 验证结果文本包含预期的字段
    QString resultStr = resultText->toPlainText();
    EXPECT_TRUE(resultStr.contains("编码完成"));
    EXPECT_TRUE(resultStr.contains("总编码时间"));
    EXPECT_TRUE(resultStr.contains("平均编码速度"));
    EXPECT_TRUE(resultStr.contains("平均码率"));
}

// 测试帧数设置的步进值
TEST_F(MainWindowTest, FrameCountStepValue) {
    auto frameCountSpin = mainWindow->findChild<QSpinBox*>("frameCountSpin_");
    ASSERT_NE(frameCountSpin, nullptr);

    // 测试步进值
    EXPECT_EQ(frameCountSpin->singleStep(), 30);
    
    // 测试值的变化
    int initialValue = frameCountSpin->value();
    frameCountSpin->stepUp();
    EXPECT_EQ(frameCountSpin->value(), initialValue + 30);
    frameCountSpin->stepDown();
    EXPECT_EQ(frameCountSpin->value(), initialValue);
}

// 测试编码器切换
TEST_F(MainWindowTest, EncoderSwitch) {
    auto encoderCombo = mainWindow->findChild<QComboBox*>("encoderCombo_");
    auto hwAccelCheck = mainWindow->findChild<QCheckBox*>("hwAccelCheck_");
    ASSERT_NE(encoderCombo, nullptr);
    ASSERT_NE(hwAccelCheck, nullptr);

    // 切换到NVENC
    encoderCombo->setCurrentText("NVENC (GPU)");
    EXPECT_TRUE(hwAccelCheck->isEnabled());
    
    // 切换回x264
    encoderCombo->setCurrentText("x264 (CPU)");
    EXPECT_TRUE(hwAccelCheck->isEnabled());
} 