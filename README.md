# Vela Music Player（音乐播放器）

基于 LVGL 的嵌入式音乐播放器，为 Gemini-S1（R528, 2.8寸屏 320x240）设计，支持 SD 卡音乐浏览、音频播放、蓝牙 A2DP Sink 及音量控制。

## 一、作品简介

一款面向 openvela 嵌入式设备的全功能音乐播放器，提供直观的触屏操作体验。支持从 SD 卡浏览和播放 MP3/WAV 音频文件，集成蓝牙 A2DP Sink 可接收手机无线音乐流，并带有底部常驻播放控制栏与设置界面。

**亮点：**
- SD 卡文件浏览器（支持中文文件名）
- MP3 / WAV 音频播放（基于 nxplayer）
- 蓝牙 A2DP Sink 无线音乐接收
- 底部常驻播放控制栏（播放/暂停/进度）
- 音量滑块调节
- 当前播放文件高亮标记
- 中文界面（FreeType / CJK 字体渲染）

## 二、选题方向

**AI 硬件产品创新** — 基于 Gemini-S1 开发板打造实用音乐播放器产品，探索嵌入式多媒体交互体验。

## 三、目录结构

- `app/musicplayer/` — 音乐播放器主应用
  - `music_player.c` — 主界面（文件浏览、播放页、蓝牙页、设置页）
  - `music_playback.c/h` — 音频播放控制（nxplayer 封装）
  - `music_bluetooth.c/h` — 蓝牙 A2DP Sink 集成
  - `config.h` — 显示/音频/颜色配置
  - `Kconfig` / `Makefile` / `CMakeLists.txt` — 构建配置
- `app/launcher/` — 应用启动器（辅助）
- `logs/` — AI Coding 日志

## 四、运行方式

### 1. 拉取工程

```bash
repo init -u https://github.com/open-vela/contest2026_060_Breezystudio \
  -b dev-ai-contest-2026 -m contest2026_060_Breezystudio.xml
repo sync -c -j8
```

### 2. 启用音乐播放器

```bash
./build.sh vendor/allwinnertech/boards/r528/r528s3-gemini-s1/configs/nsh_minidisplay menuconfig
# 找到: Applications → packages/demos → contest2026_060_musicplayer
# 勾选: CONTEST_MUSICPLAYER
```

### 3. 编译

```bash
./build.sh vendor/allwinnertech/boards/r528/r528s3-gemini-s1/configs/nsh_minidisplay -j8
```

### 4. 部署与运行

将编译产物烧录到 Gemini-S1 开发板，插上含音频文件的 SD 卡，在 NSH 命令行启动音乐播放器即可。

## 五、AI Coding 使用说明

本作品借助 AI 辅助完成需求拆解、方案设计、编码实现与调试：

- **需求拆解**：与 AI 讨论嵌入式音乐播放器的功能模块划分（文件浏览、播放控制、蓝牙、UI 布局）
- **方案设计**：AI 基于 LVGL + NuttX 技术栈给出界面架构和模块接口建议
- **编码实现**：AI 辅助生成各模块代码框架（播放控制、蓝牙 A2DP、CJK 字体渲染等）
- **调试优化**：通过 AI 分析编译错误和运行时问题，快速定位修复

完整对话日志见 `logs/` 目录。
