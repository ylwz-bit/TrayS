# TrayS 开发计划与备忘

## 当前状态

- 代码已全部学习完毕，架构分析已保存
- 编译环境已修复 (SDK 10.0.22621.0 + x64 Release 链接)
- **Phase 1 完成**: PawnIO 替代 WinRing0 + OHM，编译通过 (Release 138KB)
- 最新提交: `0631f40 feat: 用PawnIO替代WinRing0+OpenHardwareMonitorApi`

---

## 核心约束: 轻量化

TrayS 是轻量级工具，所有修改必须遵守以下原则：

- **零依赖、单文件**：最终发布只需要 TrayS.exe + PawnIO .bin 模块文件（以及首次运行时提示安装 PawnIO 驱动）
- **启动速度**：不得引入启动延迟，阻塞主线程不超过 100ms
- **运行时性能**：监控刷新周期 1 秒，单次数据采集总耗时不得超过 200ms
- **内存占用**：工作集不得显著增长
- **二进制体积**：不得引入不必要的依赖
- **无卡顿**：UI 线程不得执行耗时操作
- **无后台驻留**：不使用定时器轮询以外的后台机制

违反轻量化原则的修改一律不接受。

---

## 一、温度监控重构：原生 PawnIO 替代 WinRing0 + .NET 链路

### 目标

去掉整个 .NET 依赖链，用原生 C++ 直接调用 PawnIO 驱动，恢复零依赖单文件架构。

### 当前架构（需要去掉的部分）

```
TrayS.exe
  └─ LoadLibrary("OpenHardwareMonitorApi.dll")   ← C++/CLI DLL
       └─ 引用 LibreHardwareMonitorLib.dll       ← .NET 程序集
            └─ 内嵌 PawnIO .bin 模块              ← 通过 DeviceIoControl 调用
  └─ 备选: WinRing0x64.dll → WinRing0x64.sys     ← 已知高危漏洞
```

### 目标架构

```
TrayS.exe
  └─ 直接 DeviceIoControl("\\.\PawnIO")          ← 原生 C++ 调用
       └─ 加载 IntelMSR.bin / AMDFamily17.bin    ← 随 TrayS.exe 分发
```

### PawnIO 通信协议（纯 Win32 API，无 .NET）

1. 检查 PawnIO 驱动是否安装：读注册表 `HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\PawnIO`
2. 打开设备：`CreateFileW(L"\\\\?\\GLOBALROOT\\Device\\PawnIO", ...)`
3. 加载模块：`DeviceIoControl(hDevice, IOCTL_PIO_LOAD_BINARY, binData, binSize, ...)`
4. 调用函数：`DeviceIoControl(hDevice, IOCTL_PIO_EXECUTE_FN, input, ...)`

其中：
- `IOCTL_PIO_LOAD_BINARY = (41394 << 16) | (0x821 << 2)`
- `IOCTL_PIO_EXECUTE_FN = (41394 << 16) | (0x841 << 2)`
- 输入缓冲区：32字节 ASCII 函数名 + int64 参数
- 输出缓冲区：int64 结果

### CPU 温度读取方案

**Intel（通过 IntelMSR.bin）：**
- 调用 `ioctl_read_msr(0x1A2)` → 获取 TjMax
- 调用 `ioctl_read_msr(0x19C)` → 获取热读数
- 计算: TjMax - (读数 >> 16 & 0xFF) = 温度

**AMD（通过 AMDFamily17.bin）：**
- 调用 `ioctl_read_smn(0x00059800)` → 获取温度寄存器
- 计算: (寄存器 >> 21) & 0x7F = 温度
- 旧 AMD（AMDFamily0F.bin / AMDFamily10.bin）：通过 `ioctl_read_msr` 或 `ioctl_get_thermtrip`

**CPUID 厂商检测：**
- 直接使用编译器内联 `__cpuid()` 指令，不需要 PawnIO

### PawnIO 未安装时的处理策略

启动时检测 PawnIO 是否安装，如果未安装：

