#include "h264_parser.hpp"
#include <iostream>
#include <cmath>

H264Parser::H264Parser() = default;

H264Parser::~H264Parser() {
    cleanup();
}

void H264Parser::cleanup() {
    if (swsCtx_) {
        sws_freeContext(swsCtx_);
        swsCtx_ = nullptr;
    }
    
    if (videoCodecCtx_) {
        avcodec_free_context(&videoCodecCtx_);
    }
    
    if (formatCtx_) {
        avformat_close_input(&formatCtx_);
        formatCtx_ = nullptr;
    }
    
    videoStreamIndex_ = -1;
    nalUnits_.clear();
}

bool H264Parser::initVideoCodec() {
    AVStream* videoStream = formatCtx_->streams[videoStreamIndex_];
    const AVCodec* codec = avcodec_find_decoder(videoStream->codecpar->codec_id);
    if (!codec) {
        std::cerr << "Cannot find H264 decoder" << std::endl;
        return false;
    }

    videoCodecCtx_ = avcodec_alloc_context3(codec);
    if (!videoCodecCtx_) {
        std::cerr << "Cannot allocate video codec context" << std::endl;
        return false;
    }

    if (avcodec_parameters_to_context(videoCodecCtx_, videoStream->codecpar) < 0) {
        std::cerr << "Cannot copy codec parameters" << std::endl;
        return false;
    }

    if (avcodec_open2(videoCodecCtx_, codec, nullptr) < 0) {
        std::cerr << "Cannot open video codec" << std::endl;
        return false;
    }

    return true;
}

bool H264Parser::open(const std::string& filename) {
    cleanup();

    int ret = avformat_open_input(&formatCtx_, filename.c_str(), nullptr, nullptr);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, sizeof(errbuf));
        std::cerr << "Cannot open file: " << filename << " - " << errbuf << std::endl;
        return false;
    }

    ret = avformat_find_stream_info(formatCtx_, nullptr);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, sizeof(errbuf));
        std::cerr << "Cannot find stream info: " << errbuf << std::endl;
        cleanup();
        return false;
    }

    // 查找H264视频流
    videoStreamIndex_ = av_find_best_stream(formatCtx_, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (videoStreamIndex_ < 0) {
        std::cerr << "Cannot find H264 video stream" << std::endl;
        cleanup();
        return false;
    }

    // 检查是否是H264编码
    AVStream* videoStream = formatCtx_->streams[videoStreamIndex_];
    if (videoStream->codecpar->codec_id != AV_CODEC_ID_H264) {
        std::cerr << "Not an H264 video stream" << std::endl;
        cleanup();
        return false;
    }

    // 初始化视频解码器
    if (!initVideoCodec()) {
        cleanup();
        return false;
    }

    // 填充视频信息
    videoInfo_.width = videoCodecCtx_->width;
    videoInfo_.height = videoCodecCtx_->height;
    videoInfo_.duration = static_cast<double>(formatCtx_->duration) / AV_TIME_BASE;
    videoInfo_.bitrate = static_cast<double>(formatCtx_->bit_rate);
    videoInfo_.total_frames = videoStream->nb_frames;
    
    if (videoStream->avg_frame_rate.den != 0) {
        videoInfo_.fps = static_cast<double>(videoStream->avg_frame_rate.num) / 
                        videoStream->avg_frame_rate.den;
    } else {
        videoInfo_.fps = 0.0;
    }

    // 获取H264特定信息
    switch (videoCodecCtx_->profile) {
        case FF_PROFILE_H264_BASELINE:
            videoInfo_.profile = "Baseline";
            break;
        case FF_PROFILE_H264_MAIN:
            videoInfo_.profile = "Main";
            break;
        case FF_PROFILE_H264_EXTENDED:
            videoInfo_.profile = "Extended";
            break;
        case FF_PROFILE_H264_HIGH:
            videoInfo_.profile = "High";
            break;
        case FF_PROFILE_H264_HIGH_10:
            videoInfo_.profile = "High 10";
            break;
        case FF_PROFILE_H264_HIGH_422:
            videoInfo_.profile = "High 422";
            break;
        case FF_PROFILE_H264_HIGH_444:
            videoInfo_.profile = "High 444";
            break;
        default:
            videoInfo_.profile = "Unknown";
    }
    
    videoInfo_.level = videoCodecCtx_->level;

    // 更新NAL单元信息
    updateNALUnits();

    // 打印基本信息
    std::cout << "\nH264 Stream Information:" << std::endl;
    std::cout << "Resolution: " << videoInfo_.width << "x" << videoInfo_.height << std::endl;
    std::cout << "Duration: " << videoInfo_.duration << " seconds" << std::endl;
    std::cout << "Total Frames: " << videoInfo_.total_frames << std::endl;
    std::cout << "Keyframes: " << videoInfo_.keyframe_count << std::endl;
    std::cout << "Bitrate: " << videoInfo_.bitrate / 1000.0 << " kbps" << std::endl;
    std::cout << "FPS: " << videoInfo_.fps << std::endl;
    std::cout << "Profile: " << videoInfo_.profile << std::endl;
    std::cout << "Level: " << videoInfo_.level << std::endl;
    std::cout << std::endl;

    return true;
}

