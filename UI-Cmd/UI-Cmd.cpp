#define _AMD64_
#include <windef.h>
#include <WinBase.h>
#include <handleapi.h>
#include <winuser.h>
#include <processthreadsapi.h>
#include <libloaderapi.h>
#include <fileapi.h>
#include <WinError.h>
#include <synchapi.h>
#include <shellapi.h>
#include <combaseapi.h>
#include <tchar.h>

#include <vector>

using std::vector;

bool CreateUserUIAccessProcess(vector<TCHAR> &szFullPathName, LPTSTR lpCmdLine)
{
    vector<TCHAR> vectCmdLine;
    STARTUPINFO startupInfo { sizeof startupInfo };
    PROCESS_INFORMATION processInformation { };
    HANDLE hAccessToken = NULL, hPrimaryToken = NULL;
    DWORD dwUIAccess = 1;
    BOOL bHasToken = OpenProcessToken(::GetCurrentProcess(), MAXIMUM_ALLOWED, &hAccessToken);

    if (bHasToken)
    {
	TOKEN_PRIVILEGES seAssignPrimaryToken { };

	seAssignPrimaryToken.PrivilegeCount = 1;
	seAssignPrimaryToken.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	bHasToken = ::DuplicateTokenEx(hAccessToken, MAXIMUM_ALLOWED, NULL, SecurityIdentification, TokenPrimary, &hPrimaryToken);
	bHasToken = bHasToken && ::LookupPrivilegeValue(NULL, SE_ASSIGNPRIMARYTOKEN_NAME, &seAssignPrimaryToken.Privileges[0].Luid);
	bHasToken = bHasToken && ::AdjustTokenPrivileges(hPrimaryToken, FALSE, &seAssignPrimaryToken, sizeof seAssignPrimaryToken, NULL, NULL);
	bHasToken = bHasToken && ::LookupPrivilegeValue(NULL, SE_TCB_NAME, &seAssignPrimaryToken.Privileges[0].Luid);
	bHasToken = bHasToken && ::AdjustTokenPrivileges(hPrimaryToken, FALSE, &seAssignPrimaryToken, sizeof seAssignPrimaryToken, NULL, NULL);
    }

    if (bHasToken && ::SetTokenInformation(hPrimaryToken, TokenUIAccess, &dwUIAccess, sizeof TokenUIAccess))
    {
	int nCmdLineLength = lstrlen(lpCmdLine);
	vectCmdLine.reserve(szFullPathName.size() + 3 + nCmdLineLength + 1);
	vectCmdLine.push_back(TEXT('\"'));
	vectCmdLine.insert(vectCmdLine.end(), szFullPathName.begin(), szFullPathName.end());
	vectCmdLine.pop_back();
	vectCmdLine.push_back(TEXT('\"'));
	vectCmdLine.push_back(TEXT(' '));
	vectCmdLine.insert(vectCmdLine.end(), lpCmdLine, lpCmdLine + nCmdLineLength);
	vectCmdLine.push_back(TEXT('\0'));

	startupInfo.dwFlags |= STARTF_FORCEOFFFEEDBACK;
	BOOL bProcessCreated = ::CreateProcessAsUser(hPrimaryToken, szFullPathName.data(), vectCmdLine.data(), NULL, NULL, FALSE, HIGH_PRIORITY_CLASS, NULL, NULL, &startupInfo, &processInformation);

	if (bProcessCreated)
	{
	    ::WaitForSingleObject(processInformation.hProcess, INFINITE);

	    DWORD dwExitCode = -1;

	    if (!::GetExitCodeProcess(processInformation.hProcess, &dwExitCode))
		dwExitCode = ::GetLastError();

	    ::CloseHandle(processInformation.hThread);
	    ::CloseHandle(processInformation.hProcess);

	    return dwExitCode == 0U;
	}

	::CloseHandle(hAccessToken);
    }

    return false;
}

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

void skipQuotedWord(LPTSTR &lpStr, bool &relativePath)
{
    do
	switch (*lpStr)
	{
	case TEXT('\0'):
	case TEXT('"'):
	    return;
	case TEXT('\\'):
	case TEXT('/'):
	    relativePath = true;
	    // fall-through
	default:
	    lpStr++;
	}
    while (true);
}

