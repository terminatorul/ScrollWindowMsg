#include <Windows.h>

#include <windef.h>
#include <WinBase.h>
#include <WinUser.h>
#include <Psapi.h>
#include <tchar.h>

#include <cstddef>
#include <string>
#include <iterator>
#include <vector>

#include "RemoteProcessSearch.hpp"

#if defined(DEBUG) || defined(_DEBUG)
using std::to_wstring;
#endif

using std::size_t;
using std::move;
using std::size;
using std::max;
using std::basic_string;
using std::stoul;
using std::string;
using std::vector;

//
// Adapted from RemoteOps.cpp, from codeproject.com article by Mr Nukealizer
//

HMODULE GetRemoteModuleHandle(HANDLE hProcess, LPCTSTR lpModuleName)
{
    vector<HMODULE> ModuleArray(100U);

    DWORD NumModules = 0;
    vector<TCHAR> moduleNameStr(lpModuleName, lpModuleName + lstrlen(lpModuleName));
    vector<TCHAR> moduleNameBuffer(moduleNameStr.size() + size(TEXT(".dll")));

    CharLowerBuff(moduleNameStr.data(), static_cast<DWORD>(moduleNameStr.size()));

    if (!::EnumProcessModulesEx(hProcess, ModuleArray.data(), static_cast<DWORD>(ModuleArray.size() * sizeof(ModuleArray[0])), &NumModules, LIST_MODULES_ALL))
	return NULL;

    NumModules /= sizeof(HMODULE);

    if (NumModules > ModuleArray.size())
    {
	ModuleArray.resize(NumModules);

	if (!::EnumProcessModulesEx(hProcess, ModuleArray.data(), static_cast<DWORD>(ModuleArray.size() * sizeof(ModuleArray[0])), &NumModules, LIST_MODULES_ALL))
	    return NULL;

	NumModules /= sizeof(HMODULE);
    }

    ModuleArray.resize(NumModules);

    auto it = std::find_if(ModuleArray.cbegin(), ModuleArray.cend(), [hProcess, &moduleNameBuffer, &moduleNameStr](auto const& hModule) -> bool
	{
	    DWORD nSize = ::GetModuleBaseName(hProcess, hModule, moduleNameBuffer.data(), static_cast<DWORD>(moduleNameBuffer.size()));

	    if (!nSize)
		return false;

	    CharLowerBuff(moduleNameBuffer.data(), nSize);

	    return moduleNameStr.size() <= nSize && std::equal(moduleNameStr.begin(), moduleNameStr.end(), moduleNameBuffer.begin());
	});

    if (it == ModuleArray.end())
    {
	::SetLastError(ERROR_MOD_NOT_FOUND);
	return NULL;
    }
#if defined(DEBUG) || defined(_DEBUG)
    else
    {
	OutputDebugString((TEXT("Found remote process module ") + basic_string<TCHAR>(moduleNameBuffer.begin(), moduleNameBuffer.end())).c_str());
    }
#endif

    return *it;
}

#if defined(_MSC_VER)
# pragma warning(push)
# pragma warning(disable: 4996)		    // This function or variable may be unsafe
#endif

static bool ReadProcessString(HANDLE hProcess, UINT_PTR uStartVA, basic_string<TCHAR> &remoteString)
{
    CHAR nextChar;
    UINT i = 0U;
    basic_string<CHAR> partialString;

    /* Get the forwarder string one character at a time because we don't know how long it is */
    do
    {
	/* Get next character */
	if (!::ReadProcessMemory(hProcess, (LPCVOID)(uStartVA + i++), &nextChar, sizeof(nextChar), NULL))
	    return false;

	partialString.push_back(nextChar);
    }
    while (nextChar);

    partialString.pop_back();

    if constexpr (sizeof(TCHAR) > sizeof(CHAR))
    {
	vector<TCHAR> wcsString(partialString.size());
	std::mbstate_t mbstate { };
	CHAR const *pSrc = partialString.c_str();

	size_t wcsLen = mbsrtowcs(wcsString.data(), &pSrc, wcsString.size(), &mbstate);

	if (wcsLen == static_cast<size_t>(-1))
	{
	    ::SetLastError(ERROR_NO_UNICODE_TRANSLATION);
	    return false;
	}

	remoteString.assign(wcsString.begin(), wcsString.begin() + wcsLen);
    }

#if !defined(UNICODE) && !defined(_UNICODE)
    if constexpr (sizeof(TCHAR) == sizeof(CHAR))
    {
	remoteString = move(partialString);
    }
#endif

    return true;
}