1. **首次提示**：弹出 MessageBox 告知用户
   ```
   TrayS 需要 PawnIO 驱动来读取 CPU 温度。
   PawnIO 是一个开源的轻量级硬件访问驱动，替代已知存在安全漏洞的 WinRing0。

   [立即安装]  [跳过（无温度监控）]  [了解更多]
   ```

2. **"立即安装"**：
   - TrayS 内嵌 `PawnIO_setup.exe`（作为资源嵌入，运行时释放到临时目录执行）
   - 或者：ShellExecute 打开 PawnIO 下载页面，让用户手动安装
   - 安装完成后提示重启 TrayS

3. **"跳过"**：
   - CPU 温度显示为 `--` 或 `N/A`
   - GPU 温度仍然可用（NvAPI/ADL 不依赖 PawnIO）
   - 网速、CPU占用、内存、磁盘等监控不受影响
   - 不再重复提示（将用户选择保存到 TrayS.dat）

4. **"了解更多"**：
   - 打开 PawnIO 的 GitHub 页面

5. **后续启动**：
   - 读取 TrayS.dat 中的用户选择
   - 如果用户之前选择跳过，不再弹窗，直接跳过温度监控
   - 每次启动仍然检测驱动是否已安装，如果已安装则自动启用

### 具体修改清单

| 文件 | 操作 | 说明 |
|------|------|------|
| TrayS/PawnIo.h | **新建** | PawnIO 设备通信封装（CreateFile、DeviceIoControl、模块加载） |
| TrayS/PawnIo.cpp | **新建** | PawnIO 初始化、ReadMsr、ReadSmn、GetCpuTemp 实现 |
| TrayS/IntelMSR.bin | **添加** | 从 LibreHardwareMonitor 复制，嵌入为资源或随 exe 分发 |
| TrayS/AMDFamily17.bin | **添加** | 同上 |
| TrayS/AMDFamily0F.bin | **添加** | 同上（旧 AMD 支持） |
| TrayS/AMDFamily10.bin | **添加** | 同上（旧 AMD 支持） |
| TrayS/TrayS.rc | **修改** | 添加 .bin 文件作为资源嵌入（或选择外部文件方式） |
| TrayS/resource.h | **修改** | 添加 PawnIO 相关资源 ID |
| TrayS/TrayS.cpp LoadTemperatureDLL() | **重写** | 去掉 OHM 和 WinRing0，改为 PawnIO 初始化 + 未安装提示逻辑 |
| TrayS/TrayS.cpp FreeTemperatureDLL() | **重写** | 释放 PawnIO 设备句柄 |
| TrayS/TrayS.cpp GetCpuTemp() | **重写** | 通过 PawnIO 的 DeviceIoControl 读取 MSR/SMN |
| TrayS/TrayS.h | **修改** | 去掉 bRing0/m_hOpenLibSys 等旧变量，添加 PawnIO 句柄变量 |
| TrayS/OlsApiInit.h | **删除** | WinRing0 头文件（不再需要） |
| TrayS/OlsDef.h | **删除** | WinRing0 常量定义（不再需要） |
| TrayS/OlsApiInitDef.h | **删除** | WinRing0 函数指针类型（不再需要） |
| TrayS/OpenHardwareMonitorApi.h | **删除** | OHM 接口头文件（不再需要） |
| TrayS/OpenHardwareMonitorApi.lib | **删除** | OHM 导入库（不再需要） |
| TrayS/OpenHardwareMonitorGlobal.h | **删除** | OHM 导出宏（不再需要） |
| TrayS/WinRing0x32.sys | **删除** | WinRing0 32位驱动（不再需要） |
| TrayS/WinRing0x64.sys | **删除** | WinRing0 64位驱动（不再需要） |
| OpenHardwareMonitorApi/ | **整个目录不参与编译** | C++/CLI 桥接层不再需要，可保留源码参考但不编译 |
| TrayS/TrayS.vcxproj | **修改** | 移除 OHM 相关 lib 引用（如有） |

### 最终发布文件

```
TrayS.exe              (~101KB, 不变)
```

