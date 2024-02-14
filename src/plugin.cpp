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

bool utf16Search(ULONG_PTR addr, std::wstring userInputWStr, int strLen)
{
	if (!DbgMemIsValidReadPtr(addr))
	{
		return false;
	}

	std::string userInputStr = std::string(userInputWStr.begin(), userInputWStr.end()); // FIXME: This is not a good solution. It removes non-ascii characters.
	bool logginEnabled = StateManager::getInstance().getConfig().loggingEnabled;

	char* buffer = new char[512];
	bool strExists = DbgGetStringAt(addr, buffer);
	if (strExists)
	{
		std::string strBuffer = std::string(buffer);
		if (logginEnabled) {
			dprintf("utf16Search first iteration: addr: %p, str: %s, strBuffer: %s\n", addr, userInputStr.c_str(), strBuffer.c_str());
		}
		std::string strBufferWithoutQuotes = removeReferenceQuotes(strBuffer);
		return strBufferWithoutQuotes.find(userInputStr) != std::string::npos;
	}

	return false;
}

bool utf16SearchOnRegisters(std::wstring searchStr, int len)
{
	bool logginEnabled = StateManager::getInstance().getConfig().loggingEnabled;

	REGDUMP regdump;
	DbgGetRegDumpEx(&regdump, sizeof(regdump));

	auto& r = regdump.regcontext;
#ifdef _WIN64
	// Check each register
	if (utf16Search(r.cax, searchStr, len) || utf16Search(r.cbx, searchStr, len) || utf16Search(r.ccx, searchStr, len) || utf16Search(r.cdx, searchStr, len) || utf16Search(r.csi, searchStr, len) || utf16Search(r.cdi, searchStr, len) || utf16Search(r.cip, searchStr, len) || utf16Search(r.csp, searchStr, len) || utf16Search(r.cbp, searchStr, len))
	{
		if (logginEnabled)
		{
			dprintf("utf16SearchOnRegisters: Found on register\n");
		}
		return true;
	}

	// Check from 'csp + 0' to 'csp + 0x30' by increasing 0x4
	duint esp = r.csp;
	for (duint i = 0; i < 0x30; i += 0x4)
	{
		if (utf16Search(esp + i, searchStr, len))
		{
			if (logginEnabled)
			{
				dprintf("utf16SearchOnRegisters: Found on esp stack\n");
			}
			return true;
		}
	}
#else
	// Check each register
	if (utf16Search(r.cax, searchStr, len) || utf16Search(r.cbx, searchStr, len) || utf16Search(r.ccx, searchStr, len) || utf16Search(r.cdx, searchStr, len) || utf16Search(r.csi, searchStr, len) || utf16Search(r.cdi, searchStr, len) || utf16Search(r.cip, searchStr, len) || utf16Search(r.csp, searchStr, len) || utf16Search(r.cbp, searchStr, len))
	{
		if (logginEnabled)
		{
			dprintf("utf16SearchOnRegisters: Found on register\n");
		}
		return true;
	}

	duint esp = r.csp;
	for (duint i = 0; i < 0x30; i += 0x4)
	{
		if (utf16Search(esp + i, searchStr, len))
		{
			if (logginEnabled)
			{
				dprintf("utf16SearchOnRegisters: Found on esp stack\n");
			}
			return true;
		}
	}

#endif //_WIN64

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

	return utf16SearchOnRegisters(searchStr, len);
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
	if (!stateManager.getConfig().utf16Enabled)
	{
		return;
	}

	std::wstring searchStr = stateManager.getConfig().utf16Text;
	int len = searchStr.length();

	if (utf16SearchOnRegisters(searchStr, len))
	{
		info->stop = true;
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
