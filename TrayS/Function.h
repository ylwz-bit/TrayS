#include <windows.h>
#include <wchar.h>
#include <shellapi.h>
#include <Psapi.h>
#include <Shlobj.h>
#include <tlhelp32.h>
#include <commctrl.h>
#include <strsafe.h>

//#include "Winhttp.h"
//#pragma comment(lib,"winhttp.lib")

//GDI+
/*
#include<gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;
*/

const WCHAR lpServiceName[] = L"TrayS";//锟斤拷锟斤拷锟斤拷
const WCHAR szShellTray[] = L"Shell_TrayWnd";//锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷
const WCHAR szSecondaryTray[] = L"Shell_SecondaryTrayWnd";//锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷
typedef enum _WINDOWCOMPOSITIONATTRIB
{
	WCA_UNDEFINED = 0,
	WCA_NCRENDERING_ENABLED = 1,
	WCA_NCRENDERING_POLICY = 2,
	WCA_TRANSITIONS_FORCEDISABLED = 3,
	WCA_ALLOW_NCPAINT = 4,
	WCA_CAPTION_BUTTON_BOUNDS = 5,
	WCA_NONCLIENT_RTL_LAYOUT = 6,
	WCA_FORCE_ICONIC_REPRESENTATION = 7,
	WCA_EXTENDED_FRAME_BOUNDS = 8,
	WCA_HAS_ICONIC_BITMAP = 9,
	WCA_THEME_ATTRIBUTES = 10,
	WCA_NCRENDERING_EXILED = 11,
	WCA_NCADORNMENTINFO = 12,
	WCA_EXCLUDED_FROM_LIVEPREVIEW = 13,
	WCA_VIDEO_OVERLAY_ACTIVE = 14,
	WCA_FORCE_ACTIVEWINDOW_APPEARANCE = 15,
	WCA_DISALLOW_PEEK = 16,
	WCA_CLOAK = 17,
	WCA_CLOAKED = 18,
	WCA_ACCENT_POLICY = 19,
	WCA_FREEZE_REPRESENTATION = 20,
	WCA_EVER_UNCLOAKED = 21,
	WCA_VISUAL_OWNER = 22,
	WCA_LAST = 23
} WINDOWCOMPOSITIONATTRIB;
typedef struct _WINDOWCOMPOSITIONATTRIBDATA
{
	WINDOWCOMPOSITIONATTRIB Attrib;
	PVOID pvData;
	SIZE_T cbData;
} WINDOWCOMPOSITIONATTRIBDATA;
typedef enum _ACCENT_STATE
{
	ACCENT_DISABLED = 0,
	ACCENT_ENABLE_GRADIENT = 1,
	ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
	ACCENT_ENABLE_BLURBEHIND = 3,
	ACCENT_ENABLE_ACRYLICBLURBEHIND = 4,
	ACCENT_INVALID_STATE = 5,
	ACCENT_ENABLE_TRANSPARENT = 6,
	ACCENT_7 = 7,
	ACCENT_8 = 8,
	ACCENT_9 = 9,
	ACCENT_10 = 10,
	ACCENT_NORMAL = 150
} ACCENT_STATE;
typedef struct _ACCENT_POLICY
{
	ACCENT_STATE AccentState;
	DWORD AccentFlags;
	DWORD GradientColor;
	DWORD AnimationId;
} ACCENT_POLICY;
typedef BOOL(WINAPI* pfnSetWindowCompositionAttribute)(HWND, struct _WINDOWCOMPOSITIONATTRIBDATA*);
void		SetToCurrentPath();//锟斤拷锟矫斤拷锟斤拷路锟斤拷为锟斤拷前路锟斤拷
void		EmptyProcessMemory(DWORD pID=NULL);
BOOL		RunProcess(LPTSTR szExe, const WCHAR* szCommandLine,HANDLE *pProcess=NULL);//锟斤拷锟叫筹拷锟斤拷
BOOL		SetWindowCompositionAttribute(HWND hWnd, ACCENT_STATE mode, DWORD AlphaColor,BOOL bWin11=FALSE);//锟斤拷锟矫达拷锟斤拷WIN10锟斤拷锟?
BOOL		AutoRun(BOOL GetSet, BOOL bAutoRun, const WCHAR* szName);//锟斤拷取锟斤拷锟斤拷锟矫匡拷锟斤拷锟斤拷锟斤拷锟斤拷锟截闭匡拷锟斤拷锟斤拷锟斤拷
HICON		GetIcon(HWND hWnd, BOOL* bUWP, HWND* hUICoreWnd, int IconSize);//锟斤拷取锟斤拷锟斤拷图锟斤拷
BOOL		GetProcessFileName(DWORD dwProcessId, LPTSTR pszFileName, DWORD dwFileNameLength);//通锟斤拷锟斤拷锟斤拷ID锟斤拷取目录锟侥硷拷锟斤拷
BOOL		SetForeground(HWND hWnd);//强锟斤拷锟斤拷锟矫达拷锟斤拷为前台
void		lstrlwr(WCHAR* wString, size_t SizeInWords);//锟街凤拷锟斤拷转小写
wchar_t*	lstrstr(const wchar_t* str, const wchar_t* sub);//锟街凤拷锟斤拷锟斤拷锟斤拷
BOOL		OpenWindowPath(HWND hWnd);//锟津开达拷锟斤拷锟斤拷锟节的斤拷锟斤拷路锟斤拷
BOOL		OpenProcessPath(DWORD dwProcessId);//通锟斤拷锟斤拷锟斤拷ID锟津开斤拷锟教碉拷路锟斤拷
BOOL		EnableDebugPrivilege(BOOL bEnableDebugPrivilege);//DEBUG锟斤拷权
int			GetScreenRect(HWND hWnd, LPRECT lpRect, BOOL bTray);//锟斤拷取锟斤拷锟斤拷锟斤拷锟节碉拷锟斤拷幕锟斤拷小锟缴硷拷去锟斤拷锟斤拷锟斤拷
BOOL		GetSetVolume(BOOL bSet, HWND hWnd, DWORD dwProcessId, float* fVolume, BOOL* bMute, BOOL IsMixer);//锟斤拷取锟斤拷锟斤拷锟矫斤拷锟斤拷锟斤拷锟斤拷

