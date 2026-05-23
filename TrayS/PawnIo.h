#pragma once
#include <windows.h>

#define PIO_DEVICE_PATH L"\\\\?\\GLOBALROOT\\Device\\PawnIO"
#define PIO_DEVICE_TYPE 41394u
#define IOCTL_PIO_LOAD_BINARY ((PIO_DEVICE_TYPE << 16) | (0x821 << 2))
#define IOCTL_PIO_EXECUTE_FN ((PIO_DEVICE_TYPE << 16) | (0x841 << 2))
#define PIO_FN_NAME_LENGTH 32

typedef struct _PIORUNTIME {
	void* hDevice;
	int bIntel;
	int bLoaded;
} PIORUNTIME, *PPIORUNTIME;

int PawnIo_Init(PIORUNTIME* pRuntime);
void PawnIo_Free(PIORUNTIME* pRuntime);
int PawnIo_IsInstalled(void);
BOOL PawnIo_ReadMsr(PIORUNTIME* pRuntime, DWORD index, DWORD* pEax, DWORD* pEdx);
BOOL PawnIo_ReadSmn(PIORUNTIME* pRuntime, DWORD offset, DWORD* pValue);
int PawnIo_GetCpuTemp(PIORUNTIME* pRuntime, DWORD Core);
