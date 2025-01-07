#include "aac_parser.hpp"
#include <stdexcept>
#include <fstream>
#include <iostream>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/dict.h>
#include <libavutil/channel_layout.h>
}

AACParser::Impl::~Impl() {
    if (formatCtx) {
        avformat_close_input(&formatCtx);
    }
}

AACParser::~AACParser() = default;

bool AACParser::open(const std::string& filename)
{
    impl_ = std::make_unique<Impl>();
    
    // 打开文件
    impl_->formatCtx = avformat_alloc_context();
    if (avformat_open_input(&impl_->formatCtx, filename.c_str(), nullptr, nullptr) < 0) {
        return false;
    }
    
    // 读取流信息
    if (avformat_find_stream_info(impl_->formatCtx, nullptr) < 0) {
        return false;
    }
    
    // 查找音频流
    for (unsigned int i = 0; i < impl_->formatCtx->nb_streams; i++) {
        if (impl_->formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO &&
            impl_->formatCtx->streams[i]->codecpar->codec_id == AV_CODEC_ID_AAC) {
            impl_->audioStreamIndex = i;
            break;
        }
    }
    
    if (impl_->audioStreamIndex < 0) {
        return false;
    }

    // 获取音频信息
    AVStream* stream = impl_->formatCtx->streams[impl_->audioStreamIndex];
    AVCodecParameters* codecParams = stream->codecpar;
    
    // 获取解码器以获取更准确的信息
    const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);
    AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecCtx, codecParams);
    avcodec_open2(codecCtx, codec, nullptr);
    
    // 修复声道数解析
    impl_->audioInfo.channels = codecCtx->ch_layout.nb_channels;
    if (impl_->audioInfo.channels <= 0 || impl_->audioInfo.channels > 8) {
        // 如果声道数异常，尝试从channel_layout中获取
        if (codecCtx->ch_layout.u.mask != 0) {
            impl_->audioInfo.channels = av_get_channel_layout_nb_channels(codecCtx->ch_layout.u.mask);
        } else {
            // 从ADTS头部获取声道数
            uint8_t* data = nullptr;
            int size = 0;
            AVPacket* packet = av_packet_alloc();
            if (av_read_frame(impl_->formatCtx, packet) >= 0) {
                FrameInfo frame;
                if (parseADTSHeader(packet->data, packet->size, frame)) {
                    impl_->audioInfo.channels = frame.channels;
                }
                av_packet_unref(packet);
            }
            av_packet_free(&packet);
            
            if (impl_->audioInfo.channels <= 0) {
                impl_->audioInfo.channels = 2; // 默认立体声
            }
        }
    }
    
    impl_->audioInfo.sample_rate = codecCtx->sample_rate;
    impl_->audioInfo.profile = codecCtx->profile;
    
    // 修复duration计算
    if (stream->duration != AV_NOPTS_VALUE) {
        impl_->audioInfo.duration = stream->duration * av_q2d(stream->time_base);
    } else if (impl_->formatCtx->duration != AV_NOPTS_VALUE) {
        impl_->audioInfo.duration = impl_->formatCtx->duration / (double)AV_TIME_BASE;
    } else {
        impl_->audioInfo.duration = 0;
    }
    
    // 重置文件位置
    av_seek_frame(impl_->formatCtx, impl_->audioStreamIndex, 0, AVSEEK_FLAG_BACKWARD);
    
    // 扫描所有帧并计算实际比特率
    AVPacket* packet = av_packet_alloc();
    int64_t totalBytes = 0;
    int frameCount = 0;
    double lastTimestamp = 0;
    
    while (av_read_frame(impl_->formatCtx, packet) >= 0) {
        if (packet->stream_index == impl_->audioStreamIndex) {
            FrameInfo frame;
            if (parseADTSHeader(packet->data, packet->size, frame)) {
                frame.offset = packet->pos;
                totalBytes += packet->size;
                
                // 修复时间戳计算
                if (packet->pts != AV_NOPTS_VALUE) {
                    frame.timestamp = packet->pts * av_q2d(stream->time_base);
                } else if (packet->dts != AV_NOPTS_VALUE) {
                    frame.timestamp = packet->dts * av_q2d(stream->time_base);
                } else {
                    // 使用标准AAC帧长度（1024采样点）计算时间戳
                    frame.timestamp = frameCount * 1024.0 / frame.sample_rate;
                }
                
                lastTimestamp = frame.timestamp;
                impl_->frames.push_back(frame);
                frameCount++;
            }
        }
        av_packet_unref(packet);
    }
    
    // 更新总帧数和比特率
    impl_->audioInfo.total_frames = frameCount;
    if (impl_->audioInfo.duration > 0) {
        impl_->audioInfo.bitrate = (int64_t)(totalBytes * 8.0 / impl_->audioInfo.duration);
    } else if (lastTimestamp > 0) {
        impl_->audioInfo.bitrate = (int64_t)(totalBytes * 8.0 / lastTimestamp);
    } else {
        impl_->audioInfo.bitrate = codecCtx->bit_rate;
    }
    
    impl_->audioInfo.format = "AAC";
    
    // 清理资源
    av_packet_free(&packet);
    avcodec_free_context(&codecCtx);
    return true;
}

