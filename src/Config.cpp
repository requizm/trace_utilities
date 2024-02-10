#include "Config.h"
#include <windows.h>
#include <string>
#include "plugin.h"
#include <strsafe.h>
#include "StateManager.h"

void DisplayError(LPTSTR lpszFunction)
// Routine Description:
// Retrieve and output the system error message for the last-error code
{
	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0,
		NULL);

	lpDisplayBuf =
		(LPVOID)LocalAlloc(LMEM_ZEROINIT,
			(lstrlen((LPCTSTR)lpMsgBuf)
				+ lstrlen((LPCTSTR)lpszFunction)
				+ 40) // account for format string
			* sizeof(TCHAR));

	if (FAILED(StringCchPrintf((LPTSTR)lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error code %d as follows:\n%s"),
		lpszFunction,
		dw,
		lpMsgBuf)))
	{
		_plugin_logprintf("FATAL ERROR: Unable to output error code.\n");
	}

	_plugin_logprintf(TEXT("ERROR: %s\n"), (LPCTSTR)lpDisplayBuf);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
}

void saveConfig(Config config)
{
	std::wstring apiFile = StateManager::getInstance().getApiFile();
	auto hFile = CreateFileW(apiFile.c_str(), GENERIC_WRITE | GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		DisplayError(TEXT("CreateFile"));
		return;
	}

	auto utf16Enabled = config.utf16Enabled ? 1 : 0;
	if (!WritePrivateProfileStringW(L"Config", L"utf16Enabled", std::to_wstring(utf16Enabled).c_str(), apiFile.c_str()))
	{
		DisplayError(TEXT("WritePrivateProfileString(utf16Enabled"));
		CloseHandle(hFile);
		return;
	}
	if (!WritePrivateProfileStringW(L"Config", L"utf16Text", config.utf16Text.c_str(), apiFile.c_str()))
	{
		DisplayError(TEXT("WritePrivateProfileStringutf16Text(utf16Text)"));
		CloseHandle(hFile);
		return;
	}
	auto loggingEnabled = config.loggingEnabled ? 1 : 0;
	if (!WritePrivateProfileStringW(L"Config", L"loggingEnabled", std::to_wstring(loggingEnabled).c_str(), apiFile.c_str()))
	{
		DisplayError(TEXT("WritePrivateProfileString(loggingEnabled)"));
		CloseHandle(hFile);
		return;
	}

	CloseHandle(hFile);
}

Config loadConfig()
{
	std::wstring apiFile = StateManager::getInstance().getApiFile();
	auto hFile = CreateFileW(apiFile.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		saveConfig(Config());
		return Config();
	}

	DWORD dwFileSize = GetFileSize(hFile, NULL);
	if (dwFileSize == INVALID_FILE_SIZE)
	{
		DisplayError(TEXT("GetFileSize"));
		CloseHandle(hFile);
		return Config();
	}

	if (dwFileSize == 0)
	{
		CloseHandle(hFile);
		Config config;
		saveConfig(config);
		return config;
	}

	auto hFileMapping = CreateFileMappingW(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	if (hFileMapping == NULL)
	{
		DisplayError(TEXT("CreateFileMapping"));
		CloseHandle(hFile);
		return Config();
	}

	auto lpFileBase = MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);
	if (lpFileBase == NULL)
	{
		DisplayError(TEXT("MapViewOfFile"));
		CloseHandle(hFileMapping);
		CloseHandle(hFile);
		return Config();
	}

	auto config = Config();
	auto utf16Enabled = GetPrivateProfileIntW(L"Config", L"utf16Enabled", 0, apiFile.c_str());
	config.utf16Enabled = utf16Enabled == 1;
	auto loggingEnabled = GetPrivateProfileIntW(L"Config", L"loggingEnabled", 0, apiFile.c_str());
	config.loggingEnabled = loggingEnabled == 1;
	GetPrivateProfileStringW(L"Config", L"utf16Text", L"", &config.utf16Text[0], 256, apiFile.c_str());
	StateManager::getInstance().setConfig(config);

	UnmapViewOfFile(lpFileBase);
	CloseHandle(hFileMapping);
	CloseHandle(hFile);

	return config;
}
