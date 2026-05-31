# TrayS Beta 优化计划

## 目标
基于 1.3.9 代码 + release/v1.4.6 的正确改动, 重新整理一次干净的优化版本。

## 核心原则
1. 一次只改一个点, 验证通过再改下一个
2. 保持单文件精简架构 (PawnIO 原生方案)
3. 不盲猜路径, 先观察再动手
4. 引入外部依赖必须有容错机制
5. release 打包要完整检查

## Phase 1: 架构基础 (从 1.3.9 升级)

### 1.1 PawnIO 替代 WinRing0
- 来源: release/v1.4.6 的 PawnIo.cpp/PawnIo.h
- 验证: CPU 温度读取正确, 无闪退
- 状态: 已验证 ✓

### 1.2 静态链接 CRT
- 来源: release/v1.4.5 的 vcxproj 配置
- 验证: 单文件运行, 不依赖 VC++ 运行时
- 状态: 已验证 ✓

### 1.3 Win11 任务栏透明 (ExplorerTAP)
- 来源: release/v1.4.5 的 Win11Taskbar.h/ExplorerTAP
- 验证: Win11 透明正常, Explorer 重启后恢复
- 状态: 已验证 ✓

## Phase 2: 温度监控优化

### 2.1 CPU 温度 (PawnIO)
- 使用 MSR 0x1A2 + 0x19C 读取
- TjMax 数据库: 仅记录已验证的 CPU 型号, 不盲目 override
- 当前已验证: Panther Lake (Model=0xCC) MSR TjMax=100 正确
- 状态: 已验证 ✓

### 2.2 GPU 温度链路 (待验证)
优先级从高到低:
1. NvAPI (NVIDIA 独显)
2. ADL (AMD 独显)
3. IGCL (Intel 核显, LoadLibraryExW + DriverStore 搜索)
4. OHM (LibreHardwareMonitor, 1.3.9 风格初始化测试)
5. 显示 0 (不用 CPU Package 凑数)

#### 2.2.1 IGCL 实现
- 使用 LoadLibraryExW + LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR
- 搜索 DriverStore 目录: iigd_dch*.inf_amd64_*
- 目标 DLL: intel_gfx_api-x64.dll (已在 Panther Lake DriverStore 中确认)
- 需要验证: LoadLibraryExW 是否能解决依赖搜索
- 状态: 待验证

#### 2.2.2 OHM 实现 (可选)
- 1.3.9 风格: 初始化时测试一次, 失败则卸载
- 仅用于 GPU 温度, 不 fallback 到 CPU Package
- 需要 OpenHardwareMonitorApi.dll + LibreHardwareMonitorLib.dll
- 打破单文件架构, 仅作为 IGCL 失败时的备选
- 状态: 待验证

### 2.3 TjMax 数据库策略
- 只记录已验证的条目
- 所有域=0 表示不覆盖 MSR 默认值
- 新 CPU 型号需要多台机器交叉验证
- 对比 HWMonitor 时确认看的是 per-core 温度, 不是 Package

## Phase 3: 代码清理

### 3.1 移除不必要的代码
- 移除行情相关代码 (已做)
- 移除自定义 memcpy/memset (已做)
- 清理未使用的变量和函数

### 3.2 安全审查
- 缓冲区溢出检查
- 句柄泄漏检查
- 空指针检查
- 来源: release/v1.4.5 的安全修复

## Phase 4: 测试与发布

### 4.1 测试清单
- [ ] Win10 x64: 基本功能正常
- [ ] Win11 x64: 任务栏透明正常
- [ ] Intel CPU: 温度读取正确
- [ ] AMD CPU: 温度读取正确
- [ ] NVIDIA GPU: 温度读取正确
- [ ] Intel 核显: IGCL 温度读取 (如有)
- [ ] 资源管理器重启: 托盘图标恢复
- [ ] 长时间运行: 无内存泄漏

### 4.2 Release 打包检查
- [ ] Release exe 编译成功
- [ ] Debug exe 编译成功
- [ ] 如需 OHM: 包含 OpenHardwareMonitorApi.dll + LibreHardwareMonitorLib.dll
- [ ] 版本号更新
- [ ] CHANGELOG 更新

## 执行顺序
1. 从 release/v1.4.6 合并 PawnIO + 静态 CRT + Win11 透明 → beta
2. 验证 CPU 温度正确
3. 实现 IGCL (LoadLibraryExW) → 验证
4. 如 IGCL 失败, 实现 OHM (1.3.9 风格) → 验证
5. 代码清理 + 安全审查
6. 测试 + 发布
