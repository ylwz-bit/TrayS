#include <windows.h>
#include <shellapi.h>
#include <Psapi.h>
#include <Mmdeviceapi.h>
#include <audiopolicy.h>
#include <Shlobj.h>
#include <tlhelp32.h>
#include <commctrl.h>

//#include "Winhttp.h"
//#pragma comment(lib,"winhttp.lib")

//GDI+
/*
#include<gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;
*/

const WCHAR lpServiceName[] = L"TrayS";//������
const WCHAR szShellTray[] = L"Shell_TrayWnd";//������������
const WCHAR szSecondaryTray[] = L"Shell_SecondaryTrayWnd";//������������
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
void		SetToCurrentPath();//���ý���·��Ϊ��ǰ·��
void		EmptyProcessMemory(DWORD pID=NULL);
BOOL		RunProcess(LPTSTR szExe, const WCHAR* szCommandLine,HANDLE *pProcess=NULL);//���г���
BOOL		SetWindowCompositionAttribute(HWND hWnd, ACCENT_STATE mode, DWORD AlphaColor,BOOL bWin11=FALSE);//���ô���WIN10���
BOOL		AutoRun(BOOL GetSet, BOOL bAutoRun, const WCHAR* szName);//��ȡ�����ÿ����������رտ�������
HICON		GetIcon(HWND hWnd, BOOL* bUWP, HWND* hUICoreWnd, int IconSize);//��ȡ����ͼ��
BOOL		GetProcessFileName(DWORD dwProcessId, LPTSTR pszFileName, DWORD dwFileNameLength);//ͨ������ID��ȡĿ¼�ļ���
BOOL		SetForeground(HWND hWnd);//ǿ�����ô���Ϊǰ̨
void		lstrlwr(WCHAR* wString, size_t SizeInWords);//�ַ���תСд
wchar_t*	lstrstr(const wchar_t* str, const wchar_t* sub);//�ַ�������
BOOL		OpenWindowPath(HWND hWnd);//�򿪴������ڵĽ���·��
BOOL		OpenProcessPath(DWORD dwProcessId);//ͨ������ID�򿪽��̵�·��
BOOL		EnableDebugPrivilege(BOOL bEnableDebugPrivilege);//DEBUG��Ȩ
int			GetScreenRect(HWND hWnd, LPRECT lpRect, BOOL bTray);//��ȡ�������ڵ���Ļ��С�ɼ�ȥ������
BOOL		GetSetVolume(BOOL bSet, HWND hWnd, DWORD dwProcessId, float* fVolume, BOOL* bMute, BOOL IsMixer);//��ȡ�����ý�������

void		InitService();//��ʼ���������
BOOL		IsUserAdmin();//�ж����Թ���ԱȨ������
BOOL		InstallService();//��װ����
BOOL		UninstallService();//ж�ط���
BOOL		ServiceCtrlStart();//��������
BOOL		ServiceCtrlStop();//ֹͣ����
DWORD		ServiceRunState();//��������״̬
BOOL		IsServiceInstalled();//�����Ƿ��Ѿ���װ
void WINAPI ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv);//�������߳����

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
int			DrawShadowText(HDC hDC, LPCTSTR lpString, int nCount, LPRECT lpRect, UINT uFormat, COLORREF bColor, BOOL bYes);//������Ӱ����
DWORD		GetSystemUsesLightTheme();//��ȡϵͳ������ɫģʽ
BOOL		pChangeWindowMessageFilter(UINT message, DWORD dwFlag);
UINT		pGetDpiForWindow(HWND hWnd);
UINT_PTR	pSHAppBarMessage(DWORD dwMessage,PAPPBARDATA pData);

char* xstrstr(const char* str, const char* sub);
float xatof(const char* s);
float xwtof(const WCHAR * s);
BOOL FloatToStr(float f, WCHAR* sz);