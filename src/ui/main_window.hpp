#pragma once

#include <QMainWindow>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QIcon>
#include <QLabel>
#include <QTabBar>
#include <QTransform>
#include "x264_config_window.hpp"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private:
    void setupUI();
    void createTabs();
    void setupIcons();

    QWidget* centralWidget_;
    QHBoxLayout* mainLayout_;
    QTabWidget* tabWidget_;
    X264ConfigWindow* x264Window_;

    // 图标
    QIcon x264Icon_;
    QIcon mp4Icon_;
    QIcon x265Icon_;
    QIcon h264Icon_;
}; 