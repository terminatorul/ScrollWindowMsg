#include <Windows.h>
#include <windef.h>

#include "GUI Search.hpp"
#include "GUI Actions.hpp"

int WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    POINT ptCursorPos { };
    HWND  hWnd = GetWindowUnderCursor(ptCursorPos);

    return SendSysCommandMessage(hWnd, SC_RESTORE, ptCursorPos);
}