void AACParser::close()
{
    impl_.reset();
}

AACParser::AudioInfo AACParser::getAudioInfo() const
{
    if (!impl_) {
        return AudioInfo{};
    }
    return impl_->audioInfo;
}

std::vector<AACParser::FrameInfo> AACParser::getFrames() const
{
    if (!impl_) {
        return {};
    }
    return impl_->frames;
}

bool AACParser::parseADTSHeader(const uint8_t* data, int size, FrameInfo& frame)
{
    if (size < 7) {  // ADTS头部最小7字节
        return false;
    }

    // 检查同步字
    if (data[0] != 0xFF || (data[1] & 0xF0) != 0xF0) {
        return false;
    }

    // 解析ADTS头部
    frame.has_crc = !(data[1] & 0x01);
    
    // 修复Profile解析
    int profile_raw = ((data[2] & 0xC0) >> 6);
    switch (profile_raw) {
        case 0: frame.profile = 1; break;  // Main profile
        case 1: frame.profile = 2; break;  // LC (Low Complexity)
        case 2: frame.profile = 3; break;  // SSR (Scalable Sample Rate)
        case 3: frame.profile = 4; break;  // LTP (Long Term Prediction)
        default: frame.profile = 2; break; // 默认使用LC
    }
    
    static const int sample_rates[] = {
        96000, 88200, 64000, 48000, 44100, 32000,
        24000, 22050, 16000, 12000, 11025, 8000
    };
    int sample_rate_idx = (data[2] & 0x3C) >> 2;
    frame.sample_rate = (sample_rate_idx < 12) ? sample_rates[sample_rate_idx] : 0;
    
    // 修复声道数解析
    int channel_config = ((data[2] & 0x01) << 2) | ((data[3] & 0xC0) >> 6);
    switch (channel_config) {
        case 0: // 在AOT Specific Config中定义
            frame.channels = 2; // 默认立体声
            break;
        case 1: // 单声道
            frame.channels = 1;
            break;
        case 2: // 立体声
            frame.channels = 2;
            break;
        case 3: // 3声道
            frame.channels = 3;
            break;
        case 4: // 4声道
            frame.channels = 4;
            break;
        case 5: // 5声道
            frame.channels = 5;
            break;
        case 6: // 5.1声道
            frame.channels = 6;
            break;
        case 7: // 7.1声道
            frame.channels = 8;
            break;
        default:
            frame.channels = 2; // 默认立体声
    }
    
    // 计算帧大小（包括头部）
    frame.size = ((data[3] & 0x03) << 11) | 
                 (data[4] << 3) | 
                 ((data[5] & 0xE0) >> 5);

    // 验证帧大小的合理性
    if (frame.size > size || frame.size < (frame.has_crc ? 9 : 7)) {
        return false;
    }

    return true;
} 