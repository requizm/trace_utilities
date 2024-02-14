#pragma once

#include <Windows.h>
#include <strsafe.h>
#include <string>

#include "plugin.h"

inline void DisplayError(LPTSTR lpszFunction)
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
		dprintf("FATAL ERROR: Unable to output error code.\n");
	}

	dprintf(TEXT("ERROR: %s\n"), (LPCTSTR)lpDisplayBuf);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
}

struct Config
{
	// global
	bool loggingEnabled = false;

    // utf16 search
	bool utf16SearchEnabled = false;
	std::wstring utf16SearchText = std::wstring(256, L'\0');
	bool utf16SearchRegistersEnabled = false;

	// utf16 search on memory
	bool utf16MemoryEnabled = false;
	ULONG_PTR utf16MemoryAddress = 0;
	ULONG_PTR utf16MemorySize = 0;
};

void saveConfig(Config config);
Config loadConfig();


