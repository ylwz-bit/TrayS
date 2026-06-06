TrayS v1.4.6-test - CPU 温度校准版本
======================================

目录结构:
  Release/     日常使用版本
  Debug/       调试版本 (含详细日志, 用于排查温度问题)
  Tools/       辅助工具 (DebugView + HWMonitor)

快速开始:
  1. 运行 Release\TrayS.exe 即可使用
  2. 如需报告温度问题, 用 Debug 版本配合工具采集日志 (见下方)

======================================
温度不准确? 请按以下步骤反馈:
======================================

准备工具 (在 Tools 目录下):
  - DebugView.zip    解压后运行 DbgView.exe (Sysinternals)
  - hwmonitor_1.63.zip  解压后运行 HWMonitor.exe (CPUID)

采集步骤 (三步, 时间间隔尽量短):

  1) 打开 DebugView (DbgView.exe)
     菜单: Capture -> Capture Win32 确保勾选

  2) 运行 Debug\TrayS_Debug.exe
     等待约 10 秒, 让温度读数稳定

  3) 同时打开 HWMonitor.exe
     等待约 5 秒, 让 HWMonitor 采集到温度

  注意: DebugView 和 HWMonitor 的采集时间不要间隔太久
  (建议在 30 秒内完成)

保存日志:

  4) DebugView: File -> Save As -> 保存为 .log 文件
     命名格式: 你的电脑名.log (如 MY-PC.log)

  5) HWMonitor: File -> Save Monitoring Data -> 保存为 .txt 文件
     命名格式: 你的电脑名_HWMonitor.txt

提交反馈:

  6) 到 GitHub Issues 提交:
     https://github.com/ylwz-bit/TrayS/issues

     附上:
     - DebugView 的 .log 文件
     - HWMonitor 的 .txt 文件
     - 你的 CPU 型号 (如: Intel Core i7-13700K)
     - 温度偏差描述 (如: TrayS 显示 45 度, HWMonitor 显示 38 度)

======================================
版本说明 (中文)
======================================

本次更新:
  - 新增 CPU 温度校准系统 (TjMax Calibration)
  - 修复 Intel 混合架构处理器 E-Core 温度偏高问题
  - 已验证: Panther Lake (Intel Core Ultra 5 338H) E-Core 偏差从 7 度降至 1 度以内
  - 支持 CPU 型号数据库 + Package 温度交叉校验兜底

技术文档: docs/TjMax_Calibration_CN.md

======================================
Release Notes (English)
======================================

Changes:
  - Added CPU temperature calibration system (TjMax Calibration)
  - Fixed E-Core temperature reading too high on Intel hybrid CPUs
  - Verified: Panther Lake (Intel Core Ultra 5 338H) E-Core deviation reduced from 7C to within 1C
  - CPU model database + Package temperature cross-validation fallback

Technical docs: docs/TjMax_Calibration_EN.md