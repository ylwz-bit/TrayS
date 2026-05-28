#pragma once
// Win11 Taskbar Appearance Provider - integration with ExplorerTAP.dll
// Uses TranslucentTB's injection mechanism to achieve taskbar transparency on Win11 22H2+

#include <windows.h>
#include <objbase.h>

// TaskbarBrush enum matching ExplorerTAP's IDL definition
enum TaskbarBrush : UINT
{
	TaskbarBrush_Acrylic = 0,
	TaskbarBrush_SolidColor = 1
};

// {5bcf9150-c28a-4ef2-913c-4c3ea2f5ead0}
static const IID IID_ITaskbarAppearanceService =
	{ 0x5bcf9150, 0xc28a, 0x4ef2, { 0x91, 0x3c, 0x4c, 0x3e, 0xa2, 0xf5, 0xea, 0xd0 } };

// {50e9ab23-97b4-4dba-8f44-5cd342f30b78}
static const CLSID CLSID_TaskbarAppearanceService =
	{ 0x50e9ab23, 0x97b4, 0x4dba, { 0x8f, 0x44, 0x5c, 0xd3, 0x42, 0xf3, 0x0b, 0x78 } };

// ITaskbarAppearanceService interface (must match ExplorerTAP IDL exactly)
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

// Function pointer type for InjectExplorerTAP exported from ExplorerTAP.dll
typedef HRESULT(WINAPI* PFN_INJECT_EXPLORER_TAP)(HWND window, REFIID riid, LPVOID* ppv);

// Singleton to manage Win11 TAP connection
class Win11TaskbarManager
{
public:
	static Win11TaskbarManager& Instance()
	{
		static Win11TaskbarManager instance;
		return instance;
	}

	// Initialize the TAP - call once when Win11 taskbar is detected
	// Returns TRUE if successful
	BOOL Initialize(HWND hTaskbar)
	{
		if (m_pService)
			return TRUE;

		if (m_bFailed)
			return FALSE;

		__try
		{
			return InitializeInternal(hTaskbar);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			m_bFailed = TRUE;
			if (m_pService) { m_pService->Release(); m_pService = nullptr; }
			if (m_hTAPDll) { FreeLibrary(m_hTAPDll); m_hTAPDll = nullptr; }
			return FALSE;
		}
	}

	// Set taskbar to transparent
	BOOL SetTransparent(HWND hTaskbar, UINT color = 0)
	{
		if (!m_pService && !Initialize(hTaskbar))
			return FALSE;
		if (!m_pService)
			return FALSE;
		return SUCCEEDED(m_pService->SetTaskbarAppearance(hTaskbar, TaskbarBrush_SolidColor, color));
	}

	// Set taskbar to acrylic
	BOOL SetAcrylic(HWND hTaskbar, UINT color)
	{
		if (!m_pService && !Initialize(hTaskbar))
			return FALSE;
		if (!m_pService)
			return FALSE;
		return SUCCEEDED(m_pService->SetTaskbarAppearance(hTaskbar, TaskbarBrush_Acrylic, color));
	}

	// Restore default appearance
	BOOL RestoreDefault(HWND hTaskbar)
	{
		if (!m_pService)
			return TRUE;
		return SUCCEEDED(m_pService->ReturnTaskbarToDefaultAppearance(hTaskbar));
	}

	BOOL IsAvailable() const { return m_pService != nullptr; }
	BOOL HasFailed() const { return m_bFailed; }

	~Win11TaskbarManager()
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

private:
	Win11TaskbarManager() = default;
	Win11TaskbarManager(const Win11TaskbarManager&) = delete;
	Win11TaskbarManager& operator=(const Win11TaskbarManager&) = delete;

	BOOL InitializeInternal(HWND hTaskbar)
	{
		// Load ExplorerTAP.dll from the same directory as TrayS.exe
		if (!m_hTAPDll)
		{
			WCHAR szPath[MAX_PATH] = {};
			GetModuleFileName(NULL, szPath, MAX_PATH);
			WCHAR* pSlash = NULL; for (WCHAR* p = szPath; *p; p++) { if (*p == L'\\') pSlash = p; }
			if (pSlash)
			{
				wcscpy_s(pSlash + 1, MAX_PATH - (pSlash + 1 - szPath), L"ExplorerTAP.dll");
				m_hTAPDll = LoadLibrary(szPath);
			}
			if (!m_hTAPDll)
			{
				m_hTAPDll = LoadLibrary(L"ExplorerTAP.dll");
			}
			if (!m_hTAPDll)
			{
				m_bFailed = TRUE;
				return FALSE;
			}
		}

		PFN_INJECT_EXPLORER_TAP pfnInject = (PFN_INJECT_EXPLORER_TAP)GetProcAddress(m_hTAPDll, "InjectExplorerTAP");
		if (!pfnInject)
		{
			m_bFailed = TRUE;
			return FALSE;
		}

		IUnknown* pUnk = nullptr;
		HRESULT hr = pfnInject(hTaskbar, IID_ITaskbarAppearanceService, (LPVOID*)&pUnk);
		if (SUCCEEDED(hr) && pUnk)
		{
			pUnk->QueryInterface(IID_ITaskbarAppearanceService, (LPVOID*)&m_pService);
			pUnk->Release();
			if (m_pService)
				return TRUE;
		}

		m_bFailed = TRUE;
		return FALSE;
	}

	HMODULE m_hTAPDll = nullptr;
	ITaskbarAppearanceService* m_pService = nullptr;
	BOOL m_bFailed = FALSE;
};


