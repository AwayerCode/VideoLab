#include "vp8_param_test.hpp"
#include <chrono>
#include <iostream>
#include <fstream>
#include <thread>
#include <cmath>

const char* VP8ParamTest::presetToString(Preset preset) {
    switch (preset) {
        case Preset::BEST:     return "best";
        case Preset::GOOD:     return "good";
        case Preset::REALTIME: return "realtime";
        default:               return "unknown";
    }
}

const char* VP8ParamTest::speedToString(Speed speed) {
    return std::to_string(static_cast<int>(speed)).c_str();
}

VP8ParamTest::VP8ParamTest() = default;

VP8ParamTest::~VP8ParamTest() {
    cleanup();
}

bool VP8ParamTest::initEncoder(const TestConfig& config, const std::string& outputFile) {
    cleanup();

    // 查找VP8编码器
    const AVCodec* codec = avcodec_find_encoder_by_name("libvpx");
    if (!codec) {
        std::cerr << "找不到VP8编码器，请确保已安装libvpx" << std::endl;
        return false;
    }

    // 创建编码器上下文
    codecContext_ = avcodec_alloc_context3(codec);
    if (!codecContext_) {
        std::cerr << "无法分配编码器上下文" << std::endl;
        return false;
    }

    // 设置基本参数
    codecContext_->width = config.width;
    codecContext_->height = config.height;
    codecContext_->time_base = AVRational{1, config.fps};
    codecContext_->framerate = AVRational{config.fps, 1};
    codecContext_->pix_fmt = AV_PIX_FMT_YUV420P;
    codecContext_->codec_type = AVMEDIA_TYPE_VIDEO;
    codecContext_->thread_count = config.threads;

    // 设置VP8特定参数
    av_opt_set(codecContext_->priv_data, "quality", presetToString(config.preset), 0);
    av_opt_set(codecContext_->priv_data, "speed", speedToString(config.speed), 0);
    
    switch (config.rate_control) {
        case RateControl::CQ:
            av_opt_set(codecContext_->priv_data, "crf", std::to_string(config.cq_level).c_str(), 0);
            break;
        case RateControl::CBR:
            codecContext_->bit_rate = config.bitrate;
            codecContext_->rc_min_rate = config.bitrate;
            codecContext_->rc_max_rate = config.bitrate;
            codecContext_->rc_buffer_size = config.bitrate;
            break;
        case RateControl::VBR:
            codecContext_->bit_rate = config.bitrate;
            codecContext_->rc_max_rate = static_cast<int64_t>(config.bitrate * 2);
            codecContext_->rc_buffer_size = static_cast<int64_t>(config.bitrate * 4);
            break;
    }

    codecContext_->qmin = config.qmin;
    codecContext_->qmax = config.qmax;
    codecContext_->gop_size = config.keyint;

    // 打开编码器
    int ret = avcodec_open2(codecContext_, codec, nullptr);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, sizeof(errbuf));
        std::cerr << "无法打开编码器: " << errbuf << std::endl;
        return false;
    }

    // 分配帧和数据包
    frame_ = av_frame_alloc();
    if (!frame_) {
        std::cerr << "无法分配帧" << std::endl;
        return false;
    }

    frame_->format = codecContext_->pix_fmt;
    frame_->width = codecContext_->width;
    frame_->height = codecContext_->height;

    ret = av_frame_get_buffer(frame_, 0);
    if (ret < 0) {
        std::cerr << "无法分配帧缓冲" << std::endl;
        return false;
    }

    packet_ = av_packet_alloc();
    if (!packet_) {
        std::cerr << "无法分配数据包" << std::endl;
        return false;
    }

    // 打开输出文件
    outputFile_ = fopen(outputFile.c_str(), "wb");
    if (!outputFile_) {
        std::cerr << "无法打开输出文件" << std::endl;
        return false;
    }

    return true;
}

