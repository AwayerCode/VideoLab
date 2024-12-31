#include "x264_param_test.hpp"
#include <iostream>
#include <iomanip>
#include <random>
#include <sstream>

const char* X264ParamTest::presetToString(Preset preset) {
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

const char* X264ParamTest::tuneToString(Tune tune) {
    switch (tune) {
        case Tune::Film: return "film";
        case Tune::Animation: return "animation";
        case Tune::Grain: return "grain";
        case Tune::StillImage: return "stillimage";
        case Tune::PSNR: return "psnr";
        case Tune::SSIM: return "ssim";
        case Tune::FastDecode: return "fastdecode";
        case Tune::ZeroLatency: return "zerolatency";
        default: return nullptr;
    }
}

X264ParamTest::X264ParamTest() {}

X264ParamTest::~X264ParamTest() {
    cleanup();
}

bool X264ParamTest::initEncoder(const TestConfig& config) {
    cleanup();

    // 查找x264编码器
    const AVCodec* codec = avcodec_find_encoder_by_name("libx264");
    if (!codec) {
        std::cerr << "x264编码器未找到" << std::endl;
        return false;
    }

    // 创建编码器上下文
    encoderCtx_ = avcodec_alloc_context3(codec);
    if (!encoderCtx_) {
        std::cerr << "无法创建编码器上下文" << std::endl;
        return false;
    }

    // 设置基本编码参数
    encoderCtx_->width = config.width;
    encoderCtx_->height = config.height;
    encoderCtx_->time_base = AVRational{1, config.fps};
    encoderCtx_->framerate = AVRational{config.fps, 1};
    encoderCtx_->pix_fmt = AV_PIX_FMT_YUV420P;
    encoderCtx_->thread_count = config.threads;
    encoderCtx_->thread_type = FF_THREAD_FRAME | FF_THREAD_SLICE;

    // 设置x264特定参数
    av_opt_set(encoderCtx_->priv_data, "preset", presetToString(config.preset), 0);
    if (const char* tune = tuneToString(config.tune)) {
        av_opt_set(encoderCtx_->priv_data, "tune", tune, 0);
    }

    // 设置码率控制参数
    switch (config.rateControl) {
        case RateControl::CRF:
            av_opt_set(encoderCtx_->priv_data, "crf", std::to_string(config.crf).c_str(), 0);
            break;
        case RateControl::CQP:
            encoderCtx_->global_quality = config.qp * FF_QP2LAMBDA;
            encoderCtx_->flags |= AV_CODEC_FLAG_QSCALE;
            break;
        case RateControl::ABR:
            encoderCtx_->bit_rate = config.bitrate;
            break;
        case RateControl::CBR:
            encoderCtx_->bit_rate = config.bitrate;
            encoderCtx_->rc_max_rate = config.bitrate;
            encoderCtx_->rc_min_rate = config.bitrate;
            encoderCtx_->rc_buffer_size = config.bitrate / 2;
            break;
    }

    // 设置GOP参数
    encoderCtx_->gop_size = config.keyintMax;
    encoderCtx_->max_b_frames = config.bframes;
    encoderCtx_->refs = config.refs;

    // 设置其他质量参数
    std::string x264params = "";
    x264params += config.weightedPred ? ":weightb=1" : ":weightb=0";
    x264params += config.cabac ? ":cabac=1" : ":cabac=0";
    x264params += ":me_range=" + std::to_string(config.meRange);
    if (!x264params.empty() && x264params[0] == ':') {
        x264params = x264params.substr(1);
    }
    if (!x264params.empty()) {
        av_opt_set(encoderCtx_->priv_data, "x264opts", x264params.c_str(), 0);
    }

    // 打开编码器
    int ret = avcodec_open2(encoderCtx_, codec, nullptr);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
        std::cerr << "无法打开编码器: " << errbuf << std::endl;
        return false;
    }

    // 分配帧和数据包
    frame_ = av_frame_alloc();
    packet_ = av_packet_alloc();

    if (!frame_ || !packet_) {
        std::cerr << "无法分配帧或数据包" << std::endl;
        return false;
    }

    frame_->format = AV_PIX_FMT_YUV420P;
    frame_->width = config.width;
    frame_->height = config.height;

    ret = av_frame_get_buffer(frame_, 32);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
        std::cerr << "无法分配帧缓冲: " << errbuf << std::endl;
        return false;
    }

    startTime_ = std::chrono::steady_clock::now();
    return true;
}

