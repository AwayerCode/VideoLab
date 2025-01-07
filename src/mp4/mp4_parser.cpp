#include "mp4_parser.hpp"
#include <iostream>
#include <cmath>

MP4Parser::MP4Parser() = default;

MP4Parser::~MP4Parser() {
    cleanup();
}

void MP4Parser::cleanup() {
    if (swsCtx_) {
        sws_freeContext(swsCtx_);
        swsCtx_ = nullptr;
    }
    
    if (videoCodecCtx_) {
        avcodec_free_context(&videoCodecCtx_);
    }
    
    if (audioCodecCtx_) {
        avcodec_free_context(&audioCodecCtx_);
    }
    
    if (formatCtx_) {
        avformat_close_input(&formatCtx_);
        formatCtx_ = nullptr;
    }
    
    videoStreamIndex_ = -1;
    audioStreamIndex_ = -1;
    keyFrameInfo_.clear();
}

bool MP4Parser::initVideoCodec() {
    AVStream* videoStream = formatCtx_->streams[videoStreamIndex_];
    const AVCodec* codec = avcodec_find_decoder(videoStream->codecpar->codec_id);
    if (!codec) {
        std::cerr << "找不到视频解码器" << std::endl;
        return false;
    }

    videoCodecCtx_ = avcodec_alloc_context3(codec);
    if (!videoCodecCtx_) {
        std::cerr << "无法分配视频解码器上下文" << std::endl;
        return false;
    }

    if (avcodec_parameters_to_context(videoCodecCtx_, videoStream->codecpar) < 0) {
        std::cerr << "无法复制解码器参数" << std::endl;
        return false;
    }

    if (avcodec_open2(videoCodecCtx_, codec, nullptr) < 0) {
        std::cerr << "无法打开视频解码器" << std::endl;
        return false;
    }

    return true;
}

bool MP4Parser::initAudioCodec() {
    if (audioStreamIndex_ < 0) return false;

    AVStream* audioStream = formatCtx_->streams[audioStreamIndex_];
    const AVCodec* codec = avcodec_find_decoder(audioStream->codecpar->codec_id);
    if (!codec) {
        std::cerr << "找不到音频解码器" << std::endl;
        return false;
    }

    audioCodecCtx_ = avcodec_alloc_context3(codec);
    if (!audioCodecCtx_) {
        std::cerr << "无法分配音频解码器上下文" << std::endl;
        return false;
    }

    if (avcodec_parameters_to_context(audioCodecCtx_, audioStream->codecpar) < 0) {
        std::cerr << "无法复制音频解码器参数" << std::endl;
        return false;
    }

    if (avcodec_open2(audioCodecCtx_, codec, nullptr) < 0) {
        std::cerr << "无法打开音频解码器" << std::endl;
        return false;
    }

    return true;
}

void MP4Parser::updateKeyFrameInfo() {
    keyFrameInfo_.clear();
    
    AVStream* videoStream = formatCtx_->streams[videoStreamIndex_];
    
    // 扫描视频流中的关键帧
    AVPacket* packet = av_packet_alloc();
    int64_t pts = 0;
    
    // 保存当前位置
    int64_t original_pos = avio_tell(formatCtx_->pb);
    
    // 从头开始扫描
    avio_seek(formatCtx_->pb, 0, SEEK_SET);
    avformat_flush(formatCtx_);
    
    while (av_read_frame(formatCtx_, packet) >= 0) {
        if (packet->stream_index == videoStreamIndex_) {
            if (packet->flags & AV_PKT_FLAG_KEY) {
                KeyFrameInfo info;
                info.pts = packet->pts;
                info.pos = avio_tell(formatCtx_->pb);
                info.timestamp = packet->pts * av_q2d(videoStream->time_base);
                keyFrameInfo_.push_back(info);
            }
            pts = packet->pts;
        }
        av_packet_unref(packet);
    }
    
    // 恢复原始位置
    avio_seek(formatCtx_->pb, original_pos, SEEK_SET);
    avformat_flush(formatCtx_);
    
    av_packet_free(&packet);
    videoInfo_.keyframe_count = keyFrameInfo_.size();
}

