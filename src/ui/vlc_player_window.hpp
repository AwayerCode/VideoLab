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
#include <vlc/libvlc_media.h>
#include <vlc/libvlc_media_player.h>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMap>

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
    void updateMediaInfo();

    QWidget* videoWidget_;
    QPushButton* selectFileButton_;
    QPushButton* playPauseButton_;
    QSlider* positionSlider_;
    QLabel* timeLabel_;
    QTimer* updateTimer_;

    libvlc_instance_t* vlcInstance_;
    libvlc_media_player_t* mediaPlayer_;
    libvlc_media_t* media_;

    bool isPlaying_;

    QLabel* titleLabel_;
    QLabel* containerLabel_;
    QLabel* resolutionLabel_;
    QLabel* fpsLabel_;
    QLabel* videoCodecLabel_;
    QLabel* videoProfileLabel_;
    QLabel* videoBitrateLabel_;
    QLabel* audioCodecLabel_;
    QLabel* audioChannelsLabel_;
    QLabel* audioSampleRateLabel_;
    QLabel* audioBitrateLabel_;
    QLabel* durationLabel_;
    QWidget* mediaInfoWidget_;
};

#endif // VLC_PLAYER_WINDOW_HPP 