#include "vlc_player_window.hpp"
#include <QTimer>
#include <QMessageBox>
#include <QUrl>
#include <QCoreApplication>
#include <QDir>

VLCPlayerWindow::VLCPlayerWindow(QWidget* parent)
    : QWidget(parent)
    , vlcInstance_(nullptr)
    , mediaPlayer_(nullptr)
    , media_(nullptr)
    , isPlaying_(false)
{
    initUI();
    initVLC();
}

VLCPlayerWindow::~VLCPlayerWindow()
{
    cleanupVLC();
}

void VLCPlayerWindow::initUI()
{
    // 创建视频显示窗口
    videoWidget_ = new QWidget(this);
    videoWidget_->setStyleSheet("background-color: black;");
    videoWidget_->setMinimumSize(640, 360);

    // 创建控制按钮
    selectFileButton_ = new QPushButton(tr("选择文件"), this);
    playPauseButton_ = new QPushButton(tr("播放"), this);
    playPauseButton_->setEnabled(false);

    // 创建进度条
    positionSlider_ = new QSlider(Qt::Horizontal, this);
    positionSlider_->setRange(0, 1000);
    positionSlider_->setEnabled(false);

    // 创建时间标签
    timeLabel_ = new QLabel("00:00:00", this);

    // 创建媒体信息显示区域
    mediaInfoWidget_ = new QWidget(this);
    mediaInfoWidget_->setMinimumWidth(200);
    mediaInfoWidget_->setStyleSheet("QLabel { color: #333333; }");

    // 创建媒体信息标签
    titleLabel_ = new QLabel(tr("文件名："), mediaInfoWidget_);
    containerLabel_ = new QLabel(tr("封装格式："), mediaInfoWidget_);
    resolutionLabel_ = new QLabel(tr("分辨率："), mediaInfoWidget_);
    fpsLabel_ = new QLabel(tr("帧率："), mediaInfoWidget_);
    videoCodecLabel_ = new QLabel(tr("视频编码："), mediaInfoWidget_);
    videoProfileLabel_ = new QLabel(tr("编码配置："), mediaInfoWidget_);
    videoBitrateLabel_ = new QLabel(tr("视频码率："), mediaInfoWidget_);
    audioCodecLabel_ = new QLabel(tr("音频编码："), mediaInfoWidget_);
    audioChannelsLabel_ = new QLabel(tr("音频通道："), mediaInfoWidget_);
    audioSampleRateLabel_ = new QLabel(tr("采样率："), mediaInfoWidget_);
    audioBitrateLabel_ = new QLabel(tr("音频码率："), mediaInfoWidget_);
    durationLabel_ = new QLabel(tr("时长："), mediaInfoWidget_);

    // 创建媒体信息布局
    QVBoxLayout* infoLayout = new QVBoxLayout(mediaInfoWidget_);
    infoLayout->addWidget(new QLabel(tr("媒体信息"), this));
    infoLayout->addWidget(titleLabel_);
    infoLayout->addWidget(containerLabel_);
    infoLayout->addWidget(resolutionLabel_);
    infoLayout->addWidget(fpsLabel_);
    infoLayout->addWidget(videoCodecLabel_);
    infoLayout->addWidget(videoProfileLabel_);
    infoLayout->addWidget(videoBitrateLabel_);
    infoLayout->addWidget(audioCodecLabel_);
    infoLayout->addWidget(audioChannelsLabel_);
    infoLayout->addWidget(audioSampleRateLabel_);
    infoLayout->addWidget(audioBitrateLabel_);
    infoLayout->addWidget(durationLabel_);
    infoLayout->addStretch();

    // 创建按钮布局
    QHBoxLayout* controlLayout = new QHBoxLayout;
    controlLayout->addWidget(selectFileButton_);
    controlLayout->addWidget(playPauseButton_);
    controlLayout->addWidget(positionSlider_);
    controlLayout->addWidget(timeLabel_);

    // 创建左侧布局（视频和控制）
    QVBoxLayout* leftLayout = new QVBoxLayout;
    leftLayout->addWidget(videoWidget_);
    leftLayout->addLayout(controlLayout);

    // 创建主布局
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->addLayout(leftLayout, 7);  // 分配更多空间给视频区域
    mainLayout->addWidget(mediaInfoWidget_, 3);  // 分配较少空间给媒体信息

    // 连接信号和槽
    connect(selectFileButton_, &QPushButton::clicked, this, &VLCPlayerWindow::onSelectFile);
    connect(playPauseButton_, &QPushButton::clicked, this, &VLCPlayerWindow::onPlayPause);
    connect(positionSlider_, &QSlider::sliderMoved, this, &VLCPlayerWindow::onPositionChanged);

    // 创建定时器用于更新界面
    updateTimer_ = new QTimer(this);
    connect(updateTimer_, &QTimer::timeout, this, &VLCPlayerWindow::updateInterface);
    updateTimer_->start(1000); // 每秒更新一次
}

