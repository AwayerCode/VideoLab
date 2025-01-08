VLC SDK 使用说明

目录结构：
- lib/: 包含 libvlc 和 libvlccore 动态库
- include/: 包含所有头文件
- plugins/: 包含 VLC 插件
- pkgconfig/: 包含 pkg-config 配置文件

使用方法：
1. 将 lib 目录下的所有文件复制到你的应用程序的库目录或系统库目录
2. 将 plugins 目录复制到你的应用程序目录
3. 开发时，include 目录包含所有需要的头文件

编译示例：
gcc your_app.c -I./include -L./lib -lvlc -lvlccore -o your_app

运行时注意事项：
1. 确保 lib 目录在系统的库搜索路径中
2. 设置 VLC_PLUGIN_PATH 环境变量指向 plugins 目录
   export VLC_PLUGIN_PATH=/path/to/plugins

