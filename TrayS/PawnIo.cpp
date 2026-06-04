// PawnIo.cpp - PawnIO 驱动封装层 (重写版)
// 替代 WinRing0 读取 CPU 温度（MSR 寄存器）
// 通过 PawnIOLib.dll 加载 IntelMSR.bin 模块读取 MSR
#include "PawnIo.h"
#include <intrin.h>
#include <stdio.h>

// PawnIOLib.dll 函数类型定义
typedef HRESULT(STDAPICALLTYPE* pfn_pawnio_open)(PHANDLE handle);
typedef HRESULT(STDAPICALLTYPE* pfn_pawnio_close)(HANDLE handle);
typedef HRESULT(STDAPICALLTYPE* pfn_pawnio_load)(HANDLE handle, const UCHAR* blob, SIZE_T size);
typedef HRESULT(STDAPICALLTYPE* pfn_pawnio_execute)(HANDLE handle, PCSTR name,
    const ULONG64* in, SIZE_T in_size,
    PULONG64 out, SIZE_T out_size, PSIZE_T return_size);
typedef BOOL(WINAPI* pfn_pawnio_version_win32)(PULONG version);

struct _PAWNIO_CTX {
    HMODULE hDll;
    HANDLE hDevice;
    pfn_pawnio_open fn_open;
    pfn_pawnio_close fn_close;
    pfn_pawnio_load fn_load;
    pfn_pawnio_execute fn_execute;
    BOOL bIntel;
    BOOL bBlobLoaded;
};

// 从文件加载 blob
static UCHAR* LoadBlobFromFile(const WCHAR* path, SIZE_T* pSize)
{
    HANDLE hFile = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return NULL;
    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == INVALID_FILE_SIZE || fileSize == 0) { CloseHandle(hFile); return NULL; }
    UCHAR* buf = (UCHAR*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, fileSize);
    if (!buf) { CloseHandle(hFile); return NULL; }
    DWORD bytesRead;
    ReadFile(hFile, buf, fileSize, &bytesRead, NULL);
    CloseHandle(hFile);
    if (bytesRead != fileSize) { HeapFree(GetProcessHeap(), 0, buf); return NULL; }
    *pSize = fileSize;
    return buf;
}

// 获取 TrayS.exe 所在目录
static void GetModuleDir(WCHAR* buf, DWORD bufSize)
{
    GetModuleFileNameW(NULL, buf, bufSize);
    // 去掉文件名，保留目录
    WCHAR* p = wcsrchr(buf, L'\\');
    if (p) *(p + 1) = L'\0';
}

BOOL PawnIo_IsInstalled(void)
{
    HMODULE hDll = LoadLibraryW(L"PawnIOLib.dll");
    if (!hDll) {
        // 尝试从 PawnIO 安装目录加载
        WCHAR path[MAX_PATH];
        GetModuleDir(path, MAX_PATH);
        wcscat_s(path, L"PawnIOLib.dll");
        hDll = LoadLibraryW(path);
    }
    if (!hDll) return FALSE;

    pfn_pawnio_open fn_open = (pfn_pawnio_open)GetProcAddress(hDll, "pawnio_open");
    if (!fn_open) { FreeLibrary(hDll); return FALSE; }

    HANDLE hDev = NULL;
    HRESULT hr = fn_open(&hDev);
    if (SUCCEEDED(hr) && hDev != NULL) {
        pfn_pawnio_close fn_close = (pfn_pawnio_close)GetProcAddress(hDll, "pawnio_close");
        if (fn_close) fn_close(hDev);
        FreeLibrary(hDll);
        return TRUE;
    }
    FreeLibrary(hDll);
    return FALSE;
}

