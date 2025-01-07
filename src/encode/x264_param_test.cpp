#include <filesystem>
#include "x264_param_test.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <cmath>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <filesystem>
#include <fstream>

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
        std::cerr << "找不到x264编码器，请确保已安装libx264" << std::endl;
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

    std::cout << "编码器参数设置:" << std::endl;
    std::cout << "分辨率: " << encoderCtx_->width << "x" << encoderCtx_->height << std::endl;
    std::cout << "帧率: " << encoderCtx_->framerate.num << "/" << encoderCtx_->framerate.den << std::endl;
    std::cout << "线程数: " << encoderCtx_->thread_count << std::endl;

    // 设置码率控制
    switch (config.rateControl) {
        case RateControl::CRF:
            std::cout << "使用CRF模式，CRF值: " << config.crf << std::endl;
            if (av_opt_set_int(encoderCtx_->priv_data, "crf", config.crf, 0) < 0) {
                std::cerr << "设置CRF值失败" << std::endl;
                return false;
            }
            break;
        case RateControl::CQP:
            std::cout << "使用CQP模式，QP值: " << config.qp << std::endl;
            encoderCtx_->global_quality = config.qp * FF_QP2LAMBDA;
            encoderCtx_->flags |= AV_CODEC_FLAG_QSCALE;
            break;
        case RateControl::ABR:
            std::cout << "使用ABR模式，目标码率: " << config.bitrate << " bps" << std::endl;
            encoderCtx_->bit_rate = config.bitrate;
            break;
        case RateControl::CBR:
            std::cout << "使用CBR模式，固定码率: " << config.bitrate << " bps" << std::endl;
            encoderCtx_->bit_rate = config.bitrate;
            encoderCtx_->rc_max_rate = config.bitrate;
            encoderCtx_->rc_min_rate = config.bitrate;
            encoderCtx_->rc_buffer_size = config.bitrate / 2;
            break;
    }

    // 设置预设和调优
    const char* preset = presetToString(config.preset);
    const char* tune = tuneToString(config.tune);
    std::cout << "使用预设: " << preset << std::endl;
    std::cout << "使用调优: " << (tune ? tune : "none") << std::endl;

    if (av_opt_set(encoderCtx_->priv_data, "preset", preset, 0) < 0) {
        std::cerr << "设置预设失败" << std::endl;
        return false;
    }
    if (tune && av_opt_set(encoderCtx_->priv_data, "tune", tune, 0) < 0) {
        std::cerr << "设置调优失败" << std::endl;
        return false;
    }

    // 设置GOP参数
    encoderCtx_->gop_size = config.keyintMax;
    encoderCtx_->max_b_frames = config.bframes;
    encoderCtx_->refs = config.refs;

    std::cout << "GOP参数:" << std::endl;
    std::cout << "关键帧间隔: " << encoderCtx_->gop_size << std::endl;
    std::cout << "B帧数量: " << encoderCtx_->max_b_frames << std::endl;
    std::cout << "参考帧数: " << encoderCtx_->refs << std::endl;

    // 打开编码器
    int ret = avcodec_open2(encoderCtx_, codec, nullptr);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, sizeof(errbuf));
        std::cerr << "无法打开编码器: " << errbuf << std::endl;
        return false;
    }
    std::cout << "编码器打开成功" << std::endl;

    // 如果指定了输出文件，创建输出上下文
    if (!outputFile.empty()) {
        std::cout << "创建输出文件: " << outputFile << std::endl;
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
        ret = avio_open(&formatCtx_->pb, outputFile.c_str(), AVIO_FLAG_WRITE);
        if (ret < 0) {
            char errbuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errbuf, sizeof(errbuf));
            std::cerr << "无法打开输出文件: " << outputFile << " - " << errbuf << std::endl;
            return false;
        }

        // 创建视频流
        stream_ = avformat_new_stream(formatCtx_, codec);
        if (!stream_) {
            std::cerr << "无法创建视频流" << std::endl;
            return false;
        }

        // 复制编码器参数到流
        ret = avcodec_parameters_from_context(stream_->codecpar, encoderCtx_);
        if (ret < 0) {
            std::cerr << "无法复制编码器参数到流" << std::endl;
            return false;
        }

        // 写入文件头
        ret = avformat_write_header(formatCtx_, nullptr);
        if (ret < 0) {
            char errbuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errbuf, sizeof(errbuf));
            std::cerr << "无法写入文件头: " << errbuf << std::endl;
            return false;
        }
        std::cout << "输出文件初始化成功" << std::endl;
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

    // 在成功初始化后启动写入线程
    if (!outputFile.empty()) {
        startWriterThread();
        std::cout << "写入线程已启动" << std::endl;
    }

    return true;
}

