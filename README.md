# X264编码器测试工具

这是一个用于测试和比较x264编码器不同参数配置效果的跨平台工具。它提供了直观的图形界面，让用户可以方便地调整各种编码参数，并实时查看编码效果。

## 功能特点

- 支持多种预设场景（直播、点播、存档等）
- 可调整的编码参数：
  - 基本参数（预设、调优、线程数）
  - 视频参数（分辨率、帧数）
  - 码率控制（CRF、CQP、ABR、CBR）
  - GOP参数（关键帧间隔、B帧数、参考帧数）
  - 质量参数（运动估计、加权预测、CABAC等）
- 实时编码状态显示
- 编码历史记录和导出
- 编码完成后视频预览
- 性能指标统计（PSNR、SSIM）

## 系统要求

- C++17或更高版本
- CMake 3.16或更高版本
- Qt 6.4或更高版本
- FFmpeg开发库
- Boost库

### 依赖库版本

- FFmpeg >= 4.2
- libx264 >= 0.155
- Boost >= 1.65
- Qt >= 6.4.2

## 编译说明

1. 安装依赖（Ubuntu为例）：
```bash
sudo apt update
sudo apt install build-essential cmake
sudo apt install qtbase-dev qtmultimedia-dev
sudo apt install libavcodec-dev libavformat-dev libavutil-dev libswscale-dev
sudo apt install libboost-all-dev
sudo apt install libx264-dev
```

2. 克隆代码：
```bash
git clone https://github.com/yourusername/x264-encoder-test.git
cd x264-encoder-test
```

3. 编译：
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## 使用说明

1. 启动程序后，首先选择预设场景或手动调整参数
2. 等待帧生成完成
3. 点击"开始编码"按钮开始编码测试
4. 编码过程中可以实时查看进度和性能指标
5. 编码完成后可以：
   - 预览编码后的视频
   - 查看详细的编码报告
   - 在历史记录中比较不同配置的效果
   - 导出编码历史记录

## 注意事项

- 确保系统有足够的内存用于帧缓存
- 编码过程中避免进行其他重负载操作
- 建议先使用较小的帧数进行测试

## 贡献指南

欢迎提交Issue和Pull Request！在提交代码前，请确保：

1. 代码符合项目的编码规范
2. 添加了必要的注释和文档
3. 所有测试用例通过
4. 提交信息清晰明了

## 开源许可

本项目采用 MIT 许可证。详见 [LICENSE](LICENSE) 文件。

## 作者

- 作者名字
- 联系方式

## 致谢

感谢以下开源项目：

- [FFmpeg](https://ffmpeg.org/)
- [x264](https://www.videolan.org/developers/x264.html)
- [Qt](https://www.qt.io/)
- [Boost](https://www.boost.org/)