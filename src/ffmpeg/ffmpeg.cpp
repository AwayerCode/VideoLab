#include "ffmpeg.hpp"
#include <iostream>

FFmpeg::FFmpeg() {
    // 在新版本的 FFmpeg 中，不需要显式调用 av_register_all()
}

FFmpeg::~FFmpeg() {
    closeFile();
}

bool FFmpeg::openFile(const std::string& filename) {
    // 打开视频文件
    if (avformat_open_input(&formatContext_, filename.c_str(), nullptr, nullptr) < 0) {
        std::cerr << "Could not open file " << filename << std::endl;
        return false;
    }

    // 获取流信息
    if (avformat_find_stream_info(formatContext_, nullptr) < 0) {
        std::cerr << "Could not find stream information" << std::endl;
        return false;
    }

    // 找到视频流
    for (unsigned int i = 0; i < formatContext_->nb_streams; i++) {
        if (formatContext_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex_ = i;
            break;
        }
    }

    if (videoStreamIndex_ == -1) {
        std::cerr << "Could not find video stream" << std::endl;
        return false;
    }

    // 获取解码器
    const AVCodec* codec = avcodec_find_decoder(formatContext_->streams[videoStreamIndex_]->codecpar->codec_id);
    if (!codec) {
        std::cerr << "Unsupported codec" << std::endl;
        return false;
    }

    // 分配解码器上下文
    codecContext_ = avcodec_alloc_context3(codec);
    if (!codecContext_) {
        std::cerr << "Could not allocate video codec context" << std::endl;
        return false;
    }

    // 将编解码器参数复制到上下文
    if (avcodec_parameters_to_context(codecContext_, formatContext_->streams[videoStreamIndex_]->codecpar) < 0) {
        std::cerr << "Could not copy codec params to codec context" << std::endl;
        return false;
    }

    // 打开解码器
    if (avcodec_open2(codecContext_, codec, nullptr) < 0) {
        std::cerr << "Could not open codec" << std::endl;
        return false;
    }

    // 分配帧和数据包
    frame_ = av_frame_alloc();
    packet_ = av_packet_alloc();

    if (!frame_ || !packet_) {
        std::cerr << "Could not allocate frame or packet" << std::endl;
        return false;
    }

    return true;
}

void FFmpeg::closeFile() {
    if (codecContext_) {
        avcodec_free_context(&codecContext_);
    }
    if (formatContext_) {
        avformat_close_input(&formatContext_);
    }
    if (frame_) {
        av_frame_free(&frame_);
    }
    if (packet_) {
        av_packet_free(&packet_);
    }
    videoStreamIndex_ = -1;
}

bool FFmpeg::readFrame() {
    if (!formatContext_ || !codecContext_ || !frame_ || !packet_) {
        return false;
    }

    int response;
    while (av_read_frame(formatContext_, packet_) >= 0) {
        if (packet_->stream_index == videoStreamIndex_) {
            response = avcodec_send_packet(codecContext_, packet_);
            if (response < 0) {
                std::cerr << "Error while sending packet to decoder" << std::endl;
                return false;
            }

            response = avcodec_receive_frame(codecContext_, frame_);
            if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
                continue;
            } else if (response < 0) {
                std::cerr << "Error while receiving frame from decoder" << std::endl;
                return false;
            }

            av_packet_unref(packet_);
            return true;
        }
        av_packet_unref(packet_);
    }

    return false;
}
