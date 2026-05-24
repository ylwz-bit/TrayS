#include "PawnIo.h"
#include <intrin.h>

// min/max (不依赖CRT)
#ifndef min
#define min(a,b) (((a)<(b))?(a):(b))
#endif

// 资源ID定义 (在resource.h中添加)
#define IDR_INTEL_MSR    200
#define IDR_AMD_FAMILY0F 201
#define IDR_AMD_FAMILY10 202
#define IDR_AMD_FAMILY17 203

// 执行PawnIO函数
static BOOL PawnIo_Execute(HANDLE hDevice, const char* fnName, long long* input, int inputCount, long long* output, int outputCount)
{
	if (hDevice == INVALID_HANDLE_VALUE)
		return FALSE;

	// 构造输入缓冲区: 32字节函数名 + 参数
	DWORD inputSize = PIO_FN_NAME_LENGTH + inputCount * sizeof(long long);
	DWORD outputSize = outputCount * sizeof(long long);
	char* inputBuf = (char*)HeapAlloc(GetProcessHeap(), 0, inputSize);
	long long* outputBuf = (long long*)HeapAlloc(GetProcessHeap(), 0, outputSize);
	if (!inputBuf || !outputBuf)
	{
		if (inputBuf) HeapFree(GetProcessHeap(), 0, inputBuf);
		if (outputBuf) HeapFree(GetProcessHeap(), 0, outputBuf);
		return FALSE;
	}

	memset(inputBuf, 0, inputSize);
	int nameLen = 0;
	while (fnName[nameLen]) nameLen++;
	if (nameLen > PIO_FN_NAME_LENGTH - 1) nameLen = PIO_FN_NAME_LENGTH - 1;
	memcpy(inputBuf, fnName, nameLen);
	memcpy(inputBuf + PIO_FN_NAME_LENGTH, input, inputCount * sizeof(long long));

	DWORD bytesReturned = 0;
	BOOL result = DeviceIoControl(hDevice, IOCTL_PIO_EXECUTE_FN,
		inputBuf, inputSize,
		outputBuf, outputSize,
		&bytesReturned, NULL);

	if (result && output && outputCount > 0)
		memcpy(output, outputBuf, min(bytesReturned, outputCount * sizeof(long long)));

	HeapFree(GetProcessHeap(), 0, inputBuf);
	HeapFree(GetProcessHeap(), 0, outputBuf);
	return result;
}

// 加载嵌入资源的bin模块
static BOOL PawnIo_LoadBinFromResource(HANDLE hDevice, int resourceId)
{
	HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(resourceId), RT_RCDATA);
	if (!hRes)
		return FALSE;

	HGLOBAL hGlobal = LoadResource(NULL, hRes);
	if (!hGlobal)
		return FALSE;

	LPVOID pData = LockResource(hGlobal);
	DWORD dataSize = SizeofResource(NULL, hRes);
	if (!pData || dataSize == 0)
		return FALSE;

	DWORD bytesReturned = 0;
	BOOL result = DeviceIoControl(hDevice, IOCTL_PIO_LOAD_BINARY,
		pData, dataSize,
		NULL, 0,
		&bytesReturned, NULL);

	UnlockResource(hGlobal);
	FreeResource(hGlobal);
	return result;
}

int PawnIo_IsInstalled(void)
{
	HKEY hKey = NULL;
	int installed = 0;

	// 先检查64位注册表视图
	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
		L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\PawnIO",
		0, KEY_READ | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS)
	{
		installed = 1;
		RegCloseKey(hKey);
	}
	// 再检查32位视图 (WOW6432Node)
	else if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
		L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\PawnIO",
		0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		installed = 1;
		RegCloseKey(hKey);
	}

	// 额外检查: 设备路径是否可访问 (防止驱动已卸载但注册表残留)
	if (installed)
	{
		HANDLE hTest = CreateFileW(PIO_DEVICE_PATH,
			GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hTest == INVALID_HANDLE_VALUE)
			installed = 0;
		else
			CloseHandle(hTest);
	}

	return installed;
}