bool X264ParamTest::encodeFrame(const uint8_t* data, int size) {
    if (!encoderCtx_ || !frame_ || !packet_) {
        std::cerr << "编码器未正确初始化" << std::endl;
        return false;
    }

    auto totalStart = std::chrono::steady_clock::now();
    frameStartTime_ = totalStart;

    // 如果data为空，表示刷新编码器
    if (!data) {
        std::cout << "刷新编码器缓冲区..." << std::endl;
        int ret = avcodec_send_frame(encoderCtx_, nullptr);
        if (ret < 0) {
            char errbuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errbuf, sizeof(errbuf));
            std::cerr << "刷新编码器失败: " << errbuf << std::endl;
            return false;
        }
    } else {
        // 计时：帧数据复制
        auto copyStart = std::chrono::steady_clock::now();
        
        // 复制输入数据到帧
        int ret = av_frame_make_writable(frame_);
        if (ret < 0) {
            char errbuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errbuf, sizeof(errbuf));
            std::cerr << "无法使帧可写: " << errbuf << std::endl;
            return false;
        }

        // 复制Y平面
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

        auto copyEnd = std::chrono::steady_clock::now();
        double copyTime = std::chrono::duration<double>(copyEnd - copyStart).count();

        frame_->pts = frameCount_++;

        // 发送帧进行编码
        auto encodeStart = std::chrono::steady_clock::now();
        ret = avcodec_send_frame(encoderCtx_, frame_);
        if (ret < 0) {
            char errbuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errbuf, sizeof(errbuf));
            std::cerr << "发送帧到编码器失败: " << errbuf << std::endl;
            return false;
        }
        auto encodeSendEnd = std::chrono::steady_clock::now();
        double encodeSendTime = std::chrono::duration<double>(encodeSendEnd - encodeStart).count();

        // 更新性能指标
        perfMetrics_.totalFrameCopyTime += copyTime;
        perfMetrics_.avgFrameCopyTime = perfMetrics_.totalFrameCopyTime / frameCount_;
        perfMetrics_.maxFrameCopyTime = std::max(perfMetrics_.maxFrameCopyTime, copyTime);
    }

    // 接收编码后的包
    bool gotPacket = false;
    auto receiveStart = std::chrono::steady_clock::now();
    double totalReceiveTime = 0.0;

    while (true) {
        auto packetStart = std::chrono::steady_clock::now();
        int ret = avcodec_receive_packet(encoderCtx_, packet_);
        if (ret == AVERROR(EAGAIN)) {
            break;
        } else if (ret == AVERROR_EOF) {
            if (!data) {
                break;
            }
            std::cerr << "编码器意外返回EOF" << std::endl;
            return false;
        } else if (ret < 0) {
            char errbuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errbuf, sizeof(errbuf));
            std::cerr << "接收编码包失败: " << errbuf << std::endl;
            return false;
        }

        auto packetEnd = std::chrono::steady_clock::now();
        double packetTime = std::chrono::duration<double>(packetEnd - packetStart).count();
        totalReceiveTime += packetTime;

        gotPacket = true;
        bitrate_ = (bitrate_ * (frameCount_ - 1) + packet_->size * 8.0 * encoderCtx_->time_base.den / encoderCtx_->time_base.num) / frameCount_;

        // 如果有输出文件，将数据包添加到缓冲区
        if (formatCtx_) {
            av_packet_rescale_ts(packet_, encoderCtx_->time_base, stream_->time_base);
            if (!addPacketToBuffer(packet_)) {
                std::cerr << "添加包到写入缓冲区失败" << std::endl;
                return false;
            }
        }

        av_packet_unref(packet_);
    }

    auto totalEnd = std::chrono::steady_clock::now();
    double totalFrameTime = std::chrono::duration<double>(totalEnd - totalStart).count();

    // 更新性能指标
    perfMetrics_.totalEncodingTime += totalReceiveTime;
    perfMetrics_.totalFrames++;
    perfMetrics_.avgEncodingTimePerFrame = perfMetrics_.totalEncodingTime / perfMetrics_.totalFrames;
    perfMetrics_.maxEncodingTimePerFrame = std::max(perfMetrics_.maxEncodingTimePerFrame, totalReceiveTime);
    perfMetrics_.totalFrameTime += totalFrameTime;
    perfMetrics_.avgTotalTimePerFrame = perfMetrics_.totalFrameTime / perfMetrics_.totalFrames;

    frameTime_ = totalFrameTime;
    encodingTime_ = std::chrono::duration<double>(totalEnd - startTime_).count();
    fps_ = frameCount_ / encodingTime_;

    // 每100帧输出一次性能统计
    if (frameCount_ % 100 == 0) {
        std::cout << "\n性能统计 (帧 " << frameCount_ << "):" << std::endl;
        std::cout << "平均帧复制时间: " << std::fixed << std::setprecision(3) 
                  << perfMetrics_.avgFrameCopyTime * 1000 << " ms" << std::endl;
        std::cout << "平均编码时间: " << std::fixed << std::setprecision(3) 
                  << perfMetrics_.avgEncodingTimePerFrame * 1000 << " ms" << std::endl;
        std::cout << "平均写入时间: " << std::fixed << std::setprecision(3) 
                  << perfMetrics_.avgWritingTimePerFrame * 1000 << " ms" << std::endl;
        std::cout << "平均总处理时间: " << std::fixed << std::setprecision(3) 
                  << perfMetrics_.avgTotalTimePerFrame * 1000 << " ms" << std::endl;
        std::cout << "当前编码速度: " << std::fixed << std::setprecision(1) 
                  << fps_ << " fps" << std::endl;
        std::cout << "实际每帧间隔: " << std::fixed << std::setprecision(3)
                  << (1000.0 / fps_) << " ms" << std::endl;
    }

    return true;
}

