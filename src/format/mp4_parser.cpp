#include "mp4_parser.hpp"
#include <stdexcept>
#include <fstream>
#include <iostream>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/dict.h>
}

MP4Parser::Impl::~Impl() {
    if (formatCtx) {
        avformat_close_input(&formatCtx);
    }
}

MP4Parser::~MP4Parser() = default;

bool MP4Parser::open(const std::string& filename)
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
    
    // 查找视频和音频流
    for (unsigned int i = 0; i < impl_->formatCtx->nb_streams; i++) {
        if (impl_->formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            impl_->videoStreamIndex = i;
        } else if (impl_->formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            impl_->audioStreamIndex = i;
        }
    }
    
    return true;
}

void MP4Parser::close()
{
    impl_.reset();
}

std::vector<MP4Parser::BoxInfo> MP4Parser::getBoxes() const
{
    if (!impl_ || !impl_->formatCtx) {
        return {};
    }
    
    std::vector<BoxInfo> boxes;
    
    // 保存当前位置
    int64_t original_pos = avio_tell(impl_->formatCtx->pb);
    
    // 定位到文件开始
    avio_seek(impl_->formatCtx->pb, 0, SEEK_SET);
    
    // 解析box结构
    parseBoxes(impl_->formatCtx->pb, 0, &boxes);
    
    // 恢复原始位置
    avio_seek(impl_->formatCtx->pb, original_pos, SEEK_SET);
    
    return boxes;
}

void MP4Parser::parseBoxes(AVIOContext* pb, int level, std::vector<BoxInfo>* boxes) const
{
    while (!avio_feof(pb)) {
        int64_t start_pos = avio_tell(pb);
        
        // 读取box大小和类型
        uint32_t size = avio_rb32(pb);  // big-endian 32-bit size
        char type[5] = {0};
        avio_read(pb, (unsigned char*)type, 4);
        
        // 处理64位大小的box
        uint64_t largesize = size;
        if (size == 1) {
            largesize = avio_rb64(pb);
        }
        
        // 创建box信息
        BoxInfo box;
        box.type = type;
        box.size = (size == 1) ? largesize : size;
        box.offset = start_pos;
        box.level = level;
        boxes->push_back(box);
        
        // 处理容器box
        if (isContainerBox(type)) {
            parseBoxes(pb, level + 1, boxes);
        } else {
            // 跳过数据部分
            int64_t data_size = (size == 1) ? (largesize - 16) : (size - 8);
            if (data_size > 0) {
                avio_skip(pb, data_size);
            }
        }
        
        // 如果是最后一个box或者size为0，退出循环
        if (size == 0 || avio_feof(pb)) {
            break;
        }
    }
}

bool MP4Parser::isContainerBox(const char* type) const
{
    static const char* containers[] = {
        "moov", "trak", "edts", "mdia", "minf", "dinf", "stbl", "mvex",
        "moof", "traf", "mfra", "skip", "meta", "ipro", "sinf", "fiin",
        "paen", "meco", "mere"
    };
    
    for (const char* container : containers) {
        if (strcmp(type, container) == 0) {
            return true;
        }
    }
    return false;
}

MP4Parser::VideoInfo MP4Parser::getVideoInfo() const
{
    VideoInfo info = {};
    if (!impl_ || impl_->videoStreamIndex < 0) {
        return info;
    }
    
    AVStream* stream = impl_->formatCtx->streams[impl_->videoStreamIndex];
    info.width = stream->codecpar->width;
    info.height = stream->codecpar->height;
    info.duration = stream->duration * av_q2d(stream->time_base);
    info.bitrate = stream->codecpar->bit_rate;
    info.fps = av_q2d(stream->avg_frame_rate);
    info.total_frames = stream->nb_frames;
    info.codec_name = avcodec_get_name(stream->codecpar->codec_id);
    info.format_name = impl_->formatCtx->iformat->name;
    
    return info;
}

MP4Parser::AudioInfo MP4Parser::getAudioInfo() const
{
    AudioInfo info = {};
    if (!impl_ || impl_->audioStreamIndex < 0) {
        return info;
    }
    
    AVStream* stream = impl_->formatCtx->streams[impl_->audioStreamIndex];
    info.channels = stream->codecpar->ch_layout.nb_channels;
    info.sample_rate = stream->codecpar->sample_rate;
    info.duration = stream->duration * av_q2d(stream->time_base);
    info.bitrate = stream->codecpar->bit_rate;
    info.codec_name = avcodec_get_name(stream->codecpar->codec_id);
    
    return info;
}