int PawnIo_Init(PIORUNTIME* pRuntime)
{
	if (!pRuntime)
		return FALSE;

	pRuntime->hDevice = INVALID_HANDLE_VALUE;
	pRuntime->bIntel = FALSE;
	pRuntime->bLoaded = FALSE;

	// 检测CPU厂商
	int cpuInfo[4] = {};
	__cpuid(cpuInfo, 0);
	pRuntime->bIntel = TRUE;
	if (cpuInfo[1] == 0x68747541) // "Auth" in "AuthenticAMD"
		pRuntime->bIntel = FALSE;

	// 打开PawnIO设备
	pRuntime->hDevice = CreateFileW(PIO_DEVICE_PATH,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (pRuntime->hDevice == INVALID_HANDLE_VALUE)
		return FALSE;

	// 加载对应的bin模块
	BOOL loadOk = FALSE;
	if (pRuntime->bIntel)
	{
		loadOk = PawnIo_LoadBinFromResource(pRuntime->hDevice, IDR_INTEL_MSR);
	}
	else
	{
		// AMD: 检测family决定加载哪个模块
		int cpuInfo2[4] = {};
		__cpuid(cpuInfo2, 1);
		int family = ((cpuInfo2[0] >> 20) & 0xFF) + ((cpuInfo2[0] >> 8) & 0xF);

		if (family >= 0x17) // Family 17h+ (Zen, Zen2, Zen3, Zen4, Zen5)
		{
			loadOk = PawnIo_LoadBinFromResource(pRuntime->hDevice, IDR_AMD_FAMILY17);
		}
		else if (family > 0x0F) // Family 10h-16h
		{
			loadOk = PawnIo_LoadBinFromResource(pRuntime->hDevice, IDR_AMD_FAMILY10);
		}
		else // Family 0Fh (旧AMD)
		{
			loadOk = PawnIo_LoadBinFromResource(pRuntime->hDevice, IDR_AMD_FAMILY0F);
		}
	}

	if (!loadOk)
	{
		CloseHandle(pRuntime->hDevice);
		pRuntime->hDevice = INVALID_HANDLE_VALUE;
		return FALSE;
	}

	pRuntime->bLoaded = TRUE;
	return TRUE;
}

void PawnIo_Free(PIORUNTIME* pRuntime)
{
	if (pRuntime && pRuntime->hDevice != INVALID_HANDLE_VALUE)
	{
		CloseHandle(pRuntime->hDevice);
		pRuntime->hDevice = INVALID_HANDLE_VALUE;
		pRuntime->bLoaded = FALSE;
	}
}

BOOL PawnIo_ReadMsr(PIORUNTIME* pRuntime, DWORD index, DWORD* pEax, DWORD* pEdx)
{
	if (!pRuntime || !pRuntime->bLoaded)
		return FALSE;

	long long input[1] = { (long long)index };
	long long output[1] = {};

	if (!PawnIo_Execute(pRuntime->hDevice, "ioctl_read_msr", input, 1, output, 1))
		return FALSE;

	if (pEax) *pEax = (DWORD)(output[0] & 0xFFFFFFFF);
	if (pEdx) *pEdx = (DWORD)(output[0] >> 32);
	return TRUE;
}

BOOL PawnIo_ReadSmn(PIORUNTIME* pRuntime, DWORD offset, DWORD* pValue)
{
	if (!pRuntime || !pRuntime->bLoaded)
		return FALSE;

	long long input[1] = { (long long)offset };
	long long output[1] = {};

	if (!PawnIo_Execute(pRuntime->hDevice, "ioctl_read_smn", input, 1, output, 1))
		return FALSE;

	if (pValue) *pValue = (DWORD)(output[0] & 0xFFFFFFFF);
	return TRUE;
}

int PawnIo_GetCpuTemp(PIORUNTIME* pRuntime, DWORD Core)
{
	if (!pRuntime || !pRuntime->bLoaded)
		return 0;

	SetThreadAffinityMask(GetCurrentThread(), Core);

	if (pRuntime->bIntel)
	{
		// Intel: 读MSR获取TjMax和热读数
		DWORD eax = 0, edx = 0;
		int Tjunction = 100;

		// MSR 0x1A2: IA32_TEMPERATURE_TARGET
		if (PawnIo_ReadMsr(pRuntime, 0x1A2, &eax, &edx))
		{
			if (eax & 0x20000000)
				Tjunction = 85;
		}

		// MSR 0x19C: IA32_THERM_STATUS
		if (PawnIo_ReadMsr(pRuntime, 0x19C, &eax, &edx))
		{
			DWORD IAcore = (eax & 0xFF0000) >> 16;
			return Tjunction - IAcore;
		}
	}
	else
	{
		// AMD: 检测family
		int cpuInfo[4] = {};
		__cpuid(cpuInfo, 1);
		int family = ((cpuInfo[0] >> 20) & 0xFF) + ((cpuInfo[0] >> 8) & 0xF);

		if (family >= 0x17)
		{
			// AMD Family 17h+ (Zen系列): 通过SMN读取温度
			// SMN 0x00059800: THM_TCON_CUR_TMP
			DWORD smnValue = 0;
			if (PawnIo_ReadSmn(pRuntime, 0x00059800, &smnValue))
			{
				return (int)((smnValue >> 21) & 0x7F);
			}
		}
		else if (family > 0x0F)
		{
			// AMD Family 10h-16h: 通过MSR或MiscCtl读取
			DWORD eax = 0, edx = 0;
			if (PawnIo_ReadMsr(pRuntime, 0x000000E8, &eax, &edx)) // PStateDef可能包含温度
			{
				// 备选: 尝试MiscCtl读取
				long long input[2] = { 0, 0xA4 }; // cpu=0, offset=0xa4
				long long output[1] = {};
				if (PawnIo_Execute(pRuntime->hDevice, "ioctl_read_miscctl", input, 2, output, 1))
				{
					DWORD miscReg = (DWORD)(output[0] & 0xFFFFFFFF);
					return (int)((miscReg >> 21) >> 3);
				}
			}
		}
		else
		{
			// AMD Family 0Fh: 通过thermtrip读取
			long long input[2] = { 0, 0 }; // cpuIndex=0, coreIndex=0
			long long output[1] = {};
			if (PawnIo_Execute(pRuntime->hDevice, "ioctl_get_thermtrip", input, 2, output, 1))
			{
				DWORD miscReg = (DWORD)(output[0] & 0xFFFFFFFF);
				return (int)(((miscReg & 0xFF0000) >> 16) - 49);
			}
		}
	}

	return 0;
}
