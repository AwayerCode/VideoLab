#include "x264_param_test.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <cmath>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
}

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

bool X264ParamTest::initEncoder(const TestConfig& config, const std::string& outputFile) {
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
        std::cerr << "无法分配编码器上下文" << std::endl;
        return false;
    }

    // 设置基本参数
    encoderCtx_->width = config.width;
    encoderCtx_->height = config.height;
    encoderCtx_->time_base = AVRational{1, 25}; // 固定帧率为25fps
    encoderCtx_->framerate = AVRational{25, 1};
    encoderCtx_->pix_fmt = AV_PIX_FMT_YUV420P;
    encoderCtx_->thread_count = config.threads;
    encoderCtx_->codec_type = AVMEDIA_TYPE_VIDEO;

    // 设置码率控制
    switch (config.rateControl) {
        case RateControl::CRF:
            av_opt_set_int(encoderCtx_->priv_data, "crf", config.crf, 0);
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

    // 设置预设和调优
    if (av_opt_set(encoderCtx_->priv_data, "preset", presetToString(config.preset), 0) < 0) {
        std::cerr << "设置预设失败" << std::endl;
    }
    if (config.tune != Tune::None) {
        if (av_opt_set(encoderCtx_->priv_data, "tune", tuneToString(config.tune), 0) < 0) {
            std::cerr << "设置调优失败" << std::endl;
        }
    }

    // 设置GOP参数
    encoderCtx_->gop_size = config.keyintMax;
    encoderCtx_->max_b_frames = config.bframes;
    encoderCtx_->refs = config.refs;

    // 设置其他质量参数
    if (av_opt_set_int(encoderCtx_->priv_data, "me_range", config.meRange, 0) < 0) {
        std::cerr << "设置me_range失败" << std::endl;
    }
    if (av_opt_set_int(encoderCtx_->priv_data, "weightp", config.weightedPred ? 1 : 0, 0) < 0) {
        std::cerr << "设置weightp失败" << std::endl;
    }
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

    // 如果指定了输出文件，创建输出上下文
    if (!outputFile.empty()) {
        formatCtx_ = avformat_alloc_context();
        if (!formatCtx_) {
            std::cerr << "无法分配格式上下文" << std::endl;
            return false;
        }

        // 根据文件名推断格式
        formatCtx_->oformat = av_guess_format(nullptr, outputFile.c_str(), nullptr);
        if (!formatCtx_->oformat) {
            std::cerr << "无法推断输出格式" << std::endl;
            return false;
        }

        // 打开输出文件
        if (avio_open(&formatCtx_->pb, outputFile.c_str(), AVIO_FLAG_WRITE) < 0) {
            std::cerr << "无法打开输出文件: " << outputFile << std::endl;
            return false;
        }

        // 创建视频流
        stream_ = avformat_new_stream(formatCtx_, codec);
        if (!stream_) {
            std::cerr << "无法创建视频流" << std::endl;
            return false;
        }

        // 复制编码器参数到流
        if (avcodec_parameters_from_context(stream_->codecpar, encoderCtx_) < 0) {
            std::cerr << "无法复制编码器参数到流" << std::endl;
            return false;
        }

        // 写入文件头
        if (avformat_write_header(formatCtx_, nullptr) < 0) {
            std::cerr << "无法写入文件头" << std::endl;
            return false;
        }
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
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, sizeof(errbuf));
        std::cerr << "无法分配帧缓冲区: " << errbuf << std::endl;
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

    auto frameStart = std::chrono::steady_clock::now();
    frameStartTime_ = frameStart;

    // 如果data为空，表示刷新编码器
    if (!data) {
        int ret = avcodec_send_frame(encoderCtx_, nullptr);
        if (ret < 0) {
            return false;
        }
    } else {
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

        // 发送帧进行编码
        auto encodeStart = std::chrono::steady_clock::now();
        int ret = avcodec_send_frame(encoderCtx_, frame_);
        if (ret < 0) {
            return false;
        }
    }

    // 接收编码后的包
    bool gotPacket = false;
    auto encodeStart = std::chrono::steady_clock::now();
    double frameEncodeTime = 0.0;
    double frameWriteTime = 0.0;

    while (true) {
        int ret = avcodec_receive_packet(encoderCtx_, packet_);
        if (ret == AVERROR(EAGAIN)) {
            break;  // 需要更多输入
        } else if (ret == AVERROR_EOF) {
            if (!data) {  // 如果是在刷新阶段收到EOF，这是正常的
                break;
            }
            return false;  // 在正常编码过程中收到EOF是错误的
        } else if (ret < 0) {
            return false;
        }

        gotPacket = true;
        auto encodeEnd = std::chrono::steady_clock::now();
        frameEncodeTime = std::chrono::duration<double>(encodeEnd - encodeStart).count();

        bitrate_ = (bitrate_ * (frameCount_ - 1) + packet_->size * 8.0 * encoderCtx_->time_base.den / encoderCtx_->time_base.num) / frameCount_;

        // 如果有输出文件，写入数据包
        if (formatCtx_) {
            auto writeStart = std::chrono::steady_clock::now();
            
            // 转换时间戳
            av_packet_rescale_ts(packet_, encoderCtx_->time_base, stream_->time_base);
            ret = av_interleaved_write_frame(formatCtx_, packet_);
            
            auto writeEnd = std::chrono::steady_clock::now();
            frameWriteTime = std::chrono::duration<double>(writeEnd - writeStart).count();

            if (ret < 0) {
                return false;
            }
        }

        // 更新性能指标
        perfMetrics_.totalEncodingTime += frameEncodeTime;
        perfMetrics_.totalWritingTime += frameWriteTime;
        perfMetrics_.totalFrames++;
        
        perfMetrics_.avgEncodingTimePerFrame = perfMetrics_.totalEncodingTime / perfMetrics_.totalFrames;
        perfMetrics_.avgWritingTimePerFrame = perfMetrics_.totalWritingTime / perfMetrics_.totalFrames;
        
        perfMetrics_.maxEncodingTimePerFrame = std::max(perfMetrics_.maxEncodingTimePerFrame, frameEncodeTime);
        perfMetrics_.maxWritingTimePerFrame = std::max(perfMetrics_.maxWritingTimePerFrame, frameWriteTime);

        av_packet_unref(packet_);
    }

    auto now = std::chrono::steady_clock::now();
    frameTime_ = std::chrono::duration<double>(now - frameStartTime_).count();
    encodingTime_ = std::chrono::duration<double>(now - startTime_).count();
    fps_ = frameCount_ / encodingTime_;

    // 每100帧输出一次性能统计
    if (frameCount_ % 100 == 0) {
        std::cout << "\n性能统计 (帧 " << frameCount_ << "):" << std::endl;
        std::cout << "平均编码时间/帧: " << std::fixed << std::setprecision(3) 
                  << perfMetrics_.avgEncodingTimePerFrame * 1000 << " ms" << std::endl;
        std::cout << "平均写入时间/帧: " << std::fixed << std::setprecision(3) 
                  << perfMetrics_.avgWritingTimePerFrame * 1000 << " ms" << std::endl;
        std::cout << "最大编码时间/帧: " << std::fixed << std::setprecision(3) 
                  << perfMetrics_.maxEncodingTimePerFrame * 1000 << " ms" << std::endl;
        std::cout << "最大写入时间/帧: " << std::fixed << std::setprecision(3) 
                  << perfMetrics_.maxWritingTimePerFrame * 1000 << " ms" << std::endl;
        std::cout << "当前编码速度: " << std::fixed << std::setprecision(1) 
                  << fps_ << " fps" << std::endl;
    }

    return true;
}

