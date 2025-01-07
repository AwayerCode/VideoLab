#include "encoder_test.hpp"
#include <iostream>
#include <iomanip>
#include <vector>
#include <random>

EncoderTest::EncoderTest() {}

EncoderTest::~EncoderTest() {
    cleanup();
}

bool EncoderTest::initNvencEncoder(int width, int height, int bitrate) {
    cleanup();  // 确保清理之前的资源
    
    // 创建NVIDIA硬件设备上下文
    int ret = av_hwdevice_ctx_create(&hwDeviceCtx_, AV_HWDEVICE_TYPE_CUDA, nullptr, nullptr, 0);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
        std::cerr << "Failed to create NVIDIA hardware device context: " << errbuf << std::endl;
        return false;
    }
    std::cout << "NVIDIA硬件设备上下文创建成功" << std::endl;

    // 查找NVENC编码器
    const AVCodec* codec = avcodec_find_encoder_by_name("h264_nvenc");
    if (!codec) {
        std::cerr << "找不到NVENC编码器，请确保已安装NVIDIA驱动和CUDA" << std::endl;
        return false;
    }
    std::cout << "找到NVENC编码器" << std::endl;

    // 创建编码器上下文
    encoderCtx_ = avcodec_alloc_context3(codec);
    if (!encoderCtx_) {
        std::cerr << "无法创建编码器上下文" << std::endl;
        return false;
    }

    // 创建硬件帧上下文
    AVBufferRef* hw_frames_ref = av_hwframe_ctx_alloc(hwDeviceCtx_);
    if (!hw_frames_ref) {
        std::cerr << "Failed to create hardware frame context" << std::endl;
        return false;
    }
    
    auto* frames_ctx = reinterpret_cast<AVHWFramesContext*>(hw_frames_ref->data);
    frames_ctx->format = AV_PIX_FMT_CUDA;
    frames_ctx->sw_format = AV_PIX_FMT_NV12;
    frames_ctx->width = width;
    frames_ctx->height = height;
    frames_ctx->initial_pool_size = 20;  // 预分配20个帧缓冲

    ret = av_hwframe_ctx_init(hw_frames_ref);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
        std::cerr << "Failed to initialize hardware frame context: " << errbuf << std::endl;
        av_buffer_unref(&hw_frames_ref);
        return false;
    }

    // 设置编码参数
    encoderCtx_->width = width;
    encoderCtx_->height = height;
    encoderCtx_->time_base = AVRational{1, 25};
    encoderCtx_->framerate = AVRational{25, 1};
    encoderCtx_->bit_rate = bitrate;
    encoderCtx_->pix_fmt = AV_PIX_FMT_CUDA;  // 修改为CUDA格式
    encoderCtx_->max_b_frames = 0;
    encoderCtx_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    
    // 设置硬件帧上下文
    encoderCtx_->hw_frames_ctx = av_buffer_ref(hw_frames_ref);
    av_buffer_unref(&hw_frames_ref);
    
    // 设置NVENC特定参数
    av_opt_set(encoderCtx_->priv_data, "preset", "p1", 0);  // 使用p1预设
    av_opt_set(encoderCtx_->priv_data, "tune", "ull", 0);
    av_opt_set(encoderCtx_->priv_data, "gpu", "any", 0);

    ret = avcodec_open2(encoderCtx_, codec, nullptr);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
        std::cerr << "Failed to open NVENC encoder: " << errbuf << std::endl;
        return false;
    }
    std::cout << "NVENC编码器初始化成功" << std::endl;

    // 分配帧和数据包
    frame_ = av_frame_alloc();
    hwFrame_ = av_frame_alloc();
    packet_ = av_packet_alloc();

    if (!frame_ || !hwFrame_ || !packet_) {
        std::cerr << "无法分配帧或数据包" << std::endl;
        return false;
    }

    frame_->format = AV_PIX_FMT_NV12;  // 输入帧使用NV12格式
    frame_->width = width;
    frame_->height = height;

    ret = av_frame_get_buffer(frame_, 32);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
        std::cerr << "Failed to allocate frame buffer: " << errbuf << std::endl;
        return false;
    }

    std::cout << "帧缓冲区分配成功" << std::endl;
    startTime_ = std::chrono::steady_clock::now();
    return true;
}