.bin 模块可以选择：
- **方案A**：嵌入 TrayS.exe 作为资源（运行时释放到临时目录或内存加载）→ 真正的单文件
- **方案B**：随 TrayS.exe 分发为独立文件 → 多几个小文件但更灵活

GPU 温度不受影响（NvAPI/ADL 是独立的动态加载，已有备用方案）。

---

## 二、代码审计发现的问题

### 严重级别 (CRITICAL)

| # | 问题 | 位置 | 说明 |
|---|------|------|------|
| 1 | **TerminateThread** | TrayS.cpp:1117-1118 | 线程正在执行堆操作/HTTP/PDH时被强杀，可能导致死锁。应改为信号退出+WaitForSingleObject |
| 2 | **全局变量竞态条件** | TrayS.h 全局变量 | TraySave/TrayData/iCPU/MemoryStatusEx 等被工作线程和UI线程同时读写，无任何同步 |
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

## 三、执行顺序

### 第一阶段: 原生 PawnIO 替代 WinRing0 + .NET 链路 [已完成]

1. [x] 创建 PawnIo.h / PawnIo.cpp（DeviceIoControl 通信封装）
2. [x] 实现 CPU 温度读取（Intel ReadMsr + AMD ReadSmn）
3. [x] 实现 PawnIO 未安装检测 + 用户提示逻辑（MessageBox）
4. [x] 嵌入 .bin 模块到 TrayS.exe 资源
5. [x] 重写 LoadTemperatureDLL() / FreeTemperatureDLL() / GetCpuTemp()
6. [x] 移除 WinRing0 相关文件和代码
7. [x] 移除 OpenHardwareMonitorApi 相关代码引用
8. [x] 编译 TrayS.exe（单文件 138KB，无外部 DLL 依赖）
9. [ ] **功能测试**：需在实机上验证 Intel/AMD CPU 温度读取
10. [ ] **轻量化验证**：启动耗时、内存占用

### 第二阶段: 修复审计问题（按严重程度）

1. 先修 CRITICAL 项（TerminateThread、竞态条件、Debug 映射大小）
2. 再修 HIGH 项（缓冲区溢出、句柄泄漏、返回值检查）
3. 最后修 MEDIUM 项（资源泄漏、性能、弃用 API）
4. 每修一类问题单独 commit
5. **注意**：修复竞态条件时，锁的粒度要细，不能引入性能瓶颈

### 第三阶段: 代码整理（可选）

1. TrayS.cpp 拆分（5000行太大）
2. 全局变量封装
3. 统一字符串处理函数

---

## 四、关键文件速查

| 文件 | 行数 | 内容 |
|------|------|------|
| TrayS/TrayS.cpp | ~5000 | 主逻辑: WinMain、线程、窗口过程、绘图 |
| TrayS/Function.cpp | ~1800 | 辅助函数: DLL加载、服务管理、HTTP、字符串 |
| TrayS/TrayS.h | ~534 | 数据结构 (TRAYSAVE/TRAYDATA/TRAFFIC) + 全局变量 |
| TrayS/Function.h | ~163 | 函数声明 + ACCENT_POLICY 结构 |
| TrayS/PawnIo.h | ~22 | PawnIO 原生通信头文件 |
| TrayS/PawnIo.cpp | ~285 | PawnIO 原生通信实现 (DeviceIoControl) |

---

## 五、已完成

- [x] 项目代码全面学习 (2026-05-24)
- [x] 编译环境修复 - SDK 版本 + x64 Release 链接错误 (2026-05-24)
- [x] 架构文档保存到 memory 系统 (2026-05-24)
- [x] README.md 编写 (2026-05-24)
- [x] PLAN.md 编写 (2026-05-24)
- [x] 代码审计完成 (2026-05-24)
- [x] Phase 1: PawnIO 替代 WinRing0 + OHM，编译通过 (2026-05-24)

---

## 六、Git 工作流约定

- 每次功能修改单独提交，便于回滚
- 提交信息格式: `类型: 简要描述`
  - `feat:` 新功能
  - `fix:` 修复
  - `refactor:` 重构
  - `docs:` 文档
  - `build:` 构建配置
