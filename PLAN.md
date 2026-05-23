# TrayS 开发计划与备忘

## 当前状态

- 代码已全部学习完毕，架构分析已保存
- 编译环境已修复 (SDK 10.0.22621.0 + x64 Release 链接)
- 最新提交: `526314c fix: 更新Windows SDK版本并修复x64 Release链接错误`

---

## 待优化项

### 代码质量

- [ ] TrayS.cpp 过于庞大 (~5000行)，考虑拆分为多个模块
- [ ] 全局变量过多 (~50+个)，考虑封装为结构体或类
- [ ] 自定义字符串/浮点函数 (xstrstr/lstrstr/xatof/xwtof) 可考虑统一风格

### 功能改进

- [ ] 网速监控支持更多单位格式
- [ ] 温度监控增加 AMD CPU 更多型号支持
- [ ] 行情模块增加更多数据源
- [ ] Tips 窗口增加更多进程信息 (如命令行参数)
- [ ] 设置界面优化 (当前控件较多，可考虑分页/分组)

### 构建与发布

- [ ] 添加 CI/CD 自动构建脚本
- [ ] 添加版本号自动管理
- [ ] 考虑添加安装程序 (NSIS/Inno Setup)

---

## 已完成

- [x] 项目代码全面学习 (2026-05-24)
- [x] 编译环境修复 - SDK 版本 + x64 Release 链接错误 (2026-05-24)
- [x] 架构文档保存到 memory 系统 (2026-05-24)
- [x] README.md 编写 (2026-05-24)
- [x] PLAN.md 编写 (2026-05-24)

---

## 关键文件速查

| 文件 | 行数 | 内容 |
|------|------|------|
| TrayS/TrayS.cpp | ~5000 | 主逻辑: WinMain、线程、窗口过程、绘图 |
| TrayS/Function.cpp | ~1800 | 辅助函数: DLL加载、服务管理、HTTP、字符串 |
| TrayS/TrayS.h | ~534 | 数据结构 (TRAYSAVE/TRAYDATA/TRAFFIC) + 全局变量 |
| TrayS/Function.h | ~163 | 函数声明 + ACCENT_POLICY 结构 |
| TrayS/resource.h | ~171 | 对话框和控件 ID |
| OpenHardwareMonitorApi/OpenHardwareMonitorImp.cpp | ~363 | 硬件温度读取实现 |

---

## Git 工作流约定

- 每次功能修改单独提交，便于回滚
- 提交信息格式: `类型: 简要描述`
  - `feat:` 新功能
  - `fix:` 修复
  - `refactor:` 重构
  - `docs:` 文档
  - `build:` 构建配置
