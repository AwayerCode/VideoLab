#pragma once

#include <QMainWindow>
#include <QTabWidget>
#include "x264_config_window.hpp"
#include "vp8_config_window.hpp"
#include "aac_config_window.hpp"
#include "mp4_config_window.hpp"
#include "vlc_player_window.hpp"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

private:
    void setupUI();
    void createTabs();

    QTabWidget* tabWidget_{nullptr};
    X264ConfigWindow* x264ConfigWindow_{nullptr};
    VP8ConfigWindow* vp8ConfigWindow_{nullptr};
    AACConfigWindow* aacConfigWindow_{nullptr};
    MP4ConfigWindow* mp4ConfigWindow_{nullptr};
    VLCPlayerWindow* vlcPlayerWindow_{nullptr};
}; 