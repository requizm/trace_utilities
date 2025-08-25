#include "plugin.h"
#include <windows.h>
#include "StateManager.h"
#include "ManagerDialog.h"
#include "Config.h"
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <cwctype>

std::wstring stringToWstring(const char* utf8Bytes)
{
	int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8Bytes, -1, NULL, 0);
	if (wlen == 0)
		return {};
	std::wstring wstr(wlen - 1, 0);
	MultiByteToWideChar(CP_UTF8, 0, utf8Bytes, -1, &wstr[0], wlen);
	return wstr;
}

bool searchMemoryForString(const std::wstring& searchString) {
	StateManager& stateManager = StateManager::getInstance();
	const auto& config = stateManager.getConfig();
	if (searchString.empty()) return false;

	duint startAddress = config.utf16MemoryAddress;
	duint totalSize = config.utf16MemorySize;

	// Prepare the search term based on the mode.
	// For "Equals" mode, we search for the exact string followed by a null terminator.
	std::wstring effectiveSearchString = searchString;
	if (!config.utf16SearchModeContains) {
		effectiveSearchString += L'\0';
	}

	duint searchStrBytes = effectiveSearchString.length() * sizeof(wchar_t);
	if (searchStrBytes == 0) return false;

	// Scan the memory region in chunks for performance and to handle large regions.
	const duint CHUNK_SIZE = 4 * 1024 * 1024; // 4MB chunks
	std::vector<wchar_t> buffer(CHUNK_SIZE / sizeof(wchar_t));
	duint readOffset = 0;

	while (readOffset < totalSize) {
		duint currentAddress = startAddress + readOffset;
		duint bytesToRead = min(CHUNK_SIZE, totalSize - readOffset);

		if (config.loggingEnabled) {
			dprintf("Scanning chunk at %p, size %p\n", (void*)currentAddress, (void*)bytesToRead);
		}

		// Don't bother reading if the remaining block is smaller than our search term
		if (bytesToRead < searchStrBytes) {
			break;
		}

		if (!DbgMemRead(currentAddress, buffer.data(), bytesToRead)) {
			// Skip unreadable pages by jumping to the next page boundary
			duint nextPage = (currentAddress / 4096 + 1) * 4096;
			readOffset = (nextPage > currentAddress) ? (nextPage - startAddress) : (readOffset + 4096);
			continue;
		}
		
		size_t bufferLen = bytesToRead / sizeof(wchar_t);
		auto bufferBegin = buffer.begin();
		auto bufferEnd = buffer.begin() + bufferLen;

		auto it = bufferEnd;
		if (config.utf16SearchCaseSensitive) {
			it = std::search(bufferBegin, bufferEnd, effectiveSearchString.begin(), effectiveSearchString.end());
		} else {
			it = std::search(bufferBegin, bufferEnd, effectiveSearchString.begin(), effectiveSearchString.end(),
				[](wchar_t ch1, wchar_t ch2) { return std::towlower(ch1) == std::towlower(ch2); });
		}

		if (it != bufferEnd) {
			duint foundAddress = currentAddress + (std::distance(bufferBegin, it) * sizeof(wchar_t));
			dprintf("Found the string at address: %p\n", (void*)foundAddress);
			return true;
		}

		// To avoid missing a match that spans a chunk boundary, we overlap the reads
		// by the length of the search string, minus one character for efficiency.
		if (readOffset + bytesToRead < totalSize) {
			readOffset += bytesToRead - (searchStrBytes > sizeof(wchar_t) ? searchStrBytes - sizeof(wchar_t) : 0);
		} else {
			readOffset += bytesToRead;
		}
	}

	return false;
}


// This function checks if an address contains a UTF-16 string that matches user input
// based on current settings (contains/equals, case-sensitive).
bool utf16SearchAt(duint addr, const std::wstring& userInputWStr)
{
	if (!DbgMemIsValidReadPtr(addr))
	{
		return false;
	}

	StateManager& stateManager = StateManager::getInstance();
	const auto& config = stateManager.getConfig();

	// Read a reasonable amount of memory to capture a potentially null-terminated string.
	// This is a heuristic for checking registers/stack, not for full memory scans.
	const size_t max_read_len = 512;
	std::vector<wchar_t> buffer(max_read_len);
	if (!DbgMemRead(addr, buffer.data(), max_read_len * sizeof(wchar_t)))
	{
		return false;
	}
	buffer.back() = L'\0'; // Ensure null-termination for safety

	std::wstring memStr(buffer.data()); // Creates string up to first null character

	if (config.loggingEnabled)
	{
		dprintf("utf16SearchAt: addr: %p, searching for '%ls' in '%ls'\n", (void*)addr, userInputWStr.c_str(), memStr.c_str());
	}

	std::wstring haystack = memStr;
	std::wstring needle = userInputWStr;

	if (!config.utf16SearchCaseSensitive)
	{
		std::transform(haystack.begin(), haystack.end(), haystack.begin(), ::towlower);
		std::transform(needle.begin(), needle.end(), needle.begin(), ::towlower);
	}

	bool match = false;
	if (config.utf16SearchModeContains)
	{
		if (haystack.find(needle) != std::wstring::npos)
		{
			match = true;
		}
	}
	else // Equals mode
	{
		if (haystack == needle)
		{
			match = true;
		}
	}

	if (match)
	{
		dprintf("utf16Search: Found match at address %p. User string: '%ls', Memory string: '%ls'\n", (void*)addr, userInputWStr.c_str(), memStr.c_str());
	}

	return match;
}

