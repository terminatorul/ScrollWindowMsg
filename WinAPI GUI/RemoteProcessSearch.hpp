#if !defined(WINAPI_REMOTE_PROCESS_SEARCH_HPP)
#define WINAPI_REMOTE_PROCESS_SEARCH_HPP

#include <windef.h>

HMODULE GetRemoteModuleHandle(HANDLE hProcess, LPCTSTR lpModuleName);
FARPROC GetRemoteProcAddress(HANDLE hProcess, HMODULE hModule, LPCTSTR lpProcName, UINT Ordinal = 0);

#endif // !defined(WINAPI_REMOTE_PROCESS_SEARCH_HPP)