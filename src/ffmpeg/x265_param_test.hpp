#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
}

#include <string>
#include <chrono>
#include <memory>
#include <functional>
#include <vector>

class X265ParamTest {
public:
    // 编码预设
    enum class Preset {
        UltraFast,
        SuperFast,
        VeryFast,
        Faster,
        Fast,
        Medium,
        Slow,
        Slower,
        VerySlow,
        Placebo
    };

    // 调优模式
    enum class Tune {
        None,
        PSNR,       // PSNR优化
        SSIM,       // SSIM优化
        Grain,      // 胶片颗粒
        ZeroLatency,// 零延迟
        FastDecode, // 快速解码
        Animation   // 动画
    };

    // 码率控制模式
    enum class RateControl {
        CRF,        // 恒定质量因子
        CQP,        // 恒定量化参数
        ABR,        // 平均码率
        CBR         // 恒定码率
    };

    struct TestConfig {
        int width;
        int height;
        int frameCount;
        
        // 基本编码参数
        Preset preset;
        Tune tune;
        int threads;
        int fps;  // 添加fps参数
        
        // 码率控制参数
        RateControl rateControl;
        int crf;    // 0-51, 默认28
        int qp;     // 0-51
        int bitrate;// kbps
        
        // x265特有参数
        bool bFrames;           // 是否启用B帧
        int bFrameCount;        // B帧数量
        bool weightedPred;      // 加权预测
        int keyintMax;          // 最大关键帧间隔
        int refFrames;          // 参考帧数量
        bool aqMode;            // 自适应量化
        int aqStrength;         // 自适应量化强度
        bool psyRd;            // 心理视觉优化
        double psyRdStrength;   // 心理视觉优化强度
        
        TestConfig() 
            : width(1920)
            , height(1080)
            , frameCount(300)
            , preset(Preset::Medium)
            , tune(Tune::None)
            , threads(0)
            , fps(30)  // 设置默认帧率为30fps
            , rateControl(RateControl::CRF)
            , crf(28)
            , qp(23)
            , bitrate(2000)
            , bFrames(true)
            , bFrameCount(3)
            , weightedPred(true)
            , keyintMax(250)
            , refFrames(3)
            , aqMode(true)
            , aqStrength(1)
            , psyRd(true)
            , psyRdStrength(1.0)
        {}
    };

    struct TestResult {
        double encodingTime;    // 编码时间
        double fps;            // 编码速度
        double bitrate;        // 实际码率
        double psnr;          // 峰值信噪比
        double ssim;          // 结构相似度
        bool success;
        std::string errorMessage;
    };

    X265ParamTest();
    ~X265ParamTest();

    // 初始化编码器
    bool initEncoder(const TestConfig& config);
    
    // 编码单帧
    bool encodeFrame(const uint8_t* data, int size);
    
    // 获取性能数据
    double getEncodingTime() const { return encodingTime_; }
    double getFPS() const { return fps_; }
    double getBitrate() const { return bitrate_; }
    double getPSNR() const { return psnr_; }
    double getSSIM() const { return ssim_; }
    
    // 清理资源
    void cleanup();

    // 运行单次参数测试
    static TestResult runTest(
        const TestConfig& config,
        std::function<void(int, const TestResult&)> progressCallback = nullptr
    );

    // 运行预设测试
    static std::vector<TestResult> runPresetTest(
        int width = 1920,
        int height = 1080,
        int frameCount = 300,
        int fps = 30
    );

    // 运行码率控制测试
    static std::vector<TestResult> runRateControlTest(
        RateControl mode,
        const std::vector<int>& values,
        int width = 1920,
        int height = 1080,
        int frameCount = 300,
        int fps = 30
    );

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
    double bitrate_{0.0};
    double psnr_{0.0};
    double ssim_{0.0};

    // 将预设枚举转换为字符串
    static const char* presetToString(Preset preset);
    // 将调优模式枚举转换为字符串
    static const char* tuneToString(Tune tune);
}; 