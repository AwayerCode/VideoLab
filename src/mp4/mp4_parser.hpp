#pragma once

#include <string>
#include <vector>
#include <memory>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

class MP4Parser {
public:
    struct VideoInfo {
        int width;
        int height;
        double duration;  // 单位：秒
        double bitrate;   // 单位：bps
        double fps;
        std::string codec_name;
        std::string format_name;
    };

    MP4Parser();
    ~MP4Parser();

    // 打开并解析 MP4 文件
    bool open(const std::string& filename);
    
    // 获取视频信息
    VideoInfo getVideoInfo() const;
    
    // 关闭文件
    void close();

private:
    AVFormatContext* formatCtx_{nullptr};
    int videoStreamIndex_{-1};
    VideoInfo videoInfo_;

    void cleanup();
}; 