bool VP8ParamTest::encodeFrame(const uint8_t* data, int size) {
    if (!codecContext_ || !frame_ || !packet_ || !outputFile_) {
        return false;
    }

    // 复制数据到帧
    av_image_copy_plane(frame_->data[0], frame_->linesize[0],
                       data, codecContext_->width,
                       codecContext_->width, codecContext_->height);
    
    av_image_copy_plane(frame_->data[1], frame_->linesize[1],
                       data + codecContext_->width * codecContext_->height,
                       codecContext_->width / 2,
                       codecContext_->width / 2, codecContext_->height / 2);
    
    av_image_copy_plane(frame_->data[2], frame_->linesize[2],
                       data + codecContext_->width * codecContext_->height * 5 / 4,
                       codecContext_->width / 2,
                       codecContext_->width / 2, codecContext_->height / 2);

    // 发送帧到编码器
    int ret = avcodec_send_frame(codecContext_, frame_);
    if (ret < 0) {
        return false;
    }

    // 接收编码后的数据包
    while (ret >= 0) {
        ret = avcodec_receive_packet(codecContext_, packet_);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            return false;
        }

        // 写入数据包到文件
        if (!addPacketToBuffer(packet_)) {
            return false;
        }

        av_packet_unref(packet_);
    }

    return true;
}

void VP8ParamTest::cleanup() {
    if (codecContext_) {
        avcodec_free_context(&codecContext_);
    }
    if (frame_) {
        av_frame_free(&frame_);
    }
    if (packet_) {
        av_packet_free(&packet_);
    }
    if (outputFile_) {
        fclose(outputFile_);
        outputFile_ = nullptr;
    }
}

bool VP8ParamTest::initFrameCache(const TestConfig& config) {
    if (framesGenerated_) {
        return true;
    }

    return generateFrames(config);
}

bool VP8ParamTest::generateFrames(const TestConfig& config) {
    size_t thread_count = std::thread::hardware_concurrency();
    if (thread_count == 0) thread_count = 1;
    
    if (thread_count > 1) {
        generateFramesThreaded(config, thread_count);
    } else {
        frameCache_.resize(config.frames);
        for (int i = 0; i < config.frames; ++i) {
            frameCache_[i] = generateSingleFrame(config.width, config.height, i);
        }
    }

    framesGenerated_ = true;
    return true;
}

std::vector<uint8_t> VP8ParamTest::generateSingleFrame(int width, int height, int frameIndex) {
    std::vector<uint8_t> frameData(width * height * 3 / 2);
    uint8_t* y = frameData.data();
    uint8_t* u = y + width * height;
    uint8_t* v = u + (width * height) / 4;

    // 生成YUV测试图案
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            // Y分量：生成移动的图案
            int pattern = (i + j + frameIndex) % 255;
            y[i * width + j] = pattern;

            // U和V分量：每4个像素一个色度采样
            if (i % 2 == 0 && j % 2 == 0) {
                int uv_i = i / 2;
                int uv_j = j / 2;
                u[uv_i * (width/2) + uv_j] = 128 + pattern / 2;
                v[uv_i * (width/2) + uv_j] = 128 - pattern / 2;
            }
        }
    }

    return frameData;
}

