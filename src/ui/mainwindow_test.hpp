#pragma once

#include <gtest/gtest.h>
#include <QApplication>
#include "mainwindow.hpp"

class MainWindowTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        qputenv("QT_QPA_PLATFORM", "offscreen");  // 使用离屏渲染
        argc = 1;
        argv[0] = const_cast<char*>("test");
        app = new QApplication(argc, argv);
    }

    static void TearDownTestSuite() {
        delete app;
    }

    void SetUp() override {
        mainWindow = new MainWindow();
    }

    void TearDown() override {
        delete mainWindow;
    }

    MainWindow* mainWindow;
    static int argc;
    static char* argv[];
    static QApplication* app;
};

// 静态成员初始化
int MainWindowTest::argc = 1;
char* MainWindowTest::argv[] = {const_cast<char*>("test")};
QApplication* MainWindowTest::app = nullptr; 