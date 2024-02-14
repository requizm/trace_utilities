#include "plugin.h"
#include <windows.h>
#include "StateManager.h"
#include "ManagerDialog.h"
#include "Config.h"
#include <filesystem>
#include <codecvt>
#include <locale>

std::wstring stringToWstring(const char* utf8Bytes)
{
	// setup converter
	using convert_type = std::codecvt_utf8<typename std::wstring::value_type>;
	std::wstring_convert<convert_type, typename std::wstring::value_type> converter;

	// use converter (.to_bytes: wstr->str, .from_bytes: str->wstr)
	return converter.from_bytes(utf8Bytes);
}

// Recursive func. Example patterns: `&".."` `&L"..."` `L"..."` `""`
std::string removeReferenceQuotes(std::string str)
{
	std::string result = str;
	if (str[0] == '&' && str[1] == '"' && str[str.length() - 1] == '"')
	{
		result = str.substr(2, str.length() - 3);
		return removeReferenceQuotes(result);
	}
	else if (str[0] == '&' && str[1] == 'L' && str[2] == '"' && str[str.length() - 1] == '"')
	{
		result = str.substr(3, str.length() - 4);
		return removeReferenceQuotes(result);
	}
	else if (str[0] == '"' && str[str.length() - 1] == '"')
	{
		result = str.substr(1, str.length() - 2);
		return removeReferenceQuotes(result);
	}
	else if (str[0] == 'L' && str[1] == '"' && str[str.length() - 1] == '"')
	{
		result = str.substr(2, str.length() - 3);
		return removeReferenceQuotes(result);
	}
	return result;
}

BOOL changeMemoryPageProtection(HANDLE hProcess, LPVOID lpAddress, SIZE_T dwSize, PDWORD lpflOldProtect) {
    return VirtualProtectEx(hProcess, lpAddress, dwSize, PAGE_EXECUTE_READWRITE, lpflOldProtect);
}

bool readProcessMemoryWideString(HANDLE hProcess, LPCVOID lpBaseAddress, LPVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesRead) {
    MEMORY_BASIC_INFORMATION mbi;
    SIZE_T bytesRead =   0;
    SIZE_T totalBytesRead =   0;

    while (totalBytesRead < nSize) {
        if (!VirtualQueryEx(hProcess, (LPVOID)(totalBytesRead + (SIZE_T)lpBaseAddress), &mbi, sizeof(MEMORY_BASIC_INFORMATION))) {
            // Handle error querying memory information
            DisplayError(TEXT("VirtualQueryEx"));
            break;
        }

        DWORD oldProtect;
        if (!changeMemoryPageProtection(hProcess, mbi.BaseAddress, mbi.RegionSize, &oldProtect)) {
            // Handle error changing memory protection
            DisplayError(TEXT("changeMemoryPageProtection"));
            break;
        }

        SIZE_T bytesToRead = min(nSize - totalBytesRead, mbi.RegionSize);
        if (!ReadProcessMemory(hProcess, (LPVOID)(totalBytesRead + (SIZE_T)lpBaseAddress), (LPVOID)((SIZE_T)lpBuffer + totalBytesRead), bytesToRead, &bytesRead)) {
            // Handle error reading the memory
            DisplayError(TEXT("ReadProcessMemory"));
            break;
        }

        totalBytesRead += bytesRead;

        // Restore the original memory protection
        if (!VirtualProtectEx(hProcess, mbi.BaseAddress, mbi.RegionSize, oldProtect, &oldProtect)) {
            // Handle error restoring memory protection
            DisplayError(TEXT("VirtualProtectEx"));
            break;
        }
    }

    *lpNumberOfBytesRead = totalBytesRead;
    return totalBytesRead >   0;
}


