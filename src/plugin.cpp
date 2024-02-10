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

bool utf16Search(ULONG_PTR addr, std::wstring str, int strLen)
{
	if (!DbgMemIsValidReadPtr(addr))
	{
		return false;
	}


	//std::wstring addressStr = std::wstring(strLen + 1, L'\0');
	//if (!DbgMemRead(addr, &addressStr[0], strLen * sizeof(wchar_t)))
	//{
	//	_plugin_logprintf("DbgMemRead failed\n");
	//	return false;
	//}
	//addressStr[strLen] = 0;

	wchar_t* wchar = new wchar_t[strLen];
	DbgMemRead(addr, wchar, strLen * sizeof(wchar_t));
	wchar_t* nullTerminatedP = new wchar_t[strLen + 1];
	memcpy(nullTerminatedP, wchar, strLen * sizeof(wchar_t));
	nullTerminatedP[strLen] = 0;

	bool logginEnabled = StateManager::getInstance().getConfig().loggingEnabled;
	if (logginEnabled)
	{
		_plugin_logprintf("utf16Search: addr: %p, str: %ls, nullTerminatedP: %ls\n", addr, str.c_str(), nullTerminatedP);
	}

	bool result = wcscmp(nullTerminatedP, str.data()) == 0;
	if (!result)
	{
		// Maybe addr is a pointer to a string. Use DbgMemIsValidReadPtr and DbgMemRead again
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
					_plugin_logprintf("utf16Search second iteration: addrP: %p, str: %ls, nullTerminatedPP: %ls\n", addrP, str.c_str(), nullTerminatedPP);
				}

				result = wcscmp(nullTerminatedPP, str.data()) == 0;

				delete[] nullTerminatedPP;
				delete[] wcharP;
			}
		}
	}

	delete[] nullTerminatedP;
	delete[] wchar;

	return result;
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
			_plugin_logprintf("utf16SearchOnRegisters: Found on register\n");
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
				_plugin_logprintf("utf16SearchOnRegisters: Found on esp stack\n");
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
			_plugin_logprintf("utf16SearchOnRegisters: Found on register\n");
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
				_plugin_logprintf("utf16SearchOnRegisters: Found on esp stack\n");
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