void X264ParamTest::cleanup() {
    // 停止写入线程
    stopWriterThread();
    
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

bool X264ParamTest::initFrameCache(const TestConfig& config) {
    frameCache_.clear();
    frameCache_.frame_size = config.width * config.height * 3 / 2;  // YUV420P
    frameCache_.total_frames = config.frameCount;

    // 计算总内存需求
    size_t total_memory_needed = frameCache_.frame_size * frameCache_.total_frames;
    
    // 使用磁盘缓存以避免内存问题
    frameCache_.use_disk_cache = true;
    frameCache_.cache_dir = std::string(getenv("HOME")) + "/frame_cache";
    std::filesystem::create_directories(frameCache_.cache_dir);

    return generateFrames(config);
}

std::vector<uint8_t> X264ParamTest::generateSingleFrame(int width, int height, int frameIndex) {
    std::vector<uint8_t> frameData(width * height * 3 / 2);  // YUV420P
    
    // 生成Y平面（亮度）- 使用更简单的模式
    float time = frameIndex * 0.1f;
    int pattern_size = 32;  // 增大模式尺寸，减少计算次数
    
    #pragma omp parallel for collapse(2)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // 简化的条纹模式
            int stripe = ((x + y + frameIndex * 4) / pattern_size) & 1;
            
            // 简化的波浪效果
            float wave = sin((x + y + frameIndex * 3) * 0.02f) * 64.0f;
            
            // 组合效果
            float value = 128.0f + stripe * 64.0f + wave;
            frameData[y * width + x] = static_cast<uint8_t>(std::clamp(value, 0.0f, 255.0f));
        }
    }

    // 生成U和V平面（色度）- 使用简化的彩色模式
    int uvWidth = width / 2;
    int uvHeight = height / 2;
    uint8_t* uPlane = frameData.data() + width * height;
    uint8_t* vPlane = uPlane + uvWidth * uvHeight;

    #pragma omp parallel for collapse(2)
    for (int y = 0; y < uvHeight; y++) {
        for (int x = 0; x < uvWidth; x++) {
            // 简化的色彩变化
            float colorPhase = (x + y + frameIndex * 2) * 0.1f;
            
            // U分量 - 蓝色变化
            float u_value = 128.0f + 64.0f * sin(colorPhase);
            
            // V分量 - 红色变化
            float v_value = 128.0f + 64.0f * cos(colorPhase);
            
            uPlane[y * uvWidth + x] = static_cast<uint8_t>(u_value);
            vPlane[y * uvWidth + x] = static_cast<uint8_t>(v_value);
        }
    }

    return frameData;
}

