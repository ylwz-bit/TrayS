# TrayS

Windows 系统托盘监控工具，在任务栏嵌入实时系统监控面板。

## 功能

- **网速监控** - 实时显示上传/下载速度，支持多网卡选择
- **CPU/内存占用** - 显示 CPU 和内存使用百分比
- **温度监控** - CPU/GPU/硬盘温度（通过 PawnIO 直读 MSR/SMN）
- **磁盘监控** - 磁盘读写速率和占用率
- **秒表显示** - 在系统时钟旁显示秒数
- **任务栏美化** - 支持任务栏透明/模糊/亚克力效果
- **Tips 详情** - 鼠标悬停弹出详细信息（各网卡流量、Top6 进程、磁盘/内存使用条形图）
- **进程管理** - 在 Tips 中可终止进程或打开进程路径
- **音量控制** - 按进程调节音量
- **Windows 服务** - 支持以服务方式后台运行
- **开机自启** - 支持注册表、任务计划、服务三种自启方式

## 硬件访问：WinRing0 → PawnIO

### 为什么替换 WinRing0

旧版 TrayS 使用 WinRing0 驱动读取 CPU 温度（MSR 寄存器）。WinRing0 存在已知安全漏洞（CVE-2020-14979 等），且已停止维护。

### PawnIO

[PawnIO](https://github.com/A3NP/PawnIO) 是一个开源的轻量级 Windows 硬件访问驱动，替代 WinRing0/OlsApi，支持：
- 直读 MSR 寄存器（Intel/AMD CPU 温度）
- SMN 寄存器访问（AMD Family 17h+ 温度）
- 无已知安全漏洞

### 安装 PawnIO

1. 运行项目根目录的 `PawnIO_setup.exe` 安装驱动
2. 安装完成后启动 TrayS.exe 即可自动检测并使用

> TrayS 启动时会检测 PawnIO 驱动是否已安装。如果未安装，CPU 温度显示为 `--`，其他功能（GPU 温度、网速、CPU 占用等）不受影响。

### 旧版兼容说明

- **OpenHardwareMonitorApi/** 目录已弃用，不再参与编译，保留供参考
- **WinRing0 相关文件已移除**（OlsApiInit.h、OlsDef.h、WinRing0x64.sys 等）
- 无 .NET 运行时依赖，TrayS 为纯原生 Win32 程序

## 项目结构

```
TrayS.sln
├── TrayS/                         # 主程序 (Win32 Application)
│   ├── TrayS.cpp                  # 核心逻辑 (~5000行)
│   ├── TrayS.h                    # 数据结构 + 全局变量
│   ├── Function.cpp               # 辅助函数 (~1800行)
│   ├── Function.h                 # 函数声明
│   ├── PawnIo.h / PawnIo.cpp      # PawnIO 原生通信封装
│   ├── IntelMSR.bin               # Intel MSR 读取模块
│   ├── AMDFamily17.bin            # AMD Zen 系列模块
│   ├── AMDFamily10.bin            # AMD Family 10h-16h 模块
│   ├── AMDFamily0F.bin            # AMD 旧系列模块
│   ├── nvapi.h                    # NVIDIA GPU 接口
│   ├── adl_sdk.h 等               # ATI/AMD GPU 接口
│   └── resource.h                 # 控件 ID
├── PawnIO_setup.exe               # PawnIO 驱动安装程序
├── OpenHardwareMonitorApi/        # [已弃用] 旧 C++/CLI DLL 桥接层
└── LibreHardwareMonitor/          # [参考] LibreHardwareMonitor 源码
```

## 编译环境

- **Visual Studio 2022** (v143 工具集)
- **Windows SDK** 10.0.22621.0
- **配置**: Release | x64
- **运行库**: ucrt.lib + msvcrt.lib (IgnoreAllDefaultLibraries 模式)
- **延迟加载**: user32.dll, gdi32.dll, advapi32.dll, OLEAUT32.dll, ole32.dll
- **权限**: 需要管理员权限 (UAC RequireAdministrator)
- **DPI**: PerMonitorHighDPIAware

### 编译步骤

1. 用 Visual Studio 2022 打开 `TrayS.sln`
2. 选择 `Release | x64` 配置
3. 生成解决方案 (Ctrl+Shift+B)
4. 输出文件位于 `x64/Release/` 目录

## 运行依赖

- **PawnIO 驱动** - 需预先安装（使用 `PawnIO_setup.exe`）
- **TrayS.exe** - 单文件，无其他 DLL 依赖
- Windows 10/11 (利用了 SetWindowCompositionAttribute 等现代 API)

## 核心技术

| 技术 | 用途 |
|------|------|
| SetWindowCompositionAttribute | 任务栏透明效果 (未导出 API) |
| PawnIO 驱动 | 直读 MSR/SMN 寄存器获取 CPU 温度 |
| nvapi64.dll | NVIDIA GPU 温度 |
| atiadlxx.dll (ADL SDK) | AMD GPU 温度 |
| PDH (Performance Data Helper) | CPU 使用率、磁盘 I/O |
| iphlpapi.dll | 网络流量监控 |
| IAccessible COM | 任务栏图标位置枚举 |
| FileMapping 共享内存 | 守护进程与子进程通信 |
| GDI 双缓冲 | 所有自绘 UI |

## 配置文件

设置保存在 `TrayS.dat` (二进制格式，TRAYSAVE 结构体直接序列化)。

## 命令行参数

| 参数 | 功能 |
|------|------|
| `c` | 打开控制面板 |
| `o` | ShellExecute 打开文件 |
| `s` | 打开任务计划程序 |
| `/install` | 安装 Windows 服务 |
| `/uninstall` | 卸载 Windows 服务 |
| `/start` | 启动服务 |
| `/stop` | 停止服务 |

## 致谢

- **原作者 [cgbsmy](https://github.com/cgbsmy/TrayS)** - 感谢制作出如此精简优秀的系统监控工具，纯 Win32 API 实现，零依赖、单文件、极低资源占用，是轻量化软件的典范
- **[PawnIO](https://github.com/namazso/PawnIOLib)** - 提供安全轻量的硬件访问驱动，替代存在安全漏洞的 WinRing0