bool EncoderTest::initX264Encoder(int width, int height, int bitrate) {
    // 查找x264编码器
    const AVCodec* codec = avcodec_find_encoder_by_name("libx264");
    if (!codec) {
        std::cerr << "x264 encoder not found" << std::endl;
        return false;
    }

    // 创建编码器上下文
    encoderCtx_ = avcodec_alloc_context3(codec);
    if (!encoderCtx_) {
        return false;
    }

    // 设置编码参数
    encoderCtx_->width = width;
    encoderCtx_->height = height;
    encoderCtx_->time_base = AVRational{1, 25};
    encoderCtx_->framerate = AVRational{25, 1};
    encoderCtx_->bit_rate = bitrate;
    encoderCtx_->pix_fmt = AV_PIX_FMT_YUV420P;
    encoderCtx_->thread_count = 20;  // 设置使用20个线程
    encoderCtx_->thread_type = FF_THREAD_FRAME | FF_THREAD_SLICE;  // 启用帧级和切片级并行
    
    // 设置x264特定参数
    av_opt_set(encoderCtx_->priv_data, "preset", "medium", 0);
    av_opt_set(encoderCtx_->priv_data, "tune", "zerolatency", 0);
    av_opt_set(encoderCtx_->priv_data, "threads", "20", 0);  // 通过x264参数也设置20线程

    if (avcodec_open2(encoderCtx_, codec, nullptr) < 0) {
        std::cerr << "Failed to open x264 encoder" << std::endl;
        return false;
    }

    std::cout << "x264编码器初始化成功（使用20线程）" << std::endl;

    // 分配帧和数据包
    frame_ = av_frame_alloc();
    packet_ = av_packet_alloc();

    frame_->format = AV_PIX_FMT_YUV420P;
    frame_->width = width;
    frame_->height = height;

    av_frame_get_buffer(frame_, 0);

    startTime_ = std::chrono::steady_clock::now();
    return true;
}

bool EncoderTest::encodeFrame(const uint8_t* data, int size) {
    auto now = std::chrono::steady_clock::now();
    
    // 复制输入数据到AVFrame
    int ret = av_image_fill_arrays(frame_->data, frame_->linesize, data,
                        static_cast<AVPixelFormat>(frame_->format),
                        frame_->width, frame_->height, 1);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
        std::cerr << "Failed to fill image data: " << errbuf << std::endl;
        return false;
    }

    frame_->pts = frameCount_++;

    // 对于NVENC，需要获取一个硬件帧并传输数据
    if (encoderCtx_->hw_frames_ctx) {
        ret = av_hwframe_get_buffer(encoderCtx_->hw_frames_ctx, hwFrame_, 0);
        if (ret < 0) {
            char errbuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
            std::cerr << "Error getting hardware frame buffer: " << errbuf << std::endl;
            return false;
        }

        ret = av_hwframe_transfer_data(hwFrame_, frame_, 0);
        if (ret < 0) {
            char errbuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
            std::cerr << "Error transferring data to GPU: " << errbuf << std::endl;
            return false;
        }
        hwFrame_->pts = frame_->pts;
    }

    // 编码
    AVFrame* encode_frame = encoderCtx_->hw_frames_ctx ? hwFrame_ : frame_;
    ret = avcodec_send_frame(encoderCtx_, encode_frame);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
        std::cerr << "Error sending frame for encoding: " << errbuf << std::endl;
        return false;
    }

    while (true) {
        ret = avcodec_receive_packet(encoderCtx_, packet_);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            char errbuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
            std::cerr << "Error receiving packet from encoder: " << errbuf << std::endl;
            return false;
        }

        av_packet_unref(packet_);
    }

    // 更新性能统计
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - startTime_).count() / 1000.0;
    encodingTime_ = duration;
    fps_ = frameCount_ / duration;

    return true;
}

void EncoderTest::cleanup() {
    if (encoderCtx_) {
        avcodec_free_context(&encoderCtx_);
    }
    if (frame_) {
        av_frame_free(&frame_);
    }
    if (hwFrame_) {
        av_frame_free(&hwFrame_);
    }
    if (packet_) {
        av_packet_free(&packet_);
    }
    if (hwDeviceCtx_) {
        av_buffer_unref(&hwDeviceCtx_);
    }
}