void VP8ParamTest::generateFramesThreaded(const TestConfig& config, size_t thread_count) {
    frameCache_.resize(config.frames);
    
    std::vector<std::thread> threads;
    size_t frames_per_thread = config.frames / thread_count;
    size_t remaining_frames = config.frames % thread_count;
    
    size_t start_frame = 0;
    for (size_t i = 0; i < thread_count; ++i) {
        size_t thread_frames = frames_per_thread + (i < remaining_frames ? 1 : 0);
        size_t end_frame = start_frame + thread_frames;
        
        threads.emplace_back(frameGenerationWorker,
                           this, std::ref(config),
                           start_frame, end_frame);
        
        start_frame = end_frame;
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
}

void VP8ParamTest::frameGenerationWorker(
    VP8ParamTest* self,
    const TestConfig& config,
    size_t start_frame,
    size_t end_frame)
{
    for (size_t i = start_frame; i < end_frame; ++i) {
        self->frameCache_[i] = VP8ParamTest::generateSingleFrame(config.width, config.height, i);
    }
}

const uint8_t* VP8ParamTest::getFrameData(size_t frameIndex) const {
    if (frameIndex >= frameCache_.size()) {
        return nullptr;
    }
    return frameCache_[frameIndex].data();
}

VP8ParamTest::TestResult VP8ParamTest::runTest(
    const TestConfig& config,
    ProgressCallback progress_callback)
{
    TestResult result = {};
    VP8ParamTest test;

    // 初始化帧缓存
    if (!test.initFrameCache(config)) {
        std::cerr << "无法初始化帧缓存" << std::endl;
        return result;
    }

    // 初始化编码器
    if (!test.initEncoder(config, config.output_file)) {
        std::cerr << "无法初始化编码器" << std::endl;
        return result;
    }

    // 启动写入线程
    test.startWriterThread();

    // 开始编码
    auto start_time = std::chrono::high_resolution_clock::now();
    size_t total_size = 0;

    for (int i = 0; i < config.frames; ++i) {
        const uint8_t* frame_data = test.getFrameData(i);
        if (!frame_data) {
            std::cerr << "无法获取帧 " << i << std::endl;
            continue;
        }

        if (!test.encodeFrame(frame_data, config.width * config.height * 3 / 2)) {
            std::cerr << "编码帧 " << i << " 失败" << std::endl;
            continue;
        }

        if (progress_callback) {
            auto current_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                current_time - start_time).count() / 1000.0;
            
            result.encoding_time = duration;
            result.fps = (i + 1) / duration;
            
            progress_callback(i, result);
        }
    }

    // 停止写入线程
    test.stopWriterThread();

    // 计算结果
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time).count() / 1000.0;

    result.encoding_time = duration;
    result.fps = config.frames / duration;
    result.bitrate = (total_size * 8.0) / duration;

    // TODO: 计算PSNR和SSIM
    result.psnr = 0.0;
    result.ssim = 0.0;

    return result;
}

std::vector<VP8ParamTest::TestResult> VP8ParamTest::runPresetTest(
    TestConfig base_config,
    const std::string& output_prefix)
{
    std::vector<TestResult> results;
    std::vector<Preset> presets = {Preset::BEST, Preset::GOOD, Preset::REALTIME};

    std::cout << "\n开始VP8预设测试...\n" << std::endl;

    for (auto preset : presets) {
        std::cout << "测试预设: " << presetToString(preset) << std::endl;
        
        base_config.preset = preset;
        base_config.output_file = output_prefix + "_" + presetToString(preset) + ".webm";
        
        auto result = runTest(base_config);
        results.push_back(result);
        
        std::cout << "编码时间: " << result.encoding_time << "秒" << std::endl;
        std::cout << "编码速度: " << result.fps << " fps" << std::endl;
        std::cout << "实际码率: " << result.bitrate / 1000.0 << " kbps" << std::endl;
        std::cout << "PSNR: " << result.psnr << " dB" << std::endl;
        std::cout << "SSIM: " << result.ssim << std::endl;
        std::cout << std::endl;
    }

    return results;
}

