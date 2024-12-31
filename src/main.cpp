#include <iostream>
#include "ffmpeg/x264_param_test.hpp"

int main() 
{
    std::cout << "开始x264编码器参数测试\n" << std::endl;

    // 1. 测试不同预设
    std::cout << "\n=== 测试不同预设的性能差异 ===" << std::endl;
    auto presetResults = X264ParamTest::runPresetTest(1920, 1080, 300);

    // 2. 测试不同CRF值
    std::cout << "\n=== 测试不同CRF值的质量和性能权衡 ===" << std::endl;
    std::vector<int> crfValues = {18, 23, 28, 33};  // 从高质量到低质量
    auto crfResults = X264ParamTest::runRateControlTest(
        X264ParamTest::RateControl::CRF,
        crfValues,
        1920, 1080, 300
    );

    // 3. 测试不同码率的CBR模式
    std::cout << "\n=== 测试不同码率的CBR模式 ===" << std::endl;
    std::vector<int> bitrateValues = {
        2000000,   // 2Mbps
        5000000,   // 5Mbps
        10000000,  // 10Mbps
        20000000   // 20Mbps
    };
    auto cbrResults = X264ParamTest::runRateControlTest(
        X264ParamTest::RateControl::CBR,
        bitrateValues,
        1920, 1080, 300
    );

    // 4. 测试质量参数组合
    std::cout << "\n=== 测试质量参数组合 ===" << std::endl;
    X264ParamTest::TestConfig baseConfig;
    baseConfig.preset = X264ParamTest::Preset::Fast;  // 使用较快的预设
    baseConfig.rateControl = X264ParamTest::RateControl::CRF;
    baseConfig.crf = 23;

    std::vector<std::pair<std::string, std::vector<bool>>> params = {
        {"weightedPred", {false, true}},
        {"cabac", {false, true}}
    };
    auto qualityResults = X264ParamTest::runQualityTest(baseConfig, params);

    return 0;
}   