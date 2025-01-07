#pragma once

#include <string>
#include "core/ffmpeg.hpp"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/pixdesc.h>
}

class VideoAnalyzer {
public:
    VideoAnalyzer();
    ~VideoAnalyzer();

    // 分析视频文件
    bool analyze(const std::string& filename);

    // 获取分析结果
    std::string getFormatInfo() const;
    std::string getVideoInfo() const;
    std::string getAudioInfo() const;
    std::string getStreamInfo() const;
    std::string getH264Info() const;

private:
    // 解析H.264特定信息
    std::string parseH264Info(const AVCodecParameters* codecParams) const;
    // 获取像素格式名称
    std::string getPixelFormatName(AVPixelFormat format) const;
    // 格式化时间
    std::string formatTime(int64_t duration) const;
    // 格式化比特率
    std::string formatBitrate(int64_t bitrate) const;

    AVFormatContext* formatContext_;
    const AVCodec* videoCodec_;
    const AVCodec* audioCodec_;
    int videoStreamIndex_;
    int audioStreamIndex_;
}; 