bool MP4Parser::open(const std::string& filename) {
    cleanup();

    int ret = avformat_open_input(&formatCtx_, filename.c_str(), nullptr, nullptr);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, sizeof(errbuf));
        std::cerr << "无法打开文件: " << filename << " - " << errbuf << std::endl;
        return false;
    }

    ret = avformat_find_stream_info(formatCtx_, nullptr);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, sizeof(errbuf));
        std::cerr << "无法读取流信息: " << errbuf << std::endl;
        cleanup();
        return false;
    }

    // 查找视频流
    videoStreamIndex_ = av_find_best_stream(formatCtx_, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (videoStreamIndex_ < 0) {
        std::cerr << "找不到视频流" << std::endl;
        cleanup();
        return false;
    }

    // 查找音频流
    audioStreamIndex_ = av_find_best_stream(formatCtx_, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);

    // 初始化视频解码器
    if (!initVideoCodec()) {
        cleanup();
        return false;
    }

    // 如果有音频流，初始化音频解码器
    if (audioStreamIndex_ >= 0) {
        if (!initAudioCodec()) {
            std::cerr << "警告：音频解码器初始化失败" << std::endl;
            audioStreamIndex_ = -1;
        }
    }

    // 获取视频流
    AVStream* videoStream = formatCtx_->streams[videoStreamIndex_];
    
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

    videoInfo_.codec_name = avcodec_get_name(videoCodecCtx_->codec_id);
    videoInfo_.format_name = formatCtx_->iformat->name;

    // 如果有音频流，填充音频信息
    if (audioStreamIndex_ >= 0) {
        AVStream* audioStream = formatCtx_->streams[audioStreamIndex_];
        audioInfo_.channels = audioCodecCtx_->channels;
        audioInfo_.sample_rate = audioCodecCtx_->sample_rate;
        audioInfo_.total_samples = audioStream->nb_frames * audioCodecCtx_->frame_size;
        audioInfo_.codec_name = avcodec_get_name(audioCodecCtx_->codec_id);
        audioInfo_.duration = static_cast<double>(audioStream->duration) * 
                            av_q2d(audioStream->time_base);
        audioInfo_.bitrate = audioCodecCtx_->bit_rate;
    }

    // 更新关键帧信息
    updateKeyFrameInfo();

    // 打印基本信息
    std::cout << "\n视频文件信息:" << std::endl;
    std::cout << "格式: " << videoInfo_.format_name << std::endl;
    std::cout << "视频编码器: " << videoInfo_.codec_name << std::endl;
    std::cout << "分辨率: " << videoInfo_.width << "x" << videoInfo_.height << std::endl;
    std::cout << "时长: " << videoInfo_.duration << " 秒" << std::endl;
    std::cout << "总帧数: " << videoInfo_.total_frames << std::endl;
    std::cout << "关键帧数: " << videoInfo_.keyframe_count << std::endl;
    std::cout << "比特率: " << videoInfo_.bitrate / 1000.0 << " kbps" << std::endl;
    std::cout << "帧率: " << videoInfo_.fps << " fps" << std::endl;

    if (audioStreamIndex_ >= 0) {
        std::cout << "\n音频信息:" << std::endl;
        std::cout << "编码器: " << audioInfo_.codec_name << std::endl;
        std::cout << "声道数: " << audioInfo_.channels << std::endl;
        std::cout << "采样率: " << audioInfo_.sample_rate << " Hz" << std::endl;
        std::cout << "比特率: " << audioInfo_.bitrate / 1000.0 << " kbps" << std::endl;
    }
    
    std::cout << std::endl;
    return true;
}

