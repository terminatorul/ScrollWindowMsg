#include <windows.h>

#include <windef.h>
#include <tchar.h>
#include "GUI Search.hpp"
#include "GUI Actions.hpp"

int WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    GUI_DebugString(TEXT("Starting Send Escape Key"));

    POINT ptCursorPos { };
    HWND  hWnd = GetWindowUnderCursor(ptCursorPos);

    return SendKeyCode(hWnd, VK_ESCAPE, false);
}