void X264ParamTest::cleanup() {
    if (formatCtx_) {
        if (formatCtx_->pb) {
            avio_closep(&formatCtx_->pb);
        }
        avformat_free_context(formatCtx_);
        formatCtx_ = nullptr;
    }
    if (encoderCtx_) {
        avcodec_free_context(&encoderCtx_);
    }
    if (frame_) {
        av_frame_free(&frame_);
    }
    if (packet_) {
        av_packet_free(&packet_);
    }
    stream_ = nullptr;
}

X264ParamTest::TestResult X264ParamTest::runTest(
    const TestConfig& config,
    std::function<void(int, const TestResult&)> progressCallback
) {
    TestResult result;
    result.success = false;

    // 创建输出文件名，使用绝对路径
    std::string outputFile = std::string(getenv("HOME")) + "/output_" + 
        std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + ".mp4";

    X264ParamTest test;
    if (!test.initEncoder(config, outputFile)) {
        result.errorMessage = "初始化编码器失败";
        return result;
    }

    // 生成测试数据（彩色渐变和动态图案）
    std::vector<uint8_t> testData(config.width * config.height * 3 / 2);  // YUV420P
    
    // 编码所有帧
    for (int i = 0; i < config.frameCount; i++) {
        auto frameGenStart = std::chrono::steady_clock::now();
        
        // 生成Y平面（亮度）- 创建一个动态的图案
        for (int y = 0; y < config.height; y++) {
            for (int x = 0; x < config.width; x++) {
                // 创建动态图案
                float time = i * 0.1f;  // 时间因子
                
                // 移动的条纹
                int stripes = (
                    ((y / 32 + i) % 2) * 128 +      // 水平移动条纹
                    ((x / 32 + i) % 2) * 128        // 垂直移动条纹
                ) / 2;
                
                // 扩散的圆形
                int centerX = config.width / 2;
                int centerY = config.height / 2;
                int dx = x - centerX;
                int dy = y - centerY;
                float distance = sqrt(dx * dx + dy * dy);
                float circle_radius = (100.0f + 50.0f * sin(time));  // 脉动的圆形
                int circle = (distance < circle_radius) ? 255 : 0;
                
                // 旋转的图案
                float angle = atan2(dy, dx);
                float rotation = angle + time;
                int spiral = static_cast<int>(128.0f + 127.0f * sin(rotation * 4.0f + distance * 0.02f));
                
                // 组合所有效果
                testData[y * config.width + x] = static_cast<uint8_t>(
                    (stripes + circle + spiral) / 3
                );
            }
        }

        // 生成U和V平面（色度）- 创建动态的彩色效果
        int uvWidth = config.width / 2;
        int uvHeight = config.height / 2;
        uint8_t* uPlane = testData.data() + config.width * config.height;
        uint8_t* vPlane = uPlane + uvWidth * uvHeight;

        float time = i * 0.1f;  // 时间因子
        for (int y = 0; y < uvHeight; y++) {
            for (int x = 0; x < uvWidth; x++) {
                // 创建动态的彩色渐变
                float dx = (x - uvWidth/2) / float(uvWidth/2);
                float dy = (y - uvHeight/2) / float(uvHeight/2);
                float angle = atan2(dy, dx);
                float dist = sqrt(dx * dx + dy * dy);
                
                // 旋转的色彩
                float colorPhase = angle + time;
                float distortion = sin(dist * 5.0f - time * 2.0f) * 0.5f;
                
                // U分量 - 动态蓝色分量
                uPlane[y * uvWidth + x] = static_cast<uint8_t>(
                    128 + 127 * sin(colorPhase + distortion)
                );
                
                // V分量 - 动态红色分量
                vPlane[y * uvWidth + x] = static_cast<uint8_t>(
                    128 + 127 * cos(colorPhase - distortion)
                );
            }
        }

        auto frameGenEnd = std::chrono::steady_clock::now();
        double frameGenTime = std::chrono::duration<double>(frameGenEnd - frameGenStart).count();
        
        // 更新帧生成性能指标
        test.perfMetrics_.totalFrameGenTime += frameGenTime;
        test.perfMetrics_.avgFrameGenTimePerFrame = test.perfMetrics_.totalFrameGenTime / (i + 1);
        test.perfMetrics_.maxFrameGenTimePerFrame = std::max(
            test.perfMetrics_.maxFrameGenTimePerFrame, frameGenTime);

        if (!test.encodeFrame(testData.data(), testData.size())) {
            result.errorMessage = "编码帧失败";
            return result;
        }

        if (progressCallback) {
            TestResult current;
            current.encodingTime = test.frameTime_;
            current.fps = test.getFPS();
            current.bitrate = test.getBitrate();
            current.psnr = test.getPSNR();
            current.ssim = test.getSSIM();
            
            // 添加性能指标到回调结果
            std::stringstream perfInfo;
            perfInfo << "\n帧生成时间: " << std::fixed << std::setprecision(3) 
                    << test.perfMetrics_.avgFrameGenTimePerFrame * 1000 << " ms/帧"
                    << "\n编码时间: " << test.perfMetrics_.avgEncodingTimePerFrame * 1000 << " ms/帧"
                    << "\n写入时间: " << test.perfMetrics_.avgWritingTimePerFrame * 1000 << " ms/帧";
            current.errorMessage = perfInfo.str();  // 使用errorMessage字段传递性能信息
            
            progressCallback(i + 1, current);
        }
    }

    // 编码完成后，刷新编码器缓冲区
    if (!test.encodeFrame(nullptr, 0)) {
        result.errorMessage = "刷新编码器失败";
        return result;
    }

    // 写入文件尾并关闭文件
    if (test.formatCtx_ && test.formatCtx_->pb) {
        // 写入文件尾
        if (av_write_trailer(test.formatCtx_) < 0) {
            result.errorMessage = "写入文件尾失败";
            return result;
        }
        // 关闭输出文件
        avio_closep(&test.formatCtx_->pb);
    }

    result.success = true;
    result.encodingTime = test.getEncodingTime();
    result.fps = test.getFPS();
    result.bitrate = test.getBitrate();
    result.psnr = test.getPSNR();
    result.ssim = test.getSSIM();
    result.outputFile = outputFile;

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