void VLCPlayerWindow::initVLC()
{
    // 设置 VLC 环境变量
    QString appDir = QCoreApplication::applicationDirPath();
    QString pluginPath = appDir + "/plugins";
    
    // 检查插件目录是否存在
    QDir pluginDir(pluginPath);
    if (!pluginDir.exists()) {
        QMessageBox::critical(this, tr("错误"), 
            tr("VLC 插件目录不存在：%1").arg(pluginPath));
        return;
    }

    // 设置环境变量
    qputenv("VLC_PLUGIN_PATH", pluginPath.toUtf8());
    qputenv("VLC_DATA_PATH", appDir.toUtf8());
    qputenv("VLC_VERBOSE", "3");  // 最详细的日志级别

    // 设置 VLC 参数
    const char* args[] = {
        "--verbose=3",             // 详细日志
        "--no-video-title-show",   // 不显示视频标题
        "--no-osd",               // 不显示屏幕显示
        "--no-snapshot-preview",   // 禁用快照预览
        "--no-stats",             // 禁用统计信息
        "--no-sub-autodetect-file", // 禁用字幕自动检测
        "--no-inhibit",           // 禁用屏幕保护程序抑制
        "--vout=x11",             // 使用 X11 视频输出
        "--aout=pulse"            // 使用 PulseAudio 音频输出
    };

    // 创建 VLC 实例
    vlcInstance_ = libvlc_new(sizeof(args) / sizeof(args[0]), args);
    if (!vlcInstance_) {
        const char* error = libvlc_errmsg();
        QString errorMsg = error ? QString::fromUtf8(error) : tr("未知错误");
        QMessageBox::critical(this, tr("错误"), 
            tr("无法初始化 VLC：%1").arg(errorMsg));
        return;
    }

    // 创建媒体播放器
    mediaPlayer_ = libvlc_media_player_new(vlcInstance_);
    if (!mediaPlayer_) {
        const char* error = libvlc_errmsg();
        QString errorMsg = error ? QString::fromUtf8(error) : tr("未知错误");
        QMessageBox::critical(this, tr("错误"), 
            tr("无法创建媒体播放器：%1").arg(errorMsg));
        return;
    }

    // 设置视频输出窗口
#if defined(Q_OS_WIN)
    libvlc_media_player_set_hwnd(mediaPlayer_, (void*)videoWidget_->winId());
#elif defined(Q_OS_LINUX)
    libvlc_media_player_set_xwindow(mediaPlayer_, videoWidget_->winId());
#endif
}

void VLCPlayerWindow::cleanupVLC()
{
    if (mediaPlayer_) {
        libvlc_media_player_stop_async(mediaPlayer_);
        libvlc_media_player_release(mediaPlayer_);
        mediaPlayer_ = nullptr;
    }

    if (media_) {
        libvlc_media_release(media_);
        media_ = nullptr;
    }

    if (vlcInstance_) {
        libvlc_release(vlcInstance_);
        vlcInstance_ = nullptr;
    }
}

void VLCPlayerWindow::onSelectFile()
{
    QString filePath = QFileDialog::getOpenFileName(this,
        tr("选择视频文件"),
        QString(),
        tr("视频文件 (*.mp4 *.avi *.mkv *.mov);;所有文件 (*.*)"));

    if (!filePath.isEmpty()) {
        playFile(filePath);
    }
}

void VLCPlayerWindow::playFile(const QString& filePath)
{
    if (!mediaPlayer_) {
        return;
    }

    // 停止当前播放
    stop();

    // 创建新的媒体
    QByteArray utf8Path = filePath.toUtf8();
    media_ = libvlc_media_new_path(utf8Path.constData());
    if (!media_) {
        QMessageBox::critical(this, tr("错误"), tr("无法加载媒体文件"));
        return;
    }

    // 设置媒体到播放器
    libvlc_media_player_set_media(mediaPlayer_, media_);

    // 开始播放
    if (libvlc_media_player_play(mediaPlayer_) == 0) {
        isPlaying_ = true;
        playPauseButton_->setText(tr("暂停"));
        playPauseButton_->setEnabled(true);
        positionSlider_->setEnabled(true);
        
        // 等待一小段时间后更新媒体信息（确保媒体已经开始播放）
        QTimer::singleShot(500, this, &VLCPlayerWindow::updateMediaInfo);
    }
}

