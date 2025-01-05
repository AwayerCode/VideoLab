#include "videofilter.hpp"
#include <sstream>
#include <iostream>

VideoFilter::VideoFilter() = default;

VideoFilter::~VideoFilter() {
    close();
}

bool VideoFilter::init(const std::string& inputFile, const std::string& outputFile) {
    // 打开输入文件
    if (avformat_open_input(&inputFormatContext_, inputFile.c_str(), nullptr, nullptr) < 0) {
        std::cerr << "无法打开输入文件" << std::endl;
        return false;
    }

    // 获取流信息
    if (avformat_find_stream_info(inputFormatContext_, nullptr) < 0) {
        std::cerr << "无法获取流信息" << std::endl;
        return false;
    }

    // 找到视频流
    for (unsigned int i = 0; i < inputFormatContext_->nb_streams; i++) {
        if (inputFormatContext_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex_ = i;
            break;
        }
    }

    if (videoStreamIndex_ == -1) {
        std::cerr << "找不到视频流" << std::endl;
        return false;
    }

    // 获取解码器
    const AVCodec* decoder = avcodec_find_decoder(inputFormatContext_->streams[videoStreamIndex_]->codecpar->codec_id);
    if (!decoder) {
        std::cerr << "找不到解码器" << std::endl;
        return false;
    }

    // 创建解码器上下文
    decoderContext_ = avcodec_alloc_context3(decoder);
    if (!decoderContext_) {
        std::cerr << "无法分配解码器上下文" << std::endl;
        return false;
    }

    // 复制编解码器参数
    if (avcodec_parameters_to_context(decoderContext_, inputFormatContext_->streams[videoStreamIndex_]->codecpar) < 0) {
        std::cerr << "无法复制编解码器参数" << std::endl;
        return false;
    }

    // 打开解码器
    if (avcodec_open2(decoderContext_, decoder, nullptr) < 0) {
        std::cerr << "无法打开解码器" << std::endl;
        return false;
    }

    // 创建输出上下文
    avformat_alloc_output_context2(&outputFormatContext_, nullptr, nullptr, outputFile.c_str());
    if (!outputFormatContext_) {
        std::cerr << "无法创建输出上下文" << std::endl;
        return false;
    }

    // 创建输出流
    AVStream* outStream = avformat_new_stream(outputFormatContext_, nullptr);
    if (!outStream) {
        std::cerr << "无法创建输出流" << std::endl;
        return false;
    }

    // 复制编解码器参数到输出流
    avcodec_parameters_copy(outStream->codecpar, inputFormatContext_->streams[videoStreamIndex_]->codecpar);

    // 分配帧和包
    frame_ = av_frame_alloc();
    filteredFrame_ = av_frame_alloc();
    packet_ = av_packet_alloc();

    if (!frame_ || !filteredFrame_ || !packet_) {
        std::cerr << "无法分配帧或包" << std::endl;
        return false;
    }

    return true;
}

bool VideoFilter::updateFilter(const FilterParams& params) {
    // 清理旧的滤镜图
    if (filterGraph_) {
        avfilter_graph_free(&filterGraph_);
        bufferSrcContext_ = nullptr;
        bufferSinkContext_ = nullptr;
    }

    return initFilter(params);
}

