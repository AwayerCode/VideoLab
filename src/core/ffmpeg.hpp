#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

#include <string>
#include <memory>

class FFmpeg {
public:
    FFmpeg();
    ~FFmpeg();

    // 打开视频文件
    bool openFile(const std::string& filename);
    // 关闭视频文件
    void closeFile();
    // 读取下一帧
    bool readFrame();
    // 获取格式上下文
    AVFormatContext* getFormatContext() const { return formatContext_; }

private:
    AVFormatContext* formatContext_{nullptr};
    AVCodecContext* codecContext_{nullptr};
    AVFrame* frame_{nullptr};
    AVPacket* packet_{nullptr};
    int videoStreamIndex_{-1};
};