void H264Parser::updateNALUnits() {
    nalUnits_.clear();
    videoInfo_.keyframe_count = 0;
    
    AVPacket* packet = av_packet_alloc();
    
    // 保存当前位置
    int64_t original_pos = avio_tell(formatCtx_->pb);
    
    // 从头开始扫描
    avio_seek(formatCtx_->pb, 0, SEEK_SET);
    avformat_flush(formatCtx_);
    
    while (av_read_frame(formatCtx_, packet) >= 0) {
        if (packet->stream_index == videoStreamIndex_) {
            // 解析NAL单元
            const uint8_t* data = packet->data;
            int remaining_size = packet->size;
            int offset = 0;
            
            while (remaining_size >= 4) {
                // 查找开始码
                if (data[0] == 0 && data[1] == 0 && data[2] == 1) {
                    // 找到3字节开始码
                    NALUnitInfo nal;
                    nal.type = getNALUnitType(data[3]);
                    nal.offset = packet->pos + offset;
                    nal.size = remaining_size;
                    nal.is_keyframe = isKeyframeNALUnit(nal.type);
                    nal.timestamp = packet->pts * av_q2d(formatCtx_->streams[videoStreamIndex_]->time_base);
                    
                    if (nal.is_keyframe) {
                        videoInfo_.keyframe_count++;
                    }
                    
                    nalUnits_.push_back(nal);
                    
                    // 解析SPS和PPS
                    if (nal.type == NALUnitType::SPS) {
                        parseSPS(data + 4, remaining_size - 4);
                    } else if (nal.type == NALUnitType::PPS) {
                        parsePPS(data + 4, remaining_size - 4);
                    }
                }
                
                data++;
                remaining_size--;
                offset++;
            }
        }
        av_packet_unref(packet);
    }
    
    // 恢复原始位置
    avio_seek(formatCtx_->pb, original_pos, SEEK_SET);
    avformat_flush(formatCtx_);
    
    av_packet_free(&packet);
}

H264Parser::NALUnitType H264Parser::getNALUnitType(uint8_t byte) {
    return static_cast<NALUnitType>(byte & 0x1F);
}

bool H264Parser::isKeyframeNALUnit(NALUnitType type) {
    return type == NALUnitType::CODED_SLICE_IDR;
}

void H264Parser::parseSPS(const uint8_t* data, int size) {
    // TODO: 实现SPS解析
    // 这需要实现H264标准中定义的SPS语法解析
}

void H264Parser::parsePPS(const uint8_t* data, int size) {
    // TODO: 实现PPS解析
    // 这需要实现H264标准中定义的PPS语法解析
}

bool H264Parser::readNextFrame(std::vector<uint8_t>& frameData, bool& isKeyFrame) {
    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    bool success = false;

    while (av_read_frame(formatCtx_, packet) >= 0) {
        if (packet->stream_index == videoStreamIndex_) {
            int ret = avcodec_send_packet(videoCodecCtx_, packet);
            if (ret < 0) {
                break;
            }

            ret = avcodec_receive_frame(videoCodecCtx_, frame);
            if (ret >= 0) {
                // 计算帧大小并分配内存
                int bufferSize = av_image_get_buffer_size(
                    static_cast<AVPixelFormat>(frame->format),
                    frame->width,
                    frame->height,
                    1  // align
                );

                frameData.resize(bufferSize);
                isKeyFrame = (frame->flags & AV_FRAME_FLAG_KEY) != 0;

                // 复制帧数据
                uint8_t* dst_data[4] = { frameData.data(), nullptr, nullptr, nullptr };
                int dst_linesize[4] = { frame->width * 4, 0, 0, 0 };  // 假设 RGBA 格式

                av_image_copy(dst_data, dst_linesize,
                            const_cast<const uint8_t**>(frame->data),
                            frame->linesize,
                            static_cast<AVPixelFormat>(frame->format),
                            frame->width,
                            frame->height);

                success = true;
                break;
            }
        }
        av_packet_unref(packet);
    }

    av_frame_free(&frame);
    av_packet_free(&packet);
    return success;
}

bool H264Parser::seekFrame(double timestamp) {
    if (!formatCtx_ || videoStreamIndex_ < 0) return false;

    AVStream* videoStream = formatCtx_->streams[videoStreamIndex_];
    int64_t timeBase = static_cast<int64_t>(timestamp / av_q2d(videoStream->time_base));
    
    int ret = av_seek_frame(formatCtx_, videoStreamIndex_, timeBase, AVSEEK_FLAG_BACKWARD);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, sizeof(errbuf));
        std::cerr << "Seek failed: " << errbuf << std::endl;
        return false;
    }

    avcodec_flush_buffers(videoCodecCtx_);
    return true;
}

H264Parser::VideoInfo H264Parser::getVideoInfo() const {
    return videoInfo_;
}

std::vector<H264Parser::NALUnitInfo> H264Parser::getNALUnits() const {
    return nalUnits_;
}

H264Parser::SPSInfo H264Parser::getSPSInfo() const {
    return spsInfo_;
}

H264Parser::PPSInfo H264Parser::getPPSInfo() const {
    return ppsInfo_;
}

void H264Parser::close() {
    cleanup();
} 