bool searchMemoryForString(std::wstring& searchString) {
	// TODO: Do not open the process each time, open when trace_into or trace_over is called
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, DbgGetProcessId());
    if (!hProcess) {
        // Failed to open the process
		DisplayError(TEXT("OpenProcess"));
        return false;
    }

	StateManager& stateManager = StateManager::getInstance();
	bool logginEnabled = stateManager.getConfig().loggingEnabled;


	LPCVOID startAddress = (LPCVOID)stateManager.getConfig().utf16MemoryAddress;
	SIZE_T size = stateManager.getConfig().utf16MemorySize;

	if (logginEnabled)
	{
		dprintf("searchMemoryForString: addr: %p, size: %p\n", startAddress, size);
	}

    // Convert the search string to a vector of wchar_t for comparison
    std::vector<wchar_t> searchVector(searchString.begin(), searchString.end());


    // Buffer to hold the memory contents
    std::vector<wchar_t> buffer(size / sizeof(wchar_t));

    // Read the memory from the process
    SIZE_T bytesRead;
    if (!readProcessMemoryWideString(hProcess, startAddress, buffer.data(), size, &bytesRead)) {
        CloseHandle(hProcess);
        return false;
    }

    // Use std::search to find the sequence in the memory range
    auto found = std::search(buffer.begin(), buffer.end(), searchVector.begin(), searchVector.end());

    // Close the process handle
    CloseHandle(hProcess);

    // Check if the sequence was found
    bool foundBool = found != buffer.end();
	if (foundBool) {
		dprintf("Found the string on the address: %p\n", stateManager.getConfig().utf16MemoryAddress + (found - buffer.begin()));
	}

	return foundBool;
}