void X264ParamTest::generateFramesThreaded(const TestConfig& config, size_t thread_count) {
    // 使用系统线程数，但不超过帧数
    thread_count = std::min(thread_count, frameCache_.total_frames);
    thread_count = std::max(thread_count, size_t(1));

    // 预分配内存
    if (!frameCache_.use_disk_cache) {
        frameCache_.frame_buffer.resize(frameCache_.total_frames);
        for (auto& frame : frameCache_.frame_buffer) {
            frame.reserve(frameCache_.frame_size);
        }
    }

    std::vector<std::thread> threads;
    threads.reserve(thread_count);  // 预分配线程数组

    size_t frames_per_thread = frameCache_.total_frames / thread_count;
    size_t remaining_frames = frameCache_.total_frames % thread_count;

    // 重置生成状态
    {
        std::lock_guard<std::mutex> lock(gen_status_.mutex);
        gen_status_.completed_frames = 0;
        gen_status_.is_generating = true;
        gen_status_.total_frames = frameCache_.total_frames;
        gen_status_.last_progress = 0.0f;
    }

    // 启动工作线程
    size_t start_frame = 0;
    for (size_t i = 0; i < thread_count; ++i) {
        size_t thread_frames = frames_per_thread + (i < remaining_frames ? 1 : 0);
        size_t end_frame = start_frame + thread_frames;
        
        threads.emplace_back(&X264ParamTest::frameGenerationWorker, 
                           this, config, start_frame, end_frame);
        
        start_frame = end_frame;
    }

    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }

    // 确保所有帧都已生成
    {
        std::lock_guard<std::mutex> lock(gen_status_.mutex);
        if (gen_status_.completed_frames >= gen_status_.total_frames && 
            gen_status_.progress_callback && 
            gen_status_.last_progress < 1.0f) {
            gen_status_.last_progress = 1.0f;
            gen_status_.progress_callback(1.0f);
        }
    }

    gen_status_.is_generating = false;
}

void X264ParamTest::frameGenerationWorker(
    X264ParamTest* self,
    const TestConfig& config,
    size_t start_frame,
    size_t end_frame
) {
    // 为每个线程预分配一个帧缓冲区，避免重复分配
    std::vector<uint8_t> frameBuffer;
    frameBuffer.reserve(self->frameCache_.frame_size);

    for (size_t i = start_frame; i < end_frame; ++i) {
        frameBuffer = self->generateSingleFrame(config.width, config.height, i);
        
        if (self->frameCache_.use_disk_cache) {
            std::string filename = self->frameCache_.cache_dir + "/frame_" + std::to_string(i) + ".yuv";
            std::ofstream file(filename, std::ios::binary);
            file.write(reinterpret_cast<const char*>(frameBuffer.data()), frameBuffer.size());
        } else {
            self->frameCache_.frame_buffer[i] = std::move(frameBuffer);
            frameBuffer.reserve(self->frameCache_.frame_size);  // 重新分配容量
        }

        // 更新进度
        {
            std::lock_guard<std::mutex> lock(self->gen_status_.mutex);
            size_t completed = ++self->gen_status_.completed_frames;
            if (self->gen_status_.progress_callback) {
                float progress = static_cast<float>(completed) / self->gen_status_.total_frames;
                // 确保进度不超过1.0且只在进度变化时更新
                progress = std::min(progress, 1.0f);
                if (progress > self->gen_status_.last_progress) {
                    self->gen_status_.last_progress = progress;
                    self->gen_status_.progress_callback(progress);
                }
            }
        }
    }
}

bool X264ParamTest::generateFrames(const TestConfig& config) {
    std::cout << "开始生成帧数据..." << std::endl;
    auto genStart = std::chrono::steady_clock::now();

    // 使用系统CPU核心数的4倍作为线程数，以充分利用CPU
    size_t thread_count = std::thread::hardware_concurrency() * 4;
    generateFramesThreaded(config, thread_count);

    auto genEnd = std::chrono::steady_clock::now();
    double genTime = std::chrono::duration<double>(genEnd - genStart).count();
    std::cout << "\n帧生成完成，耗时: " << std::fixed << std::setprecision(2) 
              << genTime << " 秒" << std::endl;

    frameCache_.is_initialized = true;
    return true;
}

