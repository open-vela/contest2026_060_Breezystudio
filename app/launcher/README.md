# Vela Launcher（应用启动器）

基于 LVGL 的嵌入式应用启动器，为 Gemini-S1 (R528, 2.8寸屏 240x320) 设计。

## 功能特性

- 🎨 **精美的图标网格界面** — 3列×3行布局，彩色图标 + 应用名称
- 📱 **多页面支持** — 自动分页，左右箭头导航
- 🕐 **状态栏** — 显示标题和实时时钟
- 🔍 **应用发现** — 自动扫描内置 NuttX 任务
- 💾 **SD 卡应用** — 通过 manifest.json 加载外部应用
- 🚀 **应用启动** — 支持内置任务和 ELF 可执行文件

## 启用方法

```bash
# 配置
./build.sh vendor/allwinnertech/boards/r528/r528s3-gemini-s1/configs/nsh_minidisplay menuconfig

# 找到: Applications → packages/demos → contest2026_060_launcher
# 勾选: CONTEST_LAUNCHER

# 编译
./build.sh vendor/allwinnertech/boards/r528/r528s3-gemini-s1/configs/nsh_minidisplay -j8
```

## 运行

```bash
# NSH 命令行启动
launcher &

# 或设为开机自启（menuconfig 中勾选 CONTEST_LAUNCHER_AUTOSTART）
```

## SD 卡应用格式

在 SD 卡 `/sdcard/apps/` 目录下，每个应用是一个子目录，包含 `manifest.json`：

```
/sdcard/apps/
├── calculator/
│   ├── manifest.json
│   └── calculator.elf
├── game/
│   ├── manifest.json
│   └── game.elf
└── settings/
    ├── manifest.json
    └── settings.elf
```

### manifest.json 格式

```json
{
  "name": "计算器",
  "exec": "/sdcard/apps/calculator/calculator.elf",
  "type": "elf",
  "icon": ""
}
```

| 字段 | 说明 |
|------|------|
| `name` | 应用显示名称 |
| `exec` | 可执行文件路径（ELF）或内置任务名（builtin） |
| `type` | `"builtin"` 或 `"elf"` |
| `icon` | 图标路径（可选，留空使用默认） |

## 配置选项

| 配置项 | 默认值 | 说明 |
|--------|--------|------|
| `CONTEST_LAUNCHER_PRIORITY` | 100 | 任务优先级 |
| `CONTEST_LAUNCHER_STACKSIZE` | 32768 | 任务栈大小 |
| `CONTEST_LAUNCHER_INPUT_DEVPATH` | /dev/input0 | 触摸屏设备路径 |
| `CONTEST_LAUNCHER_AUTOSTART` | n | 开机自启动 |

## 依赖

- `CONFIG_GRAPHICS_LVGL` — LVGL 图形库
- `CONFIG_LV_USE_NUTTX_LCD` — NuttX LCD 后端
- `CONFIG_INPUT_TOUCHSCREEN` — 触摸屏支持（可选）
- `CONFIG_BUILTIN` — 内置应用列表
- `CONFIG_POSIX_SPAWN` — 进程创建（启动应用）

## 文件结构

```
app/launcher/
├── launcher_config.h     # 屏幕、布局、颜色配置
├── launcher_manifest.h   # SD 卡 manifest 解析器
├── launcher_manifest.c
├── launcher_app.h        # 应用发现与启动
├── launcher_app.c
├── launcher_ui.h         # LVGL UI 界面
├── launcher_ui.c
├── launcher_main.c       # 主入口
├── Kconfig               # 构建配置
├── CMakeLists.txt
├── Makefile
└── Make.defs
```
