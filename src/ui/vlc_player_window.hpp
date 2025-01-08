#ifndef VLC_PLAYER_WINDOW_HPP
#define VLC_PLAYER_WINDOW_HPP

#include <QWidget>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <vlc/vlc.h>

class VLCPlayerWindow : public QWidget {
    Q_OBJECT

public:
    explicit VLCPlayerWindow(QWidget* parent = nullptr);
    ~VLCPlayerWindow();

    void playFile(const QString& filePath);
    void stop();

private slots:
    void onSelectFile();
    void onPlayPause();
    void onPositionChanged(int position);
    void updateInterface();

private:
    void initUI();
    void initVLC();
    void cleanupVLC();

    QWidget* videoWidget_;
    QPushButton* selectFileButton_;
    QPushButton* playPauseButton_;
    QSlider* positionSlider_;
    QLabel* timeLabel_;

    libvlc_instance_t* vlcInstance_;
    libvlc_media_player_t* mediaPlayer_;
    libvlc_media_t* media_;

    QTimer* updateTimer_;
    bool isPlaying_;
};

#endif // VLC_PLAYER_WINDOW_HPP 