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

    // 创建按钮布局
    QHBoxLayout* controlLayout = new QHBoxLayout;
    controlLayout->addWidget(selectFileButton_);
    controlLayout->addWidget(playPauseButton_);
    controlLayout->addWidget(positionSlider_);
    controlLayout->addWidget(timeLabel_);

    // 创建主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(videoWidget_);
    mainLayout->addLayout(controlLayout);

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