const uint8_t* X264ParamTest::getFrameData(size_t frameIndex) const {
    if (!frameCache_.is_initialized || frameIndex >= frameCache_.total_frames) {
        return nullptr;
    }

    if (frameCache_.use_disk_cache) {
        static std::vector<uint8_t> buffer;  // 静态缓冲区避免重复分配
        buffer.resize(frameCache_.frame_size);
        
        std::string filename = frameCache_.cache_dir + "/frame_" + std::to_string(frameIndex) + ".yuv";
        std::ifstream file(filename, std::ios::binary);
        file.read(reinterpret_cast<char*>(buffer.data()), frameCache_.frame_size);
        
        return buffer.data();
    } else {
        return frameCache_.frame_buffer[frameIndex].data();
    }
}

X264ParamTest::TestResult X264ParamTest::runTest(
    const TestConfig& config,
    std::function<void(int, const TestResult&)> progressCallback
) {
    TestResult result;
    result.success = false;

    std::cout << "开始编码测试..." << std::endl;
    std::cout << "配置信息:" << std::endl;
    std::cout << "分辨率: " << config.width << "x" << config.height << std::endl;
    std::cout << "帧数: " << config.frameCount << std::endl;
    std::cout << "线程数: " << config.threads << std::endl;
    std::cout << "预设: " << presetToString(config.preset) << std::endl;
    std::cout << "调优: " << (tuneToString(config.tune) ? tuneToString(config.tune) : "none") << std::endl;

    // 检查配置参数
    if (config.width <= 0 || config.height <= 0 || config.frameCount <= 0) {
        result.errorMessage = "无效的视频参数";
        std::cerr << result.errorMessage << std::endl;
        return result;
    }

    // 生成输出文件名
    std::string outputFile;
    const char* workDir = getenv("PWD");
    if (!workDir) {
        std::cerr << "无法获取当前工作目录" << std::endl;
        result.errorMessage = "无法获取当前工作目录";
        return result;
    }
    
    std::string outputDir = std::string(workDir) + "/datas";
    
    // 确保输出目录存在
    if (system(("mkdir -p '" + outputDir + "'").c_str()) != 0) {
        std::cerr << "无法创建输出目录: " << outputDir << std::endl;
        result.errorMessage = "无法创建输出目录";
        return result;
    }
    
    outputFile = outputDir + "/output_" +
                std::to_string(config.width) + "x" + std::to_string(config.height) + "_" +
                std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) +
                ".mp4";
    
    std::cout << "输出文件: " << outputFile << std::endl;

    X264ParamTest test;
    std::cout << "初始化编码器..." << std::endl;
    if (!test.initEncoder(config, outputFile)) {
        result.errorMessage = "初始化编码器失败";
        std::cerr << result.errorMessage << std::endl;
        return result;
    }
    std::cout << "编码器初始化成功" << std::endl;

    // 初始化并生成帧缓存
    std::cout << "初始化帧缓存..." << std::endl;
    if (!test.initFrameCache(config)) {
        result.errorMessage = "初始化帧缓存失败";
        std::cerr << result.errorMessage << std::endl;
        return result;
    }
    std::cout << "帧缓存初始化成功" << std::endl;

    std::cout << "开始编码帧..." << std::endl;
    // 编码所有帧
    for (int i = 0; i < config.frameCount; i++) {
        const uint8_t* frameData = test.getFrameData(i);
        if (!frameData) {
            result.errorMessage = "获取帧 " + std::to_string(i) + " 数据失败";
            std::cerr << result.errorMessage << std::endl;
            return result;
        }

        if (!test.encodeFrame(frameData, test.frameCache_.frame_size)) {
            result.errorMessage = "编码帧 " + std::to_string(i) + " 失败";
            std::cerr << result.errorMessage << std::endl;
            return result;
        }

        if (progressCallback) {
            TestResult current;
            current.encodingTime = test.frameTime_;
            current.fps = test.getFPS();
            current.bitrate = test.getBitrate();
            current.psnr = test.getPSNR();
            current.ssim = test.getSSIM();
            progressCallback(i + 1, current);
        }

        if ((i + 1) % 10 == 0) {
            std::cout << "\r已编码: " << (i + 1) << "/" << config.frameCount << " 帧" << std::flush;
        }
    }
    std::cout << std::endl;

    std::cout << "刷新编码器缓冲区..." << std::endl;
    // 编码完成后，刷新编码器缓冲区
    if (!test.encodeFrame(nullptr, 0)) {
        result.errorMessage = "刷新编码器失败";
        std::cerr << result.errorMessage << std::endl;
        return result;
    }

    std::cout << "写入文件尾..." << std::endl;
    // 写入文件尾并关闭文件
    if (test.formatCtx_ && test.formatCtx_->pb) {
        // 写入文件尾
        if (av_write_trailer(test.formatCtx_) < 0) {
            result.errorMessage = "写入文件尾失败";
            std::cerr << result.errorMessage << std::endl;
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

    std::cout << "编码完成!" << std::endl;
    std::cout << "编码时间: " << result.encodingTime << "秒" << std::endl;
    std::cout << "平均速度: " << result.fps << " fps" << std::endl;
    std::cout << "平均码率: " << result.bitrate / 1000.0 << " kbps" << std::endl;

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

void X264ParamTest::startWriterThread() {
    writeBuffer_.finished = false;
    writeBuffer_.writer_thread = std::thread(&X264ParamTest::writerThreadFunc, this);
}

void X264ParamTest::stopWriterThread() {
    {
        std::lock_guard<std::mutex> lock(writeBuffer_.mutex);
        writeBuffer_.finished = true;
    }
    writeBuffer_.cv.notify_one();
    if (writeBuffer_.writer_thread.joinable()) {
        writeBuffer_.writer_thread.join();
    }
}

bool X264ParamTest::addPacketToBuffer(const AVPacket* packet) {
    PacketData pkt_data;
    pkt_data.size = packet->size;
    pkt_data.data = new uint8_t[packet->size];
    memcpy(pkt_data.data, packet->data, packet->size);
    pkt_data.pts = packet->pts;
    pkt_data.dts = packet->dts;
    pkt_data.stream_index = packet->stream_index;
    pkt_data.flags = packet->flags;
    pkt_data.duration = packet->duration;

    {
        std::unique_lock<std::mutex> lock(writeBuffer_.mutex);
        // 如果缓冲区满了，等待写入线程处理
        while (writeBuffer_.packets.size() >= WriteBuffer::MAX_PACKETS) {
            writeBuffer_.cv.wait(lock);
        }
        writeBuffer_.packets.push_back(std::move(pkt_data));
    }
    writeBuffer_.cv.notify_one();
    return true;
}

void X264ParamTest::writerThreadFunc() {
    while (true) {
        PacketData pkt_data;
        bool should_exit = false;
        {
            std::unique_lock<std::mutex> lock(writeBuffer_.mutex);
            while (writeBuffer_.packets.empty() && !writeBuffer_.finished) {
                writeBuffer_.cv.wait(lock);
            }
            
            if (writeBuffer_.packets.empty() && writeBuffer_.finished) {
                break;
            }
            
            if (!writeBuffer_.packets.empty()) {
                pkt_data = std::move(writeBuffer_.packets.front());
                writeBuffer_.packets.erase(writeBuffer_.packets.begin());
            }
        }
        writeBuffer_.cv.notify_one();  // 通知可能在等待的生产者

        if (formatCtx_ && formatCtx_->pb) {
            AVPacket pkt = {0};
            av_init_packet(&pkt);
            pkt.data = pkt_data.data;
            pkt.size = pkt_data.size;
            pkt.pts = pkt_data.pts;
            pkt.dts = pkt_data.dts;
            pkt.stream_index = pkt_data.stream_index;
            pkt.flags = pkt_data.flags;
            pkt.duration = pkt_data.duration;

            auto writeStart = std::chrono::steady_clock::now();
            int ret = av_interleaved_write_frame(formatCtx_, &pkt);
            auto writeEnd = std::chrono::steady_clock::now();
            
            double writeTime = std::chrono::duration<double>(writeEnd - writeStart).count();
            perfMetrics_.totalWritingTime += writeTime;
            perfMetrics_.maxWritingTimePerFrame = std::max(
                perfMetrics_.maxWritingTimePerFrame, writeTime);
        }
        
        delete[] pkt_data.data;
    }
} 