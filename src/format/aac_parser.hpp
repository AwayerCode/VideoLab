#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

// 前向声明
struct AVFormatContext;
struct AVStream;
struct AVCodecParameters;

class AACParser {
public:
    // ADTS 帧信息
    struct FrameInfo {
        int64_t offset;          // 文件偏移
        int size;               // 帧大小
        double timestamp;       // 时间戳
        int sample_rate;        // 采样率
        int channels;           // 通道数
        int profile;            // AAC Profile
        bool has_crc;          // 是否有CRC
    };

    // 音频信息
    struct AudioInfo {
        int sample_rate;        // 采样率
        int channels;           // 通道数
        int profile;            // AAC Profile
        double duration;        // 时长(秒)
        int64_t bitrate;        // 比特率
        int total_frames;       // 总帧数
        std::string format;     // 格式(ADTS/LATM等)
    };

    // 实现类的定义
    struct Impl {
        AVFormatContext* formatCtx{nullptr};
        int audioStreamIndex{-1};
        std::vector<FrameInfo> frames;
        AudioInfo audioInfo{};

        ~Impl();  // 析构函数声明
    };

    AACParser() = default;
    ~AACParser();

    bool open(const std::string& filename);
    void close();

    // 获取音频信息
    AudioInfo getAudioInfo() const;
    
    // 获取所有帧信息
    std::vector<FrameInfo> getFrames() const;

    // 解析ADTS头部
    static bool parseADTSHeader(const uint8_t* data, int size, FrameInfo& frame);

private:
    std::unique_ptr<Impl> impl_;
}; 