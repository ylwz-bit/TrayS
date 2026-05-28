# TrayS

> 当前版本: v1.4.5 (2026-05-28) | [更新日志](CHANGELOG.md)

Windows 系统托盘监控工具，在任务栏嵌入实时系统监控面板。

## 下载

> **请务必从官方渠道下载，不要使用来历不明的第三方分发版本。**
> TrayS 以管理员权限运行，且 Win11 版本会通过 ExplorerTAP.dll 注入 explorer.exe，使用被篡改的程序存在严重安全风险。

| 渠道 | 地址 |
|------|------|
| GitHub Releases (官方) | https://github.com/ylwz-bit/TrayS/releases |

**首次使用前请核验：**
- 确认下载来源为上方官方地址
- Release 页面的 TrayS.exe 和 ExplorerTAP.dll 均由项目作者发布

## 功能

- **网速监控** - 实时显示上传/下载速度，支持多网卡选择
- **CPU/内存占用** - 显示 CPU 和内存使用百分比
- **温度监控** - CPU/GPU/硬盘温度（通过 PawnIO 直读 MSR/SMN）
- **磁盘监控** - 磁盘读写速率和占用率
- **秒表显示** - 在系统时钟旁显示秒数
- **任务栏美化** - 支持任务栏透明/模糊/亚克力效果（Windows 11 22H2+ 通过 ExplorerTAP 实现）
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

## Windows 11 任务栏透明

Windows 11 22H2 之后，任务栏从传统 Win32 窗口切换为 XAML Islands 实现，原有的 `SetWindowCompositionAttribute` API 无法再实现透明效果。

