#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
}

#include <string>
#include <memory>

class VideoFilter {
public:
    enum class FilterType {
        BRIGHTNESS_CONTRAST,
        HSV_ADJUST,
        SHARPEN_BLUR
    };

    struct FilterParams {
        FilterType type;
        union {
            struct {
                int brightness;    // -100 to 100
                int contrast;      // -100 to 100
            } bc;
            struct {
                int hue;          // -180 to 180
                int saturation;   // -100 to 100
                int value;        // -100 to 100
            } hsv;
            struct {
                double sharpen;   // 0 to 10
                double blur;      // 0 to 10
            } sb;
        };
    };

    VideoFilter();
    ~VideoFilter();

    bool init(const std::string& inputFile, const std::string& outputFile);
    bool updateFilter(const FilterParams& params);
    bool processFrame();
    void close();

private:
    bool initFilter(const FilterParams& params);
    std::string getFilterString(const FilterParams& params);

    AVFormatContext* inputFormatContext_{nullptr};
    AVFormatContext* outputFormatContext_{nullptr};
    AVCodecContext* decoderContext_{nullptr};
    AVCodecContext* encoderContext_{nullptr};
    AVFilterGraph* filterGraph_{nullptr};
    AVFilterContext* bufferSrcContext_{nullptr};
    AVFilterContext* bufferSinkContext_{nullptr};
    AVFrame* frame_{nullptr};
    AVFrame* filteredFrame_{nullptr};
    AVPacket* packet_{nullptr};
    int videoStreamIndex_{-1};
}; 