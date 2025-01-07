#include "aac_parser.hpp"
#include <stdexcept>
#include <fstream>
#include <iostream>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/dict.h>
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
    impl_->audioInfo.sample_rate = stream->codecpar->sample_rate;
    impl_->audioInfo.channels = stream->codecpar->ch_layout.nb_channels;
    impl_->audioInfo.profile = stream->codecpar->profile;
    impl_->audioInfo.duration = stream->duration * av_q2d(stream->time_base);
    impl_->audioInfo.bitrate = stream->codecpar->bit_rate;
    impl_->audioInfo.total_frames = stream->nb_frames;
    impl_->audioInfo.format = "AAC";

    // 扫描所有帧
    AVPacket* packet = av_packet_alloc();
    av_seek_frame(impl_->formatCtx, -1, 0, AVSEEK_FLAG_BACKWARD);
    
    while (av_read_frame(impl_->formatCtx, packet) >= 0) {
        if (packet->stream_index == impl_->audioStreamIndex) {
            FrameInfo frame;
            if (parseADTSHeader(packet->data, packet->size, frame)) {
                frame.offset = packet->pos;
                frame.timestamp = packet->pts * av_q2d(stream->time_base);
                impl_->frames.push_back(frame);
            }
        }
        av_packet_unref(packet);
    }
    
    av_packet_free(&packet);
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
    frame.profile = ((data[2] & 0xC0) >> 6) + 1;
    
    static const int sample_rates[] = {
        96000, 88200, 64000, 48000, 44100, 32000,
        24000, 22050, 16000, 12000, 11025, 8000
    };
    int sample_rate_idx = (data[2] & 0x3C) >> 2;
    frame.sample_rate = (sample_rate_idx < 12) ? sample_rates[sample_rate_idx] : 0;
    
    frame.channels = ((data[2] & 0x01) << 2) | ((data[3] & 0xC0) >> 6);
    
    frame.size = ((data[3] & 0x03) << 11) | 
                 (data[4] << 3) | 
                 ((data[5] & 0xE0) >> 5);

    return true;
} 