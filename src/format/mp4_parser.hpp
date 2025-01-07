#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <map>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/dict.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/frame.h>
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
        int64_t total_frames;
        int64_t keyframe_count;
    };

    struct AudioInfo {
        int channels;
        int sample_rate;
        int64_t total_samples;
        std::string codec_name;
        double duration;  // 单位：秒
        double bitrate;   // 单位：bps
    };

    struct Frame {
        std::vector<uint8_t> data;
        int64_t pts;
        bool is_key_frame;
        double timestamp;  // 单位：秒
    };

    struct KeyFrameInfo {
        int64_t pts;
        int64_t pos;      // 文件位置
        double timestamp; // 单位：秒
    };

    struct BoxInfo {
        std::string type;    // box类型
        int64_t size;       // box大小
        int64_t offset;     // 文件偏移
        int level;          // 嵌套层级
    };

    MP4Parser();
    ~MP4Parser();

    // 基本文件操作
    bool open(const std::string& filename);
    void close();
    
    // 获取信息
    VideoInfo getVideoInfo() const;
    AudioInfo getAudioInfo() const;
    std::map<std::string, std::string> getMetadata() const;
    std::vector<KeyFrameInfo> getKeyFrameInfo() const;
    std::vector<BoxInfo> getBoxes() const;
    
    // 帧操作
    bool readNextFrame(Frame& frame);
    bool seekFrame(double timestamp);
    
    // 音频操作
    bool readAudioSamples(std::vector<float>& samples, int max_samples);
    bool seekAudio(double timestamp);

private:
    AVFormatContext* formatCtx_{nullptr};
    AVCodecContext* videoCodecCtx_{nullptr};
    AVCodecContext* audioCodecCtx_{nullptr};
    SwsContext* swsCtx_{nullptr};
    
    int videoStreamIndex_{-1};
    int audioStreamIndex_{-1};
    
    VideoInfo videoInfo_;
    AudioInfo audioInfo_;
    std::vector<BoxInfo> boxes_;
    
    void cleanup();
    bool initVideoCodec();
    bool initAudioCodec();
    void updateKeyFrameInfo();
    void parseBoxes();
    void parseBoxesRecursive(int64_t start_offset, int64_t total_size, int level = 0);
    std::vector<KeyFrameInfo> keyFrameInfo_;
}; 