#if defined(_MSC_VER)
# pragma warning(pop)
#endif

static bool ReadImageExportDirectory
    (
	HANDLE		      hProcess,
	UINT_PTR	      uFileHeaderVA,
	IMAGE_FILE_HEADER    &FileHeader,
	IMAGE_DATA_DIRECTORY &ExportDirectory
    )
{
    /* Which type of optional header is the right size? */
    IMAGE_OPTIONAL_HEADER64 OptHeader64 = { 0 };

    if (FileHeader.SizeOfOptionalHeader == sizeof(OptHeader64))
    {
	/* Read the optional header and check it's magic number */
	if (!::ReadProcessMemory(hProcess, (LPCVOID)(uFileHeaderVA + sizeof(FileHeader)), &OptHeader64, FileHeader.SizeOfOptionalHeader, NULL))
	{
	    return false;
	}
	else
	    if (OptHeader64.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
	    {
		::SetLastError(ERROR_BAD_FORMAT);
		return false;
	    }

	if (OptHeader64.NumberOfRvaAndSizes >= IMAGE_DIRECTORY_ENTRY_EXPORT + 1)
	{
	    ExportDirectory.VirtualAddress = (OptHeader64.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]).VirtualAddress;
	    ExportDirectory.Size = (OptHeader64.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]).Size;
	}
	else
	{
	    ::SetLastError(ERROR_BAD_FORMAT);
	    return false;
	}
    }
    else
    {
	IMAGE_OPTIONAL_HEADER32 OptHeader32 = { 0 };

	if (FileHeader.SizeOfOptionalHeader == sizeof(OptHeader32))
	{
	    /* Read the optional header and check it's magic number */
	    if (!::ReadProcessMemory(hProcess, (LPCVOID)(uFileHeaderVA + sizeof(FileHeader)), &OptHeader32, FileHeader.SizeOfOptionalHeader, NULL))
	    {
		return false;
	    }
	    else
		if (OptHeader32.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
		{
		    ::SetLastError(ERROR_BAD_FORMAT);
		    return false;
		}

	    if (OptHeader32.NumberOfRvaAndSizes >= IMAGE_DIRECTORY_ENTRY_EXPORT + 1)
	    {
		ExportDirectory.VirtualAddress = (OptHeader32.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]).VirtualAddress;
		ExportDirectory.Size = (OptHeader32.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]).Size;
	    }
	    else
	    {
		::SetLastError(ERROR_BAD_FORMAT);
		return false;
	    }
	}
	else
	{
	    ::SetLastError(ERROR_BAD_FORMAT);
	    return false;
	}
    }

    return true;
}

static bool LoadModuleExportDirectory(HANDLE hProcess, HMODULE hModule, UINT_PTR &RemoteModuleBaseVA, IMAGE_DATA_DIRECTORY &ExportDirectory)
{
    MODULEINFO RemoteModuleInfo = { 0 };

    /* Get the base address of the remote module along with some other info we don't need */
    if (!::GetModuleInformation(hProcess, hModule, &RemoteModuleInfo, sizeof(RemoteModuleInfo)))
	return false;

    RemoteModuleBaseVA = (UINT_PTR)RemoteModuleInfo.lpBaseOfDll;

    /* Read the DOS header and check it's magic number */
    IMAGE_DOS_HEADER DosHeader = { 0 };

    if (!::ReadProcessMemory(hProcess, (LPCVOID)RemoteModuleBaseVA, &DosHeader, sizeof(DosHeader), NULL) || DosHeader.e_magic != IMAGE_DOS_SIGNATURE)
	return false;

    DWORD Signature = 0;

    if (!::ReadProcessMemory(hProcess, (LPCVOID)(RemoteModuleBaseVA + DosHeader.e_lfanew), &Signature, sizeof(Signature), NULL))
	return false;
    else
	if (Signature != IMAGE_NT_SIGNATURE)
	{
	    ::SetLastError(ERROR_BAD_FORMAT);
	    return false;
	}

    IMAGE_FILE_HEADER FileHeader = { 0 };

    if (!::ReadProcessMemory(hProcess, (LPCVOID)(RemoteModuleBaseVA + DosHeader.e_lfanew + sizeof(Signature)), &FileHeader, sizeof(FileHeader), NULL))
	return false;

    return ReadImageExportDirectory(hProcess, RemoteModuleBaseVA + DosHeader.e_lfanew + sizeof(Signature) , FileHeader, ExportDirectory);
}

