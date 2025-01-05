#pragma once

#include <QTest>
#include <QSignalSpy>
#include "mainwindow.hpp"

class MainWindowTest : public QObject {
    Q_OBJECT

private slots:
    // 初始化和清理
    void initTestCase();    // 在第一个测试函数之前调用
    void cleanupTestCase(); // 在最后一个测试函数之后调用
    void init();           // 每个测试函数之前调用
    void cleanup();        // 每个测试函数之后调用

    // 测试用例
    void testInitialState();           // 测试初始状态
    void testSceneSelection();         // 测试场景选择
    void testParameterValidation();    // 测试参数验证
    void testProgressUpdate();         // 测试进度更新
    void testEncoderSelection();       // 测试编码器选择

private:
    MainWindow* window_;
}; 