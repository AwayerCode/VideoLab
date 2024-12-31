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

class X264ParamTest {
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
        Film,       // 电影：偏重视觉质量
        Animation,  // 动画：处理平面色块
        Grain,      // 胶片颗粒：保留噪点
        StillImage, // 静态图像：高质量静态图像
        PSNR,       // PSNR优化：峰值信噪比优化
        SSIM,       // SSIM优化：结构相似度优化
        FastDecode, // 快速解码：优化解码速度
        ZeroLatency // 零延迟：实时编码
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

        // 码率控制参数
        RateControl rateControl;
        union {
            int crf;       // CRF模式：质量因子(0-51)
            int qp;        // CQP模式：量化参数(0-51)
            int bitrate;   // ABR/CBR模式：目标码率(bps)
        };
        
        // 帧率控制
        int fps;
        bool vfr;         // 可变帧率
        
        // GOP参数
        int keyintMax;    // 最大关键帧间隔
        int bframes;      // B帧数量
        int refs;         // 参考帧数量
        
        // 质量参数
        bool fastFirstPass;   // 快速首遍编码
        int meRange;         // 运动估计范围
        bool weightedPred;   // 加权预测
        bool cabac;          // CABAC熵编码

        TestConfig() 
            : width(1920)
            , height(1080)
            , frameCount(300)
            , preset(Preset::Medium)
            , tune(Tune::None)
            , threads(20)
            , rateControl(RateControl::CRF)
            , crf(23)
            , fps(30)
            , vfr(false)
            , keyintMax(250)
            , bframes(3)
            , refs(3)
            , fastFirstPass(true)
            , meRange(16)
            , weightedPred(true)
            , cabac(true)
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

    X264ParamTest();
    ~X264ParamTest();

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

    // 运行预设对比测试
    static std::vector<TestResult> runPresetTest(
        int width = 1920,
        int height = 1080,
        int frameCount = 300
    );

    // 运行码率控制模式对比测试
    static std::vector<TestResult> runRateControlTest(
        RateControl mode,
        std::vector<int> values,
        int width = 1920,
        int height = 1080,
        int frameCount = 300
    );

    // 运行质量参数优化测试
    static std::vector<TestResult> runQualityTest(
        const TestConfig& baseConfig,
        std::vector<std::pair<std::string, std::vector<bool>>> params
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