FARPROC GetRemoteForwardedFunction(HANDLE hProcess, UINT_PTR uForwardEntryStringAddress)
{
    basic_string<TCHAR> forwardString;

    if (!ReadProcessString(hProcess, uForwardEntryStringAddress, forwardString))
        return nullptr;

    /* Find the dot that seperates the module name and the function name/ordinal */
    size_t dotIndex = forwardString.find(TEXT('.'));
    if (dotIndex == forwardString.npos)
    {
	::SetLastError(ERROR_BAD_FORMAT);
	return nullptr;
    }

    /* Temporary variables that hold parts of the forwarder string */
    basic_string<TCHAR>
	realModuleName = forwardString.substr(0, dotIndex),
	realFunctionId = forwardString.substr(dotIndex + 1, forwardString.npos);

    HMODULE hRealModule = GetRemoteModuleHandle(hProcess, realModuleName.c_str());

    if (*realFunctionId.cbegin() == TEXT('#')) // Exported by ordinal
    {
	realFunctionId.erase(0, 1);
	return GetRemoteProcAddress(hProcess, hRealModule, nullptr, stoul(realFunctionId));
    }

    return GetRemoteProcAddress(hProcess, hRealModule, realFunctionId.c_str(), 0);
}

static FARPROC GetRemoteOrdinalProcAddres(HANDLE hProcess, UINT Ordinal, IMAGE_DATA_DIRECTORY const &ExportDirectory, IMAGE_EXPORT_DIRECTORY const &ExportTable, vector<DWORD> const &ExportFunctionTable, UINT_PTR RemoteModuleBaseVA)
{
    /* NOTE: Microsoft's PE/COFF specification does NOT say we need to subtract the ordinal base
    /* from our ordinal but it seems to always give the wrong function if we don't */

    /* Make sure the ordinal is valid */
    if (Ordinal < ExportTable.Base || (Ordinal - ExportTable.Base) >= ExportTable.NumberOfFunctions)
    {
	::SetLastError(ERROR_INVALID_ORDINAL);
	return nullptr;
    }

    UINT FunctionTableIndex = Ordinal - ExportTable.Base;

    /* Check if the function is forwarded and if so get the real address*/
    if (ExportFunctionTable[FunctionTableIndex] >= ExportDirectory.VirtualAddress &&
	ExportFunctionTable[FunctionTableIndex] <= ExportDirectory.VirtualAddress + ExportDirectory.Size)
    {
	return GetRemoteForwardedFunction(hProcess, RemoteModuleBaseVA + ExportFunctionTable[FunctionTableIndex]);
    }
    else // Not Forwarded
    {
	return (FARPROC)(RemoteModuleBaseVA + ExportFunctionTable[FunctionTableIndex]);
    }
}

