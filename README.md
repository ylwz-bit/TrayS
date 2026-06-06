# TrayS

Windows 任务栏监控工具 — 在系统托盘区域显示 CPU/GPU 温度、网络流量、磁盘读写、内存/CPU 利用率等实时信息。

## 功能特性

### 任务栏监控信息
- **CPU 温度**：通过 PawnIO 安全驱动读取（替代 WinRing0）
- **GPU 温度**：通过 LibreHardwareMonitor 读取（支持 NVIDIA/AMD/Intel 核显）
- **网络流量**：实时上传/下载速度显示
- **磁盘读写**：实时硬盘读写速度
- **CPU/内存利用率**：系统资源使用率
- **硬盘温度**：通过 S.M.A.R.T. 读取
- **时间显示**：可选秒级时间显示

### Windows 11 任务栏透明
- 集成 ExplorerTAP.dll，支持 Win11 任务栏透明效果
- 自动检测 Win11 环境并启用 TAP
- 资源管理器崩溃后自动恢复透明效果
- 退出时恢复任务栏原生样式

### 系统托盘图标
- 自定义托盘图标
- 支持窗口置顶/取消置顶
- 支持打开进程路径/窗口路径
- 音量控制（静音/调节）

## 系统要求

| 要求 | 说明 |
|------|------|
| 操作系统 | Windows 10/11 x64 |
| VC++ Redistributable | 2015-2022 (x64) — 用户需自行安装 |
| .NET Framework | 4.7.2+（Win10 1809+ 自带） |
| PawnIO 驱动 | 可选，用于 CPU 温度读取 |

## 下载

