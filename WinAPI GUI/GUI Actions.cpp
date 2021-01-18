#include <Windows.h>

#include <windef.h>
#include <winuser.h>
#include <limits.h>

#include <iterator>
#include <string>
#include <sstream>

#include "RemoteProcessSearch.hpp"
#include "GUI Search.hpp"

using std::size;
using std::to_wstring;

static_assert(sizeof(HWND) == sizeof(LPVOID), "Expected same size for HWND and LPVOID to inject GUI calls to other processes.");

static SHORT const SHIFTED = static_cast<SHORT>(1 << (sizeof(SHORT) * CHAR_BIT - 1));

WPARAM MakeHScrollWParam(WPARAM wScrollbarMsg)
{
    WPARAM wParam = 0;

    if (GetAsyncKeyState(VK_SHIFT) & SHIFTED)
	wParam |= MK_SHIFT;

    if (GetAsyncKeyState(VK_CONTROL) & SHIFTED)
	wParam |= MK_CONTROL;

    if (GetAsyncKeyState(VK_LBUTTON) & SHIFTED)
	wParam |= MK_LBUTTON;

    if (GetAsyncKeyState(VK_RBUTTON) & SHIFTED)
	wParam |= MK_RBUTTON;

    if (GetAsyncKeyState(VK_MBUTTON) & SHIFTED)
	wParam |= MK_MBUTTON;

    if (GetAsyncKeyState(VK_XBUTTON1) & SHIFTED)
	wParam |= MK_XBUTTON1;

    if (GetAsyncKeyState(VK_XBUTTON2) & SHIFTED)
	wParam |= MK_XBUTTON2;

    return MAKEWPARAM(wParam, wScrollbarMsg == SB_LINERIGHT ? WHEEL_DELTA : -WHEEL_DELTA);
}

LPARAM MakeHScrollLParam(POINT const &ptCursorPos)
{
    return MAKELPARAM(ptCursorPos.x, ptCursorPos.y);
}

int SendHorizontalScroll(HWND hWnd, bool isDirectionLeft, POINT &ptCursorPos)
{
    WPARAM wScrollbarMsg = isDirectionLeft ? SB_LINELEFT : SB_LINERIGHT;

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

	    GUI_DebugString((TEXT("HorizontalScroll: sent WM_HSCROLL messages to hwnd ") + to_wstring((uintptr_t)(void *)hScrollableWnd)).data());
	}
	else
	{
	    PostMessage(hWnd, WM_MOUSEHWHEEL, MakeHScrollWParam(wScrollbarMsg), MakeHScrollLParam(ptCursorPos));
	    GUI_DebugString(TEXT("HorizontalScroll: no scrollable parent window detected."));
	}


	return 0;
    }
    else
	GUI_DebugString(TEXT("HorizontalScroll: no window under cursor detected."));

    return -1;
}

int SendSysCommandMessage(HWND hWnd, UINT iSysCommandMessage, POINT &ptCursorPos)
{
    if (hWnd)
    {
	hWnd = GetTopLevelParentWindow(hWnd, (iSysCommandMessage != SC_CLOSE));

	if (hWnd)
	{
	    PostMessage(hWnd, WM_SYSCOMMAND, iSysCommandMessage, MAKELPARAM(ptCursorPos.x, ptCursorPos.y));

	    return 0;
	}
    }

    return -1;
}

LPARAM MakeKeystrokeLParamData(USHORT uRepeatCount, BYTE uScanCode)
{
    return static_cast<LPARAM>(uRepeatCount & 0xFFFFU) | static_cast<LPARAM>(uScanCode & 0xFF) << 16;
}

LPARAM MakeKeystrokeLParamFlags(bool extendedKey, bool dialogMode, bool systemContext, bool previousState, bool releaseTransition)
{
    return
	MAKELPARAM
	    (
		0, 
		(extendedKey ? KF_EXTENDED : 0) | (dialogMode ? KF_DLGMODE : 0) | (systemContext ? KF_ALTDOWN : 0) 
		    |
		(previousState ? KF_REPEAT : 0) | (releaseTransition ? KF_UP : 0)
	    );
}

