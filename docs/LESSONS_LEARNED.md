# TrayS 开发教训与优化计划

## 一、历史教训 (2026-05)

### 做对了的事情

#### 架构级
- PawnIO 替代 WinRing0 + OpenHardwareMonitorApi → 零依赖单文件
- 静态链接 CRT → 不依赖 VC++ 运行时
- LTCG + /O1 优化 → 体积 248KB → 216KB

#### 功能性
- Win11 任务栏透明 (ExplorerTAP)
- Win11 Tips 卡顿修复
- 移除行情代码 → 精简 + 解决 GDI 泄漏
- 资源管理器重启恢复托盘图标
- Win11 透明持久化

#### 稳定性
- 全面安全审查 (缓冲区溢出、句柄泄漏 11+ 项)
- ReadReg 兼容旧版 TrayS.dat
- 修复 Release 版崩溃 (自定义 memcpy/memset)

### 走弯路的事情

#### 1. TjMax=93 override 错误
- 现象: 一台 Panther Lake E-Core 偏高 7°C
- 错误操作: 直接改数据库 eTjMax=93
- 根因: HWMonitor 对比方式错误 (看 Package 不是 E-Core Max)
- 结果: MSR 原始值 100 反而是正确的, 93 让 E/LP 核心偏低 7°C
- 教训: **数据对比要准确, 确认 HWMonitor 看的是哪个温度**

#### 2. IGCL 搜索反复失败
- 现象: intel_gfx_api-x64.dll 就在 DriverStore 目录里
- 错误操作: 盲目加搜索模式 (5 种 pattern + System32 兜底)
- 根因: 没先让用户列目录, LoadLibraryW 缺依赖搜索
- 教训: **先观察再动手, 不盲猜路径**

#### 3. D3DKMT 方案编译失败
- 错误操作: 没验证头文件兼容性就加代码
- 教训: **先验证可行性再写代码**

#### 4. OHM 导致卡顿闪退
- 错误操作: 没参考 1.3.9 的初始化测试机制, 直接加载 LibreHardwareMonitor
- 根因: 全量扫描硬件阻塞主线程, 可能和 PawnIo 冲突
- 教训: **引入外部依赖必须有容错 (初始化测试 + 失败卸载)**

#### 5. WMI 不是 GPU 温度
- 错误操作: MSAcpi_ThermalZoneTemperature 当 GPU 温度用
- 根因: 系统热区温度不对应 GPU
- 教训: **WMI 接口要验证返回值含义**

#### 6. 反复提交反复修改
- 现象: 一天内提交 10+ 次, release 更新 5+ 次
- 根因: 没充分测试就推送
- 教训: **本地验证充分后再推送**

#### 7. release 包缺 DLL
- 错误操作: 只传 exe, 漏了 OHM 依赖
- 教训: **release 打包要完整检查**

#### 8. 越改越乱
- 现象: 同时改 TjMax + IGCL + OHM + WMI, 互相干扰
- 教训: **一次只改一个点, 验证通过再改下一个**

## 二、核心原则

1. 一次只改一个点, 验证通过再改下一个
2. 先观察再动手, 不盲猜路径
3. 数据对比要准确, 确认参考值来源
4. 引入外部依赖必须有容错机制
5. 本地验证充分后再推送
6. release 打包要完整检查
7. 保持单文件精简架构 (PawnIO 原生方案)
8. 不要"好心"加功能 (如 CPU Package 作为 GPU fallback)
