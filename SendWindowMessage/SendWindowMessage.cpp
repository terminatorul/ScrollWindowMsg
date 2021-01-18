#include <windows.h>

#include <windef.h>
#include <winbase.h>
#include <tchar.h>
#include "GUI Search.hpp"
#include "GUI Actions.hpp"

void skipWhiteSpace(LPTSTR &lpStr)
{
    do
	switch (*lpStr)
	{
	case TEXT(' '):
	case TEXT('\b'):
	case TEXT('\t'):
	case TEXT('\r'):
	case TEXT('\n'):
	case TEXT('\v'):
	case TEXT('\f'):
	    lpStr++;
	default:
	    return;
	}
    while (true);
}

#if defined(UNICODE) || defined(_UNICODE)
int WINAPI wWinMain
#else
int WINAPI WinMain
#endif
(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPTSTR    lpCmdLine,
    int       nShowCmd
)
{
    int exitCode = -1;

    POINT ptCursorPos { };
    HWND  hWnd = GetWindowUnderCursor(ptCursorPos);

    skipWhiteSpace(lpCmdLine);

    if (!lstrcmp(lpCmdLine, TEXT("left")))
	exitCode = SendHorizontalScroll(hWnd, true, ptCursorPos);
    else
	if (!lstrcmp(lpCmdLine, TEXT("right")))
	    exitCode = SendHorizontalScroll(hWnd, false, ptCursorPos);
	else
	    if (!lstrcmp(lpCmdLine, TEXT("close")))
		exitCode = SendSysCommandMessage(hWnd, SC_CLOSE, ptCursorPos);
	    else
		if (!lstrcmp(lpCmdLine, TEXT("minimize")))
		    exitCode = SendSysCommandMessage(hWnd, SC_MINIMIZE, ptCursorPos);
		else
		    if (!lstrcmp(lpCmdLine, TEXT("esc")))
			exitCode = SendKeyCode(hWnd, VK_ESCAPE, false);
		    else
			if (!lstrcmp(lpCmdLine, TEXT("return")))
			    exitCode = SendKeyCode(hWnd, VK_RETURN, false);
			else
			    if (!lstrcmp(lpCmdLine, TEXT("tab")))
				exitCode = SendKeyCode(hWnd, VK_TAB, false);
			    else
				if (!lstrcmp(lpCmdLine, TEXT("space")))
				    exitCode = SendKeyCode(hWnd, VK_SPACE, false);
				else
				    if (!lstrcmp(lpCmdLine, TEXT("backspace")))
					exitCode = SendKeyCode(hWnd, VK_BACK, false);
				    else
					if (!lstrcmp(lpCmdLine, TEXT("del")))
					    exitCode = SendKeyCode(hWnd, VK_DELETE, false);
					else
					    return ERROR_INVALID_COMMAND_LINE;

    return exitCode;
}
