# TrayS

Windows 系统托盘监控工具，在任务栏嵌入实时系统监控面板。

## 功能

- **网速监控** - 实时显示上传/下载速度，支持多网卡选择
- **CPU/内存占用** - 显示 CPU 和内存使用百分比
- **温度监控** - CPU/GPU/硬盘温度（通过 OpenHardwareMonitor 或 WinRing0 直读 MSR）
- **磁盘监控** - 磁盘读写速率和占用率
- **行情监控** - 支持 OKX 加密货币和新浪财经股票实时价格
- **秒表显示** - 在系统时钟旁显示秒数
- **任务栏美化** - 支持任务栏透明/模糊/亚克力效果
- **Tips 详情** - 鼠标悬停弹出详细信息（各网卡流量、Top6 进程、磁盘/内存使用条形图）
- **进程管理** - 在 Tips 中可终止进程或打开进程路径
- **音量控制** - 按进程调节音量
- **Windows 服务** - 支持以服务方式后台运行
- **开机自启** - 支持注册表、任务计划、服务三种自启方式

## 项目结构

```
TrayS.sln
├── TrayS/                         # 主程序 (Win32 Application)
│   ├── TrayS.cpp                  # 核心逻辑 (~5000行)
│   ├── TrayS.h                    # 数据结构 + 全局变量
│   ├── Function.cpp               # 辅助函数 (~1800行)
│   ├── Function.h                 # 函数声明
│   ├── resource.h                 # 控件 ID
│   ├── TrayS.rc                   # 资源文件 (对话框/图标/字符串)
│   ├── framework.h                # Windows 头文件包含
│   ├── OlsApiInit.h / OlsDef.h    # WinRing0 驱动接口
│   ├── nvapi.h                    # NVIDIA GPU 接口
│   ├── adl_sdk.h 等               # ATI/AMD GPU 接口
│   └── OpenHardwareMonitorApi.h   # 温度监控 DLL 接口
├── OpenHardwareMonitorApi/        # C++/CLI DLL (桥接 LibreHardwareMonitor)
│   ├── OpenHardwareMonitorImp.cpp # 实现类
│   ├── OpenHardwareMonitorImp.h   # COpenHardwareMonitor + MonitorGlobal
│   ├── OpenHardwareMonitorApi.h   # 接口定义
│   └── UpdateVisitor.cpp/.h       # 硬件更新访问者
└── LibreHardwareMonitor/          # C# 库 (独立编译)
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

- **OpenHardwareMonitorApi.dll** - 温度/硬件监控 (需与 TrayS.exe 同目录)
- **LibreHardwareMonitorLib.dll** - OpenHardwareMonitorApi 的依赖
- Windows 10/11 (利用了 SetWindowCompositionAttribute 等现代 API)

## 核心技术

| 技术 | 用途 |
|------|------|
| SetWindowCompositionAttribute | 任务栏透明效果 (未导出 API) |
| WinRing0 驱动 | 直读 MSR 寄存器获取 CPU 温度 |
| nvapi64.dll | NVIDIA GPU 温度 |
| atiadlxx.dll (ADL SDK) | AMD GPU 温度 |
| PDH (Performance Data Helper) | CPU 使用率、磁盘 I/O |
| iphlpapi.dll | 网络流量监控 |
| WinHttp | HTTP 请求 (行情数据) |
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

## 许可

原作者: cgbsmy (https://github.com/cgbsmy/TrayS)
