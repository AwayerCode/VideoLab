#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
}

#include <string>
#include <functional>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include "core/encoder_test.hpp"

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
            , threads(1)
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
        double encodingTime{0.0};    // 编码时间
        double fps{0.0};            // 编码速度
        double bitrate{0.0};        // 实际码率
        double psnr{0.0};          // 峰值信噪比
        double ssim{0.0};          // 结构相似度
        bool success{false};
        std::string errorMessage;
        std::string outputFile;     // 输出文件路径

        TestResult() = default;
    };

    // 预定义场景配置
    struct SceneConfig {
        std::string name;        // 场景名称
        TestConfig config;       // 编码配置
        std::string description; // 场景描述
    };

    X264ParamTest();
    ~X264ParamTest();

    // 初始化编码器
    bool initEncoder(const TestConfig& config, const std::string& outputFile = "");
    
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

    // 获取预定义场景配置
    static SceneConfig getLiveStreamConfig() {
        SceneConfig cfg;
        cfg.name = "直播场景";
        cfg.description = "低延迟，稳定码率，适合实时传输";
        cfg.config.preset = Preset::VeryFast;
        cfg.config.tune = Tune::ZeroLatency;
        cfg.config.rateControl = RateControl::CBR;
        cfg.config.bitrate = 4000000;  // 4Mbps
        cfg.config.keyintMax = 60;     // 2秒一个关键帧
        cfg.config.bframes = 0;        // 禁用B帧减少延迟
        cfg.config.refs = 1;           // 最小参考帧
        return cfg;
    }

    static SceneConfig getVODConfig() {
        SceneConfig cfg;
        cfg.name = "点播场景";
        cfg.description = "平衡质量和码率，适合视频网站";
        cfg.config.preset = Preset::Medium;
        cfg.config.rateControl = RateControl::CRF;
        cfg.config.crf = 23;
        cfg.config.keyintMax = 250;
        cfg.config.bframes = 3;
        cfg.config.refs = 3;
        return cfg;
    }

    static SceneConfig getArchiveConfig() {
        SceneConfig cfg;
        cfg.name = "存档场景";
        cfg.description = "最高质量，适合视频存档";
        cfg.config.preset = Preset::Slow;
        cfg.config.rateControl = RateControl::CRF;
        cfg.config.crf = 18;
        cfg.config.keyintMax = 250;
        cfg.config.bframes = 8;
        cfg.config.refs = 16;
        cfg.config.meRange = 24;
        cfg.config.weightedPred = true;
        return cfg;
    }

    // 运行场景测试
    static std::vector<TestResult> runSceneTest(
        const std::vector<SceneConfig>& scenes,
        int width = 1920,
        int height = 1080,
        int frameCount = 300
    );

    // 写入缓冲相关
    struct PacketData {
        uint8_t* data;
        int size;
        int64_t pts;
        int64_t dts;
        int stream_index;
        int flags;
        int duration;
    };

    struct WriteBuffer {
        static const size_t MAX_PACKETS = 300;  // 缓冲区最大包数量
        std::vector<PacketData> packets;
        std::mutex mutex;
        std::condition_variable cv;
        bool finished{false};
        std::thread writer_thread;
        
        WriteBuffer() : packets() {
            packets.reserve(MAX_PACKETS);
        }
    } writeBuffer_;

    // 写入线程相关
    void startWriterThread();
    void stopWriterThread();
    void writerThreadFunc();
    bool addPacketToBuffer(const AVPacket* packet);

    // 帧缓存相关
    struct FrameCache {
        bool is_initialized{false};
        bool use_disk_cache{false};
        std::string cache_dir;
        std::vector<std::vector<uint8_t>> frame_buffer;  // 内存中的帧缓存
        size_t frame_size{0};  // 每帧的大小
        size_t total_frames{0};  // 总帧数
        
        void clear() {
            frame_buffer.clear();
            is_initialized = false;
        }
    } frameCache_;

    // 帧生成和缓存相关函数
    bool initFrameCache(const TestConfig& config);
    bool generateFrames(const TestConfig& config);
    std::vector<uint8_t> generateSingleFrame(int width, int height, int frameIndex);
    const uint8_t* getFrameData(size_t frameIndex) const;

    // 帧生成相关
    struct FrameGenerationStatus {
        bool is_generating{false};
        size_t completed_frames{0};
        size_t total_frames{0};
        float last_progress{0.0f};
        std::mutex mutex;
        std::function<void(float)> progress_callback;
    } gen_status_;

    // 多线程帧生成
    void generateFramesThreaded(const TestConfig& config, size_t thread_count);
    static void frameGenerationWorker(
        X264ParamTest* self,
        const TestConfig& config,
        size_t start_frame,
        size_t end_frame
    );

private:
    // 编码器上下文
    AVCodecContext* encoderCtx_{nullptr};
    AVFrame* frame_{nullptr};
    AVPacket* packet_{nullptr};
    
    // 输出文件相关
    AVFormatContext* formatCtx_{nullptr};
    AVStream* stream_{nullptr};
    
    // 性能测试相关
    std::chrono::steady_clock::time_point startTime_;
    std::chrono::steady_clock::time_point frameStartTime_;
    int frameCount_{0};
    double encodingTime_{0.0};
    double frameTime_{0.0};
    double fps_{0.0};
    double bitrate_{0.0};
    double psnr_{0.0};
    double ssim_{0.0};

    // 性能监控
    struct PerformanceMetrics {
        double totalEncodingTime{0.0};    // 总编码时间
        double totalWritingTime{0.0};     // 总写入时间
        double totalFrameGenTime{0.0};    // 总帧生成时间
        double totalFrameCopyTime{0.0};   // 总帧复制时间
        double totalFrameTime{0.0};       // 总帧处理时间
        int totalFrames{0};               // 总帧数
        double avgEncodingTimePerFrame{0.0}; // 平均每帧编码时间
        double avgWritingTimePerFrame{0.0};  // 平均每帧写入时间
        double avgFrameGenTimePerFrame{0.0}; // 平均每帧生成时间
        double avgFrameCopyTime{0.0};        // 平均每帧复制时间
        double avgTotalTimePerFrame{0.0};    // 平均每帧总时间
        double maxEncodingTimePerFrame{0.0}; // 最大单帧编码时间
        double maxWritingTimePerFrame{0.0};  // 最大单帧写入时间
        double maxFrameGenTimePerFrame{0.0}; // 最大单帧生成时间
        double maxFrameCopyTime{0.0};        // 最大单帧复制时间
    } perfMetrics_;

    // 将预设枚举转换为字符串
    static const char* presetToString(Preset preset);
    // 将调优模式枚举转换为字符串
    static const char* tuneToString(Tune tune);
}; 