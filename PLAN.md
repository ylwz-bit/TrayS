# TrayS 开发计划与备忘

## 当前状态

- 代码已全部学习完毕，架构分析已保存
- 编译环境已修复 (SDK 10.0.22621.0 + x64 Release 链接)
- 最新提交: `526314c fix: 更新Windows SDK版本并修复x64 Release链接错误`

---

## 核心约束: 轻量化

TrayS 是轻量级工具，所有修改必须遵守以下原则：

- **启动速度**：不得引入启动延迟。.NET CLR 初始化、DLL 加载等必须异步或延迟加载，阻塞主线程不超过 100ms
- **运行时性能**：监控刷新周期 1 秒，单次数据采集总耗时不得超过 200ms
- **内存占用**：工作集不得显著增长。新版 LHM 的 PawnIO 驱动是嵌入资源，运行时才加载，可以接受
- **二进制体积**：不得引入不必要的依赖。只编译需要的 net472 目标，不引入 WinForms/WPF 等 GUI 框架
- **无卡顿**：UI 线程不得执行耗时操作。所有硬件查询、HTTP 请求必须在工作线程完成
- **无后台驻留**：不使用定时器轮询以外的后台机制。服务模式下内存释放定时器 (ID=11) 必须正常工作

违反轻量化原则的修改一律不接受。

---

## 一、LibreHardwareMonitor 升级计划

### 现状分析

- 项目中已有新版 LibreHardwareMonitor 源码（C#/.NET），LibreHardwareMonitorLib 已支持 **PawnIO** 驱动（内嵌为资源）
- 目标框架: net472 / netstandard2.0 / net8.0 / net9.0 / net10.0
- 当前 TrayS 使用的是预编译的 LibreHardwareMonitorLib.dll（688KB，手动拷贝到 OpenHardwareMonitorApi/）
- OpenHardwareMonitorApi（C++/CLI DLL）是桥接层，通过 `using namespace LibreHardwareMonitor::Hardware` 调用 .NET API

### 升级步骤

1. **编译新版 LibreHardwareMonitorLib.dll**
   - 用 VS2022 打开 `LibreHardwareMonitor/LibreHardwareMonitor.sln`
   - 编译 LibreHardwareMonitorLib 项目（Release|x64，net472 目标）
   - 将生成的 dll 拷贝到 `OpenHardwareMonitorApi/` 替换旧版

2. **验证 API 兼容性**
   - 检查新版 Computer/IHardware/ISensor/IVisitor 接口是否有变更
   - 检查 HardwareType/SensorType 枚举值是否一致
   - 如有变更，更新 OpenHardwareMonitorImp.cpp 和 UpdateVisitor.cpp

3. **测试温度读取**
   - 验证 CPU 温度（Core Average / 各核心温度）
   - 验证 GPU 温度（NVIDIA/AMD）
   - 验证硬盘温度和使用率
   - 验证主板温度

### 风险点

- 新版 LibreHardwareMonitor 可能修改了传感器命名规则（如 "Core Average"、"GPU Core"、"Total Activity"），需逐一确认
- net472 目标可能需要确认 .NET Framework 4.7.2 运行时可用

---

## 二、WinRing0 替换为 PawnIO 计划

### 现状分析

- WinRing0 仅作为 **备选方案** 使用，当 OpenHardwareMonitorApi 加载失败时才启用
- WinRing0 仅用于 **CPU 温度** 读取，GPU 温度用的是 NvAPI/ADL
- 调用链: `LoadTemperatureDLL()` → 如果 OHM 失败 → `InitOpenLibSys()` → `GetCpuTemp()` 中使用
- 涉及的 WinRing0 函数:
  - `Cpuid(0, ...)` - CPU 厂商检测
  - `Cpuid(1, ...)` - AMD family 检测
  - `Rdmsr(0x1A2)` / `Rdmsr(0x19C)` - Intel CPU 温度
  - `ReadPciConfigDwordEx()` - AMD CPU 温度
- WinRing0 驱动文件: WinRing0x32.sys, WinRing0x64.sys, WinRing0x32.dll, WinRing0x64.dll

### PawnIO 集成方案

