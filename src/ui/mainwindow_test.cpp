#include "mainwindow_test.hpp"
#include <QApplication>

void MainWindowTest::initTestCase() {
    // 确保有QApplication实例
    if (!QApplication::instance()) {
        static int argc = 1;
        static char* argv[] = { const_cast<char*>("test") };
        new QApplication(argc, argv);
    }
}

void MainWindowTest::cleanupTestCase() {
    // 清理全局资源
}

void MainWindowTest::init() {
    // 每个测试前创建新的窗口实例
    window_ = new MainWindow();
}

void MainWindowTest::cleanup() {
    // 每个测试后删除窗口实例
    delete window_;
    window_ = nullptr;
}

void MainWindowTest::testInitialState() {
    // 测试窗口初始状态
    QVERIFY(window_->isVisible() == false);
    
    // 测试默认值
    auto* widthSpin = window_->findChild<QSpinBox*>();
    QVERIFY(widthSpin != nullptr);
    QCOMPARE(widthSpin->value(), 1920);

    auto* heightSpin = window_->findChild<QSpinBox*>();
    QVERIFY(heightSpin != nullptr);
    QCOMPARE(heightSpin->value(), 1080);

    auto* frameCountSpin = window_->findChild<QSpinBox*>();
    QVERIFY(frameCountSpin != nullptr);
    QCOMPARE(frameCountSpin->value(), 300);

    auto* threadsSpin = window_->findChild<QSpinBox*>();
    QVERIFY(threadsSpin != nullptr);
    QCOMPARE(threadsSpin->value(), 20);

    // 测试编码器选择
    auto* encoderCombo = window_->findChild<QComboBox*>();
    QVERIFY(encoderCombo != nullptr);
    QCOMPARE(encoderCombo->currentIndex(), 0);  // 默认选择x264

    // 测试场景选择
    auto* sceneCombo = window_->findChild<QComboBox*>();
    QVERIFY(sceneCombo != nullptr);
    QCOMPARE(sceneCombo->currentIndex(), 0);  // 默认选择自定义
}

void MainWindowTest::testSceneSelection() {
    // 获取场景选择组合框
    auto* sceneCombo = window_->findChild<QComboBox*>();
    QVERIFY(sceneCombo != nullptr);

    // 测试直播场景
    QTest::mouseClick(sceneCombo, Qt::LeftButton);
    QTest::keyClick(sceneCombo, Qt::Key_Down);
    QTest::keyClick(sceneCombo, Qt::Key_Return);
    
    auto* threadsSpin = window_->findChild<QSpinBox*>();
    QVERIFY(threadsSpin != nullptr);
    QCOMPARE(threadsSpin->value(), 20);  // 直播场景的默认线程数

    // 测试点播场景
    QTest::mouseClick(sceneCombo, Qt::LeftButton);
    QTest::keyClick(sceneCombo, Qt::Key_Down);
    QTest::keyClick(sceneCombo, Qt::Key_Return);
    QCOMPARE(threadsSpin->value(), 20);  // 点播场景的默认线程数
}

void MainWindowTest::testParameterValidation() {
    // 测试分辨率范围
    auto* widthSpin = window_->findChild<QSpinBox*>();
    QVERIFY(widthSpin != nullptr);
    
    widthSpin->setValue(0);  // 低于最小值
    QCOMPARE(widthSpin->value(), 320);  // 应该被限制在最小值
    
    widthSpin->setValue(4000);  // 高于最大值
    QCOMPARE(widthSpin->value(), 3840);  // 应该被限制在最大值

    // 测试帧数范围
    auto* frameCountSpin = window_->findChild<QSpinBox*>();
    QVERIFY(frameCountSpin != nullptr);
    
    frameCountSpin->setValue(0);  // 低于最小值
    QCOMPARE(frameCountSpin->value(), 30);  // 应该被限制在最小值
    
    frameCountSpin->setValue(4000);  // 高于最大值
    QCOMPARE(frameCountSpin->value(), 3000);  // 应该被限制在最大值
}

void MainWindowTest::testProgressUpdate() {
    // 获取进度条
    auto* progressBar = window_->findChild<QProgressBar*>();
    QVERIFY(progressBar != nullptr);
    QCOMPARE(progressBar->value(), 0);  // 初始值应该是0

    // 模拟进度更新
    QMetaObject::invokeMethod(window_, "onUpdateProgress",
                             Qt::DirectConnection,
                             Q_ARG(int, 50),
                             Q_ARG(double, 30.0),
                             Q_ARG(double, 5000.0));
    
    QCOMPARE(progressBar->value(), 50);  // 进度条应该更新到50%
}

void MainWindowTest::testEncoderSelection() {
    // 获取编码器选择组合框
    auto* encoderCombo = window_->findChild<QComboBox*>();
    QVERIFY(encoderCombo != nullptr);

    // 测试切换到NVENC
    QTest::mouseClick(encoderCombo, Qt::LeftButton);
    QTest::keyClick(encoderCombo, Qt::Key_Down);
    QTest::keyClick(encoderCombo, Qt::Key_Return);
    QCOMPARE(encoderCombo->currentText(), "NVENC (GPU)");

    // 测试硬件加速选项
    auto* hwAccelCheck = window_->findChild<QCheckBox*>();
    QVERIFY(hwAccelCheck != nullptr);
    QCOMPARE(hwAccelCheck->isEnabled(), true);  // 选择NVENC时应该启用
}

// 使用QTEST_MAIN宏来运行测试
QTEST_MAIN(MainWindowTest) 