从 [Releases](https://github.com/ylwz-bit/TrayS/releases) 页面下载最新版本。

### 发布包内容

| 文件 | 大小 | 用途 |
|------|------|------|
| `TrayS.exe` | 180 KB | 主程序 |
| `ExplorerTAP.dll` | 548 KB | Win11 任务栏透明 |
| `OpenHardwareMonitorApi.dll` | 426 KB | 温度监控封装（C++/CLI） |
| `LibreHardwareMonitorLib.dll` | 1174 KB | 核心硬件监控库 |
| `PawnIOLib.dll` | 38 KB | PawnIO 驱动接口 |
| `IntelMSR.bin` | 4 KB | MSR 数据文件 |
| `TrayS.dat` | 1 KB | 数据文件 |
| `BlackSharp.Core.dll` | 38 KB | LHM 依赖 |
| `HidSharp.dll` | 257 KB | HID 设备读取 |
| `DiskInfoToolkit.dll` | 997 KB | 硬盘信息读取 |
| `RAMSPDToolkit-NDD.dll` | 244 KB | 内存 SPD 读取 |
| `System.Buffers.dll` | 23 KB | .NET 内存管理 |
| `System.Memory.dll` | 142 KB | .NET 内存管理 |
| `System.Numerics.Vectors.dll` | 108 KB | .NET 向量运算 |
| `System.Runtime.CompilerServices.Unsafe.dll` | 19 KB | .NET 运行时支持 |

## 使用方法

1. 解压发布包到任意目录
2. 运行 `TrayS.exe`
3. 右键托盘图标打开设置
4. 开启需要的监控项目（温度、流量、利用率等）
5. 可选：开启「任务栏风格」使用 Win11 透明效果

### 温度监控

CPU 温度读取依赖 PawnIO 驱动：
1. 安装 [PawnIO](https://github.com/A3Sec-Lab/PawnIO) 驱动
2. 确保 `PawnIOLib.dll` 在 TrayS.exe 同目录
3. 在设置中开启「显示温度」

GPU 温度通过 LibreHardwareMonitor 读取，支持：
- NVIDIA 独立显卡（通过 nvapi）
- AMD 独立显卡（通过 ADL）
- Intel 核显（通过 IGCL）

## 构建

### 环境要求

- Visual Studio 2022 (v143) 或更高版本
- .NET Framework 4.7.2 SDK
- Windows 10 SDK 10.0.22621.0

### 编译

```
# Release 版本
MSBuild TrayS.sln /p:Configuration=Release /p:Platform=x64 /t:Build

# Debug 版本
MSBuild TrayS.sln /p:Configuration=Debug /p:Platform=x64 /t:Build
```

### 输出

- Release: `x64\Release\TrayS.exe`
- Debug: `x64\Debug\TrayS.exe`

## 项目结构

```
TrayS/
├── TrayS/
│   ├── TrayS.cpp          # 主程序逻辑（窗口、消息、绘制）
│   ├── TrayS.h            # 数据结构、全局变量定义
│   ├── Function.cpp       # 通用函数（服务、进程、系统）
│   ├── Function.h         # 函数声明
│   ├── Win11Taskbar.h     # Win11 任务栏透明管理
│   ├── PawnIo.cpp         # PawnIO 驱动接口
│   ├── PawnIo.h           # PawnIO 函数定义
│   ├── framework.h        # 预编译头
│   ├── resource.h         # 资源 ID 定义
│   └── TrayS.rc           # 资源文件（图标、对话框）
├── OpenHardwareMonitorApi/
│   ├── OpenHardwareMonitorApi.h    # 导出函数声明
│   ├── OpenHardwareMonitorImp.cpp  # C++/CLI 封装实现
│   ├── OpenHardwareMonitorImp.h    # 实现类定义
│   └── LibreHardwareMonitorLib.dll # 核心监控库
└── release_pkg/
    ├── TrayS_v1.4.0-beta0.1_x64/   # Release 文件
    ├── TrayS_v1.4.0-beta0.1_x64.zip # 发布包
    └── RELEASE_NOTES.md             # 发布说明
```

## 温度获取链路

### CPU 温度
1. **PawnIO**（优先）：通过 PawnIOLib.dll 读取 MSR 寄存器
2. **LibreHardwareMonitor**（兜底）：通过 OpenHardwareMonitorApi.dll 读取

### GPU 温度
1. **NVIDIA**：通过 nvapi64.dll 读取
2. **AMD**：通过 atiadlxx.dll 读取
3. **Intel 核显**：通过 LibreHardwareMonitor 的 IGCL 读取

## 版本历史

### v1.4.0-beta0.1 (2026-06-07)

基于 v1.3.9 重构的测试版本。

**新增功能：**
- Win11 任务栏透明（ExplorerTAP.dll 集成）
- PawnIO 安全驱动替代 WinRing0
- 资源管理器重启自动恢复
- 任务栏残留修复（颜色键色填充）

**安全修复：**
- 移除 WinRing0 驱动（CVE-2020-14979）
- 集成 PawnIO 安全驱动

**依赖更新：**
- LibreHardwareMonitorLib 升级到 v0.9.6
- 新增 System.*.dll 等 .NET 依赖

**移除功能：**
- 移除行情显示功能
- 移除 WinRing0 相关代码

**已知问题：**
- Debug 版本运行内存可能较大
- 部分旧硬件需手动安装 PawnIO 驱动

### v1.3.9

- 初始稳定版本
- 使用 WinRing0 读取 CPU 温度
- 使用旧版 LibreHardwareMonitorLib

## 问题反馈

如遇到问题，请提交 Issue 并附上：
1. 系统版本（Win10/Win11，具体版本号）
2. CPU 型号
3. 问题描述和复现步骤
4. Debug 日志（如有）

## 许可证

本项目基于原始 TrayS 项目修改。

## 致谢

- [LibreHardwareMonitor](https://github.com/LibreHardwareMonitor/LibreHardwareMonitor) — 硬件监控核心库
- [PawnIO](https://github.com/A3Sec-Lab/PawnIO) — 安全驱动接口
- [TranslucentTB](https://github.com/TranslucentTB/TranslucentTB) — Win11 任务栏透明参考
