#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstdint>

// 前向声明
struct AVFormatContext;
struct AVStream;
struct AVCodecParameters;

class MP4Parser {
public:
    struct BoxInfo {
        std::string type;
        int64_t size;
        int64_t offset;
        int level;
    };

    struct VideoInfo {
        int width;
        int height;
        double duration;
        int64_t bitrate;
        double fps;
        int total_frames;
        int keyframe_count;
        std::string codec_name;
        std::string format_name;
    };

    struct AudioInfo {
        int channels;
        int sample_rate;
        double duration;
        int64_t bitrate;
        std::string codec_name;
    };

    struct KeyFrameInfo {
        double timestamp;
        int64_t pos;
    };

    // 实现类的定义
    struct Impl {
        AVFormatContext* formatCtx{nullptr};
        int videoStreamIndex{-1};
        int audioStreamIndex{-1};
        std::vector<BoxInfo> boxes;
        
        ~Impl();  // 析构函数声明
    };

    MP4Parser() = default;
    ~MP4Parser();

    bool open(const std::string& filename);
    void close();

    std::vector<BoxInfo> getBoxes() const;
    VideoInfo getVideoInfo() const;
    AudioInfo getAudioInfo() const;
    std::map<std::string, std::string> getMetadata() const;
    std::vector<KeyFrameInfo> getKeyFrameInfo() const;

    bool extractH264(const std::string& outputPath) const;
    bool extractAAC(const std::string& outputPath) const;

private:
    std::unique_ptr<Impl> impl_;
}; 