**方案A: 如果新版 LibreHardwareMonitorLib 已内置 PawnIO（当前情况）**
- 新版 LHM 已经将 PawnIO 驱动作为嵌入资源，替代了旧的 WinRing0
- 只要 OpenHardwareMonitorApi.dll 能正常加载新版 LibreHardwareMonitorLib.dll，PawnIO 就会自动生效
- 此时 WinRing0 备选路径可以完全移除

**方案B: TrayS 原生集成 PawnIO（如果需要保留直接硬件访问的备选路径）**
- 需要 PawnIO 的 SDK/头文件和驱动文件
- 创建 PawnIO 初始化头文件（类似 OlsApiInit.h）
- 替换 GetCpuTemp() 中的 WinRing0 调用:
  - `Rdmsr` → PawnIO 的 MSR 读取接口
  - `ReadPciConfigDwordEx` → PawnIO 的 PCI 配置空间读取接口
  - `Cpuid` → PawnIO 的 CPUID 接口（或直接用 __cpuid 内联指令）

### 具体修改文件

| 文件 | 修改内容 |
|------|----------|
| TrayS/OlsApiInit.h | 移除或保留标记为弃用 |
| TrayS/OlsDef.h | 移除或保留标记为弃用 |
| TrayS/OlsApiInitDef.h | 移除或保留标记为弃用 |
| TrayS/TrayS.h | `bRing0` / `m_hOpenLibSys` / `bIntel` 变量处理 |
| TrayS/TrayS.cpp LoadTemperatureDLL() | 移除 InitOpenLibSys 调用，改为 PawnIO 初始化或完全依赖 OHM |
| TrayS/TrayS.cpp FreeTemperatureDLL() | 移除 DeinitOpenLibSys 调用 |
| TrayS/TrayS.cpp GetCpuTemp() | 移除 WinRing0 的 Rdmsr/ReadPciConfigDwordEx 调用 |
| TrayS/WinRing0x32.sys | 移除 |
| TrayS/WinRing0x64.sys | 移除 |

### 建议策略

**优先采用方案A**：让新版 LibreHardwareMonitorLib 内置的 PawnIO 负责所有硬件访问。WinRing0 备选路径在新版 LHM 可用的情况下意义不大。保留一个简化的错误提示即可。

---

## 三、代码审计发现的问题

### 严重级别 (CRITICAL)

| # | 问题 | 位置 | 说明 |
|---|------|------|------|
| 1 | **TerminateThread** | TrayS.cpp:1117-1118 | 线程正在执行堆操作/HTTP/PDH时被强杀，可能导致死锁或内存损坏。应改为信号退出+WaitForSingleObject |
| 2 | **全局变量竞态条件** | TrayS.h 全局变量 | TraySave/TrayData/iCPU/MemoryStatusEx 等被工作线程和UI线程同时读写，无任何同步机制 |
| 3 | **CreateFileMapping 大小不匹配 (Debug)** | TrayS.cpp:955 | Debug构建用 sizeof(BOOL) 创建映射但用 sizeof(TRAYDATA) 映射，越界访问 |

### 高级别 (HIGH)

| # | 问题 | 位置 | 说明 |
|---|------|------|------|
| 4 | **lstrcpy/lstrcat 缓冲区溢出** | Function.cpp:793,1360-1361 | 无边界检查的字符串操作，应改用 StringCchCopy/StringCchCat |
| 5 | **wsprintf 无缓冲区大小参数** | 全局 (100+处) | 应改用 StringCchPrintf |
| 6 | **ShellExecute 返回值误用** | TrayS.cpp:998,1003,1008 | ShellExecute 返回 HINSTANCE 不是内核句柄，不应 CloseHandle |
| 7 | **LaunchAppIntoDifferentSession 句柄泄漏** | Function.cpp:186,281-282 | hSnap 未关闭，pi.hProcess/hThread 未关闭 |
| 8 | **HeapAlloc 返回值未检查** | TrayS.cpp:3384,1309,1293,1382 | 分配失败直接使用空指针 |
| 9 | **CreateFile 检查方式错误** | Function.cpp:849-850 | 用 `if(hFile)` 检查，应为 `if(hFile != INVALID_HANDLE_VALUE)` |

### 中级别 (MEDIUM)

