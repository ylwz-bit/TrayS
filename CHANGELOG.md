# Changelog

本文档记录 TrayS 每个版本的主要变更。

## [v1.4.5] - 2026-05-27

### Added
- 支持 Windows 11 22H2+ 任务栏透明效果（通过 ExplorerTAP.dll 注入 XAML Islands）
- 新增 Win11Taskbar.h：ITaskbarAppearanceService COM 接口定义 + Win11TaskbarManager 管理类
- 从 TranslucentTB 源码编译 ExplorerTAP.dll，适配 TrayS 运行环境
- 支持透明（SolidColor）和亚克力（Acrylic）两种任务栏样式

### Fixed
- 修复 VS 2026 Insiders CRT 链接兼容性问题（改用静态 CRT /MT）

## [v1.4.4] - 2026-05-27

### Fixed
- 修复 Win10 任务栏监控信息不显示的问题（`IsChild` 对 `WS_POPUP` 窗口不可靠）
- ReadReg 兼容旧版 TrayS.dat 文件大小不匹配问题
- 退出时恢复任务栏默认样式（Win10/Win11）
- 修复 Win11 原生任务栏监控信息阴影和消失问题

## [v1.4.3] - 2026-05-26

### Fixed
- Win11 上监控窗口不再嵌入任务栏，解决退出后残留显示问题
- 资源管理器重启后自动恢复托盘图标

### Changed
- 更新 .gitignore，忽略 TranslucentBar、图片和临时文件

## [v1.4.2] - 2026-05-25

### Removed
- 移除行情（股票/加密货币价格）相关代码和显示

### Fixed
- 修复移除行情代码后遗留的 GDI 资源泄漏导致卡顿
- ReadReg 增加文件大小校验，防止加载旧版 TrayS.dat 导致字段错位
- 修复"返回"按钮退出进程的问题

## [v1.4.1] - 2026-05-25

### Added
- Windows 11 任务栏透明支持

### Fixed
- 修正 Intel CPU 温度读取偏高约 10°C 的问题（TCC offset 修正）
- 修复 Windows 11 某版本信息窗口卡顿问题
- 修复 Win11 透明失效和 Tips 卡顿
- 修复磁盘显示 bug
- 调试日志仅在 Debug 编译版本中输出

## [v1.4.0] - 2026-05-24

### Changed
- **用 PawnIO 替代 WinRing0 + OpenHardwareMonitorApi**，实现零依赖 CPU 温度监控
- 更新 Windows SDK 版本，修复 x64 Release 链接错误

### Fixed
- 修复代码审计发现的安全和稳定性问题
- 修复温度监控导致的闪退问题
- 修复设置保存导致崩溃的根本原因

## [v1.3.x 及更早版本] - 2022

由原作者 [cgbsmy](https://github.com/cgbsmy/TrayS) 维护，包含初始功能：
网速监控、CPU/内存占用、磁盘监控、秒表显示、任务栏美化、Tips 详情、
进程管理、音量控制、Windows 服务、开机自启等。