bool VideoFilter::initFilter(const FilterParams& params) {
    char args[512];
    int ret;

    // 创建滤镜图
    filterGraph_ = avfilter_graph_alloc();
    if (!filterGraph_) {
        std::cerr << "无法创建滤镜图" << std::endl;
        return false;
    }

    // 创建 buffer source
    const AVFilter* bufferSrc = avfilter_get_by_name("buffer");
    if (!bufferSrc) {
        std::cerr << "找不到 buffer 滤镜" << std::endl;
        return false;
    }

    // 获取视频流的时基
    AVRational time_base = inputFormatContext_->streams[videoStreamIndex_]->time_base;
    if (time_base.num == 0 || time_base.den == 0) {
        // 如果时基无效，使用默认值
        time_base.num = 1;
        time_base.den = 25;  // 25 fps
    }

    // 获取视频流的像素格式
    AVPixelFormat pix_fmt = decoderContext_->pix_fmt;
    if (pix_fmt == AV_PIX_FMT_NONE) {
        std::cerr << "无效的像素格式" << std::endl;
        return false;
    }

    // 获取采样宽高比
    AVRational sar = decoderContext_->sample_aspect_ratio;
    if (sar.num == 0 || sar.den == 0) {
        sar.num = 1;
        sar.den = 1;
    }

    snprintf(args, sizeof(args),
        "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
        decoderContext_->width, decoderContext_->height, 
        pix_fmt,
        time_base.num, time_base.den,
        sar.num, sar.den);

    std::cout << "Buffer source args: " << args << std::endl;

    ret = avfilter_graph_create_filter(&bufferSrcContext_, bufferSrc, "in",
                                     args, nullptr, filterGraph_);
    if (ret < 0) {
        char err[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_strerror(ret, err, AV_ERROR_MAX_STRING_SIZE);
        std::cerr << "创建 buffer source 失败: " << err << std::endl;
        return false;
    }

    // 创建 buffer sink
    const AVFilter* bufferSink = avfilter_get_by_name("buffersink");
    if (!bufferSink) {
        std::cerr << "找不到 buffersink 滤镜" << std::endl;
        return false;
    }

    ret = avfilter_graph_create_filter(&bufferSinkContext_, bufferSink, "out",
                                     nullptr, nullptr, filterGraph_);
    if (ret < 0) {
        char err[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_strerror(ret, err, AV_ERROR_MAX_STRING_SIZE);
        std::cerr << "创建 buffer sink 失败: " << err << std::endl;
        return false;
    }

    // 构建滤镜链
    std::string filterStr = getFilterString(params);
    AVFilterInOut* inputs = avfilter_inout_alloc();
    AVFilterInOut* outputs = avfilter_inout_alloc();

    if (!inputs || !outputs) {
        std::cerr << "无法分配滤镜输入/输出" << std::endl;
        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
        return false;
    }

    outputs->name = av_strdup("in");
    outputs->filter_ctx = bufferSrcContext_;
    outputs->pad_idx = 0;
    outputs->next = nullptr;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = bufferSinkContext_;
    inputs->pad_idx = 0;
    inputs->next = nullptr;

    ret = avfilter_graph_parse_ptr(filterGraph_, filterStr.c_str(),
                                 &inputs, &outputs, nullptr);
    if (ret < 0) {
        char err[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_strerror(ret, err, AV_ERROR_MAX_STRING_SIZE);
        std::cerr << "解析滤镜字符串失败: " << err << std::endl;
        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
        return false;
    }

    ret = avfilter_graph_config(filterGraph_, nullptr);
    if (ret < 0) {
        char err[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_strerror(ret, err, AV_ERROR_MAX_STRING_SIZE);
        std::cerr << "配置滤镜图失败: " << err << std::endl;
        return false;
    }

    std::cout << "滤镜初始化成功" << std::endl;
    return true;
}

std::string VideoFilter::getFilterString(const FilterParams& params) {
    std::stringstream ss;
    switch (params.type) {
        case FilterType::BRIGHTNESS_CONTRAST:
            ss << "eq=brightness=" << (params.bc.brightness / 100.0)
               << ":contrast=" << ((params.bc.contrast + 100) / 100.0);
            break;
        case FilterType::HSV_ADJUST:
            ss << "hue=h=" << params.hsv.hue
               << ":s=" << (params.hsv.saturation / 100.0)
               << ":b=" << (params.hsv.value / 100.0);
            break;
        case FilterType::SHARPEN_BLUR:
            if (params.sb.sharpen > 0) {
                ss << "unsharp=5:5:" << (params.sb.sharpen * 0.5);
            } else if (params.sb.blur > 0) {
                ss << "boxblur=" << params.sb.blur << ":1";
            } else {
                ss << "null";
            }
            break;
    }
    std::string filterStr = ss.str();
    std::cout << "Filter string: " << filterStr << std::endl;
    return filterStr;
}

bool VideoFilter::processFrame() {
    int ret;

    // 读取一帧
    if ((ret = av_read_frame(inputFormatContext_, packet_)) < 0) {
        return false;
    }

    if (packet_->stream_index == videoStreamIndex_) {
        // 发送包到解码器
        ret = avcodec_send_packet(decoderContext_, packet_);
        if (ret < 0) {
            std::cerr << "发送包到解码器失败" << std::endl;
            av_packet_unref(packet_);
            return false;
        }

        // 接收解码后的帧
        ret = avcodec_receive_frame(decoderContext_, frame_);
        if (ret < 0) {
            if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
                std::cerr << "接收解码帧失败" << std::endl;
            }
            av_packet_unref(packet_);
            return false;
        }

        // 将帧添加到滤镜
        ret = av_buffersrc_add_frame_flags(bufferSrcContext_, frame_,
            AV_BUFFERSRC_FLAG_KEEP_REF);
        if (ret < 0) {
            std::cerr << "添加帧到滤镜失败" << std::endl;
            av_frame_unref(frame_);
            av_packet_unref(packet_);
            return false;
        }

        // 从滤镜获取处理后的帧
        ret = av_buffersink_get_frame(bufferSinkContext_, filteredFrame_);
        if (ret < 0) {
            if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
                std::cerr << "从滤镜获取帧失败" << std::endl;
            }
            av_frame_unref(frame_);
            av_packet_unref(packet_);
            return false;
        }

        // 编码处理后的帧
        if (!outputFormatContext_->streams[0]->codecpar->codec_id) {
            // 如果还没有设置编码器，设置为与输入相同的编码器
            outputFormatContext_->streams[0]->codecpar->codec_id = 
                inputFormatContext_->streams[videoStreamIndex_]->codecpar->codec_id;
        }

        // 获取编码器
        const AVCodec* encoder = avcodec_find_encoder(
            outputFormatContext_->streams[0]->codecpar->codec_id);
        if (!encoder) {
            std::cerr << "找不到编码器" << std::endl;
            av_frame_unref(filteredFrame_);
            av_frame_unref(frame_);
            av_packet_unref(packet_);
            return false;
        }

        // 创建编码器上下文（如果还没有创建）
        if (!encoderContext_) {
            encoderContext_ = avcodec_alloc_context3(encoder);
            if (!encoderContext_) {
                std::cerr << "无法分配编码器上下文" << std::endl;
                av_frame_unref(filteredFrame_);
                av_frame_unref(frame_);
                av_packet_unref(packet_);
                return false;
            }

            // 设置编码器参数
            encoderContext_->width = filteredFrame_->width;
            encoderContext_->height = filteredFrame_->height;
            encoderContext_->pix_fmt = (AVPixelFormat)filteredFrame_->format;
            encoderContext_->time_base = av_inv_q(inputFormatContext_->streams[videoStreamIndex_]->r_frame_rate);
            encoderContext_->bit_rate = inputFormatContext_->streams[videoStreamIndex_]->codecpar->bit_rate;

            // 打开编码器
            if (avcodec_open2(encoderContext_, encoder, nullptr) < 0) {
                std::cerr << "无法打开编码器" << std::endl;
                av_frame_unref(filteredFrame_);
                av_frame_unref(frame_);
                av_packet_unref(packet_);
                return false;
            }

            // 打开输出文件
            if (!(outputFormatContext_->oformat->flags & AVFMT_NOFILE)) {
                if (avio_open(&outputFormatContext_->pb, 
                    outputFormatContext_->url, AVIO_FLAG_WRITE) < 0) {
                    std::cerr << "无法打开输出文件" << std::endl;
                    av_frame_unref(filteredFrame_);
                    av_frame_unref(frame_);
                    av_packet_unref(packet_);
                    return false;
                }
            }

            // 写入文件头
            if (avformat_write_header(outputFormatContext_, nullptr) < 0) {
                std::cerr << "无法写入文件头" << std::endl;
                av_frame_unref(filteredFrame_);
                av_frame_unref(frame_);
                av_packet_unref(packet_);
                return false;
            }
        }

        // 编码帧
        ret = avcodec_send_frame(encoderContext_, filteredFrame_);
        if (ret < 0) {
            std::cerr << "发送帧到编码器失败" << std::endl;
            av_frame_unref(filteredFrame_);
            av_frame_unref(frame_);
            av_packet_unref(packet_);
            return false;
        }

        while (ret >= 0) {
            ret = avcodec_receive_packet(encoderContext_, packet_);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            }
            if (ret < 0) {
                std::cerr << "从编码器接收包失败" << std::endl;
                break;
            }

            // 写入编码后的包
            av_packet_rescale_ts(packet_,
                encoderContext_->time_base,
                outputFormatContext_->streams[0]->time_base);
            
            ret = av_interleaved_write_frame(outputFormatContext_, packet_);
            if (ret < 0) {
                std::cerr << "写入包失败" << std::endl;
                break;
            }
        }

        av_frame_unref(filteredFrame_);
        av_frame_unref(frame_);
    }

    av_packet_unref(packet_);
    return true;
}

void VideoFilter::close() {
    // 写入文件尾
    if (outputFormatContext_ && outputFormatContext_->pb) {
        av_write_trailer(outputFormatContext_);
    }

    // 关闭编码器
    if (encoderContext_) {
        avcodec_free_context(&encoderContext_);
    }

    // 关闭滤镜
    if (filterGraph_) {
        avfilter_graph_free(&filterGraph_);
    }

    // 释放帧和包
    if (frame_) {
        av_frame_free(&frame_);
    }
    if (filteredFrame_) {
        av_frame_free(&filteredFrame_);
    }
    if (packet_) {
        av_packet_free(&packet_);
    }

    // 关闭解码器
    if (decoderContext_) {
        avcodec_free_context(&decoderContext_);
    }

    // 关闭输入文件
    if (inputFormatContext_) {
        avformat_close_input(&inputFormatContext_);
    }

    // 关闭输出文件
    if (outputFormatContext_) {
        if (!(outputFormatContext_->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&outputFormatContext_->pb);
        }
        avformat_free_context(outputFormatContext_);
    }

    // 重置状态
    inputFormatContext_ = nullptr;
    outputFormatContext_ = nullptr;
    decoderContext_ = nullptr;
    encoderContext_ = nullptr;
    filterGraph_ = nullptr;
    bufferSrcContext_ = nullptr;
    bufferSinkContext_ = nullptr;
    frame_ = nullptr;
    filteredFrame_ = nullptr;
    packet_ = nullptr;
    videoStreamIndex_ = -1;
} 