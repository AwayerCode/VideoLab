#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include "ffmpeg/analyzer.hpp"
#include "ffmpeg/x264_param_test.hpp"
#include "ffmpeg/x265_param_test.hpp"

// 全局变量
int width = 1920;
int height = 1080;
int frame_count = 300;
int fps = 30;

// 计算原始数据大小（以MB为单位）
double calculateRawSize() {
    // YUV420格式，每个像素1.5字节
    double frame_size = width * height * 1.5;
    double total_size = frame_size * frame_count;
    return total_size / (1024 * 1024); // 转换为MB
}

// 显示主菜单
void showMainMenu() {
    std::cout << "\n=== 视频编码性能对比工具 v1.0.0 ===\n" << std::endl;
    std::cout << "1. 预设性能对比测试" << std::endl;
    std::cout << "2. 码率控制对比测试" << std::endl;
    std::cout << "3. 修改测试参数" << std::endl;
    std::cout << "0. 退出程序" << std::endl;
    std::cout << "\n当前测试参数：" << std::endl;
    std::cout << "分辨率: " << width << "x" << height << std::endl;
    std::cout << "帧率: " << fps << " fps" << std::endl;
    std::cout << "测试帧数: " << frame_count << std::endl;
    std::cout << "原始数据大小: " << std::fixed << std::setprecision(2) << calculateRawSize() << " MB" << std::endl;
    std::cout << "\n请选择: ";
}

// 修改测试参数
void modifyTestParams() {
    std::cout << "\n=== 修改测试参数 ===\n" << std::endl;
    std::string input;

    std::cout << "输入视频宽度 [" << width << "]: ";
    std::getline(std::cin, input);
    if (!input.empty()) width = std::stoi(input);

    std::cout << "输入视频高度 [" << height << "]: ";
    std::getline(std::cin, input);
    if (!input.empty()) height = std::stoi(input);

    std::cout << "输入帧率 [" << fps << "]: ";
    std::getline(std::cin, input);
    if (!input.empty()) fps = std::stoi(input);

    std::cout << "输入测试帧数 [" << frame_count << "]: ";
    std::getline(std::cin, input);
    if (!input.empty()) frame_count = std::stoi(input);

    double raw_size = calculateRawSize();
    std::cout << "\n参数已更新！" << std::endl;
    std::cout << "原始数据大小: " << std::fixed << std::setprecision(2) << raw_size << " MB" << std::endl;
}

// 计算编码后的数据大小（以MB为单位）
double calculateEncodedSize(double bitrate, int duration_seconds) {
    return (bitrate * duration_seconds) / (8 * 1024 * 1024); // 转换为MB
}

// 预设性能对比测试
void runPresetComparisonTest() {
    std::cout << "\n=== 开始预设性能对比测试 ===\n" << std::endl;
    std::cout << "测试参数：" << std::endl;
    std::cout << "分辨率: " << width << "x" << height << std::endl;
    std::cout << "帧率: " << fps << " fps" << std::endl;
    std::cout << "测试帧数: " << frame_count << std::endl;
    double raw_size = calculateRawSize();
    std::cout << "原始数据大小: " << std::fixed << std::setprecision(2) << raw_size << " MB" << std::endl;
    std::cout << "\n----------------------------------------" << std::endl;

    // x264测试
    std::cout << "\n[x264测试]" << std::endl;
    auto x264_results = X264ParamTest::runPresetTest(width, height, frame_count);

    // x265测试
    std::cout << "\n[x265测试]" << std::endl;
    auto x265_results = X265ParamTest::runPresetTest(width, height, frame_count, fps);

    // 显示对比结果
    std::cout << "\n=== 性能对比结果 ===\n" << std::endl;
    std::cout << std::setw(12) << "预设" 
              << std::setw(15) << "x264 FPS"
              << std::setw(15) << "x265 FPS"
              << std::setw(15) << "x264大小(MB)"
              << std::setw(15) << "x265大小(MB)"
              << std::setw(12) << "速度比"
              << std::setw(12) << "压缩比"
              << std::endl;

    const char* presets[] = {
        "ultrafast", "superfast", "veryfast", "faster", "fast",
        "medium", "slow", "slower", "veryslow"
    };

    double duration = static_cast<double>(frame_count) / fps;
    for (size_t i = 0; i < x264_results.size() && i < x265_results.size(); ++i) {
        double speed_ratio = x264_results[i].fps / x265_results[i].fps;
        double x264_size = calculateEncodedSize(x264_results[i].bitrate, duration);
        double x265_size = calculateEncodedSize(x265_results[i].bitrate, duration);
        double compression_ratio = raw_size / x265_size; // 使用x265的压缩比

        std::cout << std::setw(12) << presets[i]
                  << std::setw(15) << std::fixed << std::setprecision(2) << x264_results[i].fps
                  << std::setw(15) << x265_results[i].fps
                  << std::setw(15) << x264_size
                  << std::setw(15) << x265_size
                  << std::setw(12) << speed_ratio
                  << std::setw(12) << compression_ratio
                  << std::endl;
    }
}