void VLCPlayerWindow::updateMediaInfo()
{
    if (!media_) {
        return;
    }

    // 获取文件信息
    QFileInfo fileInfo(QString::fromUtf8(libvlc_media_get_mrl(media_)));
    titleLabel_->setText(tr("文件名：%1").arg(fileInfo.fileName()));

    // 获取时长
    libvlc_time_t duration = libvlc_media_get_duration(media_);
    int totalSeconds = duration / 1000;
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;
    durationLabel_->setText(tr("时长：%1:%2:%3")
        .arg(hours, 2, 10, QChar('0'))
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0')));

    // 设置默认值
    containerLabel_->setText(tr("封装格式：") + fileInfo.suffix().toUpper());
    resolutionLabel_->setText(tr("分辨率：未知"));
    fpsLabel_->setText(tr("帧率：未知"));
    videoCodecLabel_->setText(tr("视频编码：未知"));
    videoProfileLabel_->setText(tr("编码配置：未知"));
    videoBitrateLabel_->setText(tr("视频码率：未知"));
    audioCodecLabel_->setText(tr("音频编码：未知"));
    audioChannelsLabel_->setText(tr("音频通道：未知"));
    audioSampleRateLabel_->setText(tr("采样率：未知"));
    audioBitrateLabel_->setText(tr("音频码率：未知"));

    if (mediaPlayer_) {
        // 获取视频尺寸
        unsigned int width = 0, height = 0;
        libvlc_video_get_size(mediaPlayer_, 0, &width, &height);
        if (width > 0 && height > 0) {
            resolutionLabel_->setText(tr("分辨率：%1x%2").arg(width).arg(height));
        }

        // 获取音频音量
        int volume = libvlc_audio_get_volume(mediaPlayer_);
        if (volume >= 0) {
            audioChannelsLabel_->setText(tr("音量：%1%").arg(volume));
        }

        // 获取播放进度
        libvlc_time_t current = libvlc_media_player_get_time(mediaPlayer_);
        if (current >= 0 && duration > 0) {
            float progress = (float)current / duration * 100;
            videoBitrateLabel_->setText(tr("播放进度：%.1f%%").arg(progress));
        }

        // 获取基本元数据
        char* meta;
        
        // 获取标题
        meta = libvlc_media_get_meta(media_, libvlc_meta_Title);
        if (meta) {
            titleLabel_->setText(tr("标题：%1").arg(QString::fromUtf8(meta)));
            free(meta);
        }

        // 获取描述
        meta = libvlc_media_get_meta(media_, libvlc_meta_Description);
        if (meta) {
            videoProfileLabel_->setText(tr("描述：%1").arg(QString::fromUtf8(meta)));
            free(meta);
        }

        // 获取版权信息（可能包含编码信息）
        meta = libvlc_media_get_meta(media_, libvlc_meta_Copyright);
        if (meta) {
            videoCodecLabel_->setText(tr("版权：%1").arg(QString::fromUtf8(meta)));
            free(meta);
        }

        // 获取发布者信息
        meta = libvlc_media_get_meta(media_, libvlc_meta_Publisher);
        if (meta) {
            audioCodecLabel_->setText(tr("发布者：%1").arg(QString::fromUtf8(meta)));
            free(meta);
        }
    }
}

void VLCPlayerWindow::stop()
{
    if (mediaPlayer_) {
        libvlc_media_player_stop_async(mediaPlayer_);
    }
    if (media_) {
        libvlc_media_release(media_);
        media_ = nullptr;
    }
    isPlaying_ = false;
    playPauseButton_->setText(tr("播放"));
    playPauseButton_->setEnabled(false);
    positionSlider_->setEnabled(false);
    positionSlider_->setValue(0);
    timeLabel_->setText("00:00:00");
    
    // 清空媒体信息
    titleLabel_->setText(tr("文件名："));
    containerLabel_->setText(tr("封装格式："));
    resolutionLabel_->setText(tr("分辨率："));
    fpsLabel_->setText(tr("帧率："));
    videoCodecLabel_->setText(tr("视频编码："));
    videoProfileLabel_->setText(tr("编码配置："));
    videoBitrateLabel_->setText(tr("视频码率："));
    audioCodecLabel_->setText(tr("音频编码："));
    audioChannelsLabel_->setText(tr("音频通道："));
    audioSampleRateLabel_->setText(tr("采样率："));
    audioBitrateLabel_->setText(tr("音频码率："));
    durationLabel_->setText(tr("时长："));
}

void VLCPlayerWindow::onPlayPause()
{
    if (!mediaPlayer_) {
        return;
    }

    if (isPlaying_) {
        libvlc_media_player_pause(mediaPlayer_);
        playPauseButton_->setText(tr("播放"));
    } else {
        libvlc_media_player_play(mediaPlayer_);
        playPauseButton_->setText(tr("暂停"));
    }
    isPlaying_ = !isPlaying_;
}

void VLCPlayerWindow::onPositionChanged(int position)
{
    if (!mediaPlayer_) {
        return;
    }

    float pos = position / 1000.0f;
    libvlc_media_player_set_position(mediaPlayer_, pos, true);
}

void VLCPlayerWindow::updateInterface()
{
    if (!mediaPlayer_ || !isPlaying_) {
        return;
    }

    // 更新进度条
    float pos = libvlc_media_player_get_position(mediaPlayer_);
    positionSlider_->setValue(pos * 1000);

    // 更新时间标签
    libvlc_time_t time = libvlc_media_player_get_time(mediaPlayer_);
    int seconds = time / 1000;
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    seconds = seconds % 60;
    timeLabel_->setText(QString("%1:%2:%3")
        .arg(hours, 2, 10, QChar('0'))
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0')));
} 