EncoderTest::TestResult EncoderTest::runEncodingTest(
    bool useNvenc,
    int width,
    int height,
    int bitrate,
    int frameCount,
    std::function<void(int, const TestResult&)> progressCallback
) {
    TestResult result;
    result.success = false;
    
    try {
        // 创建测试数据
        const int frameSize = width * height * 3 / 2;  // YUV420P size
        std::vector<uint8_t> frameData(frameSize);
        
        // 用随机数据填充
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        for (auto& byte : frameData) {
            byte = static_cast<uint8_t>(dis(gen));
        }

        // 创建编码器实例
        EncoderTest encoder;
        bool initSuccess = useNvenc ? 
            encoder.initNvencEncoder(width, height, bitrate) :
            encoder.initX264Encoder(width, height, bitrate);

        if (!initSuccess) {
            result.errorMessage = useNvenc ? 
                "NVENC初始化失败" : "x264初始化失败";
            return result;
        }

        // 编码测试
        for (int i = 0; i < frameCount; ++i) {
            if (!encoder.encodeFrame(frameData.data(), frameSize)) {
                result.errorMessage = useNvenc ?
                    "NVENC编码失败" : "x264编码失败";
                return result;
            }

            // 每编码30帧（1秒）或完成所有帧时，调用进度回调
            if (progressCallback && (i % 30 == 0 || i == frameCount - 1)) {
                result.encodingTime = encoder.getEncodingTime();
                result.fps = encoder.getFPS();
                result.success = true;
                progressCallback(i + 1, result);
            }
        }

        // 设置最终结果
        result.encodingTime = encoder.getEncodingTime();
        result.fps = encoder.getFPS();
        result.success = true;

    } catch (const std::exception& e) {
        result.errorMessage = std::string("测试过程发生异常: ") + e.what();
        return result;
    }

    return result;
}

void EncoderTest::runPerformanceTest(const TestConfig& config) {
    std::cout << "开始编码器性能测试\n" << std::endl;
    std::cout << "测试参数：" << std::endl;
    std::cout << "分辨率: " << config.width << "x" << config.height << std::endl;
    std::cout << "比特率: " << config.bitrate / 1000000 << "Mbps" << std::endl;
    std::cout << "测试帧数: " << config.frameCount << " (约10秒视频 @ 30fps)" << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    // 测试NVENC
    std::cout << "\n测试NVENC硬件编码器..." << std::endl;
    auto nvencResult = runEncodingTest(
        true,  // 使用NVENC
        config.width,
        config.height,
        config.bitrate,
        config.frameCount,
        [](int frame, const TestResult& progress) {
            std::cout << "\r已编码: " << std::setw(4) << frame << " 帧"
                     << " | FPS: " << std::fixed << std::setprecision(2) << progress.fps
                     << std::flush;
        }
    );

    std::cout << "\n\nNVENC测试结果：" << std::endl;
    if (nvencResult.success) {
        std::cout << "总编码时间: " << std::fixed << std::setprecision(2) 
                 << nvencResult.encodingTime << " 秒" << std::endl;
        std::cout << "平均FPS: " << nvencResult.fps << std::endl;
    } else {
        std::cout << "测试失败: " << nvencResult.errorMessage << std::endl;
    }

    std::cout << "----------------------------------------" << std::endl;

    // 测试x264
    std::cout << "\n测试x264软件编码器..." << std::endl;
    auto x264Result = runEncodingTest(
        false,  // 使用x264
        config.width,
        config.height,
        config.bitrate,
        config.frameCount,
        [](int frame, const TestResult& progress) {
            std::cout << "\r已编码: " << std::setw(4) << frame << " 帧"
                     << " | FPS: " << std::fixed << std::setprecision(2) << progress.fps
                     << std::flush;
        }
    );

    std::cout << "\n\nx264测试结果：" << std::endl;
    if (x264Result.success) {
        std::cout << "总编码时间: " << std::fixed << std::setprecision(2)
                 << x264Result.encodingTime << " 秒" << std::endl;
        std::cout << "平均FPS: " << x264Result.fps << std::endl;
    } else {
        std::cout << "测试失败: " << x264Result.errorMessage << std::endl;
    }

    std::cout << "\n----------------------------------------" << std::endl;

    // 性能对比
    if (nvencResult.success && x264Result.success) {
        std::cout << "\n性能对比：" << std::endl;
        double speedup = nvencResult.fps / x264Result.fps;
        std::cout << "NVENC相对于x264的加速比: " << std::fixed << std::setprecision(2)
                 << speedup << "x" << std::endl;
    }
} 