DWORD RemoteFunctionCall(HANDLE hRemoteProcess, LPCTSTR szModuleName, LPCTSTR szFunctionName, LPVOID lpFunctionArg, DWORD &dwFunctionResult)
{
    HMODULE hUserModule = GetRemoteModuleHandle(hRemoteProcess, szModuleName);

    if (hUserModule)
    {
	FARPROC lpfnRemoteFunction = GetRemoteProcAddress(hRemoteProcess, hUserModule, szFunctionName);

	if (lpfnRemoteFunction)
	{
	    HANDLE hRemoteThread = ::CreateRemoteThread(hRemoteProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)lpfnRemoteFunction, lpFunctionArg, 0U, NULL);

	    if (hRemoteThread)
	    {
		::WaitForSingleObject(hRemoteThread, INFINITE);

		if (::GetExitCodeThread(hRemoteThread, &dwFunctionResult))
		{
		    ::CloseHandle(hRemoteThread);
		    return ERROR_SUCCESS;
		}

		DWORD dwError = ::GetLastError();
		OutputDebugString(TEXT("SendKey: No exit code from remote thread !"));
		::CloseHandle(hRemoteThread);

		return dwError;
	    }
	}
    }

    return ::GetLastError();
}

DWORD RemoteAllowSetForgroundWindow(HANDLE hRemoteProcess)
{
    HANDLE hLocalProcess = ::OpenProcess(PROCESS_DUP_HANDLE | SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, ::GetCurrentProcessId());

    if (!hLocalProcess)
    {
	GUI_DebugString(TEXT("SendKey: No local process"));
	return ::GetLastError();
    }

    HANDLE hRemoteHandle = NULL;
    BOOL bDuplicate = ::DuplicateHandle(hLocalProcess, ::GetCurrentProcess(), hRemoteProcess, &hRemoteHandle, SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, 0U);

    if (bDuplicate)
    {
	DWORD dwFunctionResult, dwError = RemoteFunctionCall(hRemoteProcess, TEXT("USER32"), TEXT("AllowSetForegroundWindow"), static_cast<LPVOID>(hRemoteHandle), dwFunctionResult);

	if (dwError == ERROR_SUCCESS && dwFunctionResult)
	    GUI_DebugString(TEXT("Successfully called AllowSetForegroundWindow"));

	return dwError ? dwError : (dwFunctionResult ? ERROR_SUCCESS : ERROR_GENERIC_COMMAND_FAILED);
    }
    else
    {
	DWORD dwLastError = ::GetLastError();
	GUI_DebugString((TEXT("Duplicate handle: ") + std::to_wstring(dwLastError)).c_str());

	return dwLastError;
    }
}

HANDLE OpenForegroundProcess(DWORD dwOpenFlags)
{
    HWND hWndForeground = GetForegroundWindow();

    if (hWndForeground)
    {
	DWORD dwForegroundProcessId;
	DWORD dwForegroundThread = ::GetWindowThreadProcessId(hWndForeground, &dwForegroundProcessId);
	return ::OpenProcess(dwOpenFlags, FALSE, dwForegroundProcessId);
    }

    return NULL;
}

BOOL GenerateKeyUpDownInput(WORD vKey, WORD uScanCode)
{
    INPUT inputKeys[2] { INPUT { }, INPUT { } };
    INPUT &inputKeyDown = inputKeys[0], &inputKeyUp = inputKeys[1];

    inputKeyDown.type	    = INPUT_KEYBOARD;
    inputKeyDown.ki.wVk	    = vKey;
    inputKeyDown.ki.wScan   = uScanCode;

    inputKeyUp.type	    = INPUT_KEYBOARD;
    inputKeyUp.ki.wVk	    = vKey;
    inputKeyUp.ki.wScan	    = uScanCode;
    inputKeyUp.ki.dwFlags   = KEYEVENTF_KEYUP;

    UINT uEventCount = ::SendInput(static_cast<UINT>(size(inputKeys)), inputKeys, static_cast<int>(sizeof inputKeys[0])) ? TRUE : FALSE;

    GUI_DebugString(uEventCount ? TEXT("SendKey: Sent keyboard input events for KEYDOWN / KEYUP.") : TEXT("No keyboard input events could be sent."));

    return uEventCount ? TRUE : FALSE;
}

