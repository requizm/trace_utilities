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
	bool logginEnabled = config.loggingEnabled;


	auto startAddress = (duint)config.utf16MemoryAddress;
	SIZE_T size = config.utf16MemorySize;

	if (logginEnabled)
	{
		dprintf("searchMemoryForString: addr: %p, size: %p\n", startAddress, size);
	}

	// Buffer to hold the memory contents
	std::vector<wchar_t> buffer(size / sizeof(wchar_t));

	// Read the memory from the process
	if (!DbgMemRead(startAddress, buffer.data(), size)) {
		return false;
	}

	// Use std::search to find the sequence in the memory range
	auto found = std::search(buffer.begin(), buffer.end(), searchString.begin(), searchString.end());

	// Check if the sequence was found
	bool foundBool = found != buffer.end();
	if (foundBool) {
		dprintf("Found the string on the address: %p\n", (void*)(startAddress + (found - buffer.begin()) * sizeof(wchar_t)));
	}

	return foundBool;
}

// Why not DbgGetStringAt? Because I tried to it crashes in somewhere and I don't know why!
bool utf16Search(duint addr, const std::wstring& userInputWStr, bool pointer = true)
{
	if (!DbgMemIsValidReadPtr(addr))
	{
		return false;
	}
	const auto strLen = userInputWStr.length();
	const auto bufSize = (strLen + 1) * sizeof(wchar_t);
	std::vector<wchar_t> buffer(strLen + 1);

	if (!DbgMemRead(addr, buffer.data(), strLen * sizeof(wchar_t)))
		return false;

	buffer[strLen] = L'\0';

	bool logginEnabled = StateManager::getInstance().getConfig().loggingEnabled;
	if (logginEnabled)
	{
		dprintf("utf16Search: addr: %p, str: %ls, buffer: %ls\n", addr, userInputWStr.c_str(), buffer.data());
	}

	if (wcscmp(buffer.data(), userInputWStr.c_str()) == 0) {
		dprintf("utf16Search found match: addr: %p, str: %ls, buffer: %ls\n", addr, userInputWStr.c_str(), buffer.data());
		return true;
	}

	// Maybe addr is a pointer to a string. Use DbgMemIsValidReadPtr and DbgMemRead again
	if (pointer)
	{
		duint addrP = 0;
		if (DbgMemRead(addr, &addrP, sizeof(addrP)))
		{
			if (DbgMemIsValidReadPtr(addrP))
			{
				if (!DbgMemRead(addrP, buffer.data(), strLen * sizeof(wchar_t)))
					return false;

				buffer[strLen] = L'\0';

				if (logginEnabled)
				{
					dprintf("utf16Search second iteration: addrP: %p, str: %ls, buffer: %ls\n", addrP, userInputWStr.c_str(), buffer.data());
				}

				if (wcscmp(buffer.data(), userInputWStr.c_str()) == 0) {
					dprintf("utf16Search found match: addrP: %p, str: %ls, buffer: %ls\n", addrP, userInputWStr.c_str(), buffer.data());
					return true;
				}
			}
		}
	}

	return false;
}

bool utf16SearchOnRegisters(const std::wstring& searchStr)
{
	bool logginEnabled = StateManager::getInstance().getConfig().loggingEnabled;

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
			dprintf("utf16SearchOnRegisters: Found on register %s\n", reg.first);
			return true;
		}
	}

	duint esp = r.csp;
	duint ebp = r.cbp;
	for (duint i = 0; i < 0x30; i += sizeof(duint))
	{
		duint currentStackAddress = 0;
		if (DbgMemRead(esp + i, &currentStackAddress, sizeof(currentStackAddress)) && utf16Search(currentStackAddress, searchStr, false))
		{
			dprintf("utf16SearchOnRegisters: Found on stack at [esp+%X]\n", i);
			return true;
		}
		if (DbgMemRead(ebp + i, &currentStackAddress, sizeof(currentStackAddress)) && utf16Search(currentStackAddress, searchStr, false))
		{
			dprintf("utf16SearchOnRegisters: Found on stack at [ebp+%X]\n", i);
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
	if (!config.utf16SearchEnabled)
	{
		return;
	}

	const auto& searchStr = config.utf16SearchText;

	if (config.utf16SearchRegistersEnabled)
	{
		if (utf16SearchOnRegisters(searchStr))
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