// Why not DbgGetStringAt? Because I tried to it crashes in somewhere and I don't know why!
bool utf16Search(ULONG_PTR addr, std::wstring userInputWStr, bool pointer = true)
{
	if (!DbgMemIsValidReadPtr(addr))
	{
		return false;
	}
	int strLen = userInputWStr.length();

	wchar_t* wchar = new wchar_t[strLen];
	DbgMemRead(addr, wchar, strLen * sizeof(wchar_t));

	wchar_t* nullTerminatedP = new wchar_t[strLen + 1];
	memcpy(nullTerminatedP, wchar, strLen * sizeof(wchar_t));
	nullTerminatedP[strLen] = 0;

	bool logginEnabled = StateManager::getInstance().getConfig().loggingEnabled;
	if (logginEnabled)
	{
		dprintf("utf16Search: addr: %p, str: %ls, nullTerminatedP: %ls\n", addr, userInputWStr.c_str(), nullTerminatedP);
	}

	bool result = wcscmp(nullTerminatedP, userInputWStr.c_str()) == 0;
	delete[] nullTerminatedP;
	delete[] wchar;
	if (result)
	{
		return true;
	}

	// Maybe addr is a pointer to a string. Use DbgMemIsValidReadPtr and DbgMemRead again
	if (pointer)
	{
		ULONG_PTR addrP;
		if (DbgMemRead(addr, &addrP, sizeof(ULONG_PTR)))
		{
			if (DbgMemIsValidReadPtr((duint)addrP))
			{
				wchar_t* wcharP = new wchar_t[strLen];
				DbgMemRead((duint)addrP, wcharP, strLen * sizeof(wchar_t));

				wchar_t* nullTerminatedPP = new wchar_t[strLen + 1];
				memcpy(nullTerminatedPP, wcharP, strLen * sizeof(wchar_t));
				nullTerminatedPP[strLen] = 0;

				if (logginEnabled)
				{
					dprintf("utf16Search second iteration: addrP: %p, str: %ls, nullTerminatedPP: %ls\n", addrP, userInputWStr.c_str(), nullTerminatedPP);
				}

				result = wcscmp(nullTerminatedPP, userInputWStr.c_str()) == 0;
				delete[] nullTerminatedPP;
				delete[] wcharP;
				if (result)
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool utf16SearchOnRegisters(std::wstring searchStr)
{
	bool logginEnabled = StateManager::getInstance().getConfig().loggingEnabled;

	REGDUMP regdump;
	DbgGetRegDumpEx(&regdump, sizeof(regdump));

	auto& r = regdump.regcontext;
	// Check each register
	if (utf16Search(r.cax, searchStr) || utf16Search(r.cbx, searchStr) || utf16Search(r.ccx, searchStr) || utf16Search(r.cdx, searchStr) || utf16Search(r.csi, searchStr) || utf16Search(r.cdi, searchStr) || utf16Search(r.cip, searchStr) || utf16Search(r.csp, searchStr) || utf16Search(r.cbp, searchStr))
	{
		if (logginEnabled)
		{
			dprintf("utf16SearchOnRegisters: Found on register\n");
		}
		return true;
	}

	duint esp = r.csp;
	duint ebp = r.cbp;
	for (duint i = 0; i < 0x30; i += 0x4)
	{
		if (utf16Search(esp + i, searchStr))
		{
			if (logginEnabled)
			{
				dprintf("utf16SearchOnRegisters: Found on esp stack\n");
			}
			return true;
		}
		if (utf16Search(ebp + i, searchStr))
		{
			if (logginEnabled)
			{
				dprintf("utf16SearchOnRegisters: Found on ebp stack\n");
			}
			return true;
		}
	}

	return false;
}

static bool utf16SearchCommand(int argc, char** argv)
{
	if (argc < 2)
	{
		dputs("Usage: " PLUGIN_NAME "searchStrUtf16");
		return false;
	}

	std::wstring searchStr = stringToWstring(argv[1]);
	int len = searchStr.length();

	return utf16SearchOnRegisters(searchStr);
}

#define EXPAND(x) L##x
#define DOWIDEN(x) EXPAND(x)

// Initialize your plugin data here.
bool pluginInit(PLUG_INITSTRUCT* initStruct)
{
	_plugin_registercommand(pluginHandle, PLUGIN_NAME, utf16SearchCommand, true);

	std::wstring apiFile = std::wstring(MAX_PATH, L'\0');
	GetModuleFileNameW(StateManager::getInstance().getHInstance(), &apiFile[0], apiFile.size());
	std::filesystem::path filePath(apiFile);
	filePath.remove_filename();
	filePath /= std::wstring(DOWIDEN(PLUGIN_NAME)) + L".ini";
	apiFile = filePath.wstring();
	StateManager::getInstance().setApiFile(apiFile);
	loadConfig();

	// Return false to cancel loading the plugin.
	return true;
}

#define MENU_OPTIONS 0

// Do GUI/Menu related things here.
// This code runs on the GUI thread: GetCurrentThreadId() == GuiGetMainThreadId()
// You can get the HWND using GuiGetWindowHandle()
void pluginSetup()
{
	_plugin_menuaddentry(hMenu, MENU_OPTIONS, "&Options");

	dprintf("pluginSetup(pluginHandle: %d)\n", pluginHandle);
}

PLUG_EXPORT CDECL void CBMENUENTRY(CBTYPE cbType, void* callbackInfo)
{
	PLUG_CB_MENUENTRY* info = (PLUG_CB_MENUENTRY*)callbackInfo;

	switch (info->hEntry)
	{
	case MENU_OPTIONS:
		ManagerDialog* manager = new ManagerDialog();
		break;
	}
}

PLUG_EXPORT void CBTRACEEXECUTE(CBTYPE cbType, PLUG_CB_TRACEEXECUTE* info)
{
	StateManager& stateManager = StateManager::getInstance();
	if (!stateManager.getConfig().utf16SearchEnabled)
	{
		return;
	}

	std::wstring searchStr = stateManager.getConfig().utf16SearchText;

	if (stateManager.getConfig().utf16SearchRegistersEnabled)
	{
		bool foundInRegisters = utf16SearchOnRegisters(searchStr);
		if (foundInRegisters)
		{
			info->stop = true;
			return;
		}
	}

	if (stateManager.getConfig().utf16MemoryEnabled)
	{
		bool foundInMemory = searchMemoryForString(searchStr);
		if (foundInMemory)
		{
			info->stop = true;
			return;
		}
	}
}

BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		StateManager::getInstance().setHInstance(hinstDLL);
	}

	return TRUE;
}