void PostKeyUpDownMessages(HWND hTargetWnd, WORD vKey, BYTE uScanCode, bool extendedKey, bool dialogMode)
{
    UINT iDownMessage, iUpMessage;
    bool keystrokeContext;

    if (::GetAsyncKeyState(VK_MENU))
    {
	iDownMessage = WM_SYSKEYDOWN;
	iUpMessage = WM_SYSKEYUP;
	keystrokeContext = true;
    }
    else
    {
	iDownMessage = WM_KEYDOWN;
	iUpMessage = WM_KEYUP;
	keystrokeContext = false;
    }

    LPARAM lKeystrokeLParam = MakeKeystrokeLParamData(1U, uScanCode) | MakeKeystrokeLParamFlags(extendedKey, dialogMode, keystrokeContext, false, false);
    BOOL bKeyDownMessagePosted = ::PostMessage(hTargetWnd, iDownMessage, vKey, lKeystrokeLParam);

    lKeystrokeLParam = MakeKeystrokeLParamData(1U, uScanCode) | MakeKeystrokeLParamFlags(extendedKey, dialogMode, keystrokeContext, true, true);
    BOOL bKeyUpMessagePosted = ::PostMessage(hTargetWnd, iUpMessage, vKey, lKeystrokeLParam);

#if defined(DEBUG) || defined(_DEBUG)
    std::basic_stringstream<TCHAR> str;
    str << std::hex << reinterpret_cast<uintptr_t>(static_cast<void*>(hTargetWnd));

    if (bKeyDownMessagePosted && bKeyUpMessagePosted)
	GUI_DebugString(TEXT("SendKey: Sent VM_KEYDOWN / WM_KEYUP messages."));
    else
	GUI_DebugString(TEXT("SendKey: PostMessage error"));

    OutputDebugString((std::basic_string<TCHAR>(TEXT("Focus window: ")) + str.str()).data());
#endif
}

bool BringWindowToForeground(HWND hRootWnd)
{
    HANDLE hForegroundProcess = OpenForegroundProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_READ  | PROCESS_VM_READ);

    if (hForegroundProcess)
    {
	DWORD dwFunctionResult, dwRemoteCallError = RemoteFunctionCall(hForegroundProcess, TEXT("USER32"), TEXT("SetForegroundWindow"), static_cast<LPVOID>(hRootWnd), dwFunctionResult);

	if (dwRemoteCallError == ERROR_SUCCESS && dwFunctionResult)
	{
	    OutputDebugString(TEXT("SendKey: Target window brought to foreground!"));
	    return true;
	}
	else
	{
	    OutputDebugString(TEXT("SendKey: Target window not brought to foreground !"));
	}

	::CloseHandle(hForegroundProcess);
    }

    return false;
}

int SendKeyCode(HWND hWnd, WORD vKey, bool extendedKey)
{
    if (!hWnd)
    {
	GUI_DebugString(TEXT("SendKey: No window detected under cursor."));
	return -1;
    }

    bool isMenuMode;
    HWND hWndActive, hWndFocus = GetCaretWindow(hWnd, hWndActive, isMenuMode), hRootWnd = ::GetAncestor(hWnd, GA_ROOT);
    bool isTargetWindowActive = (hWndActive == hRootWnd);

    if (!isTargetWindowActive || !hWndFocus)
	if (!BringWindowToForeground(hRootWnd))
	{
	    ::SetForegroundWindow(hRootWnd);
	    Sleep(50);
	}

    if (hWndFocus && isTargetWindowActive || ::GetForegroundWindow() == hRootWnd)
    {
	if (!(::GetAsyncKeyState(static_cast<int>(vKey)) & SHIFTED))
	{
	    UINT uScanCode = MapVirtualKey(vKey, MAPVK_VK_TO_VSC);

	    if (::GetForegroundWindow() == hRootWnd)
	    {
	        GenerateKeyUpDownInput(vKey, uScanCode);
	    }
	    else
	    {
		PostKeyUpDownMessages(hWndFocus, vKey, uScanCode, extendedKey, HasWindowClass(hWndActive, WC_DIALOG, lstrlen(WC_DIALOG)));
	    }

	    return 0;
	}

	GUI_DebugString(TEXT("SendKey: Request key is already down."));
    }
#if defined(DEBUG) || defined(_DEBUG)
    else
    {
	OutputDebugString(TEXT("SendKey: No focus window found for target thread 2 !."));

    }
#endif

    return -1;
}
