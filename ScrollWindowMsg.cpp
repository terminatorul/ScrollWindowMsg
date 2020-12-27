#include <Windows.h>
#include <tchar.h>

static SHORT const SHIFTED = 1 << (sizeof(SHORT) * CHAR_BIT - 1);

BOOL CALLBACK EnumChildProc(HWND hwndCtrl, LPARAM lParam)
{
    if ((GetWindowLong(hwndCtrl, GWL_STYLE) & (WS_VISIBLE | SBS_HORZ)) && IsWindowEnabled(hwndCtrl))
    {
	static TCHAR const SCROLLBAR_CLASS[] = _T("SCROLLBAR");
	TCHAR ctrlClassName[sizeof SCROLLBAR_CLASS / sizeof SCROLLBAR_CLASS[0] + 1];

	int ctrlClassLen = GetClassName(hwndCtrl, ctrlClassName, sizeof ctrlClassName / sizeof ctrlClassName[0]);

	if (ctrlClassLen == sizeof SCROLLBAR_CLASS / sizeof SCROLLBAR_CLASS[0] - 1 && _tcsicmp(ctrlClassName, SCROLLBAR_CLASS) == 0)
	{
	    // hwndCtrl is a visible and enabled horizontal scrollbarl control

	    *(LPARAM *)lParam = LPARAM(hwndCtrl);
	    return FALSE;
	}
    }

    return TRUE;
}

bool GetScrollableParentWindow(HWND &hwndFocus, LPARAM &hwndCtrl)
{
    if (GetWindowLong(hwndFocus, GWL_STYLE) & WS_HSCROLL)
	return true;

    EnumChildWindows(hwndFocus, EnumChildProc, (LPARAM)&hwndCtrl);

    if (hwndCtrl)
	return true;

    hwndFocus = GetParent(hwndFocus);

    if (hwndFocus)
	return GetScrollableParentWindow(hwndFocus, hwndCtrl);

    return false;
}

WPARAM MakeHScrollWParam(WPARAM wScrollbarMsg)
{
    WPARAM wParam = 0;

    if (GetKeyState(VK_SHIFT) & SHIFTED)
	wParam |= MK_SHIFT;

    if (GetKeyState(VK_CONTROL) & SHIFTED)
	wParam |= MK_CONTROL;

    if (GetKeyState(VK_LBUTTON) & SHIFTED)
	wParam |= MK_LBUTTON;

    if (GetKeyState(VK_RBUTTON) & SHIFTED)
	wParam |= MK_RBUTTON;

    if (GetKeyState(VK_MBUTTON) & SHIFTED)
	wParam |= MK_MBUTTON;

    if (GetKeyState(VK_XBUTTON1) & SHIFTED)
	wParam |= MK_XBUTTON1;

    if (GetKeyState(VK_XBUTTON2) & SHIFTED)
	wParam |= MK_XBUTTON2;

    return MAKEWPARAM(wParam, wScrollbarMsg == SB_LINERIGHT ? WHEEL_DELTA : -WHEEL_DELTA);
}

LPARAM MakeHScrollLParam(POINT const &ptCursorPos)
{
    return MAKELPARAM(ptCursorPos.x, ptCursorPos.y);
}

HWND GetWindowUnderCursor(POINT &ptCursorPos)
{
    HWND hwndParent;
    HWND hwndChild = GetDesktopWindow();

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

int WinMain
(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nShowCmd
)
{
    POINT ptCursorPos { };
    HWND  hWnd = GetCursorPos(&ptCursorPos) ? GetWindowUnderCursor(ptCursorPos) : NULL;

    while (*lpCmdLine && (*lpCmdLine == ' ' || *lpCmdLine == '\t' || *lpCmdLine == '\r' || *lpCmdLine == '\n' || *lpCmdLine == '\f' || *lpCmdLine == '\v'))
	lpCmdLine++;

    WPARAM wScrollbarMsg = CompareStringA(LOCALE_INVARIANT, LINGUISTIC_IGNORECASE, lpCmdLine, 4, "left", 4) == CSTR_EQUAL ? SB_LINELEFT : SB_LINERIGHT;

    if (hWnd)
    {
	LPARAM hwndCtrl = 0L;
	HWND   hScrollableWnd = hWnd;

	if (GetScrollableParentWindow(hScrollableWnd, hwndCtrl))
	{
	    UINT nScrollChars = 3;

	    SystemParametersInfo(SPI_GETWHEELSCROLLCHARS, 0U, &nScrollChars, 0);

	    while (nScrollChars--)
		PostMessage(hScrollableWnd, WM_HSCROLL, wScrollbarMsg, hwndCtrl);
	}
	else
	    PostMessage(hWnd, WM_MOUSEHWHEEL, MakeHScrollWParam(wScrollbarMsg), MakeHScrollLParam(ptCursorPos));

	return 0;
    }

    return -1;
}