// 码率控制对比测试
void runRateControlComparisonTest() {
    std::cout << "\n=== 开始码率控制对比测试 ===\n" << std::endl;
    std::cout << "测试参数：" << std::endl;
    std::cout << "分辨率: " << width << "x" << height << std::endl;
    std::cout << "帧率: " << fps << " fps" << std::endl;
    std::cout << "测试帧数: " << frame_count << std::endl;
    double raw_size = calculateRawSize();
    std::cout << "原始数据大小: " << std::fixed << std::setprecision(2) << raw_size << " MB" << std::endl;
    std::cout << "\n----------------------------------------" << std::endl;

    std::vector<int> crf_values = {18, 23, 28, 33};

    // x264测试
    std::cout << "\n[x264测试]" << std::endl;
    auto x264_results = X264ParamTest::runRateControlTest(
        X264ParamTest::RateControl::CRF,
        crf_values,
        width, height, frame_count
    );

    // x265测试
    std::cout << "\n[x265测试]" << std::endl;
    auto x265_results = X265ParamTest::runRateControlTest(
        X265ParamTest::RateControl::CRF,
        crf_values,
        width, height, frame_count, fps
    );

    // 显示对比结果
    std::cout << "\n=== 码率控制对比结果 ===\n" << std::endl;
    std::cout << std::setw(8) << "CRF" 
              << std::setw(15) << "x264 FPS"
              << std::setw(15) << "x265 FPS"
              << std::setw(15) << "x264大小(MB)"
              << std::setw(15) << "x265大小(MB)"
              << std::setw(12) << "速度比"
              << std::setw(12) << "压缩比"
              << std::endl;

    double duration = static_cast<double>(frame_count) / fps;
    for (size_t i = 0; i < x264_results.size() && i < x265_results.size(); ++i) {
        double speed_ratio = x264_results[i].fps / x265_results[i].fps;
        double x264_size = calculateEncodedSize(x264_results[i].bitrate, duration);
        double x265_size = calculateEncodedSize(x265_results[i].bitrate, duration);
        double compression_ratio = raw_size / x265_size; // 使用x265的压缩比

        std::cout << std::setw(8) << crf_values[i]
                  << std::setw(15) << std::fixed << std::setprecision(2) << x264_results[i].fps
                  << std::setw(15) << x265_results[i].fps
                  << std::setw(15) << x264_size
                  << std::setw(15) << x265_size
                  << std::setw(12) << speed_ratio
                  << std::setw(12) << compression_ratio
                  << std::endl;
    }
}

int main() {
    std::string choice;
    while (true) {
        showMainMenu();
        std::getline(std::cin, choice);

        if (choice == "1") {
            runPresetComparisonTest();
        }
        else if (choice == "2") {
            runRateControlComparisonTest();
        }
        else if (choice == "3") {
            modifyTestParams();
        }
        else if (choice == "0") {
            std::cout << "再见！" << std::endl;
            break;
        }
        else {
            std::cout << "无效的选择，请重试" << std::endl;
        }
    }

    return 0;
}   