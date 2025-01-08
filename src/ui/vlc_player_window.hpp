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
    void onAddToPlaylist();
    void onPlaylistItemDoubleClicked(QListWidgetItem* item);
    void onRemoveFromPlaylist();
    void onClearPlaylist();

private:
    void initUI();
    void initVLC();
    void cleanupVLC();

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

    QListWidget* playlistWidget_;
    QPushButton* addToPlaylistButton_;
    QPushButton* removeFromPlaylistButton_;
    QPushButton* clearPlaylistButton_;
    QMap<QString, QString> playlistMap_;
};

#endif // VLC_PLAYER_WINDOW_HPP 