# TrayS v1.4.0-beta0.1 (测试版)

基于 v1.3.9 重构，解决安全漏洞和功能优化。

## 已完成功能

### 安全修复
- **移除 WinRing0 驱动**（存在安全漏洞 CVE-2020-14979）
- **集成 PawnIO 安全驱动**：通过 PawnIOLib.dll 读取 CPU 温度，替代 WinRing0 的直接 MSR 读取
- **更新 LibreHardwareMonitorLib**：从 2022 旧版升级到 v0.9.6，支持更多硬件

### Windows 11 任务栏透明
- **新增 Win11Taskbar.h**：集成 ExplorerTAP.dll，支持 Win11 任务栏透明效果
- **自动检测 Win11**：通过 Explorer 版本号判断是否启用 TAP
- **资源管理器重启恢复**：监听 TaskbarCreated 消息，Explorer 崩溃后自动恢复透明效果和托盘图标
- **修复任务栏残留**：用颜色键色（oPixelColor）填充 hTaskBar，防止 Explorer 缓存旧像素
- **退出恢复原生样式**：WM_CLOSE 时调用 RestoreAll() 恢复任务栏

### 移除的功能
- **移除行情显示**：删除 PriceProc、GetPriceThreadProc、DrawPrice 等行情相关代码
- **移除 WinRing0 相关**：删除 OlsApiInit.h、OlsDef.h、WinRing0x32/x64.sys

### 代码优化
- **移除自定义 WinMainCRTStartup**：统一使用标准 wWinMain 入口
- **移除自定义 memset/memcpy**：从 framework.h 中移除，使用标准 CRT
- **设置窗口关闭优化**：IDCANCEL 只关闭设置窗口，不退出进程（防止 guardian 重启）
- **GPU 温度 fallback 移除**：不再将 CPU Package 温度作为 GPU 温度兜底

## 系统要求

- Windows 10/11 x64
- VC++ 2015-2022 Redistributable (x64)（用户需自行安装）
- .NET Framework 4.7.2+（Windows 10 1809+ 自带）
- PawnIO 驱动（可选，用于 CPU 温度读取）

## 发布包内容

| 文件 | 大小 | 用途 |
|------|------|------|
| TrayS.exe | 180 KB | 主程序 |
| ExplorerTAP.dll | 548 KB | Win11 任务栏透明 |
| OpenHardwareMonitorApi.dll | 426 KB | 温度监控封装 |
| LibreHardwareMonitorLib.dll | 1174 KB | 核心硬件监控库 |
| PawnIOLib.dll | 38 KB | PawnIO 驱动接口 |
| IntelMSR.bin | 4 KB | MSR 数据文件 |
| TrayS.dat | 1 KB | 数据文件 |
| BlackSharp.Core.dll | 38 KB | LHM 依赖 |
| HidSharp.dll | 257 KB | HID 设备读取 |
| DiskInfoToolkit.dll | 997 KB | 硬盘信息读取 |
| RAMSPDToolkit-NDD.dll | 244 KB | 内存 SPD 读取 |
| System.Buffers.dll | 23 KB | .NET 内存管理 |
| System.Memory.dll | 142 KB | .NET 内存管理 |
| System.Numerics.Vectors.dll | 108 KB | .NET 向量运算 |
| System.Runtime.CompilerServices.Unsafe.dll | 19 KB | .NET 运行时支持 |

## 已知问题

- Debug 版本可能比 Release 版本运行内存更大
- 部分旧硬件可能需要手动安装 PawnIO 驱动
- Win10 上 ExplorerTAP.dll 可选，程序会自动跳过

## 测试建议

1. 开启任务栏风格 → 切换温度显示开关 → 观察是否有残留
2. 重启资源管理器 → 检查透明效果是否自动恢复
3. 退出 TrayS → 检查任务栏是否恢复原生样式
4. 检查 CPU/GPU 温度显示是否正常
