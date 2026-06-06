#pragma once
// Win11 任务栏外观提供器 - 集成 ExplorerTAP.dll
// 使用 TranslucentTB 的注入机制实现 Win11 22H2+ 任务栏透明

#include <windows.h>
#include <objbase.h>

#ifdef _DEBUG  // 调试版：写入日志文件，带时间戳
static void TAPLog(const WCHAR* msg)
{
	WCHAR szPath[MAX_PATH] = {};
	GetModuleFileName(NULL, szPath, MAX_PATH);
	WCHAR* pSlash = NULL;
	for (WCHAR* p = szPath; *p; p++) { if (*p == L'\\') pSlash = p; }
	if (pSlash)
		wcscpy_s(pSlash + 1, MAX_PATH - (pSlash + 1 - szPath), L"TrayS_TAP.log");
	else
		lstrcpyW(szPath, L"TrayS_TAP.log");
	HANDLE hLog = CreateFileW(szPath, GENERIC_WRITE, FILE_SHARE_READ,
		NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hLog != INVALID_HANDLE_VALUE)
	{
		SetFilePointer(hLog, 0, NULL, FILE_END);
		DWORD bw = 0;
		// 写入时间戳
		SYSTEMTIME st;
		GetLocalTime(&st);
		WCHAR ts[64];
		int len = wsprintfW(ts, L"[%02d:%02d:%02d.%03d] ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
		WriteFile(hLog, ts, (DWORD)(len * sizeof(WCHAR)), &bw, NULL);
		WriteFile(hLog, msg, (DWORD)(lstrlenW(msg) * sizeof(WCHAR)), &bw, NULL);
		CloseHandle(hLog);
	}
}
#else  // 发布版：空函数，零开销
static void TAPLog(const WCHAR*) {}
#endif

// 任务栏画刷枚举（对应 ExplorerTAP IDL）
enum TaskbarBrush : UINT
{
	TaskbarBrush_Acrylic = 0,
	TaskbarBrush_SolidColor = 1
};

static const IID IID_ITaskbarAppearanceService =
	{ 0x5bcf9150, 0xc28a, 0x4ef2, { 0x91, 0x3c, 0x4c, 0x3e, 0xa2, 0xf5, 0xea, 0xd0 } };

static const CLSID CLSID_TaskbarAppearanceService =
	{ 0x50e9ab23, 0x97b4, 0x4dba, { 0x8f, 0x44, 0x5c, 0xd3, 0x42, 0xf3, 0x0b, 0x78 } };

MIDL_INTERFACE("5bcf9150-c28a-4ef2-913c-4c3ea2f5ead0")
ITaskbarAppearanceService : public IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE SetTaskbarAppearance(HWND taskbar, TaskbarBrush brush, UINT color) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetTaskbarBlur(HWND taskbar, UINT color, FLOAT blurAmount) = 0;
	virtual HRESULT STDMETHODCALLTYPE ReturnTaskbarToDefaultAppearance(HWND taskbar) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetTaskbarBorderVisibility(HWND taskbar, BOOL visible) = 0;
	virtual HRESULT STDMETHODCALLTYPE RestoreAllTaskbarsToDefault() = 0;
	virtual HRESULT STDMETHODCALLTYPE RestoreAllTaskbarsToDefaultWhenProcessDies(DWORD pid) = 0;
	virtual HRESULT STDMETHODCALLTYPE KillExplorerWhenPackageUninstalls(LPCWSTR packageFullName) = 0;
};

typedef HRESULT(WINAPI* PFN_INJECT_EXPLORER_TAP)(HWND window, REFIID riid, LPVOID* ppv);

class Win11TaskbarManager
{
public:
	static Win11TaskbarManager& Instance()
	{
		static Win11TaskbarManager instance;
		return instance;
	}

	BOOL Initialize(HWND hTaskbar)
	{
		if (m_pService)
		{
			TAPLog(L"[TrayS] TAP Init: m_pService exists, OK\n");
			return TRUE;
		}
		if (m_bFailed)
		{
			TAPLog(L"[TrayS] TAP Init: m_bFailed, skip\n");
			return FALSE;
		}
		TAPLog(L"[TrayS] TAP Init: calling InitializeInternal...\n");
		__try
		{
			BOOL ret = InitializeInternal(hTaskbar);
			WCHAR dbg[128];
			wsprintfW(dbg, L"[TrayS] TAP Init: result=%d m_pService=%p\n", ret, m_pService);
			TAPLog(dbg);
			return ret;
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			TAPLog(L"[TrayS] TAP Init: EXCEPTION!\n");
			m_bFailed = TRUE;
			ReleaseResources();
			return FALSE;
		}
	}

	BOOL SetTransparent(HWND hTaskbar, UINT color = 0)
	{
		if (!m_pService && !Initialize(hTaskbar))
			return FALSE;
		if (!m_pService)
			return FALSE;
		return SUCCEEDED(m_pService->SetTaskbarAppearance(hTaskbar, TaskbarBrush_SolidColor, color));
	}

