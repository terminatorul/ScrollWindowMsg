#if !defined(WINAPI_GUI_ACTIONS_HPP)
#define WINAPI_GUI_ACTIONS_HPP

#include <windef.h>

int SendHorizontalScroll(HWND hWnd, bool isDirectionLeft, POINT &ptCursorPos);
int SendSysCommandMessage(HWND hWnd, UINT iSysCommandMessage, POINT &ptCursorPos);
int SendKeyCode(HWND hWnd, WORD vKey, bool extendedKey);

#endif // !defined(WINAPI_GUI_ACTIONS_HPP)