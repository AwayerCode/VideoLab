#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
}

class VP8ParamTest {
public:
    // 预设选项
    enum class Preset {
        BEST,
        GOOD,
        REALTIME
    };

    // 速度级别
    enum class Speed {
        SPEED_0,
        SPEED_1,
        SPEED_2,
        SPEED_3,
        SPEED_4,
        SPEED_5,
        SPEED_6,
        SPEED_7,
        SPEED_8,
        SPEED_9,
        SPEED_10,
        SPEED_11,
        SPEED_12,
        SPEED_13,
        SPEED_14,
        SPEED_15,
        SPEED_16
    };

    // 码率控制模式
    enum class RateControl {
        CQ,    // 固定质量
        CBR,   // 固定码率
        VBR    // 可变码率
    };

    // 测试配置
    struct TestConfig {
        int width = 1920;
        int height = 1080;
        int fps = 30;
        int frames = 300;
        int threads = 1;
        int bitrate = 2000000;  // 比特率 (bps)
        int keyint = 250;       // 关键帧间隔
        int qmin = 0;           // 最小量化参数
        int qmax = 63;          // 最大量化参数
        int cq_level = 10;      // 固定质量级别
        Preset preset = Preset::GOOD;
        Speed speed = Speed::SPEED_0;
        RateControl rate_control = RateControl::VBR;
        std::string output_file;
    };

    // 测试结果
    struct TestResult {
        double encoding_time;    // 编码时间 (秒)
        double fps;             // 编码速度 (帧/秒)
        double bitrate;         // 实际码率 (bps)
        double psnr;            // PSNR值
        double ssim;            // SSIM值
    };

    using ProgressCallback = std::function<void(int, const TestResult&)>;

    VP8ParamTest();
    ~VP8ParamTest();

    static const char* presetToString(Preset preset);
    static const char* speedToString(Speed speed);

    // 运行单次测试
    static TestResult runTest(
        const TestConfig& config,
        ProgressCallback progress_callback = nullptr
    );

    // 运行预设测试
    static std::vector<TestResult> runPresetTest(
        TestConfig base_config,
        const std::string& output_prefix
    );

    // 运行码率控制测试
    static std::vector<TestResult> runRateControlTest(
        TestConfig base_config,
        const std::string& output_prefix
    );

    // 运行质量参数测试
    static std::vector<TestResult> runQualityTest(
        TestConfig base_config,
        const std::string& output_prefix,
        const std::vector<int>& quality_values
    );

    // 初始化帧缓存
    bool initFrameCache(const TestConfig& config);

protected:
    bool initEncoder(const TestConfig& config, const std::string& outputFile);
    bool encodeFrame(const uint8_t* data, int size);
    void cleanup();

    bool generateFrames(const TestConfig& config);
    const uint8_t* getFrameData(size_t frameIndex) const;

    // 帧生成相关的静态方法
    static std::vector<uint8_t> generateSingleFrame(int width, int height, int frameIndex);
    void generateFramesThreaded(const TestConfig& config, size_t thread_count);
    static void frameGenerationWorker(VP8ParamTest* self, const TestConfig& config,
                             size_t start_frame, size_t end_frame);

private:
    // 编码器相关
    AVCodecContext* codecContext_{nullptr};
    AVFrame* frame_{nullptr};
    AVPacket* packet_{nullptr};
    FILE* outputFile_{nullptr};

    // 帧缓存
    std::vector<std::vector<uint8_t>> frameCache_;
    bool framesGenerated_{false};

    // 写入缓冲
    struct WriteBuffer {
        std::mutex mutex;
        std::condition_variable cv;
        std::queue<std::vector<uint8_t>> packets;
        bool done{false};
        std::thread writer_thread;
    } writeBuffer_;

    void startWriterThread();
    void stopWriterThread();
    bool addPacketToBuffer(const AVPacket* packet);
    void writerThreadFunc();
}; 