static FARPROC GetRemoteNamedProcAddress(HANDLE hProcess, LPCTSTR lpProcName, IMAGE_DATA_DIRECTORY const &ExportDirectory, IMAGE_EXPORT_DIRECTORY const &ExportTable, vector<DWORD> const &ExportFunctionTable, UINT_PTR RemoteModuleBaseVA)
{
    basic_string<TCHAR> functionName;
    DWORD procNameLength = lstrlen(lpProcName);

    UINT_PTR ExportNameTableVA = RemoteModuleBaseVA + ExportTable.AddressOfNames;
    vector<DWORD> ExportNameTable(ExportTable.NumberOfNames);
    if (!::ReadProcessMemory(hProcess, (LPCVOID)ExportNameTableVA, ExportNameTable.data(), ExportNameTable.size() * sizeof(DWORD), NULL))
	return nullptr;

    UINT_PTR ExportOrdinalTableVA = RemoteModuleBaseVA + ExportTable.AddressOfNameOrdinals;
    vector<WORD>  ExportOrdinalTable(ExportTable.NumberOfNames);
    if (!::ReadProcessMemory(hProcess, (LPCVOID)ExportOrdinalTableVA, ExportOrdinalTable.data(), ExportOrdinalTable.size() * sizeof(WORD), NULL))
	return nullptr;

    /* Iterate through all the names and see if they match the one we are looking for */
    for (DWORD i = 0; i < ExportTable.NumberOfNames; ++i)
    {

	if (!ReadProcessString(hProcess, RemoteModuleBaseVA + ExportNameTable[i], functionName))
	    return nullptr;

	if (!functionName.find(lpProcName) && (functionName.size() == procNameLength || functionName[procNameLength] == TEXT('.')))
	{
	    /* NOTE: Microsoft's PE/COFF specification says we need to subtract the ordinal base
	    /* from the value in the ordinal table, but that seems to always give the wrong function. */

	    /* Check if the function is forwarded and if so get the real address*/
	    if (ExportFunctionTable[ExportOrdinalTable[i]] >= ExportDirectory.VirtualAddress &&
		ExportFunctionTable[ExportOrdinalTable[i]] <= ExportDirectory.VirtualAddress + ExportDirectory.Size)
	    {
		return GetRemoteForwardedFunction(hProcess, RemoteModuleBaseVA + ExportFunctionTable[i]);
	    }
	    else
	    {
		/* NOTE:
		/* Microsoft's PE/COFF specification says we need to subtract the ordinal base
		/*from the value in the ordinal table but that seems to always give the wrong function */
		//TempReturn = (FARPROC)(RemoteModuleBaseVA + ExportFunctionTable[ExportOrdinalTable[i] - ExportTable.Base]);

		/* So we do it this way instead */

    #if defined(DEBUG) || defined(_DEBUG)
		OutputDebugString((TEXT("Found remote process exported function ") + functionName + TEXT(" at address ") + std::to_wstring((uintptr_t)(RemoteModuleBaseVA + ExportFunctionTable[ExportOrdinalTable[i]]))).c_str());
    #endif
		return (FARPROC)(RemoteModuleBaseVA + ExportFunctionTable[ExportOrdinalTable[i]]);
	    }
	}
    }

    ::SetLastError(ERROR_PROC_NOT_FOUND);

    return nullptr;
}

FARPROC GetRemoteProcAddress(HANDLE hProcess, HMODULE hModule, LPCTSTR lpProcName, UINT Ordinal)
{
    UINT_PTR RemoteModuleBaseVA = 0;
    IMAGE_DATA_DIRECTORY ExportDirectory = { 0 };
    IMAGE_EXPORT_DIRECTORY ExportTable = { 0 };

    if (!LoadModuleExportDirectory(hProcess, hModule, RemoteModuleBaseVA, ExportDirectory))
	return nullptr;

    /* Read the main export table */
    if (!::ReadProcessMemory(hProcess, (LPCVOID)(RemoteModuleBaseVA + ExportDirectory.VirtualAddress), &ExportTable, sizeof(ExportTable), NULL))
	return nullptr;

    /* Save the absolute address of the tables so we don't need to keep adding the base address */
    UINT_PTR ExportFunctionTableVA = RemoteModuleBaseVA + ExportTable.AddressOfFunctions;

    vector<DWORD> ExportFunctionTable(ExportTable.NumberOfFunctions);

    /* Get a copy of the function table */
    if (!::ReadProcessMemory(hProcess, (LPCVOID)ExportFunctionTableVA, ExportFunctionTable.data(), ExportFunctionTable.size() * sizeof(DWORD), NULL))
	return nullptr;

    /* If we are searching for an ordinal we do that now */
    if (lpProcName)
	return GetRemoteNamedProcAddress(hProcess, lpProcName, ExportDirectory, ExportTable, ExportFunctionTable, RemoteModuleBaseVA);

    return GetRemoteOrdinalProcAddres(hProcess, Ordinal, ExportDirectory, ExportTable, ExportFunctionTable, RemoteModuleBaseVA);
}