void		InitService();//锟斤拷始锟斤拷锟斤拷锟斤拷锟斤拷锟?
BOOL		IsUserAdmin();//锟叫讹拷锟斤拷锟皆癸拷锟斤拷员权锟斤拷锟斤拷锟斤拷
BOOL		InstallService();//锟斤拷装锟斤拷锟斤拷
BOOL		UninstallService();//卸锟截凤拷锟斤拷
BOOL		ServiceCtrlStart();//锟斤拷锟斤拷锟斤拷锟斤拷
BOOL		ServiceCtrlStop();//停止锟斤拷锟斤拷
DWORD		ServiceRunState();//锟斤拷锟斤拷锟斤拷锟斤拷状态
BOOL		IsServiceInstalled();//锟斤拷锟斤拷锟角凤拷锟窖撅拷锟斤拷装
void WINAPI ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv);//锟斤拷锟斤拷锟斤拷锟竭筹拷锟斤拷锟?

HRESULT		pSHLoadIndirectString(LPCWSTR pszSource, LPWSTR pszOutBuf, UINT cchOutBuf, void** ppvReserved);
UINT		pDragQueryFile(HDROP hDrop, UINT iFile, LPTSTR lpszFile, UINT cch);
HICON		pExtractIcon(HINSTANCE hInst, LPCTSTR lpszExeFileName, UINT nIconIndex);
DWORD		pSHGetFileInfo(LPCTSTR pszPath, DWORD dwFileAttributes, SHFILEINFO FAR* psfi, UINT cbFileInfo, UINT uFlags);
HRESULT		pSHDefExtractIcon(LPCWSTR pszIconFile, int iIndex, UINT uFlags, HICON* phiconLarge, HICON* phiconSmall, UINT nIconSize);
HINSTANCE	pShellExecute(_In_opt_ HWND hwnd, _In_opt_ LPCWSTR lpOperation, _In_ LPCWSTR lpFile, _In_opt_ LPCWSTR lpParameters, _In_opt_ LPCWSTR lpDirectory, _In_ INT nShowCmd);
BOOL		pShell_NotifyIcon(DWORD dwMessage, _In_ PNOTIFYICONDATAW lpData);
BOOL		pWTSQueryUserToken(ULONG SessionId, PHANDLE phToken);
BOOL		pCreateEnvironmentBlock(_At_((PZZWSTR*)lpEnvironment, _Outptr_)LPVOID* lpEnvironment, _In_opt_ HANDLE  hToken, _In_ BOOL bInherit);
ULONG		pCallNtPowerInformation(_In_ POWER_INFORMATION_LEVEL InformationLevel, _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer, _In_ ULONG InputBufferLength, _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer, _In_ ULONG OutputBufferLength);
int			DrawShadowText(HDC hDC, LPCTSTR lpString, int nCount, LPRECT lpRect, UINT uFormat, COLORREF bColor, BOOL bYes);//锟斤拷锟斤拷锟斤拷影锟斤拷锟斤拷
DWORD		GetSystemUsesLightTheme();//锟斤拷取系统锟斤拷锟斤拷锟斤拷色模式
BOOL		pChangeWindowMessageFilter(UINT message, DWORD dwFlag);
UINT		pGetDpiForWindow(HWND hWnd);
UINT_PTR	pSHAppBarMessage(DWORD dwMessage,PAPPBARDATA pData);

char* xstrstr(const char* str, const char* sub);
float xatof(const char* s);
float xwtof(const WCHAR * s);
BOOL FloatToStr(float f, WCHAR* sz, size_t bufSize);
void Win11TaskbarReset();

