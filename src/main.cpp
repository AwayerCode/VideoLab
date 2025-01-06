#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <map>
#include <functional>
#include "ffmpeg/analyzer.hpp"
#include "ffmpeg/x264_param_test.hpp"
#include "ffmpeg/x265_param_test.hpp"

// 命令处理函数类型
using CommandHandler = std::function<void()>;

// 命令帮助信息
struct CommandHelp {
    std::string description;
    std::string usage;
};

// 全局变量
std::map<std::string, std::pair<CommandHandler, CommandHelp>> commands;
std::string input_file;
std::string output_file;
std::string encoder = "libx264";
std::string filter_name;
int bitrate = 2000;
std::string test_type = "preset";
std::string codec = "x264";
int width = 1920;
int height = 1080;
int frame_count = 300;

// 字符串分割函数
std::vector<std::string> split(const std::string& s, char delimiter = ' ') {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

// 显示帮助信息
void showHelp() {
    std::cout << "\n可用命令：" << std::endl;
    for (const auto& cmd : commands) {
        std::cout << cmd.first << " - " << cmd.second.second.description << std::endl;
        std::cout << "  用法: " << cmd.second.second.usage << std::endl;
    }
}

// 分析视频文件
void analyzeVideo() {
    std::cout << "请输入视频文件路径: ";
    std::getline(std::cin, input_file);

    VideoAnalyzer analyzer;
    if (!analyzer.analyze(input_file)) {
        std::cerr << "无法打开文件: " << input_file << std::endl;
        return;
    }

    std::cout << "\n=== 视频文件分析结果 ===\n\n"
              << analyzer.getFormatInfo() << "\n"
              << analyzer.getVideoInfo() << "\n"
              << analyzer.getAudioInfo() << "\n"
              << analyzer.getStreamInfo() << std::endl;
}

// 编码测试
void testEncoder() {
    std::cout << "选择编码器 (1: x264, 2: x265) [1]: ";
    std::string choice;
    std::getline(std::cin, choice);
    codec = choice == "2" ? "x265" : "x264";

    std::cout << "选择测试类型 (1: preset, 2: ratecontrol) [1]: ";
    std::getline(std::cin, choice);
    test_type = choice == "2" ? "ratecontrol" : "preset";

    std::cout << "输入视频宽度 [1920]: ";
    std::getline(std::cin, choice);
    if (!choice.empty()) width = std::stoi(choice);

    std::cout << "输入视频高度 [1080]: ";
    std::getline(std::cin, choice);
    if (!choice.empty()) height = std::stoi(choice);

    std::cout << "输入测试帧数 [300]: ";
    std::getline(std::cin, choice);
    if (!choice.empty()) frame_count = std::stoi(choice);

    if (codec == "x264") {
        if (test_type == "preset") {
            X264ParamTest::runPresetTest(width, height, frame_count);
        } else {
            std::vector<int> crf_values = {18, 23, 28, 33};
            X264ParamTest::runRateControlTest(
                X264ParamTest::RateControl::CRF,
                crf_values,
                width, height, frame_count
            );
        }
    } else {
        if (test_type == "preset") {
            X265ParamTest::runPresetTest(width, height, frame_count);
        } else {
            std::vector<int> crf_values = {18, 23, 28, 33};
            X265ParamTest::runRateControlTest(
                X265ParamTest::RateControl::CRF,
                crf_values,
                width, height, frame_count
            );
        }
    }
}

// 编码视频
void encodeVideo() {
    std::cout << "请输入源视频文件路径: ";
    std::getline(std::cin, input_file);

    std::cout << "请输入目标视频文件路径: ";
    std::getline(std::cin, output_file);

    std::cout << "选择编码器 (1: x264, 2: x265) [1]: ";
    std::string choice;
    std::getline(std::cin, choice);
    encoder = choice == "2" ? "libx265" : "libx264";

    std::cout << "输入目标码率(Kbps) [2000]: ";
    std::getline(std::cin, choice);
    if (!choice.empty()) bitrate = std::stoi(choice);

    std::cout << "编码视频文件:" << std::endl
              << "输入: " << input_file << std::endl
              << "输出: " << output_file << std::endl
              << "编码器: " << encoder << std::endl
              << "比特率: " << bitrate << "Kbps" << std::endl;
    // TODO: 实现视频编码功能
}

// 应用滤镜
void applyFilter() {
    std::cout << "请输入源视频文件路径: ";
    std::getline(std::cin, input_file);

    std::cout << "请输入目标视频文件路径: ";
    std::getline(std::cin, output_file);

    std::cout << "请输入滤镜名称: ";
    std::getline(std::cin, filter_name);

    std::cout << "应用视频滤镜:" << std::endl
              << "输入: " << input_file << std::endl
              << "输出: " << output_file << std::endl
              << "滤镜: " << filter_name << std::endl;
    // TODO: 实现视频滤镜功能
}

// 退出程序
void quit() {
    std::cout << "再见！" << std::endl;
    exit(0);
}

int main() {
    std::cout << "欢迎使用跨平台视频工具 v1.0.0" << std::endl;
    std::cout << "输入 'help' 获取帮助信息" << std::endl;

    // 注册命令
    commands["help"] = {showHelp, {"显示帮助信息", "help"}};
    commands["analyze"] = {analyzeVideo, {"分析视频文件", "analyze"}};
    commands["encode"] = {encodeVideo, {"编码视频文件", "encode"}};
    commands["filter"] = {applyFilter, {"应用视频滤镜", "filter"}};
    commands["test"] = {testEncoder, {"编码器测试", "test"}};
    commands["quit"] = {quit, {"退出程序", "quit"}};
    commands["exit"] = {quit, {"退出程序", "exit"}};

    // 主循环
    std::string line;
    while (true) {
        std::cout << "\nvideo> ";
        std::getline(std::cin, line);

        // 跳过空行
        if (line.empty()) {
            continue;
        }

        // 执行命令
        auto it = commands.find(line);
        if (it != commands.end()) {
            try {
                it->second.first();
            } catch (const std::exception& e) {
                std::cerr << "错误: " << e.what() << std::endl;
            }
        } else {
            std::cout << "未知命令: " << line << "\n输入 'help' 获取帮助" << std::endl;
        }
    }

    return 0;
}   