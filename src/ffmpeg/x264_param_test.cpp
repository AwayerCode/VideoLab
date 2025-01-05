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
        case Tune::None:
        default: return nullptr;
    }
}

X264ParamTest::X264ParamTest() = default;
X264ParamTest::~X264ParamTest() {
    cleanup();
}

bool X264ParamTest::initEncoder(const TestConfig& config) {
    cleanup();

    // 查找编码器
    const AVCodec* codec = avcodec_find_encoder_by_name("libx264");
    if (!codec) {
        std::cerr << "找不到x264编码器" << std::endl;
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

    // 设置x264特定参数
    av_opt_set(encoderCtx_->priv_data, "preset", presetToString(config.preset), 0);
    if (const char* tune = tuneToString(config.tune)) {
        av_opt_set(encoderCtx_->priv_data, "tune", tune, 0);
    }

    // 设置码率控制
    switch (config.rateControl) {
        case RateControl::CRF:
            av_opt_set_int(encoderCtx_->priv_data, "crf", config.crf, 0);
            break;
        case RateControl::CBR:
            encoderCtx_->bit_rate = config.bitrate;
            encoderCtx_->rc_max_rate = config.bitrate;
            encoderCtx_->rc_min_rate = config.bitrate;
            encoderCtx_->rc_buffer_size = config.bitrate / 2;
            break;
        case RateControl::ABR:
            encoderCtx_->bit_rate = config.bitrate;
            break;
        case RateControl::CQP:
            encoderCtx_->global_quality = config.qp * FF_QP2LAMBDA;
            encoderCtx_->flags |= AV_CODEC_FLAG_QSCALE;
            break;
    }

    // 设置GOP参数
    encoderCtx_->gop_size = config.keyintMax;
    encoderCtx_->max_b_frames = config.bframes;
    encoderCtx_->refs = config.refs;

    // 设置其他质量参数
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", config.meRange);
    av_opt_set(encoderCtx_->priv_data, "me_range", buf, 0);
    av_opt_set(encoderCtx_->priv_data, "weightp", config.weightedPred ? "1" : "0", 0);
    if (!config.cabac) {
        encoderCtx_->flags &= ~AV_CODEC_FLAG_LOOP_FILTER;
    }

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

bool X264ParamTest::encodeFrame(const uint8_t* data, int size) {
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

    X264ParamTest test;
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

std::vector<X264ParamTest::TestResult> X264ParamTest::runSceneTest(
    const std::vector<SceneConfig>& scenes,
    int width,
    int height,
    int frameCount
) {
    std::vector<TestResult> results;
    results.reserve(scenes.size());

    for (const auto& scene : scenes) {
        std::cout << "\n测试场景: " << scene.name << std::endl;
        std::cout << "场景描述: " << scene.description << std::endl;

        // 使用场景配置，但更新分辨率和帧数
        auto config = scene.config;
        config.width = width;
        config.height = height;
        config.frameCount = frameCount;

        // 运行测试
        auto result = runTest(config, [](int progress, const TestResult& current) {
            std::cout << "进度: " << progress << "% - "
                     << "当前速度: " << current.fps << " fps" << std::endl;
        });

        results.push_back(result);
    }

    return results;
} 