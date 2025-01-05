#include "analyzer.hpp"
#include <sstream>
#include <iomanip>

VideoAnalyzer::VideoAnalyzer()
    : formatContext_(nullptr)
    , videoCodec_(nullptr)
    , audioCodec_(nullptr)
    , videoStreamIndex_(-1)
    , audioStreamIndex_(-1)
{
}

VideoAnalyzer::~VideoAnalyzer() {
    if (formatContext_) {
        avformat_close_input(&formatContext_);
    }
}

bool VideoAnalyzer::analyze(const std::string& filename) {
    // 打开文件
    if (avformat_open_input(&formatContext_, filename.c_str(), nullptr, nullptr) < 0) {
        return false;
    }

    // 读取流信息
    if (avformat_find_stream_info(formatContext_, nullptr) < 0) {
        avformat_close_input(&formatContext_);
        return false;
    }

    // 查找视频和音频流
    for (unsigned int i = 0; i < formatContext_->nb_streams; i++) {
        AVStream* stream = formatContext_->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && videoStreamIndex_ < 0) {
            videoStreamIndex_ = i;
            videoCodec_ = avcodec_find_decoder(stream->codecpar->codec_id);
        }
        else if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audioStreamIndex_ < 0) {
            audioStreamIndex_ = i;
            audioCodec_ = avcodec_find_decoder(stream->codecpar->codec_id);
        }
    }

    return true;
}

std::string VideoAnalyzer::getFormatInfo() const {
    if (!formatContext_) return "";

    std::stringstream ss;
    ss << "格式信息:\n"
       << "  格式: " << formatContext_->iformat->long_name << "\n"
       << "  时长: " << formatTime(formatContext_->duration) << "\n"
       << "  比特率: " << formatBitrate(formatContext_->bit_rate) << "\n"
       << "  流数量: " << formatContext_->nb_streams << "\n";
    return ss.str();
}

std::string VideoAnalyzer::getVideoInfo() const {
    if (!formatContext_ || videoStreamIndex_ < 0) return "";

    AVStream* stream = formatContext_->streams[videoStreamIndex_];
    AVCodecParameters* codecParams = stream->codecpar;

    std::stringstream ss;
    ss << "视频信息:\n"
       << "  编解码器: " << videoCodec_->long_name << "\n"
       << "  分辨率: " << codecParams->width << "x" << codecParams->height << "\n"
       << "  像素格式: " << getPixelFormatName((AVPixelFormat)codecParams->format) << "\n"
       << "  帧率: " << av_q2d(stream->avg_frame_rate) << " fps\n"
       << "  比特率: " << formatBitrate(codecParams->bit_rate) << "\n";

    if (codecParams->codec_id == AV_CODEC_ID_H264) {
        ss << parseH264Info(codecParams);
    }

    return ss.str();
}

std::string VideoAnalyzer::getAudioInfo() const {
    if (!formatContext_ || audioStreamIndex_ < 0) return "";

    AVStream* stream = formatContext_->streams[audioStreamIndex_];
    AVCodecParameters* codecParams = stream->codecpar;

    std::stringstream ss;
    ss << "音频信息:\n"
       << "  编解码器: " << audioCodec_->long_name << "\n"
       << "  采样率: " << codecParams->sample_rate << " Hz\n"
       << "  声道数: " << codecParams->ch_layout.nb_channels << "\n"
       << "  比特率: " << formatBitrate(codecParams->bit_rate) << "\n";
    return ss.str();
}

std::string VideoAnalyzer::getStreamInfo() const {
    if (!formatContext_) return "";

    std::stringstream ss;
    ss << "流信息:\n";
    for (unsigned int i = 0; i < formatContext_->nb_streams; i++) {
        AVStream* stream = formatContext_->streams[i];
        const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
        ss << "  流 #" << i << ":\n"
           << "    类型: " << av_get_media_type_string(stream->codecpar->codec_type) << "\n"
           << "    编解码器: " << (codec ? codec->long_name : "未知") << "\n";
    }
    return ss.str();
}

std::string VideoAnalyzer::parseH264Info(const AVCodecParameters* codecParams) const {
    std::stringstream ss;
    ss << "H.264 特定信息:\n";
    
    // 检查是否有extradata
    if (codecParams->extradata && codecParams->extradata_size > 0) {
        ss << "  包含 SPS/PPS 数据\n";
        // TODO: 解析 SPS/PPS 数据获取更多信息
    } else {
        ss << "  无 SPS/PPS 数据\n";
    }
    
    return ss.str();
}

std::string VideoAnalyzer::getPixelFormatName(AVPixelFormat format) const {
    return av_get_pix_fmt_name(format) ? av_get_pix_fmt_name(format) : "未知";
}

std::string VideoAnalyzer::formatTime(int64_t duration) const {
    if (duration < 0) return "N/A";
    
    int64_t hours, mins, secs, us;
    duration = duration + (duration <= INT64_MAX - 5000 ? 5000 : 0);
    secs = duration / AV_TIME_BASE;
    us = duration % AV_TIME_BASE;
    mins = secs / 60;
    secs %= 60;
    hours = mins / 60;
    mins %= 60;

    std::stringstream ss;
    ss << hours << ":" 
       << std::setfill('0') << std::setw(2) << mins << ":" 
       << std::setfill('0') << std::setw(2) << secs << "." 
       << std::setfill('0') << std::setw(3) << (us / 1000);
    return ss.str();
}

std::string VideoAnalyzer::formatBitrate(int64_t bitrate) const {
    if (bitrate <= 0) return "N/A";
    
    const char* units[] = {"bps", "Kbps", "Mbps", "Gbps"};
    int unit = 0;
    double size = bitrate;
    
    while (size >= 1024.0 && unit < 3) {
        size /= 1024.0;
        unit++;
    }
    
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << size << " " << units[unit];
    return ss.str();
} 