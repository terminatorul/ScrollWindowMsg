#if !defined(WINAPI_GUI_SEARCH_HPP)
#define WINAPI_GUI_SEARCH_HPP

#include <windef.h>

BOOL HasWindowClass(HWND hWnd, TCHAR const *szClassName, DWORD nClassNameLength);
HWND GetWindowUnderCursor(POINT &ptCursorPos);
bool GetScrollableParentWindow(HWND &hwndFocus, LPARAM &hwndCtrl);
HWND GetTopLevelParentWindow(HWND &hwndFocus, bool withWindowMenuOnly);
HWND GetCaretWindow(HWND hWnd, HWND &hWndActive, bool &isMenuMode);

#if defined(DEBUG) || defined(_DEBUG)
# define GUI_DebugString(strArg) OutputDebugString(strArg)
#else
# define GUI_DebugString(strArg) do { } while (false)
#endif

#endif // !defined(WINAPI_GUI_SEARCH_HPP)
