#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <map>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/dict.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/frame.h>
}

class H264Parser {
public:
    // H264 NAL单元类型
    enum class NALUnitType {
        UNSPECIFIED = 0,
        CODED_SLICE_NON_IDR = 1,  // 非IDR图像中的编码条带
        CODED_SLICE_DATA_PARTITION_A = 2,  // 编码条带数据分区A
        CODED_SLICE_DATA_PARTITION_B = 3,  // 编码条带数据分区B
        CODED_SLICE_DATA_PARTITION_C = 4,  // 编码条带数据分区C
        CODED_SLICE_IDR = 5,      // IDR图像的编码条带
        SEI = 6,                  // 补充增强信息
        SPS = 7,                  // 序列参数集
        PPS = 8,                  // 图像参数集
        AUD = 9,                  // 访问单元分隔符
        END_SEQUENCE = 10,        // 序列结束
        END_STREAM = 11,          // 码流结束
        FILLER_DATA = 12,         // 填充数据
        SPS_EXT = 13,            // 序列参数集扩展
        PREFIX_NAL = 14,         // 前缀NAL单元
        SUBSET_SPS = 15,         // 子序列参数集
        RESERVED_16 = 16,
        RESERVED_17 = 17,
        RESERVED_18 = 18,
        CODED_SLICE_AUX = 19,    // 辅助编码图像的编码条带
        CODED_SLICE_EXT = 20,    // 扩展编码条带
        RESERVED_21 = 21,
        RESERVED_22 = 22,
        RESERVED_23 = 23,
        UNSPECIFIED_24 = 24
    };

    // H264基本信息
    struct VideoInfo {
        int width;
        int height;
        double duration;      // 单位：秒
        double bitrate;       // 单位：bps
        double fps;
        std::string profile;  // 配置(Baseline/Main/High等)
        int level;           // 级别(1/1.1/1.2等)
        int64_t total_frames;
        int64_t keyframe_count;
    };

    // NAL单元信息
    struct NALUnitInfo {
        NALUnitType type;
        int64_t offset;      // 在文件中的偏移
        int size;           // NAL单元大小
        bool is_keyframe;   // 是否是关键帧
        double timestamp;   // 时间戳（秒）
    };

    // SPS信息
    struct SPSInfo {
        int profile_idc;
        int level_idc;
        int width;
        int height;
        int fps_num;
        int fps_den;
        int max_num_ref_frames;
        bool frame_mbs_only_flag;
        int log2_max_frame_num;
        int log2_max_poc_lsb;
        int num_ref_frames;
    };

    // PPS信息
    struct PPSInfo {
        int pic_parameter_set_id;
        int seq_parameter_set_id;
        bool entropy_coding_mode_flag;
        bool pic_order_present_flag;
        int num_slice_groups_minus1;
        int num_ref_idx_l0_active_minus1;
        int num_ref_idx_l1_active_minus1;
        bool weighted_pred_flag;
        int weighted_bipred_idc;
        int pic_init_qp_minus26;
        int pic_init_qs_minus26;
        int chroma_qp_index_offset;
        bool deblocking_filter_control_present_flag;
        bool constrained_intra_pred_flag;
        bool redundant_pic_cnt_present_flag;
    };

    H264Parser();
    ~H264Parser();

    // 基本文件操作
    bool open(const std::string& filename);
    void close();
    
    // 获取信息
    VideoInfo getVideoInfo() const;
    std::vector<NALUnitInfo> getNALUnits() const;
    SPSInfo getSPSInfo() const;
    PPSInfo getPPSInfo() const;
    
    // 帧操作
    bool readNextFrame(std::vector<uint8_t>& frameData, bool& isKeyFrame);
    bool seekFrame(double timestamp);

private:
    AVFormatContext* formatCtx_{nullptr};
    AVCodecContext* videoCodecCtx_{nullptr};
    SwsContext* swsCtx_{nullptr};
    
    int videoStreamIndex_{-1};
    VideoInfo videoInfo_;
    std::vector<NALUnitInfo> nalUnits_;
    SPSInfo spsInfo_;
    PPSInfo ppsInfo_;
    
    void cleanup();
    bool initVideoCodec();
    void updateNALUnits();
    void parseSPS(const uint8_t* data, int size);
    void parsePPS(const uint8_t* data, int size);
    NALUnitType getNALUnitType(uint8_t byte);
    bool isKeyframeNALUnit(NALUnitType type);
}; 