PAWNIO_CTX* PawnIo_Init(void)
{
    PAWNIO_CTX* ctx = (PAWNIO_CTX*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PAWNIO_CTX));
    if (!ctx) return NULL;

    // 加载 PawnIOLib.dll - 先从当前目录，再从 PawnIO 安装目录
    ctx->hDll = LoadLibraryW(L"PawnIOLib.dll");
    if (!ctx->hDll) {
        WCHAR path[MAX_PATH];
        GetModuleDir(path, MAX_PATH);
        wcscat_s(path, L"PawnIOLib.dll");
        ctx->hDll = LoadLibraryW(path);
    }
    if (!ctx->hDll) {
        // 尝试 Program Files
        ctx->hDll = LoadLibraryW(L"C:\\Program Files\\PawnIO\\PawnIOLib.dll");
    }
    if (!ctx->hDll) {
        HeapFree(GetProcessHeap(), 0, ctx);
        return NULL;
    }

    ctx->fn_open = (pfn_pawnio_open)GetProcAddress(ctx->hDll, "pawnio_open");
    ctx->fn_close = (pfn_pawnio_close)GetProcAddress(ctx->hDll, "pawnio_close");
    ctx->fn_load = (pfn_pawnio_load)GetProcAddress(ctx->hDll, "pawnio_load");
    ctx->fn_execute = (pfn_pawnio_execute)GetProcAddress(ctx->hDll, "pawnio_execute");

    if (!ctx->fn_open || !ctx->fn_close || !ctx->fn_load || !ctx->fn_execute) {
        FreeLibrary(ctx->hDll);
        HeapFree(GetProcessHeap(), 0, ctx);
        return NULL;
    }

    // 打开 PawnIO 设备
    HRESULT hr = ctx->fn_open(&ctx->hDevice);
    if (FAILED(hr) || ctx->hDevice == NULL) {
        FreeLibrary(ctx->hDll);
        HeapFree(GetProcessHeap(), 0, ctx);
        return NULL;
    }

    // 检测 CPU 厂商
    int cpuInfo[4] = {0};
    __cpuid(cpuInfo, 0);
    if (cpuInfo[1] == 0x756e6547 && cpuInfo[3] == 0x49656e69 && cpuInfo[2] == 0x6c65746e)
        ctx->bIntel = TRUE;
    else
        ctx->bIntel = FALSE;

    // 加载 IntelMSR.bin 模块
    ctx->bBlobLoaded = FALSE;
    WCHAR blobPath[MAX_PATH];
    GetModuleDir(blobPath, MAX_PATH);
    wcscat_s(blobPath, L"IntelMSR.bin");

    SIZE_T blobSize = 0;
    UCHAR* blob = LoadBlobFromFile(blobPath, &blobSize);
    if (blob && blobSize > 0) {
        hr = ctx->fn_load(ctx->hDevice, blob, blobSize);
        if (SUCCEEDED(hr))
            ctx->bBlobLoaded = TRUE;
        HeapFree(GetProcessHeap(), 0, blob);
    }

    if (!ctx->bBlobLoaded) {
        ctx->fn_close(ctx->hDevice);
        FreeLibrary(ctx->hDll);
        HeapFree(GetProcessHeap(), 0, ctx);
        return NULL;
    }

    return ctx;
}

void PawnIo_Free(PAWNIO_CTX* ctx)
{
    if (!ctx) return;
    if (ctx->hDevice && ctx->fn_close)
        ctx->fn_close(ctx->hDevice);
    if (ctx->hDll)
        FreeLibrary(ctx->hDll);
    HeapFree(GetProcessHeap(), 0, ctx);
}

BOOL PawnIo_ReadMsr(PAWNIO_CTX* ctx, DWORD msr, ULONGLONG* pValue)
{
    if (!ctx || !ctx->fn_execute || !ctx->bBlobLoaded || !pValue)
        return FALSE;

    ULONG64 in[1] = { msr };
    ULONG64 out[1] = { 0 };
    SIZE_T returned = 0;
    HRESULT hr = ctx->fn_execute(ctx->hDevice, "ioctl_read_msr", in, 1, out, 1, &returned);
    if (SUCCEEDED(hr) && returned >= 1) {
        *pValue = out[0];
        return TRUE;
    }
    return FALSE;
}

BOOL PawnIo_Cpuid(PAWNIO_CTX* ctx, DWORD leaf, DWORD subleaf,
                  DWORD* pEax, DWORD* pEbx, DWORD* pEcx, DWORD* pEdx)
{
    if (!ctx || !ctx->fn_execute || !ctx->bBlobLoaded)
        return FALSE;

    ULONG64 in[2] = { leaf, subleaf };
    ULONG64 out[4] = { 0 };
    SIZE_T returned = 0;
    HRESULT hr = ctx->fn_execute(ctx->hDevice, "ioctl_read_msr", in, 2, out, 4, &returned);
    if (SUCCEEDED(hr) && returned >= 4) {
        if (pEax) *pEax = (DWORD)out[0];
        if (pEbx) *pEbx = (DWORD)out[1];
        if (pEcx) *pEcx = (DWORD)out[2];
        if (pEdx) *pEdx = (DWORD)out[3];
        return TRUE;
    }
    return FALSE;
}

int PawnIo_GetCpuTemp(PAWNIO_CTX* ctx, DWORD core)
{
    if (!ctx) return 0;

    if (ctx->bIntel) {
        ULONGLONG msrVal = 0;
        int Tjunction = 100;
        if (PawnIo_ReadMsr(ctx, 0x1A2, &msrVal)) {
            int tjMax = (int)((msrVal >> 16) & 0xFF);
            if (tjMax > 0 && tjMax < 150)
                Tjunction = tjMax;
        }
        if (PawnIo_ReadMsr(ctx, 0x19C, &msrVal)) {
            if (msrVal & 0x80000000) {  // reading valid
                int digitalReadout = (int)((msrVal >> 16) & 0x7F);
                return Tjunction - digitalReadout;
            }
        }
    }
    return 0;
}

BOOL PawnIo_IsIntel(PAWNIO_CTX* ctx)
{
    return ctx ? ctx->bIntel : FALSE;
}