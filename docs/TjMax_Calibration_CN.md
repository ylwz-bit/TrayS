# CPU 温度校准系统 (TjMax Calibration)

## 问题背景

Intel 混合架构处理器 (Alder Lake 及以后) 包含多种核心类型:
- P-Core (Performance Core, 性能核)
- E-Core (Efficient Core, 能效核)
- LP-Core (Low-Power Core, 低功耗能效核)

每种核心域有各自的 TjMax (最高结温), 用于将 MSR 0x19C 的 deltaT 转换为实际温度:
  温度 = TjMax - deltaT

问题: MSR 0x1A2 (IA32_TEMPERATURE_TARGET) 在部分混合架构 CPU 上对所有核心域返回相同的 TjMax 值,
但 E-Core 的实际 TjMax 可能更低。

实测数据 (Intel Core Ultra 5 338H, Panther Lake):
- MSR 0x1A2 报告: TjMax = 100 (所有核心)
- E-Core 实际 TjMax: ~93 (通过 HWMonitor 对比推算)
- 偏差: 7 度

## 解决方案

### 方案 1: CPU 型号数据库 (主方案)

维护一个已知混合架构 CPU 的 TjMax 映射表:
  { family, model, P_TjMax, E_TjMax, LP_TjMax, name }

- TjMax=0 表示与 MSR 0x1A2 一致, 无需修正
- 通过 CPUID 0x1A 检测每个逻辑处理器的核心类型 (P/E/LP)
- 命中数据库时, 使用数据库中的 TjMax 替代 MSR 0x1A2

### 方案 2: Package 温度交叉校验 (兜底方案)

对于不在数据库中的混合架构 CPU:
1. 读取 P-Core 和 E-Core 的平均温度
2. 如果 E-Core 平均温度 > P-Core 平均温度 + 3 度, 认为 E-Core TjMax 偏大
3. 自动修正: 新TjMax = 100 - (E_avg - P_avg)

原理: 空闲时, P-Core (性能核) 通常比 E-Core (能效核) 更热。
如果 E-Core 读数反超 P-Core, 说明 TjMax 有误。

局限: 修正精度有限, 仅作为未知 CPU 的应急兜底。

## 修改的文件

TrayS/PawnIo.cpp:
- 新增 CpuTjMaxEntry 结构体和 g_cpuTjDb[] 数据库
- 新增 PawnIo_InitTjCalibration(): CPU 型号检测 + 数据库查找 + 核心类型识别
- 新增 PawnIo_CrossValidateTjMax(): Package 温度交叉校验
- 修改 PawnIo_GetCpuTemp(): 应用校准后的 TjMax
- 修改 PawnIo_GetCpuTempMax(): 调用交叉校验

## 调试日志 (Debug 版本, 通过 DebugView 查看)

[TJMAX-DB]      数据库匹配结果
[TJMAX-CROSS]   交叉校验过程和结果
[TDBG] Core=X TjMax override: Y -> Z   TjMax 覆盖生效

## 添加新 CPU 到数据库

1. 在 Debug 版本中运行, 收集 [TDBG] 日志
2. 对比 HWMonitor 温度, 计算实际 TjMax: 实际TjMax = HWMonitor温度 + deltaT
3. 在 g_cpuTjDb[] 中添加新条目
4. 重新编译测试

## 已知数据库条目

CPU              Family  Model  P_TjMax  E_TjMax  LP_TjMax  状态
Panther Lake     6       0xCC   0(100)   93       0(100)    已验证
Arrow Lake-S     6       0xC5   0        94       0         待验证
Arrow Lake-P     6       0xC6   0        94       0         待验证