#include "x265_param_test.hpp"
#include <iostream>
#include <iomanip>
#include <random>
#include <sstream>

const char* X265ParamTest::presetToString(Preset preset) {
    switch (preset) {
        case Preset::UltraFast: return "ultrafast";
        case Preset::SuperFast: return "superfast";
        case Preset::VeryFast: return "veryfast";
        case Preset::Faster: return "faster";
        case Preset::Fast: return "fast";
        case Preset::Medium: return "medium";
        case Preset::Slow: return "slow";
        case Preset::Slower: return "slower";
        case Preset::VerySlow: return "veryslow";
        case Preset::Placebo: return "placebo";
        default: return "medium";
    }
}

const char* X265ParamTest::tuneToString(Tune tune) {
    switch (tune) {
        case Tune::PSNR: return "psnr";
        case Tune::SSIM: return "ssim";
        case Tune::Grain: return "grain";
        case Tune::ZeroLatency: return "zerolatency";
        case Tune::FastDecode: return "fastdecode";
        case Tune::Animation: return "animation";
        case Tune::None:
        default: return nullptr;
    }
}

X265ParamTest::X265ParamTest() = default;
X265ParamTest::~X265ParamTest() {
    cleanup();
}

bool X265ParamTest::initEncoder(const TestConfig& config) {
    cleanup();

    // 查找编码器
    const AVCodec* codec = avcodec_find_encoder_by_name("libx265");
    if (!codec) {
        std::cerr << "找不到x265编码器" << std::endl;
        return false;
    }

    // 创建编码器上下文
    encoderCtx_ = avcodec_alloc_context3(codec);
    if (!encoderCtx_) {
        std::cerr << "无法创建编码器上下文" << std::endl;
        return false;
    }

    // 设置基本参数
    encoderCtx_->width = config.width;
    encoderCtx_->height = config.height;
    encoderCtx_->time_base = AVRational{1, config.fps};
    encoderCtx_->framerate = AVRational{config.fps, 1};
    encoderCtx_->pix_fmt = AV_PIX_FMT_YUV420P;
    encoderCtx_->thread_count = config.threads;

    // 设置x265特定参数
    av_opt_set(encoderCtx_->priv_data, "preset", presetToString(config.preset), 0);
    if (const char* tune = tuneToString(config.tune)) {
        av_opt_set(encoderCtx_->priv_data, "tune", tune, 0);
    }

    // 设置码率控制参数
    switch (config.rateControl) {
        case RateControl::CRF:
            av_opt_set_int(encoderCtx_->priv_data, "crf", config.crf, 0);
            break;
        case RateControl::CQP:
            av_opt_set_int(encoderCtx_->priv_data, "qp", config.qp, 0);
            break;
        case RateControl::ABR:
            encoderCtx_->bit_rate = config.bitrate * 1000;
            break;
        case RateControl::CBR:
            encoderCtx_->bit_rate = config.bitrate * 1000;
            encoderCtx_->rc_max_rate = config.bitrate * 1000;
            encoderCtx_->rc_min_rate = config.bitrate * 1000;
            encoderCtx_->rc_buffer_size = config.bitrate * 1000;
            break;
    }

    // 设置x265特有参数
    std::string x265_params;
    if (!config.bFrames) {
        x265_params += "bframes=0:";
    } else {
        x265_params += "bframes=" + std::to_string(config.bFrameCount) + ":";
    }
    x265_params += "weightp=" + std::to_string(config.weightedPred ? 1 : 0) + ":";
    x265_params += "keyint=" + std::to_string(config.keyintMax) + ":";
    x265_params += "ref=" + std::to_string(config.refFrames) + ":";
    x265_params += "aq-mode=" + std::to_string(config.aqMode ? 1 : 0) + ":";
    x265_params += "aq-strength=" + std::to_string(config.aqStrength) + ":";
    x265_params += "psy-rd=" + std::to_string(config.psyRd ? config.psyRdStrength : 0.0);

    av_opt_set(encoderCtx_->priv_data, "x265-params", x265_params.c_str(), 0);

    // 打开编码器
    int ret = avcodec_open2(encoderCtx_, codec, nullptr);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, sizeof(errbuf));
        std::cerr << "无法打开编码器: " << errbuf << std::endl;
        return false;
    }

    // 分配帧和包
    frame_ = av_frame_alloc();
    packet_ = av_packet_alloc();
    if (!frame_ || !packet_) {
        std::cerr << "无法分配帧或包" << std::endl;
        return false;
    }

    frame_->format = encoderCtx_->pix_fmt;
    frame_->width = encoderCtx_->width;
    frame_->height = encoderCtx_->height;

    ret = av_frame_get_buffer(frame_, 0);
    if (ret < 0) {
        std::cerr << "无法分配帧缓冲区" << std::endl;
        return false;
    }

    startTime_ = std::chrono::steady_clock::now();
    frameCount_ = 0;
    return true;
}

