#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/hwcontext.h>
#include <libavutil/imgutils.h>
}

#include <string>
#include <chrono>
#include <memory>
#include <functional>

class EncoderTest {
public:
    struct TestResult {
        double encodingTime;
        double fps;
        bool success;
        std::string errorMessage;
    };

    struct TestConfig {
        int width;
        int height;
        int bitrate;
        int frameCount;

        TestConfig() 
            : width(1920)
            , height(1080)
            , bitrate(5000000)  // 5Mbps
            , frameCount(300)   // 10秒视频 @ 30fps
        {}
    };

    EncoderTest();
    ~EncoderTest();

    // 初始化编码器
    bool initNvencEncoder(int width, int height, int bitrate);
    bool initX264Encoder(int width, int height, int bitrate);

    // 编码测试
    bool encodeFrame(const uint8_t* data, int size);
    
    // 获取性能数据
    double getEncodingTime() const { return encodingTime_; }
    double getFPS() const { return fps_; }
    
    // 清理资源
    void cleanup();

    // 运行完整的编码测试
    static TestResult runEncodingTest(
        bool useNvenc,
        int width,
        int height,
        int bitrate,
        int frameCount,
        std::function<void(int, const TestResult&)> progressCallback = nullptr
    );

    // 运行完整的性能对比测试
    static void runPerformanceTest(const TestConfig& config = TestConfig());

private:
    // 编码器上下文
    AVCodecContext* encoderCtx_{nullptr};
    AVFrame* frame_{nullptr};
    AVPacket* packet_{nullptr};
    
    // 性能测试相关
    std::chrono::steady_clock::time_point startTime_;
    int frameCount_{0};
    double encodingTime_{0.0};
    double fps_{0.0};
    
    // NVIDIA硬件加速相关
    AVBufferRef* hwDeviceCtx_{nullptr};
    AVFrame* hwFrame_{nullptr};
}; 