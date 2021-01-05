#include <Windows.h>
#include <tchar.h>

#undef min
#undef max

#if defined(_DEBUG)
#include <iterator>
#endif

#include "PropFindHandler.hpp"

#if defined(_DEBUG)
using std::size;
#endif

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

char const szLuaScriptLine[] = "OutputDebugMessage('Pipe script input\\n')\r\n";

DWORD WINAPI PipeThreadFunction(LPVOID lpParam)
{
    HANDLE hNamedPipe = (HANDLE)lpParam;
    DWORD dwWriteSize = 0;

    WriteFile(hNamedPipe, szLuaScriptLine, sizeof szLuaScriptLine - 1, &dwWriteSize, NULL);
    FlushFileBuffers(hNamedPipe);
    DisconnectNamedPipe(hNamedPipe);
    CloseHandle(hNamedPipe);

    return 0;
}

struct
{
    HANDLE	hNamedPipe;
    OVERLAPPED	overlapped;
}
    instances[32];

bool NamedPipeServer()
{
    for (auto& instance : instances)
    {
	instance.hNamedPipe = ::CreateNamedPipe
		    (
			_T("\\\\.\\pipe\\GHub-script.lua"),
			PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED,
			PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_ACCEPT_REMOTE_CLIENTS,
			PIPE_UNLIMITED_INSTANCES,
			32,
			32,
			1000,
			NULL
		    );

	if (instance.hNamedPipe)
	{
	    instance.overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	    if (instance.overlapped.hEvent)
	    {
		BOOL fConnected = ConnectNamedPipe(instance.hNamedPipe, &instance.overlapped);
	    }
	    else
		return false;
	}
	else
	    return false;;
    }

    HANDLE hEvents[32];

    for (unsigned i = 0; i < 32; i++)
	hEvents[i] = instances[i].overlapped.hEvent;

    unsigned count = 32;

    while (count)
    {
	DWORD dwWaitObject = ::WaitForMultipleObjects(count, hEvents, FALSE, INFINITE);

	if (dwWaitObject >= WAIT_OBJECT_0 && dwWaitObject < WAIT_OBJECT_0 + count)
	{
	    unsigned idx = dwWaitObject - WAIT_OBJECT_0;

	    count--;
	    ResetEvent(hEvents[idx]);

	    HANDLE hThread = CreateThread(NULL, 0, PipeThreadFunction, (LPVOID)instances[idx].hNamedPipe, 0, NULL);

	    if (hThread)
		CloseHandle(hThread);
	    else
		return false;
	}
    }

    return true;
}

static XML_Char const defaultRequestEncoding[] = "US-ASCII";

static char const
    xmlAllPropDocument[] = "<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
"   <D:propfind xmlns:D=\"DAV:\">"
"	<D:allprop/>"
"	<D:include>"
"	    <D:supported-live-property-set/>"
"	    <D:supported-report-set/>"
"	</D:include>"
"   </D:propfind>",
    xmlPropDocument[] = "<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
"   <D:propfind xmlns : D = \"DAV:\" >"
"	<D:prop xmlns:R = \"http://ns.example.com/boxschema/\">"
"	    <R:bigbox/>"
"	    <R:author/>"
"	    <R:DingALing/>"
"	    <R:Random/>"
"	</D:prop>"
"   </D:propfind>",
    xmlNsPropDocument[] = "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n"
"   <propfind xmlns=\"DAV:\">\n"
"	<prop xmlns:R = \"http://ns.example.com/boxschema/\">\n"
"	    <R:bigbox/>\n"
"	    <R:author/>\n"
"	    <R:DingALing/>\n"
"	    <R:Random/>\n"
"	</prop>\n"
"   </propfind>",
    xmlPropNameDocument[] = "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n"
"    <propfind xmlns = \"DAV:\" >\n"
"	<propname/>"
"    </propfind>";

int WinMain
(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nShowCmd
)
{
    // while (NamedPipeServer())
    //	;

    PropFindHandler propFindHandler;
    // xml::parser parser(xmlDocument, sizeof xmlDocument - 1, "PROPFIND request body"s);

    propFindHandler.Create(defaultRequestEncoding);

#if defined(_DEBUG)
    bool parseSuccessful = propFindHandler.Parse(xmlAllPropDocument, static_cast<int>(size(xmlAllPropDocument) - 1), true);

    if (!parseSuccessful)
    {
	XML_Char const *errorMessage = propFindHandler.GetErrorString();
	size_t errorLen = strlen(errorMessage);
	WriteFile(GetStdHandle(STD_ERROR_HANDLE), errorMessage, static_cast<DWORD>(errorLen), nullptr, nullptr);
	WriteFile(GetStdHandle(STD_ERROR_HANDLE), "\r\n", 2, nullptr, nullptr);
    }

    if (propFindHandler.error() == PropFindHandler::RequestError::None)
    {
	switch (propFindHandler.requestType())
	{
	case PropFindHandler::Request::PropName:
	    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), "PropName request.\r\n", 19, nullptr, nullptr);
	    break;

	case PropFindHandler::Request::AllProps:
	    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), "AllProps request.\r\n", 19, nullptr, nullptr);
	    break;

	case PropFindHandler::Request::Prop:
	    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), "Prop request.\r\n", 15, nullptr, nullptr);
	    break;
	}

	WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), "Properties list:\r\n", 18, nullptr, nullptr);

	for (auto const &entry: propFindHandler.propertiesMap())
	{
	    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), entry.first.data(), static_cast<DWORD>(entry.first.length()), nullptr, nullptr);
	    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), " => ", 4, nullptr, nullptr);
	    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), propFindHandler.namespacePrefix(entry.first).data(), static_cast<DWORD>(propFindHandler.namespacePrefix(entry.first).length()), nullptr, nullptr);
	    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), "\r\n", 2, nullptr, nullptr);

	    for (auto const &name: entry.second)
	    {
		WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), "\t", 1, nullptr, nullptr);
		WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), name.data(), static_cast<DWORD>(name.length()), nullptr, nullptr);
		WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), "\r\n", 2, nullptr, nullptr);

	    }
	}
    }

    char inputLine[32];
    (void)ReadFile(GetStdHandle(STD_INPUT_HANDLE), inputLine, 1, NULL, NULL);
#endif

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