std::map<std::string, std::string> MP4Parser::getMetadata() const
{
    std::map<std::string, std::string> metadata;
    if (!impl_ || !impl_->formatCtx) {
        return metadata;
    }
    
    AVDictionaryEntry* tag = nullptr;
    while ((tag = av_dict_get(impl_->formatCtx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
        metadata[tag->key] = tag->value;
    }
    
    return metadata;
}

std::vector<MP4Parser::KeyFrameInfo> MP4Parser::getKeyFrameInfo() const
{
    std::vector<KeyFrameInfo> keyFrames;
    if (!impl_ || impl_->videoStreamIndex < 0) {
        return keyFrames;
    }
    
    AVStream* stream = impl_->formatCtx->streams[impl_->videoStreamIndex];
    AVPacket* packet = av_packet_alloc();
    
    // 定位到文件开始
    av_seek_frame(impl_->formatCtx, -1, 0, AVSEEK_FLAG_BACKWARD);
    
    // 扫描视频流寻找关键帧
    while (av_read_frame(impl_->formatCtx, packet) >= 0) {
        if (packet->stream_index == impl_->videoStreamIndex) {
            if (packet->flags & AV_PKT_FLAG_KEY) {
                KeyFrameInfo info;
                info.timestamp = packet->pts * av_q2d(stream->time_base);
                info.pos = packet->pos;
                keyFrames.push_back(info);
            }
        }
        av_packet_unref(packet);
    }
    
    av_packet_free(&packet);
    return keyFrames;
}

bool MP4Parser::extractH264(const std::string& outputPath) const
{
    if (!impl_ || impl_->videoStreamIndex < 0) {
        return false;
    }
    
    AVStream* stream = impl_->formatCtx->streams[impl_->videoStreamIndex];
    if (stream->codecpar->codec_id != AV_CODEC_ID_H264) {
        return false;
    }
    
    std::ofstream outFile(outputPath, std::ios::binary);
    if (!outFile) {
        return false;
    }
    
    AVPacket* packet = av_packet_alloc();
    
    // 写入 SPS/PPS
    if (stream->codecpar->extradata && stream->codecpar->extradata_size > 0) {
        const uint8_t* extradata = stream->codecpar->extradata;
        int extradata_size = stream->codecpar->extradata_size;
        
        // 遍历 extradata 中的 NALU
        int offset = 0;
        while (offset + 4 < extradata_size) {
            // 获取 NALU 长度
            int nalu_size = (extradata[offset] << 24) | 
                           (extradata[offset + 1] << 16) | 
                           (extradata[offset + 2] << 8) | 
                           extradata[offset + 3];
            
            // 写入起始码
            const uint8_t start_code[] = {0x00, 0x00, 0x00, 0x01};
            outFile.write(reinterpret_cast<const char*>(start_code), 4);
            
            // 写入 NALU 数据
            outFile.write(reinterpret_cast<const char*>(extradata + offset + 4), nalu_size);
            
            offset += 4 + nalu_size;
        }
    }
    
    // 定位到文件开始
    av_seek_frame(impl_->formatCtx, -1, 0, AVSEEK_FLAG_BACKWARD);
    
    // 写入H264数据
    while (av_read_frame(impl_->formatCtx, packet) >= 0) {
        if (packet->stream_index == impl_->videoStreamIndex) {
            const uint8_t* data = packet->data;
            int size = packet->size;
            int offset = 0;
            
            // 处理 AVC1/H264 封装格式
            while (offset + 4 < size) {
                // 获取 NALU 长度
                int nalu_size = (data[offset] << 24) | 
                               (data[offset + 1] << 16) | 
                               (data[offset + 2] << 8) | 
                               data[offset + 3];
                
                if (nalu_size <= 0 || offset + 4 + nalu_size > size) {
                    break;
                }
                
                // 写入起始码
                const uint8_t start_code[] = {0x00, 0x00, 0x00, 0x01};
                outFile.write(reinterpret_cast<const char*>(start_code), 4);
                
                // 写入 NALU 数据
                outFile.write(reinterpret_cast<const char*>(data + offset + 4), nalu_size);
                
                offset += 4 + nalu_size;
            }
        }
        av_packet_unref(packet);
    }
    
    av_packet_free(&packet);
    outFile.close();
    return true;
}

bool MP4Parser::extractAAC(const std::string& outputPath) const
{
    if (!impl_ || impl_->audioStreamIndex < 0) {
        return false;
    }
    
    AVStream* stream = impl_->formatCtx->streams[impl_->audioStreamIndex];
    if (stream->codecpar->codec_id != AV_CODEC_ID_AAC) {
        return false;
    }
    
    std::ofstream outFile(outputPath, std::ios::binary);
    if (!outFile) {
        return false;
    }
    
    AVPacket* packet = av_packet_alloc();
    
    // 定位到文件开始
    av_seek_frame(impl_->formatCtx, -1, 0, AVSEEK_FLAG_BACKWARD);
    
    // 写入AAC数据
    while (av_read_frame(impl_->formatCtx, packet) >= 0) {
        if (packet->stream_index == impl_->audioStreamIndex) {
            // 写入ADTS头
            uint8_t adts_header[7];
            int aac_frame_length = packet->size + 7;
            
            adts_header[0] = 0xFF;  // Sync Point
            adts_header[1] = 0xF1;  // MPEG-4, Layer 0, Protected
            adts_header[2] = ((stream->codecpar->profile - 1) << 6) |
                            ((stream->codecpar->ch_layout.nb_channels > 1 ? 1 : 2) << 2) |
                            ((aac_frame_length >> 11) & 0x03);
            adts_header[3] = (aac_frame_length >> 3) & 0xFF;
            adts_header[4] = ((aac_frame_length & 0x07) << 5) | 0x1F;
            adts_header[5] = 0xFC;
            adts_header[6] = 0x00;
            
            outFile.write(reinterpret_cast<const char*>(adts_header), 7);
            outFile.write(reinterpret_cast<const char*>(packet->data), packet->size);
        }
        av_packet_unref(packet);
    }
    
    av_packet_free(&packet);
    outFile.close();
    return true;
} 