void skipWord(LPTSTR &lpStr, bool &relativePath)
{
    do
	switch (*lpStr)
	{
	case TEXT('\0'):
	case TEXT(' '):
	case TEXT('\b'):
	case TEXT('\t'):
	case TEXT('\r'):
	case TEXT('\n'):
	case TEXT('\v'):
	case TEXT('\f'):
	    return;
	case TEXT('\\'):
	case TEXT('/'):
	    relativePath = true;
	    // fall-through
	default:
	    lpStr++;
	}
    while (true);
}

bool parseFirstCommandLineArgument(vector<TCHAR> &szCommandPath, LPTSTR &lpCmdLine)
{
    bool quotedName = false;
    bool relativePath = false;
    LPTSTR szCmdName = nullptr;

    skipWhiteSpace(lpCmdLine);

    if (*lpCmdLine)
	if (*lpCmdLine == TEXT('"'))
	{
	    lpCmdLine++;
	    szCmdName = lpCmdLine;
	    quotedName = true;

	    skipQuotedWord(lpCmdLine, relativePath);
	}
	else
	{
	    szCmdName = lpCmdLine;
	    skipWord(lpCmdLine, relativePath);
	}
    else
	return false;

    if (relativePath)
	szCommandPath.assign(szCmdName, lpCmdLine);
    else
	szCommandPath.insert(szCommandPath.end(), szCmdName, lpCmdLine);

    szCommandPath.push_back(TEXT('\0'));

    if (quotedName && *lpCmdLine)
	lpCmdLine++;

    skipWhiteSpace(lpCmdLine);

    return true;
}

int shellExecute(vector<TCHAR> &szFullPathName, LPTSTR lpCmdLine)
{
    SHELLEXECUTEINFO shellExecuteInfo { sizeof shellExecuteInfo, SEE_MASK_DEFAULT | SEE_MASK_NOASYNC | SEE_MASK_NOCLOSEPROCESS, NULL, NULL };

    shellExecuteInfo.lpFile = szFullPathName.data();
    shellExecuteInfo.lpParameters = lpCmdLine;
    shellExecuteInfo.lpDirectory = NULL;
    shellExecuteInfo.nShow = SW_SHOW;

    DWORD dwExitCode = -1;
    HRESULT hResult = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    if (hResult == S_OK || hResult == S_FALSE)
    {
	if (::ShellExecuteEx(&shellExecuteInfo))
	{
	    if (shellExecuteInfo.hProcess)
	    {
		if (::WaitForSingleObject(shellExecuteInfo.hProcess, INFINITE) != WAIT_FAILED)
		    if (::GetExitCodeProcess(shellExecuteInfo.hProcess, &dwExitCode))
			;
		    else
			dwExitCode = ::GetLastError();
		else
		    dwExitCode = ::GetLastError();

		::CloseHandle(shellExecuteInfo.hProcess);
	    }
	    else
		dwExitCode = ERROR_SUCCESS;
	}
	else
	    dwExitCode = ::GetLastError();

	::CoUninitialize();
    }
    else
	dwExitCode = ::GetLastError();

    return dwExitCode;
}

#if defined(UNICODE) || defined(_UNICODE)
int wWinMain
#else
int WinMain
#endif
(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPTSTR    lpCmdLine,
    int       nShowCmd
    )
{
    HMODULE hExeModule = ::GetModuleHandle(NULL);

    if (hExeModule)
    {
	vector<TCHAR> szModuleFileName(128);

	while (DWORD dwCharCount = ::GetModuleFileName(hExeModule, szModuleFileName.data(), static_cast<DWORD>(szModuleFileName.size())))
	    if (dwCharCount == szModuleFileName.size())
		szModuleFileName.resize(szModuleFileName.size() * 2);
	    else
	    {
		vector<TCHAR> szFullPathName(szModuleFileName.size());
		TCHAR *lpFilePath = NULL;

		while (DWORD dwLength = ::GetFullPathName(szModuleFileName.data(), static_cast<DWORD>(szFullPathName.size()), szFullPathName.data(), &lpFilePath))
		    if (dwLength >= szFullPathName.size())
			szFullPathName.resize(szFullPathName.size() * 2);
		    else
		    {
			szFullPathName.resize(lpFilePath - szFullPathName.data());

			if (parseFirstCommandLineArgument(szFullPathName, lpCmdLine))
			{
			    return shellExecute(szFullPathName, lpCmdLine);
			}
			else
			    ::SetLastError(ERROR_INVALID_COMMAND_LINE);

			break;
		    }

		break;
	    }
    }

    return ::GetLastError();
}
