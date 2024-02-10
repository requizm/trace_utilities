#include "plugin.h"
#include <windows.h>
#include "StateManager.h"
#include "ManagerDialog.h"
#include "Config.h"
#include <filesystem>

bool utf16Search(ULONG_PTR addr, std::wstring str, int strLen)
{
	// dprintf("utf16Search(%p, %p, %d)\n", addr, str, strLen);
	if (!DbgMemIsValidReadPtr(addr))
	{
		// dprintf("DbgMemIsValidReadPtr\n");
		return false;
	}

	wchar_t* wchar = new wchar_t[strLen];
	DbgMemRead(addr, wchar, strLen * sizeof(wchar_t));

	wchar_t* nullTerminatedP = new wchar_t[strLen + 1];
	memcpy(nullTerminatedP, wchar, strLen * sizeof(wchar_t));
	nullTerminatedP[strLen] = 0;

	// Print both strings
	// dprintf("input = %ls\n", nullTerminatedP);
	// dprintf("str = %ls\n", str);

	bool result = wcscmp(nullTerminatedP, str.data()) == 0;
	if (!result)
	{
		// dprintf("Not equals.\n");
		// Maybe addr is a pointer to a string. Use DbgMemIsValidReadPtr and DbgMemRead again
		ULONG_PTR addrP;
		if (DbgMemRead(addr, &addrP, sizeof(ULONG_PTR)))
		{
			// dprintf("Seems to be a pointer to a string at %p in 0x%p\n", addrP, addr);
			if (DbgMemIsValidReadPtr((duint)addrP))
			{
				wchar_t* wcharP = new wchar_t[strLen];
				DbgMemRead((duint)addrP, wcharP, strLen * sizeof(wchar_t));

				wchar_t* nullTerminatedPP = new wchar_t[strLen + 1];
				memcpy(nullTerminatedPP, wcharP, strLen * sizeof(wchar_t));
				nullTerminatedPP[strLen] = 0;

				result = wcscmp(nullTerminatedPP, str.data()) == 0;
				// if (result)
				// {
				// 	dprintf("Found at %p\n", addrP);
				// }
				// else
				// {
				// 	dprintf("Still not found at %p\n", addrP);
				// }

				delete[] nullTerminatedPP;
				delete[] wcharP;
			}
		}
		// delete addrP;
	}

	delete[] nullTerminatedP;
	delete[] wchar;

	return result;
}

static wchar_t* charToWChar(const char* text)
{
	const size_t size = strlen(text) + 1;
	wchar_t* wText = new wchar_t[size];
	mbstowcs(wText, text, size);
	return wText;
}

static bool utf16SearchCommand(int argc, char** argv)
{
	dprintf("utf16SearchCommand(%d, %p)\n", argc, argv);
	if (argc < 2)
	{
		dputs("Usage: " PLUGIN_NAME "searchStrUtf16");
		return false;
	}

	wchar_t* searchStr = charToWChar(argv[1]);
	int len = wcslen(searchStr);
	dprintf("searchStr = %p, len = %d\n", searchStr, len);

	REGDUMP regdump;
	DbgGetRegDumpEx(&regdump, sizeof(regdump));

	auto& r = regdump.regcontext;
#ifdef _WIN64
	if (utf16Search(r.cax, searchStr, len) || utf16Search(r.cbx, searchStr, len) || utf16Search(r.ccx, searchStr, len) || utf16Search(r.cdx, searchStr, len) || utf16Search(r.csi, searchStr, len) || utf16Search(r.cdi, searchStr, len) || utf16Search(r.cip, searchStr, len) || utf16Search(r.csp, searchStr, len) || utf16Search(r.cbp, searchStr, len))
	{
		_plugin_logprintf("found\n");
	}
	else
	{
		_plugin_logprintf("not found\n");
	}
#else
	if (utf16Search(r.cax, searchStr, len) || utf16Search(r.cbx, searchStr, len) || utf16Search(r.ccx, searchStr, len) || utf16Search(r.cdx, searchStr, len) || utf16Search(r.csi, searchStr, len) || utf16Search(r.cdi, searchStr, len) || utf16Search(r.cip, searchStr, len) || utf16Search(r.csp, searchStr, len) || utf16Search(r.cbp, searchStr, len))
	{
		_plugin_logprintf("found\n");
	}
	else
	{
		_plugin_logprintf("not found\n");
	}

#endif //_WIN64
	return true;
}