bool X265ParamTest::encodeFrame(const uint8_t* data, int size) {
    if (!encoderCtx_ || !frame_ || !packet_) {
        return false;
    }

    // 复制输入数据到帧
    av_frame_make_writable(frame_);
    for (int i = 0; i < frame_->height; i++) {
        memcpy(frame_->data[0] + i * frame_->linesize[0],
               data + i * frame_->width,
               frame_->width);
    }
    // 复制U和V平面（假设YUV420P格式）
    int uvHeight = frame_->height / 2;
    int uvWidth = frame_->width / 2;
    uint8_t* uData = const_cast<uint8_t*>(data + frame_->width * frame_->height);
    uint8_t* vData = const_cast<uint8_t*>(uData + uvWidth * uvHeight);

    for (int i = 0; i < uvHeight; i++) {
        memcpy(frame_->data[1] + i * frame_->linesize[1],
               uData + i * uvWidth,
               uvWidth);
        memcpy(frame_->data[2] + i * frame_->linesize[2],
               vData + i * uvWidth,
               uvWidth);
    }

    frame_->pts = frameCount_++;

    // 编码帧
    int ret = avcodec_send_frame(encoderCtx_, frame_);
    if (ret < 0) {
        return false;
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(encoderCtx_, packet_);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            return false;
        }

        bitrate_ = (bitrate_ * (frameCount_ - 1) + packet_->size * 8.0 * encoderCtx_->time_base.den / encoderCtx_->time_base.num) / frameCount_;
        av_packet_unref(packet_);
    }

    auto now = std::chrono::steady_clock::now();
    encodingTime_ = std::chrono::duration<double>(now - startTime_).count();
    fps_ = frameCount_ / encodingTime_;

    return true;
}

void X265ParamTest::cleanup() {
    if (encoderCtx_) {
        avcodec_free_context(&encoderCtx_);
    }
    if (frame_) {
        av_frame_free(&frame_);
    }
    if (packet_) {
        av_packet_free(&packet_);
    }
}

X265ParamTest::TestResult X265ParamTest::runTest(
    const TestConfig& config,
    std::function<void(int, const TestResult&)> progressCallback
) {
    TestResult result;
    result.success = false;

    X265ParamTest test;
    if (!test.initEncoder(config)) {
        result.errorMessage = "初始化编码器失败";
        return result;
    }

    // 生成测试数据（灰阶渐变）
    std::vector<uint8_t> testData(config.width * config.height * 3 / 2);  // YUV420P
    for (int i = 0; i < config.height; i++) {
        for (int j = 0; j < config.width; j++) {
            testData[i * config.width + j] = (i + j) % 256;  // Y
        }
    }
    // 填充U和V平面
    int uvSize = config.width * config.height / 4;
    std::fill_n(testData.begin() + config.width * config.height, uvSize, 128);      // U
    std::fill_n(testData.begin() + config.width * config.height + uvSize, uvSize, 128);  // V

    // 编码所有帧
    for (int i = 0; i < config.frameCount; i++) {
        if (!test.encodeFrame(testData.data(), testData.size())) {
            result.errorMessage = "编码帧失败";
            return result;
        }

        if (progressCallback && (i % 30 == 0 || i == config.frameCount - 1)) {
            TestResult current;
            current.fps = test.getFPS();
            current.bitrate = test.getBitrate();
            progressCallback((i + 1) * 100 / config.frameCount, current);
        }
    }

    result.success = true;
    result.encodingTime = test.getEncodingTime();
    result.fps = test.getFPS();
    result.bitrate = test.getBitrate();
    result.psnr = test.getPSNR();
    result.ssim = test.getSSIM();

    return result;
}

