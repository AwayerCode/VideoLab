#include "mainwindow_test.hpp"
#include <QTest>

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