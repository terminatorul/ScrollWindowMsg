#define NOMINMAX
#include <Windows.h>

#include <windef.h>
#include <winbase.h>
#include <winuser.h>

#include <cstdint>
#include <utility>
#include <algorithm>
#include <iterator>

using std::uintptr_t;
using std::size;

static TCHAR const SCROLLBAR_CLASS[] = TEXT("SCROLLBAR");

BOOL HasWindowClass(HWND hWnd, TCHAR const *szClassName, DWORD nClassNameLength)
{
    TCHAR szWindowClassName[128];

    int iClassLen = GetClassName(hWnd, szWindowClassName, sizeof szWindowClassName / sizeof szWindowClassName[0] - 1);

    return iClassLen == nClassNameLength && !lstrcmpi(szWindowClassName, szClassName) ? TRUE : FALSE;
}

static BOOL CALLBACK EnumChildProc(HWND hwndCtrl, LPARAM lParam)
{
    LONG lWnd = GetWindowLong(hwndCtrl, GWL_STYLE);

    if ((lWnd & (WS_VISIBLE | SBS_HORZ)) && (!(lWnd & SBS_VERT)) && IsWindowEnabled(hwndCtrl))
    {
	if (HasWindowClass(hwndCtrl, SCROLLBAR_CLASS, sizeof SCROLLBAR_CLASS / sizeof SCROLLBAR_CLASS[0] - 1))
	{
	    // hwndCtrl is a visible and enabled horizontal scrollbarl control

	    HWND **pChildProcData = (HWND **)(void *)(uintptr_t)lParam;
	    HWND hParentWnd = ::GetParent(hwndCtrl);

	    if (hParentWnd == *pChildProcData[0])
	    {
		*(LPARAM *)(void *)pChildProcData[1] = (LPARAM)(uintptr_t)(void *)hwndCtrl;
		return FALSE;
	    }
	    else
		if
		    (
			HasWindowClass(hParentWnd, TEXT("CtrlNotifySink"), static_cast<DWORD>(size(TEXT("CtrlNotifySink")) - 1))
			    &&
			(GetWindowLong(hParentWnd, GWL_STYLE) & WS_VISIBLE) && IsWindowEnabled(hParentWnd)
			    &&
			::GetParent(hParentWnd) == *pChildProcData[0]
		    )
		{
		    *pChildProcData[0] = hParentWnd;
		    *(LPARAM *)(void *)pChildProcData[1] = (LPARAM)(uintptr_t)(void *)hwndCtrl;
		    return FALSE;
		}
	}
    }

    return TRUE;
}

bool GetScrollableParentWindow(HWND &hwndFocus, LPARAM &hwndCtrl)
{
    if (GetWindowLong(hwndFocus, GWL_STYLE) & WS_HSCROLL)
	return true;

    HWND *childProcData[2] = { &hwndFocus, (HWND *)(void *)&hwndCtrl };
    EnumChildWindows(hwndFocus, EnumChildProc, (LPARAM)(uintptr_t)(void *)childProcData);

    if (hwndCtrl)
	return true;

    hwndFocus = GetParent(hwndFocus);

    if (hwndFocus)
	return GetScrollableParentWindow(hwndFocus, hwndCtrl);

    return false;
}

HWND GetTopLevelParentWindow(HWND &hwndFocus, bool withWindowMenuOnly)
{
    if (withWindowMenuOnly)
	while (hwndFocus && !(GetWindowLong(hwndFocus, GWL_STYLE) & WS_SYSMENU))
	    hwndFocus = GetParent(hwndFocus);
    else
	hwndFocus = ::GetAncestor(hwndFocus, GA_ROOT);

    return hwndFocus;
}

HWND GetWindowUnderCursor(POINT &ptCursorPos)
{
    HWND hwndParent = NULL;
    HWND hwndChild = GetDesktopWindow();

    if (GetCursorPos(&ptCursorPos))
	do
	{
	    POINT ptClientCursorPos = ptCursorPos;

	    hwndParent = hwndChild;

	    if (ScreenToClient(hwndParent, &ptClientCursorPos))
		hwndChild = RealChildWindowFromPoint(hwndParent, ptClientCursorPos);
	    else
		return NULL;
	}
	while (hwndChild && hwndChild != hwndParent);

    return hwndParent;
}

HWND GetCaretWindow(HWND hWnd, HWND &hWndActive, bool &isMenuMode)
{
    GUITHREADINFO guiThreadInfo { sizeof guiThreadInfo };
    DWORD dwThread = ::GetWindowThreadProcessId(hWnd, NULL);
    
    if (::GetGUIThreadInfo(dwThread, &guiThreadInfo))
    {
	if (guiThreadInfo.hwndActive)
	    hWndActive = ::GetAncestor(guiThreadInfo.hwndActive, GA_ROOT);
	else
	    hWndActive = guiThreadInfo.hwndActive;

	isMenuMode = !!(guiThreadInfo.flags & GUI_INMENUMODE);
	return guiThreadInfo.hwndFocus;
    }

    return NULL;
}