std::vector<X265ParamTest::TestResult> X265ParamTest::runPresetTest(
    int width, int height, int frameCount, int fps
) {
    std::vector<TestResult> results;
    std::vector<Preset> presets = {
        Preset::UltraFast, Preset::SuperFast, Preset::VeryFast,
        Preset::Faster, Preset::Fast, Preset::Medium,
        Preset::Slow, Preset::Slower, Preset::VerySlow
    };

    std::cout << "\n开始x265预设测试...\n" << std::endl;
    std::cout << "测试参数：" << std::endl;
    std::cout << "分辨率: " << width << "x" << height << std::endl;
    std::cout << "帧数: " << frameCount << std::endl;
    std::cout << "帧率: " << fps << " fps" << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    for (auto preset : presets) {
        std::cout << "\n测试预设: " << presetToString(preset) << std::endl;
        
        TestConfig config;
        config.width = width;
        config.height = height;
        config.frameCount = frameCount;
        config.preset = preset;
        config.fps = fps;  // 使用传入的fps参数

        auto result = runTest(config, [](int frame, const TestResult& progress) {
            std::cout << "\r已编码: " << std::setw(4) << frame << " 帧"
                     << " | FPS: " << std::fixed << std::setprecision(2) << progress.fps
                     << " | 码率: " << std::setw(8) << static_cast<int>(progress.bitrate / 1000) << " Kbps"
                     << std::flush;
        });

        if (result.success) {
            results.push_back(result);
        }
        std::cout << std::endl;
    }

    return results;
}

std::vector<X265ParamTest::TestResult> X265ParamTest::runRateControlTest(
    RateControl mode,
    const std::vector<int>& values,
    int width, int height, int frameCount, int fps
) {
    std::vector<TestResult> results;

    std::cout << "\n开始x265码率控制测试...\n" << std::endl;
    std::cout << "测试参数：" << std::endl;
    std::cout << "分辨率: " << width << "x" << height << std::endl;
    std::cout << "帧数: " << frameCount << std::endl;
    std::cout << "帧率: " << fps << " fps" << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    for (int value : values) {
        TestConfig config;
        config.width = width;
        config.height = height;
        config.frameCount = frameCount;
        config.rateControl = mode;
        config.fps = fps;  // 使用传入的fps参数

        switch (mode) {
            case RateControl::CRF:
                config.crf = value;
                std::cout << "\n测试CRF: " << value << std::endl;
                break;
            case RateControl::CQP:
                config.qp = value;
                std::cout << "\n测试QP: " << value << std::endl;
                break;
            case RateControl::ABR:
            case RateControl::CBR:
                config.bitrate = value;
                std::cout << "\n测试码率: " << value << " kbps" << std::endl;
                break;
        }

        auto result = runTest(config, [](int frame, const TestResult& progress) {
            std::cout << "\r已编码: " << std::setw(4) << frame << " 帧"
                     << " | FPS: " << std::fixed << std::setprecision(2) << progress.fps
                     << " | 码率: " << std::setw(8) << static_cast<int>(progress.bitrate / 1000) << " Kbps"
                     << std::flush;
        });

        if (result.success) {
            results.push_back(result);
        }
        std::cout << std::endl;
    }

    return results;
} 