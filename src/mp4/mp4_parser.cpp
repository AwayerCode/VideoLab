#include "mp4_parser.hpp"
#include <iostream>

MP4Parser::MP4Parser() = default;

MP4Parser::~MP4Parser() {
    cleanup();
}

void MP4Parser::cleanup() {
    if (formatCtx_) {
        avformat_close_input(&formatCtx_);
        formatCtx_ = nullptr;
    }
    videoStreamIndex_ = -1;
}

bool MP4Parser::open(const std::string& filename) {
    cleanup();

    // 打开文件
    int ret = avformat_open_input(&formatCtx_, filename.c_str(), nullptr, nullptr);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, sizeof(errbuf));
        std::cerr << "无法打开文件: " << filename << " - " << errbuf << std::endl;
        return false;
    }

    // 读取流信息
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

    // 获取视频流
    AVStream* videoStream = formatCtx_->streams[videoStreamIndex_];
    AVCodecParameters* codecParams = videoStream->codecpar;

    // 填充视频信息
    videoInfo_.width = codecParams->width;
    videoInfo_.height = codecParams->height;
    videoInfo_.duration = static_cast<double>(formatCtx_->duration) / AV_TIME_BASE;
    videoInfo_.bitrate = static_cast<double>(formatCtx_->bit_rate);
    
    // 计算帧率
    if (videoStream->avg_frame_rate.den != 0) {
        videoInfo_.fps = static_cast<double>(videoStream->avg_frame_rate.num) / 
                        videoStream->avg_frame_rate.den;
    } else {
        videoInfo_.fps = 0.0;
    }

    // 获取编解码器名称
    const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);
    if (codec) {
        videoInfo_.codec_name = codec->name;
    }

    // 获取格式名称
    videoInfo_.format_name = formatCtx_->iformat->name;

    // 打印基本信息
    std::cout << "\n视频文件信息:" << std::endl;
    std::cout << "格式: " << videoInfo_.format_name << std::endl;
    std::cout << "编码器: " << videoInfo_.codec_name << std::endl;
    std::cout << "分辨率: " << videoInfo_.width << "x" << videoInfo_.height << std::endl;
    std::cout << "时长: " << videoInfo_.duration << " 秒" << std::endl;
    std::cout << "比特率: " << videoInfo_.bitrate / 1000.0 << " kbps" << std::endl;
    std::cout << "帧率: " << videoInfo_.fps << " fps\n" << std::endl;

    return true;
}

MP4Parser::VideoInfo MP4Parser::getVideoInfo() const {
    return videoInfo_;
}

void MP4Parser::close() {
    cleanup();
} 