	BOOL SetAcrylic(HWND hTaskbar, UINT color)
	{
		if (!m_pService && !Initialize(hTaskbar))
			return FALSE;
		if (!m_pService)
			return FALSE;
		return SUCCEEDED(m_pService->SetTaskbarAppearance(hTaskbar, TaskbarBrush_Acrylic, color));
	}

	BOOL RestoreDefault(HWND hTaskbar)
	{
		if (!m_pService)
			return TRUE;
		return SUCCEEDED(m_pService->ReturnTaskbarToDefaultAppearance(hTaskbar));
	}

	BOOL RestoreAll()
	{
		if (!m_pService)
			return TRUE;
		return SUCCEEDED(m_pService->RestoreAllTaskbarsToDefault());
	}

	BOOL IsAvailable() const { return m_pService != nullptr; }
	BOOL HasFailed() const { return m_bFailed; }

	void Reset()
	{
		TAPLog(L"[TrayS] TAP: Reset() called - releasing resources\n");
		ReleaseResources();
		m_bFailed = FALSE;
	}

	~Win11TaskbarManager()
	{
		ReleaseResources();
	}

private:
	Win11TaskbarManager() = default;
	Win11TaskbarManager(const Win11TaskbarManager&) = delete;
	Win11TaskbarManager& operator=(const Win11TaskbarManager&) = delete;

	void ReleaseResources()
	{
		if (m_pService)
		{
			m_pService->Release();
			m_pService = nullptr;
		}
		if (m_hTAPDll)
		{
			FreeLibrary(m_hTAPDll);
			m_hTAPDll = nullptr;
		}
	}

	BOOL InitializeInternal(HWND hTaskbar)
	{
		WCHAR dbg[512];
		if (!m_hTAPDll)
		{
			WCHAR szPath[MAX_PATH] = {};
			GetModuleFileName(NULL, szPath, MAX_PATH);
			WCHAR* pSlash = NULL;
			for (WCHAR* p = szPath; *p; p++) { if (*p == L'\\') pSlash = p; }
			if (pSlash)
			{
				wcscpy_s(pSlash + 1, MAX_PATH - (pSlash + 1 - szPath), L"ExplorerTAP.dll");
				m_hTAPDll = LoadLibrary(szPath);
				wsprintfW(dbg, L"[TrayS] TAP: LoadLibrary(%s)=%p\n", szPath, m_hTAPDll);
				TAPLog(dbg);
			}
			if (!m_hTAPDll)
			{
				m_hTAPDll = LoadLibrary(L"ExplorerTAP.dll");
				wsprintfW(dbg, L"[TrayS] TAP: LoadLib fallback=%p\n", m_hTAPDll);
				TAPLog(dbg);
			}
			if (!m_hTAPDll)
			{
				wsprintfW(dbg, L"[TrayS] TAP: LoadLib FAILED err=%d\n", GetLastError());
				TAPLog(dbg);
				m_bFailed = TRUE;
				return FALSE;
			}
		}

		PFN_INJECT_EXPLORER_TAP pfnInject = (PFN_INJECT_EXPLORER_TAP)GetProcAddress(m_hTAPDll, "InjectExplorerTAP");
		if (!pfnInject)
		{
			TAPLog(L"[TrayS] TAP: GetProcAddr FAILED\n");
			m_bFailed = TRUE;
			return FALSE;
		}

		// 确保 COM 已初始化（Reset 中 FreeLibrary 可能已清理）
		BOOL bComInit = FALSE;
		HRESULT hrCom = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
		if (hrCom == S_OK || hrCom == S_FALSE)
			bComInit = TRUE;

		TAPLog(L"[TrayS] TAP: InjectExplorerTAP calling...\n");
		IUnknown* pUnk = nullptr;
		HRESULT hr = pfnInject(hTaskbar, IID_ITaskbarAppearanceService, (LPVOID*)&pUnk);
		wsprintfW(dbg, L"[TrayS] TAP: Inject hr=0x%08X pUnk=%p\n", hr, pUnk);
		TAPLog(dbg);
		if (SUCCEEDED(hr) && pUnk)
		{
			pUnk->QueryInterface(IID_ITaskbarAppearanceService, (LPVOID*)&m_pService);
			pUnk->Release();
			wsprintfW(dbg, L"[TrayS] TAP: QI m_pService=%p\n", m_pService);
			TAPLog(dbg);
			if (m_pService)
				return TRUE;
		}

		// 注入失败 - 清理 COM
		if (bComInit)
			CoUninitialize();
		m_bFailed = TRUE;
		return FALSE;
	}

	HMODULE m_hTAPDll = nullptr;
	ITaskbarAppearanceService* m_pService = nullptr;
	BOOL m_bFailed = FALSE;
};