bool MP4Parser::readNextFrame(Frame& frame) {
    AVPacket* packet = av_packet_alloc();
    AVFrame* avFrame = av_frame_alloc();
    bool success = false;

    while (av_read_frame(formatCtx_, packet) >= 0) {
        if (packet->stream_index == videoStreamIndex_) {
            int ret = avcodec_send_packet(videoCodecCtx_, packet);
            if (ret < 0) {
                break;
            }

            ret = avcodec_receive_frame(videoCodecCtx_, avFrame);
            if (ret >= 0) {
                // 计算帧大小并分配内存
                int bufferSize = av_image_get_buffer_size(
                    static_cast<AVPixelFormat>(avFrame->format),
                    avFrame->width,
                    avFrame->height,
                    1  // align
                );

                frame.data.resize(bufferSize);
                frame.pts = avFrame->pts;
                frame.is_key_frame = (avFrame->flags & AV_FRAME_FLAG_KEY) != 0;
                frame.timestamp = frame.pts * av_q2d(formatCtx_->streams[videoStreamIndex_]->time_base);

                // 复制帧数据
                uint8_t* dst_data[4] = { frame.data.data(), nullptr, nullptr, nullptr };
                int dst_linesize[4] = { avFrame->width * 4, 0, 0, 0 };  // 假设 RGBA 格式

                av_image_copy(dst_data, dst_linesize,
                            const_cast<const uint8_t**>(avFrame->data),
                            avFrame->linesize,
                            static_cast<AVPixelFormat>(avFrame->format),
                            avFrame->width,
                            avFrame->height);

                success = true;
                break;
            }
        }
        av_packet_unref(packet);
    }

    av_frame_free(&avFrame);
    av_packet_free(&packet);
    return success;
}

bool MP4Parser::seekFrame(double timestamp) {
    if (!formatCtx_ || videoStreamIndex_ < 0) return false;

    AVStream* videoStream = formatCtx_->streams[videoStreamIndex_];
    int64_t timeBase = static_cast<int64_t>(timestamp / av_q2d(videoStream->time_base));
    
    int ret = av_seek_frame(formatCtx_, videoStreamIndex_, timeBase, AVSEEK_FLAG_BACKWARD);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, sizeof(errbuf));
        std::cerr << "跳转失败: " << errbuf << std::endl;
        return false;
    }

    avcodec_flush_buffers(videoCodecCtx_);
    return true;
}

bool MP4Parser::readAudioSamples(std::vector<float>& samples, int max_samples) {
    if (!formatCtx_ || audioStreamIndex_ < 0) return false;

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    bool success = false;

    while (av_read_frame(formatCtx_, packet) >= 0) {
        if (packet->stream_index == audioStreamIndex_) {
            int ret = avcodec_send_packet(audioCodecCtx_, packet);
            if (ret < 0) break;

            ret = avcodec_receive_frame(audioCodecCtx_, frame);
            if (ret >= 0) {
                int numSamples = frame->nb_samples * frame->channels;
                numSamples = std::min(numSamples, max_samples);
                
                samples.resize(numSamples);
                float* floatData = reinterpret_cast<float*>(frame->data[0]);
                std::copy(floatData, floatData + numSamples, samples.begin());
                
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

bool MP4Parser::seekAudio(double timestamp) {
    if (!formatCtx_ || audioStreamIndex_ < 0) return false;

    AVStream* audioStream = formatCtx_->streams[audioStreamIndex_];
    int64_t timeBase = static_cast<int64_t>(timestamp / av_q2d(audioStream->time_base));
    
    int ret = av_seek_frame(formatCtx_, audioStreamIndex_, timeBase, AVSEEK_FLAG_BACKWARD);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, sizeof(errbuf));
        std::cerr << "音频跳转失败: " << errbuf << std::endl;
        return false;
    }

    avcodec_flush_buffers(audioCodecCtx_);
    return true;
}

std::map<std::string, std::string> MP4Parser::getMetadata() const {
    std::map<std::string, std::string> metadata;
    AVDictionaryEntry* tag = nullptr;
    
    if (formatCtx_ && formatCtx_->metadata) {
        while ((tag = av_dict_get(formatCtx_->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
            metadata[tag->key] = tag->value;
        }
    }
    
    return metadata;
}

std::vector<MP4Parser::KeyFrameInfo> MP4Parser::getKeyFrameInfo() const {
    return keyFrameInfo_;
}

MP4Parser::VideoInfo MP4Parser::getVideoInfo() const {
    return videoInfo_;
}

MP4Parser::AudioInfo MP4Parser::getAudioInfo() const {
    return audioInfo_;
}

void MP4Parser::close() {
    cleanup();
} 