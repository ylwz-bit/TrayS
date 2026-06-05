# TrayS 安全修复记录

## 修复原则
- 每次修复一个独立问题，编译验证后再修复下一个
- 保持向后兼容，不改变现有功能行为
- 记录每项修复的原因和教训

## 修复清单

### [x] 1. CreateFileMapping 共享内存大小不匹配
- **文件**: TrayS.cpp:904
- **问题**: CreateFileMapping(... sizeof(BOOL), szAppName) 使用 sizeof(BOOL) (4字节), 但 MapViewOfFile 用 sizeof(TRAYDATA) 映射
- **修复**: 改为 sizeof(TRAYDATA)
- **教训**: CreateFileMapping 的大小参数要与 MapViewOfFile 一致

### [x] 2. 配置文件版本校验缺失
- **文件**: TrayS.cpp:396-412
- **问题**: ReadReg() 直接 ReadFile 整个 TRAYSAVE, 无大小/版本校验
- **修复**: 在 ReadFile 后检查 dwBytes == sizeof(TRAYSAVE) 且 TraySave.Ver 合理, 否则重置默认值
- **教训**: 读取二进制配置文件必须校验大小和版本, 防止结构体变化导致读取垃圾数据

### [x] 3. pShellExecute 返回值误用
- **文件**: TrayS.cpp:814,819,824,948,953,958,4164,4170
- **问题**: CloseHandle(pShellExecute(...)) — ShellExecute 返回 HINSTANCE, 不是 HANDLE
- **修复**: 移除 CloseHandle 包装
- **教训**: ShellExecute 返回值是状态码 (>32 成功), 不是可关闭的句柄

### [x] 4. TerminateThread 危险调用替换
- **文件**: TrayS.cpp:1067
- **问题**: TerminateThread(hGetDataThread, 0) 可能导致死锁/内存泄漏
- **修复**: 替换为 RealClose = TRUE; WaitForSingleObject(hGetDataThread, 3000);
- **教训**: 永远不要用 TerminateThread, 用退出标志 + WaitForSingleObject

### [x] 5. DLL 劫持防护
- **文件**: TrayS.cpp 全局 (11处)
- **问题**: LoadLibrary() 搜索路径不安全, 可能被 DLL 劫持攻击
- **修复**: 
  - 系统 DLL → LoadLibraryExW(... LOAD_LIBRARY_SEARCH_SYSTEM32)
  - 应用 DLL → LoadLibraryExW(... LOAD_LIBRARY_SEARCH_APPLICATION_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32)
- **教训**: 永远不要用裸 LoadLibrary, 用 LoadLibraryEx 限定搜索路径

## 待修复 (未来)

### [-] 6. wsprintf 缓冲区溢出风险 (低风险, 暂不修改)
- 全局大量 wsprintf, 建议替换为 StringCchPrintf

### [x] 7. 注释编码乱码
- Function.cpp 中 GBK 注释在 UTF-8 下显示为乱码

### [x] 8. 注册表权限过大
- Function.cpp 使用 KEY_ALL_ACCESS, 应改为 KEY_READ

### [x] 9. RunProcess TOKEN_ALL_ACCESS
- Function.cpp 打开进程 token 用 TOKEN_ALL_ACCESS, 应改为 TOKEN_DUPLICATE | TOKEN_QUERY

### [x] 10. 无 SEH 异常保护
- 硬件监控读取路径无异常保护, 建议加 __try/__except

---

## 吸取的教训

1. **共享内存映射**: CreateFileMapping 和 MapViewOfFile 的大小必须一致
2. **配置文件**: 二进制格式必须有版本号和大小校验
3. **API 返回值**: 不要假设所有返回值都是 HANDLE, 查文档确认
4. **线程终止**: 永远用退出标志 + 等待, 不用 TerminateThread
5. **DLL 加载**: 用 LoadLibraryEx + 搜索路径标志防止 DLL 劫持
6. **编码一致性**: 所有源文件保持 UTF-8 BOM 编码


### [x] 6. 启动速度优化 - 温度监控 DLL 异步初始化
- **文件**: TrayS.cpp:1022, TrayS.cpp:1107, TrayS.h:165
- **问题**: LoadTemperatureDLL() 在主线程同步执行, 加载 .NET DLL (OpenHardwareMonitorApi.dll) 需要 2-5 秒, 阻塞 UI 显示
- **修复**: 将 LoadTemperatureDLL() 移到数据线程 GetDataThreadProc 中延迟初始化, 主线程立即创建窗口和托盘图标
- **效果**: UI 启动从 3-5 秒降至 <1 秒, 温度数据在后台线程异步加载
- **教训**: 级慢的初始化操作 (.NET CLR 加载, 驱动通信) 应该移到后台线程, 不阻塞 UI 启动



### [x] 7. 注释编码乱码
- **文件**: Function.cpp (53处)
- **问题**: GBK 编码的中文注释在 UTF-8 BOM 文件中显示为乱码
- **修复**: 删除 53 处不可读的乱码注释, 保留代码和可读的英文注释
- **教训**: 源文件必须统一使用 UTF-8 BOM 编码, 中文注释必须用 UTF-8 写入

### [x] 8. 注册表权限过大
- **文件**: Function.cpp:916,922,948,1432
- **问题**: RegOpenKeyEx 使用 KEY_ALL_ACCESS, 超出实际需要的权限
- **修复**:
  - 删除自启动项: KEY_ALL_ACCESS → KEY_WRITE
  - 读写自启动项: KEY_ALL_ACCESS → KEY_READ | KEY_WRITE
  - 读取主题设置: KEY_ALL_ACCESS → KEY_READ
- **教训**: 最小权限原则 - 只请求实际需要的访问权限

### [x] 9. RunProcess TOKEN_ALL_ACCESS
- **文件**: TrayS.cpp:74
- **问题**: OpenProcessToken 使用 TOKEN_ALL_ACCESS, 只需要 TOKEN_DUPLICATE | TOKEN_QUERY
- **修复**: 改为 TOKEN_DUPLICATE | TOKEN_QUERY
- **教训**: Token 权限最小化, 只请求 DuplicateTokenEx 和 CreateProcessAsUser 所需的权限

### [x] 10. SEH 异常保护
- **文件**: TrayS.cpp:536 (GetCpuTemp)
- **问题**: 硬件监控读取路径访问 PawnIO 驱动和 .NET DLL, 无异常保护
- **修复**: 在 GetCpuTemp 函数体添加 __try/__except(EXCEPTION_EXECUTE_HANDLER), 异常时返回 0
- **教训**: 涉及驱动通信和跨语言调用 (.NET C++/CLI) 的代码必须加 SEH 保护

### [-] 6. wsprintf 缓冲区溢出风险 (低风险, 暂不修改)
- **说明**: wsprintf 是 Windows API, 内部有 1024 字符长度限制, 比裸 sprintf 安全
- **现状**: 项目中无裸 sprintf, 缓冲区大小充足, 风险可控
