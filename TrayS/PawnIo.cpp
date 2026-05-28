#include "PawnIo.h"
#include <intrin.h>
#include <wchar.h>

// min/max (涓嶄緷璧朇RT)
#ifndef min
#define min(a,b) (((a)<(b))?(a):(b))
#endif

// 璧勬簮ID瀹氫箟 (鍦╮esource.h涓坊鍔?
#define IDR_INTEL_MSR    200
#define IDR_AMD_FAMILY0F 201
#define IDR_AMD_FAMILY10 202
#define IDR_AMD_FAMILY17 203

// 鎵цPawnIO鍑芥暟
static BOOL PawnIo_Execute(HANDLE hDevice, const char* fnName, long long* input, int inputCount, long long* output, int outputCount)
{
	if (hDevice == INVALID_HANDLE_VALUE)
		return FALSE;

	// 鏋勯€犺緭鍏ョ紦鍐插尯: 32瀛楄妭鍑芥暟鍚?+ 鍙傛暟
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
	if (input && inputCount > 0)
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

// 鍔犺浇宓屽叆璧勬簮鐨刡in妯″潡
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

	// 鍏堟鏌?4浣嶆敞鍐岃〃瑙嗗浘
	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
		L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\PawnIO",
		0, KEY_READ | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS)
	{
		installed = 1;
		RegCloseKey(hKey);
	}
	// 鍐嶆鏌?2浣嶈鍥?(WOW6432Node)
	else if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
		L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\PawnIO",
		0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		installed = 1;
		RegCloseKey(hKey);
	}

	// 棰濆妫€鏌? 璁惧璺緞鏄惁鍙闂?(闃叉椹卞姩宸插嵏杞戒絾娉ㄥ唽琛ㄦ畫鐣?
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

	// 妫€娴婥PU鍘傚晢
	int cpuInfo[4] = {};
	__cpuid(cpuInfo, 0);
	pRuntime->bIntel = TRUE;
	if (cpuInfo[1] == 0x68747541) // "Auth" in "AuthenticAMD"
		pRuntime->bIntel = FALSE;

	// 鎵撳紑PawnIO璁惧
	pRuntime->hDevice = CreateFileW(PIO_DEVICE_PATH,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (pRuntime->hDevice == INVALID_HANDLE_VALUE)
		return FALSE;

	// 鍔犺浇瀵瑰簲鐨刡in妯″潡
	BOOL loadOk = FALSE;
	if (pRuntime->bIntel)
	{
		loadOk = PawnIo_LoadBinFromResource(pRuntime->hDevice, IDR_INTEL_MSR);
	}
	else
	{
		// AMD: 妫€娴媐amily鍐冲畾鍔犺浇鍝釜妯″潡
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
		else // Family 0Fh (鏃MD)
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
	DWORD eax, edx, eax1a2, edx1a2;
	int Tjunction, tjMax, deltaT;
#ifdef _DEBUG
	WCHAR dbg[128];
#endif

	if (!pRuntime || !pRuntime->bLoaded)
		return 0;

	SetThreadAffinityMask(GetCurrentThread(), Core);

	if (pRuntime->bIntel)
	{
		eax = edx = 0;
		Tjunction = 100;

		// MSR 0x1A2: IA32_TEMPERATURE_TARGET
		// TjMax 鍦?bits 23:16 (Intel SDM)
		// TCC Activation Offset 鍦?bits 29:24
		eax1a2 = edx1a2 = 0;
		if (PawnIo_ReadMsr(pRuntime, 0x1A2, &eax1a2, &edx1a2))
		{
			tjMax = (eax1a2 >> 16) & 0xFF;
			if (tjMax > 0 && tjMax <= 150)
				Tjunction = tjMax;
		}
#ifdef _DEBUG
		else
		{
			OutputDebugStringW(L"[TDBG] ReadMsr 0x1A2 FAILED\n");
		}
#endif

		// MSR 0x19C: IA32_THERM_STATUS
		// bit 31: Digital Readout Valid (蹇呴』涓?)
		// bits 22:16: Digital Readout (7-bit distance to TjMax)
		if (PawnIo_ReadMsr(pRuntime, 0x19C, &eax, &edx))
		{
			if (eax & 0x80000000) // Digital Readout Valid
			{
				int tccOffset;
				int rawTemp;

				deltaT = (eax & 0x007F0000) >> 16;
				tccOffset = (eax1a2 >> 24) & 0x3F; // bits 29:24
				rawTemp = Tjunction - deltaT - tccOffset * 2;
#ifdef _DEBUG
				swprintf_s(dbg, ARRAYSIZE(dbg), L"[TDBG] Core=%d 0x1A2=%08X TjMax=%d TCC=%d 0x19C=%08X dT=%d temp=%d\n",
					Core, eax1a2, Tjunction, tccOffset, eax, deltaT, rawTemp);
				OutputDebugStringW(dbg);
#endif
				return rawTemp;
			}
#ifdef _DEBUG
			else
			{
				swprintf_s(dbg, ARRAYSIZE(dbg), L"[TDBG] 0x19C=%08X bit31=0 INVALID\n", eax);
				OutputDebugStringW(dbg);
			}
#endif
		}
#ifdef _DEBUG
		else
		{
			OutputDebugStringW(L"[TDBG] ReadMsr 0x19C FAILED\n");
		}
#endif
	}
	else
	{
		int cpuInfo[4] = {};
		int family, temp;
		BOOL tempOffsetFlag;
		DWORD smnValue;

		// AMD: 妫€娴媐amily
		__cpuid(cpuInfo, 1);
		family = ((cpuInfo[0] >> 20) & 0xFF) + ((cpuInfo[0] >> 8) & 0xF);

		if (family >= 0x17)
		{
			// AMD Family 17h+ (Zen绯诲垪): 閫氳繃SMN璇诲彇娓╁害
			// SMN 0x00059800: THM_TCON_CUR_TMP
			smnValue = 0;
			if (PawnIo_ReadSmn(pRuntime, 0x00059800, &smnValue))
			{
				// bits 31:21: 娓╁害鍊?(1/128 掳C)
				temp = (int)((smnValue >> 21) & 0x7F);

				// 妫€鏌ユ槸鍚﹂渶瑕?-49 鍋忕Щ (鏌愪簺 Ryzen 鏃у瀷鍙?
				// RANGE_SEL (bit 19) 鎴?TJ_SEL (bits 17:16 閮戒负1)
				tempOffsetFlag = ((smnValue & 0x80000) != 0) ||
				                      ((smnValue & 0x30000) == 0x30000);
				if (tempOffsetFlag)
					temp -= 49;

				return temp;
			}
		}
		else if (family > 0x0F)
		{
			// AMD Family 10h-16h: 閫氳繃MiscCtl璇诲彇娓╁害
			long long input2[2] = { 0, 0xA4 };
			long long output2[1] = {};
			if (PawnIo_Execute(pRuntime->hDevice, "ioctl_read_miscctl", input2, 2, output2, 1))
			{
				DWORD miscReg = (DWORD)(output2[0] & 0xFFFFFFFF);
				return (int)(miscReg >> 21);
			}
			// 澶囬€? MSR 0xE8
			eax = edx = 0;
			if (PawnIo_ReadMsr(pRuntime, 0x000000E8, &eax, &edx))
			{
				return (int)((eax >> 21) & 0x7F);
			}
		}
		else
		{
			// AMD Family 0Fh: 閫氳繃thermtrip璇诲彇
			long long input2[2] = { 0, 0 };
			long long output2[1] = {};
			if (PawnIo_Execute(pRuntime->hDevice, "ioctl_get_thermtrip", input2, 2, output2, 1))
			{
				DWORD miscReg = (DWORD)(output2[0] & 0xFFFFFFFF);
				return (int)((miscReg >> 16) & 0x7F) - 49;
			}
		}
	}

	return 0;
}

