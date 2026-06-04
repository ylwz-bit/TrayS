// PawnIo.h - PawnIO 驱动封装层
// 用于替代 WinRing0 (有安全漏洞) 读取 CPU 温度
// 通过 PawnIO 安全驱动读取 MSR 寄存器
#pragma once
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

// PawnIO 上下文（不透明句柄）
typedef struct _PAWNIO_CTX PAWNIO_CTX;

// 初始化 PawnIO（检查驱动是否安装，加载 PawnIOLib.dll）
// 返回上下文指针，失败返回 NULL
PAWNIO_CTX* PawnIo_Init(void);

// 释放 PawnIO 资源
void PawnIo_Free(PAWNIO_CTX* ctx);

// 检查 PawnIO 驱动是否已安装
BOOL PawnIo_IsInstalled(void);

// 读取 MSR 寄存器
// ctx: PawnIo_Init 返回的上下文
// msr: MSR 地址
// pValue: 输出值（64位）
// 返回 TRUE 成功，FALSE 失败
BOOL PawnIo_ReadMsr(PAWNIO_CTX* ctx, DWORD msr, ULONGLONG* pValue);

// 执行 CPUID
// ctx: PawnIo_Init 返回的上下文
// leaf: CPUID 叶子号
// subleaf: 子叶子号
// pEax, pEbx, pEcx, pEdx: 输出寄存器
// 返回 TRUE 成功，FALSE 失败
BOOL PawnIo_Cpuid(PAWNIO_CTX* ctx, DWORD leaf, DWORD subleaf,
                  DWORD* pEax, DWORD* pEbx, DWORD* pEcx, DWORD* pEdx);

// 获取 CPU 温度（通过 MSR 读取）
// ctx: PawnIo_Init 返回的上下文
// core: 核心编号
// 返回温度（摄氏度），失败返回 0
int PawnIo_GetCpuTemp(PAWNIO_CTX* ctx, DWORD core);

// 检查是否为 Intel CPU
BOOL PawnIo_IsIntel(PAWNIO_CTX* ctx);

#ifdef __cplusplus
}
#endif