bool X264ParamTest::encodeFrame(const uint8_t* data, int size) {
    auto now = std::chrono::steady_clock::now();
    
    // 复制输入数据到AVFrame
    int ret = av_image_fill_arrays(frame_->data, frame_->linesize, data,
                        AV_PIX_FMT_YUV420P,
                        frame_->width, frame_->height, 1);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
        std::cerr << "无法填充图像数据: " << errbuf << std::endl;
        return false;
    }

    frame_->pts = frameCount_++;

    // 编码
    ret = avcodec_send_frame(encoderCtx_, frame_);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
        std::cerr << "发送帧到编码器失败: " << errbuf << std::endl;
        return false;
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(encoderCtx_, packet_);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            char errbuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
            std::cerr << "从编码器接收数据包失败: " << errbuf << std::endl;
            return false;
        }

        // 更新码率统计
        bitrate_ = (bitrate_ * (frameCount_ - 1) + packet_->size * 8.0 * encoderCtx_->time_base.den / encoderCtx_->time_base.num) / frameCount_;

        av_packet_unref(packet_);
    }

    // 更新性能统计
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - startTime_).count() / 1000.0;
    encodingTime_ = duration;
    fps_ = frameCount_ / duration;

    return true;
}

void X264ParamTest::cleanup() {
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

X264ParamTest::TestResult X264ParamTest::runTest(
    const TestConfig& config,
    std::function<void(int, const TestResult&)> progressCallback
) {
    TestResult result;
    result.success = false;
    
    try {
        // 创建测试数据
        const int frameSize = config.width * config.height * 3 / 2;  // YUV420P size
        std::vector<uint8_t> frameData(frameSize);
        
        // 用随机数据填充
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        for (auto& byte : frameData) {
            byte = static_cast<uint8_t>(dis(gen));
        }

        // 创建编码器实例
        X264ParamTest encoder;
        if (!encoder.initEncoder(config)) {
            result.errorMessage = "编码器初始化失败";
            return result;
        }

        // 编码测试
        for (int i = 0; i < config.frameCount; ++i) {
            if (!encoder.encodeFrame(frameData.data(), frameSize)) {
                result.errorMessage = "编码失败";
                return result;
            }

            // 每编码一秒或完成所有帧时，调用进度回调
            if (progressCallback && (i % config.fps == 0 || i == config.frameCount - 1)) {
                result.encodingTime = encoder.getEncodingTime();
                result.fps = encoder.getFPS();
                result.bitrate = encoder.getBitrate();
                result.psnr = encoder.getPSNR();
                result.ssim = encoder.getSSIM();
                result.success = true;
                progressCallback(i + 1, result);
            }
        }

        // 设置最终结果
        result.encodingTime = encoder.getEncodingTime();
        result.fps = encoder.getFPS();
        result.bitrate = encoder.getBitrate();
        result.psnr = encoder.getPSNR();
        result.ssim = encoder.getSSIM();
        result.success = true;

    } catch (const std::exception& e) {
        result.errorMessage = std::string("测试过程发生异常: ") + e.what();
        return result;
    }

    return result;
}

std::vector<X264ParamTest::TestResult> X264ParamTest::runPresetTest(
    int width, int height, int frameCount
) {
    std::vector<TestResult> results;
    std::vector<Preset> presets = {
        Preset::UltraFast, Preset::SuperFast, Preset::VeryFast,
        Preset::Faster, Preset::Fast, Preset::Medium,
        Preset::Slow, Preset::Slower, Preset::VerySlow
    };

    std::cout << "\n开始x264预设测试...\n" << std::endl;
    std::cout << "测试参数：" << std::endl;
    std::cout << "分辨率: " << width << "x" << height << std::endl;
    std::cout << "帧数: " << frameCount << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    for (auto preset : presets) {
        std::cout << "\n测试预设: " << presetToString(preset) << std::endl;
        
        TestConfig config;
        config.width = width;
        config.height = height;
        config.frameCount = frameCount;
        config.preset = preset;

        auto result = runTest(config, [](int frame, const TestResult& progress) {
            std::cout << "\r已编码: " << std::setw(4) << frame << " 帧"
                     << " | FPS: " << std::fixed << std::setprecision(2) << progress.fps
                     << " | 码率: " << std::setw(8) << static_cast<int>(progress.bitrate / 1000) << " Kbps"
                     << std::flush;
        });

        if (result.success) {
            std::cout << "\n结果:" << std::endl;
            std::cout << "编码速度: " << result.fps << " fps" << std::endl;
            std::cout << "平均码率: " << static_cast<int>(result.bitrate / 1000) << " Kbps" << std::endl;
            if (result.psnr > 0) std::cout << "PSNR: " << result.psnr << " dB" << std::endl;
            if (result.ssim > 0) std::cout << "SSIM: " << result.ssim << std::endl;
        } else {
            std::cout << "\n测试失败: " << result.errorMessage << std::endl;
        }

        results.push_back(result);
    }

    return results;
}

std::vector<X264ParamTest::TestResult> X264ParamTest::runRateControlTest(
    RateControl mode,
    std::vector<int> values,
    int width, int height, int frameCount
) {
    std::vector<TestResult> results;

    std::cout << "\n开始x264码率控制测试...\n" << std::endl;
    std::cout << "测试参数：" << std::endl;
    std::cout << "模式: " << (mode == RateControl::CRF ? "CRF" :
                           mode == RateControl::CQP ? "CQP" :
                           mode == RateControl::ABR ? "ABR" : "CBR") << std::endl;
    std::cout << "分辨率: " << width << "x" << height << std::endl;
    std::cout << "帧数: " << frameCount << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    for (int value : values) {
        std::cout << "\n测试值: " << value << std::endl;
        
        TestConfig config;
        config.width = width;
        config.height = height;
        config.frameCount = frameCount;
        config.rateControl = mode;
        
        switch (mode) {
            case RateControl::CRF: config.crf = value; break;
            case RateControl::CQP: config.qp = value; break;
            case RateControl::ABR:
            case RateControl::CBR: config.bitrate = value; break;
        }

        auto result = runTest(config, [](int frame, const TestResult& progress) {
            std::cout << "\r已编码: " << std::setw(4) << frame << " 帧"
                     << " | FPS: " << std::fixed << std::setprecision(2) << progress.fps
                     << " | 码率: " << std::setw(8) << static_cast<int>(progress.bitrate / 1000) << " Kbps"
                     << std::flush;
        });

        if (result.success) {
            std::cout << "\n结果:" << std::endl;
            std::cout << "编码速度: " << result.fps << " fps" << std::endl;
            std::cout << "平均码率: " << static_cast<int>(result.bitrate / 1000) << " Kbps" << std::endl;
            if (result.psnr > 0) std::cout << "PSNR: " << result.psnr << " dB" << std::endl;
            if (result.ssim > 0) std::cout << "SSIM: " << result.ssim << std::endl;
        } else {
            std::cout << "\n测试失败: " << result.errorMessage << std::endl;
        }

        results.push_back(result);
    }

    return results;
}

std::vector<X264ParamTest::TestResult> X264ParamTest::runQualityTest(
    const TestConfig& baseConfig,
    std::vector<std::pair<std::string, std::vector<bool>>> params
) {
    std::vector<TestResult> results;

    std::cout << "\n开始x264质量参数测试...\n" << std::endl;
    std::cout << "基础配置：" << std::endl;
    std::cout << "分辨率: " << baseConfig.width << "x" << baseConfig.height << std::endl;
    std::cout << "预设: " << presetToString(baseConfig.preset) << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    // 生成所有参数组合
    int totalCombinations = 1;
    for (const auto& param : params) {
        totalCombinations *= param.second.size();
    }

    int combination = 0;
    std::vector<size_t> indices(params.size(), 0);
    bool done = false;

    while (!done) {
        TestConfig config = baseConfig;
        std::stringstream paramDesc;
        
        // 设置当前组合的参数
        for (size_t i = 0; i < params.size(); ++i) {
            bool value = params[i].second[indices[i]];
            if (params[i].first == "weightedPred") {
                config.weightedPred = value;
                paramDesc << "weightedPred=" << value << " ";
            } else if (params[i].first == "cabac") {
                config.cabac = value;
                paramDesc << "cabac=" << value << " ";
            }
            // 添加其他参数...
        }

        std::cout << "\n测试组合 " << ++combination << "/" << totalCombinations << ": "
                 << paramDesc.str() << std::endl;

        auto result = runTest(config, [](int frame, const TestResult& progress) {
            std::cout << "\r已编码: " << std::setw(4) << frame << " 帧"
                     << " | FPS: " << std::fixed << std::setprecision(2) << progress.fps
                     << " | 码率: " << std::setw(8) << static_cast<int>(progress.bitrate / 1000) << " Kbps"
                     << std::flush;
        });

        if (result.success) {
            std::cout << "\n结果:" << std::endl;
            std::cout << "编码速度: " << result.fps << " fps" << std::endl;
            std::cout << "平均码率: " << static_cast<int>(result.bitrate / 1000) << " Kbps" << std::endl;
            if (result.psnr > 0) std::cout << "PSNR: " << result.psnr << " dB" << std::endl;
            if (result.ssim > 0) std::cout << "SSIM: " << result.ssim << std::endl;
        } else {
            std::cout << "\n测试失败: " << result.errorMessage << std::endl;
        }

        results.push_back(result);

        // 更新索引
        for (size_t i = 0; i < indices.size(); ++i) {
            indices[i]++;
            if (indices[i] < params[i].second.size()) {
                break;
            }
            indices[i] = 0;
            if (i == indices.size() - 1) {
                done = true;
            }
        }
    }

    return results;
} 