// This function checks an address, and if it's a pointer, it checks the pointed-to address.
bool utf16Search(duint addr, const std::wstring& userInputWStr, bool pointer = true)
{
	if (utf16SearchAt(addr, userInputWStr))
	{
		return true;
	}

	// Maybe addr is a pointer to a string. Use DbgMemIsValidReadPtr and DbgMemRead again
	if (pointer)
	{
		duint addrP = 0;
		if (DbgMemRead(addr, &addrP, sizeof(addrP)))
		{
			if (utf16SearchAt(addrP, userInputWStr))
			{
				dprintf("Found via pointer at %p -> %p\n", (void*)addr, (void*)addrP);
				return true;
			}
		}
	}

	return false;
}

bool searchOnRegisters(const std::wstring& searchStr)
{
	REGDUMP regdump;
	DbgGetRegDumpEx(&regdump, sizeof(regdump));

	const auto& r = regdump.regcontext;
	const std::pair<const char*, duint> registers[] = {
		{"cax", r.cax}, {"cbx", r.cbx}, {"ccx", r.ccx}, {"cdx", r.cdx},
		{"csi", r.csi}, {"cdi", r.cdi}, {"cip", r.cip}, {"csp", r.csp},
		{"cbp", r.cbp}
	};

	for (const auto& reg : registers) {
		if (utf16Search(reg.second, searchStr)) {
			dprintf("searchOnRegisters: Found on register %s\n", reg.first);
			return true;
		}
	}
	return false;
}

bool searchOnStack(const std::wstring& searchStr)
{
	REGDUMP regdump;
	DbgGetRegDumpEx(&regdump, sizeof(regdump));
	const auto& r = regdump.regcontext;

	duint esp = r.csp;
	duint ebp = r.cbp;
	for (duint i = 0; i < 0x30; i += sizeof(duint))
	{
		duint currentStackAddress = 0;
		if (DbgMemRead(esp + i, &currentStackAddress, sizeof(currentStackAddress)) && utf16Search(currentStackAddress, searchStr, false))
		{
			dprintf("searchOnStack: Found on stack at [esp+%X]\n", i);
			return true;
		}
		if (DbgMemRead(ebp + i, &currentStackAddress, sizeof(currentStackAddress)) && utf16Search(currentStackAddress, searchStr, false))
		{
			dprintf("searchOnStack: Found on stack at [ebp+%X]\n", i);
			return true;
		}
	}

	return false;
}

static bool utf16SearchCommand(int argc, char** argv)
{
	if (argc < 2)
	{
		dputs("Usage: " PLUGIN_NAME " searchStrUtf16");
		return false;
	}

	std::wstring searchStr = stringToWstring(argv[1]);
	
	if (searchOnRegisters(searchStr)) return true;
	if (searchOnStack(searchStr)) return true;

	return false;
}

#define EXPAND(x) L##x
#define DOWIDEN(x) EXPAND(x)

// Initialize your plugin data here.
bool pluginInit(PLUG_INITSTRUCT* initStruct)
{
	_plugin_registercommand(pluginHandle, PLUGIN_NAME, utf16SearchCommand, true);

	std::wstring apiFile(MAX_PATH, L'\0');
	apiFile.resize(GetModuleFileNameW(StateManager::getInstance().getHInstance(), &apiFile[0], MAX_PATH));
	std::filesystem::path filePath(apiFile);
	filePath.remove_filename();
	filePath /= std::wstring(DOWIDEN(PLUGIN_NAME)) + L".ini";
	StateManager::getInstance().setApiFile(filePath.wstring());
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
	const auto& config = stateManager.getConfig();
	if (!config.utf16SearchEnabled || config.utf16SearchText.empty())
	{
		return;
	}

	const auto& searchStr = config.utf16SearchText;

	if (config.utf16SearchRegistersEnabled)
	{
		if (searchOnRegisters(searchStr))
		{
			info->stop = true;
			return;
		}
	}
    
	if (config.utf16SearchStackEnabled)
	{
		if (searchOnStack(searchStr))
		{
			info->stop = true;
			return;
		}
	}

	if (config.utf16MemoryEnabled)
	{
		if (searchMemoryForString(searchStr))
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