static duint utf16SearchFunction(int argc, const duint* argv, void* userdata)
{
	dprintf("utf16SearchFunction(%d, %p, %p)\n", argc, argv, userdata);
	if (argc < 2)
	{
		dputs("Usage: " PLUGIN_NAME "searchStrUtf16");
		return 0;
	}

	/*wchar_t *searchStr = charToWChar(argv[1]);
	int len = wcslen(searchStr);
	dprintf("searchStr = %p, len = %d\n", searchStr, len);*/
	dprintf("searchStr = %p, len = %d\n", argv[1], argv[2]);
	return 1;
}

void cbInitDebug(CBTYPE cbType, void* arg)
{
	// dprintf("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n");
}

#define EXPAND(x) L##x
#define DOWIDEN(x) EXPAND(x)

// Initialize your plugin data here.
bool pluginInit(PLUG_INITSTRUCT* initStruct)
{
	dprintf("pluginInit(pluginHandle: %d)\n", pluginHandle);

	// Prefix of the functions to call here: _plugin_register
	//_plugin_registercommand(pluginHandle, PLUGIN_NAME, cbExampleCommand, true);
	_plugin_registercommand(pluginHandle, PLUGIN_NAME, utf16SearchCommand, true);
	_plugin_registercallback(pluginHandle, CB_DEBUGEVENT, cbInitDebug);
	if (!_plugin_registerexprfunction(pluginHandle, "special.search", 2, utf16SearchFunction, nullptr))
	{
		_plugin_logprintf("Failed to register special.search\n");
	}

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

// Deinitialize your plugin data here.
// NOTE: you are responsible for gracefully closing your GUI
// This function is not executed on the GUI thread, so you might need
// to use WaitForSingleObject or similar to wait for everything to close.
void pluginStop()
{
	// Prefix of the functions to call here: _plugin_unregister

	dprintf("pluginStop(pluginHandle: %d)\n", pluginHandle);
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
	if (!stateManager.getUtf16Enabled())
	{
		return;
	}

	std::wstring searchStr = stateManager.getUtf16Text();
	int len = searchStr.length();

	REGDUMP regdump;
	DbgGetRegDumpEx(&regdump, sizeof(regdump));

	auto& r = regdump.regcontext;
#ifdef _WIN64
	// Check each register
	if (utf16Search(r.cax, searchStr, len) || utf16Search(r.cbx, searchStr, len) || utf16Search(r.ccx, searchStr, len) || utf16Search(r.cdx, searchStr, len) || utf16Search(r.csi, searchStr, len) || utf16Search(r.cdi, searchStr, len) || utf16Search(r.cip, searchStr, len) || utf16Search(r.csp, searchStr, len) || utf16Search(r.cbp, searchStr, len))
	{
		info->stop = true;
		_plugin_logprintf("stop\n");
	}

	// Check from 'csp + 0' to 'csp + 0x30' by increasing 0x4
	if (!info->stop)
	{
		duint esp = r.csp;
		for (duint i = 0; i < 0x30; i += 0x4)
		{
			if (utf16Search(esp + i, searchStr, len))
			{
				info->stop = true;
				_plugin_logprintf("stop\n");
				break;
			}
		}
	}
#else
	// Check each register
	if (utf16Search(r.cax, searchStr, len) || utf16Search(r.cbx, searchStr, len) || utf16Search(r.ccx, searchStr, len) || utf16Search(r.cdx, searchStr, len) || utf16Search(r.csi, searchStr, len) || utf16Search(r.cdi, searchStr, len) || utf16Search(r.cip, searchStr, len) || utf16Search(r.csp, searchStr, len) || utf16Search(r.cbp, searchStr, len))
	{
		info->stop = true;
		_plugin_logprintf("stop\n");
	}

	// Check from 'esp + 0' to 'esp + 0x30' by increasing 0x4
	if (!info->stop)
	{
		duint esp = r.csp;
		for (duint i = 0; i < 0x30; i += 0x4)
		{
			if (utf16Search(esp + i, searchStr, len))
			{
				info->stop = true;
				_plugin_logprintf("stop\n");
				break;
			}
		}
	}

#endif //_WIN64
}

BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		StateManager::getInstance().setHInstance(hinstDLL);
	}

	return TRUE;
}