| # | 问题 | 位置 | 说明 |
|---|------|------|------|
| 10 | **PDH 资源泄漏** | TrayS.cpp:219-303 | 退出时只 FreeLibrary 未 PdhCloseQuery |
| 11 | **GetPixel 性能差** | TrayS.cpp:22 | 每次调用都往返显示驱动，应缓存或用其他方法 |
| 12 | **AttachThreadInput 不安全** | Function.cpp:1317-1322 | 合并输入队列可能死锁 |
| 13 | **SERVICE_INTERACTIVE_PROCESS 已弃用** | Function.cpp:301 | Vista 起已无效 |
| 14 | **MapViewOfFile 返回值未检查** | TrayS.cpp:949,956 | 映射失败直接使用空指针 |
| 15 | **命令注入风险** | Function.cpp:858-861 | schtasks 命令行参数未转义 |

### 低级别 (LOW)

| # | 问题 | 位置 | 说明 |
|---|------|------|------|
| 16 | **GetProcessImageFileName 返回设备路径** | Function.cpp:1063 | 显示 \Device\HarddiskVolume1\ 路径用户不理解 |
| 17 | **硬编码版本号** | TrayS.cpp 多处 | 22000/25000 等 build 号硬编码，未来可能不兼容 |

---

## 四、执行顺序建议

### 第一阶段: 升级 LibreHardwareMonitor（优先）
1. 编译新版 LibreHardwareMonitorLib.dll（只编译 net472 目标，保持轻量）
2. 替换 OpenHardwareMonitorApi/ 中的旧 dll
3. 验证 OpenHardwareMonitorImp.cpp 的 API 兼容性，必要时修改
4. 编译 TrayS 全解决方案，确保通过
5. 功能测试温度读取
6. **轻量化验证**：测量启动耗时、内存占用、单次数据采集耗时
7. Git commit

### 第二阶段: 移除 WinRing0
1. 确认新版 LHM + PawnIO 工作正常后
2. 移除 WinRing0 相关文件 (.sys/.dll/.h)
3. 简化 LoadTemperatureDLL() / FreeTemperatureDLL() / GetCpuTemp()
4. 编译测试
5. **轻量化验证**：确认移除后启动更快（少了 WinRing0 驱动加载）
6. Git commit

### 第三阶段: 修复审计问题（按严重程度）
1. 先修 CRITICAL 项（TerminateThread、竞态条件、Debug 映射大小）
2. 再修 HIGH 项（缓冲区溢出、句柄泄漏、返回值检查）
3. 最后修 MEDIUM 项（资源泄漏、性能、弃用 API）
4. 每修一类问题单独 commit
5. **注意**：修复竞态条件时，锁的粒度要细，不能引入性能瓶颈

### 第四阶段: 代码整理（可选）
1. TrayS.cpp 拆分（5000行太大）
2. 全局变量封装
3. 统一字符串处理函数

---

## 五、关键文件速查

| 文件 | 行数 | 内容 |
|------|------|------|
| TrayS/TrayS.cpp | ~5000 | 主逻辑: WinMain、线程、窗口过程、绘图 |
| TrayS/Function.cpp | ~1800 | 辅助函数: DLL加载、服务管理、HTTP、字符串 |
| TrayS/TrayS.h | ~534 | 数据结构 (TRAYSAVE/TRAYDATA/TRAFFIC) + 全局变量 |
| TrayS/Function.h | ~163 | 函数声明 + ACCENT_POLICY 结构 |
| TrayS/OlsApiInit.h | ~360 | WinRing0 初始化和函数指针（待移除） |
| OpenHardwareMonitorApi/OpenHardwareMonitorImp.cpp | ~363 | 硬件温度读取实现（需适配新版API） |
| LibreHardwareMonitor/ | 整个目录 | 新版 LHM 源码（含 PawnIO 驱动） |

---

## 六、已完成

- [x] 项目代码全面学习 (2026-05-24)
- [x] 编译环境修复 - SDK 版本 + x64 Release 链接错误 (2026-05-24)
- [x] 架构文档保存到 memory 系统 (2026-05-24)
- [x] README.md 编写 (2026-05-24)
- [x] PLAN.md 编写 (2026-05-24)
- [x] 代码审计完成 (2026-05-24)

---

## 七、Git 工作流约定

- 每次功能修改单独提交，便于回滚
- 提交信息格式: `类型: 简要描述`
  - `feat:` 新功能
  - `fix:` 修复
  - `refactor:` 重构
  - `docs:` 文档
  - `build:` 构建配置