std::vector<VP8ParamTest::TestResult> VP8ParamTest::runRateControlTest(
    TestConfig base_config,
    const std::string& output_prefix)
{
    std::vector<TestResult> results;
    std::vector<RateControl> modes = {RateControl::CQ, RateControl::CBR, RateControl::VBR};
    std::vector<int> bitrates = {1000000, 2000000, 4000000}; // 1Mbps, 2Mbps, 4Mbps

    std::cout << "\n开始VP8码率控制测试...\n" << std::endl;

    for (auto mode : modes) {
        std::string mode_str;
        switch (mode) {
            case RateControl::CQ:  mode_str = "cq";  break;
            case RateControl::CBR: mode_str = "cbr"; break;
            case RateControl::VBR: mode_str = "vbr"; break;
        }

        if (mode == RateControl::CQ) {
            std::vector<int> cq_levels = {10, 20, 30, 40};
            for (int cq : cq_levels) {
                std::cout << "测试CQ级别: " << cq << std::endl;
                
                base_config.rate_control = mode;
                base_config.cq_level = cq;
                base_config.output_file = output_prefix + "_" + mode_str + "_" + std::to_string(cq) + ".webm";
                
                auto result = runTest(base_config);
                results.push_back(result);
                
                std::cout << "编码时间: " << result.encoding_time << "秒" << std::endl;
                std::cout << "编码速度: " << result.fps << " fps" << std::endl;
                std::cout << "实际码率: " << result.bitrate / 1000.0 << " kbps" << std::endl;
                std::cout << "PSNR: " << result.psnr << " dB" << std::endl;
                std::cout << "SSIM: " << result.ssim << std::endl;
                std::cout << std::endl;
            }
        } else {
            for (int bitrate : bitrates) {
                std::cout << "测试" << mode_str << "码率: " << bitrate / 1000000.0 << "Mbps" << std::endl;
                
                base_config.rate_control = mode;
                base_config.bitrate = bitrate;
                base_config.output_file = output_prefix + "_" + mode_str + "_" + std::to_string(bitrate/1000000) + "m.webm";
                
                auto result = runTest(base_config);
                results.push_back(result);
                
                std::cout << "编码时间: " << result.encoding_time << "秒" << std::endl;
                std::cout << "编码速度: " << result.fps << " fps" << std::endl;
                std::cout << "实际码率: " << result.bitrate / 1000.0 << " kbps" << std::endl;
                std::cout << "PSNR: " << result.psnr << " dB" << std::endl;
                std::cout << "SSIM: " << result.ssim << std::endl;
                std::cout << std::endl;
            }
        }
    }

    return results;
}

std::vector<VP8ParamTest::TestResult> VP8ParamTest::runQualityTest(
    TestConfig base_config,
    const std::string& output_prefix,
    const std::vector<int>& quality_values)
{
    std::vector<TestResult> results;

    std::cout << "\n开始VP8质量参数测试...\n" << std::endl;

    for (int qp : quality_values) {
        std::cout << "测试质量参数: " << qp << std::endl;
        
        base_config.qmin = qp;
        base_config.qmax = qp;
        base_config.output_file = output_prefix + "_qp" + std::to_string(qp) + ".webm";
        
        auto result = runTest(base_config);
        results.push_back(result);
        
        std::cout << "编码时间: " << result.encoding_time << "秒" << std::endl;
        std::cout << "编码速度: " << result.fps << " fps" << std::endl;
        std::cout << "实际码率: " << result.bitrate / 1000.0 << " kbps" << std::endl;
        std::cout << "PSNR: " << result.psnr << " dB" << std::endl;
        std::cout << "SSIM: " << result.ssim << std::endl;
        std::cout << std::endl;
    }

    return results;
}

void VP8ParamTest::startWriterThread() {
    writeBuffer_.done = false;
    writeBuffer_.writer_thread = std::thread(&VP8ParamTest::writerThreadFunc, this);
}

void VP8ParamTest::stopWriterThread() {
    {
        std::lock_guard<std::mutex> lock(writeBuffer_.mutex);
        writeBuffer_.done = true;
    }
    writeBuffer_.cv.notify_one();
    if (writeBuffer_.writer_thread.joinable()) {
        writeBuffer_.writer_thread.join();
    }
}

bool VP8ParamTest::addPacketToBuffer(const AVPacket* packet) {
    std::vector<uint8_t> data(packet->data, packet->data + packet->size);
    
    {
        std::lock_guard<std::mutex> lock(writeBuffer_.mutex);
        writeBuffer_.packets.push(std::move(data));
    }
    writeBuffer_.cv.notify_one();
    
    return true;
}

void VP8ParamTest::writerThreadFunc() {
    while (true) {
        std::vector<uint8_t> packet_data;
        
        {
            std::unique_lock<std::mutex> lock(writeBuffer_.mutex);
            writeBuffer_.cv.wait(lock, [this]() {
                return !writeBuffer_.packets.empty() || writeBuffer_.done;
            });
            
            if (writeBuffer_.packets.empty() && writeBuffer_.done) {
                break;
            }
            
            packet_data = std::move(writeBuffer_.packets.front());
            writeBuffer_.packets.pop();
        }
        
        if (!packet_data.empty()) {
            fwrite(packet_data.data(), 1, packet_data.size(), outputFile_);
        }
    }
} 