TrayS v1.4.5 通过集成 [TranslucentTB](https://github.com/TranslucentTB/TranslucentTB) 的 ExplorerTAP 注入机制来解决此问题：

### 工作原理

1. TrayS 检测到 Windows 11 XAML 任务栏时，加载同目录下的 `ExplorerTAP.dll`
2. 通过 `SetWindowsHookEx` 将 DLL 注入 `explorer.exe` 进程
3. 利用 `InitializeXamlDiagnosticsEx` 连接 XAML Islands
4. 通过 COM 接口 `ITaskbarAppearanceService` 修改任务栏背景画刷

### 支持的效果

| 模式 | 说明 |
|------|------|
| 透明 (SolidColor) | 完全透明任务栏 |
| 亚克力 (Acrylic) | 半透明模糊效果 |

### 系统要求

- **Windows 11 22H2 及以上** — 需要 `ExplorerTAP.dll`（与 TrayS.exe 同目录）
- **Windows 11 21H2 及以下 / Windows 10** — 使用原有 `SetWindowCompositionAttribute` 方式，不需要 `ExplorerTAP.dll`

### 运行文件

| 文件 | 说明 | 必需 |
|------|------|------|
| `TrayS.exe` | 主程序 | 是 |
| `ExplorerTAP.dll` | Win11 XAML 任务栏透明支持 | Win11 22H2+ 需要 |

## 项目结构

```
TrayS.sln
├── TrayS/                         # 主程序 (Win32 Application)
│   ├── TrayS.cpp                  # 核心逻辑 (~5000行)
│   ├── TrayS.h                    # 数据结构 + 全局变量
│   ├── Function.cpp               # 辅助函数 (~1800行)
│   ├── Function.h                 # 函数声明
│   ├── Win11Taskbar.h             # Win11 任务栏透明 (ExplorerTAP COM 接口)
│   ├── PawnIo.h / PawnIo.cpp      # PawnIO 原生通信封装
│   ├── IntelMSR.bin               # Intel MSR 读取模块
│   ├── AMDFamily17.bin            # AMD Zen 系列模块
│   ├── AMDFamily10.bin            # AMD Family 10h-16h 模块
│   ├── AMDFamily0F.bin            # AMD 旧系列模块
│   ├── nvapi.h                    # NVIDIA GPU 接口
│   ├── adl_sdk.h 等               # ATI/AMD GPU 接口
│   └── resource.h                 # 控件 ID
├── TranslucentTB/                 # TranslucentTB 源码（编译 ExplorerTAP.dll）
│   └── ExplorerTAP/               # XAML Islands 任务栏透明注入模块
├── PawnIO_setup.exe               # PawnIO 驱动安装程序
├── OpenHardwareMonitorApi/        # [已弃用] 旧 C++/CLI DLL 桥接层
└── LibreHardwareMonitor/          # [参考] LibreHardwareMonitor 源码
```

## 编译环境

- **Visual Studio 2022/2026** (v143/v145 工具集)
- **Windows SDK** 10.0.22621.0
- **配置**: Release | x64
- **运行库**: 静态 CRT (/MT)
- **延迟加载**: user32.dll, gdi32.dll, advapi32.dll, OLEAUT32.dll, ole32.dll
- **权限**: 需要管理员权限 (UAC RequireAdministrator)
- **DPI**: PerMonitorHighDPIAware

### 编译步骤

1. 用 Visual Studio 2022 打开 `TrayS.sln`
2. 选择 `Release | x64` 配置
3. 生成解决方案 (Ctrl+Shift+B)
4. 输出文件位于 `x64/Release/` 目录

### 编译 ExplorerTAP.dll

**编译环境：**

| 项目 | 版本 |
|------|------|
| Visual Studio | 2022/2026 (v143/v145 工具集) |
| Windows SDK | 10.0.22621.0 或更高 |
| CppWinRT | 2.0.250303.1 (NuGet) |
| Windows SDK CPP | 10.0.26100.7463 (NuGet) |
| Detours | Mile.Detours 1.0.2180 (NuGet) |
| WIL | Microsoft.Windows.ImplementationLibrary 1.0.240803.1 (NuGet) |
| 配置 | Release / x64 |
| Spectre 缓解 | 禁用 (/p:SpectreMitigation=false) |
| 签名 | 跳过 (/p:SkipSigning=True) |

**依赖获取：**

ExplorerTAP.dll 需要从 TranslucentTB 源码单独编译：

依赖获取：
```powershell
# NuGet 包（Windows SDK、CppWinRT、WIL、Detours）
tools\nuget.exe restore TranslucentTB\ExplorerTAP\packages.config -PackagesDirectory TranslucentTB\packages
tools\nuget.exe install Mile.Detours -OutputDirectory deps\nuget -Version 1.0.2180
tools\nuget.exe install Microsoft.Windows.ImplementationLibrary -OutputDirectory deps\nuget
```

编译命令：
```powershell
MSBuild.exe TranslucentTB\ExplorerTAP\ExplorerTAP.vcxproj /p:Configuration=Release /p:Platform=x64 /p:SkipSigning=True /p:SpectreMitigation=false /p:VcpkgEnableManifest=false /p:ForceImportAfterCppTargets="deps\ExplorerTAP-deps.props"
```

编译完成后将 `ExplorerTAP.dll` 复制到 TrayS.exe 同目录即可。

## 运行依赖

- **PawnIO 驱动** - 需预先安装（使用 `PawnIO_setup.exe`）
- **TrayS.exe** - 主程序
- **ExplorerTAP.dll** - Win11 22H2+ 任务栏透明支持（与 TrayS.exe 同目录）
- Windows 10/11

## 核心技术

| 技术 | 用途 |
|------|------|
| SetWindowCompositionAttribute | 任务栏透明效果 (Win10 / Win11 21H2) |
| ExplorerTAP (TranslucentTB) | Win11 22H2+ XAML 任务栏透明注入 |
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
- **[TranslucentTB](https://github.com/TranslucentTB/TranslucentTB)** - 提供 ExplorerTAP 注入机制，实现 Windows 11 22H2+ XAML 任务栏透明效果。源码位于 `TranslucentTB/` 目录，`ExplorerTAP.dll` 从源码编译
- **[PawnIO](https://github.com/A3NP/PawnIO)** - 提供安全轻量的硬件访问驱动，替代存在安全漏洞的 WinRing0
