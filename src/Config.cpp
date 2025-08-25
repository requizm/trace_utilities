#include "Config.h"
#include <windows.h>
#include <string>
#include "plugin.h"
#include "StateManager.h"

void saveConfig(const Config& config)
{
	std::wstring apiFile = StateManager::getInstance().getApiFile();

	auto writeBool = [&](const wchar_t* key, bool value) {
		return WritePrivateProfileStringW(L"Config", key, value ? L"1" : L"0", apiFile.c_str());
	};

	if (!writeBool(L"loggingEnabled", config.loggingEnabled))
		DisplayError(TEXT("WritePrivateProfileString(loggingEnabled)"));

	if (!writeBool(L"utf16SearchEnabled", config.utf16SearchEnabled))
		DisplayError(TEXT("WritePrivateProfileString(utf16SearchEnabled)"));

	if (!WritePrivateProfileStringW(L"Config", L"utf16SearchText", config.utf16SearchText.c_str(), apiFile.c_str()))
		DisplayError(TEXT("WritePrivateProfileString(utf16SearchText)"));

	if (!writeBool(L"utf16SearchRegistersEnabled", config.utf16SearchRegistersEnabled))
		DisplayError(TEXT("WritePrivateProfileString(utf16SearchRegistersEnabled)"));

	if (!writeBool(L"utf16SearchStackEnabled", config.utf16SearchStackEnabled))
		DisplayError(TEXT("WritePrivateProfileString(utf16SearchStackEnabled)"));

	if (!writeBool(L"utf16SearchCaseSensitive", config.utf16SearchCaseSensitive))
		DisplayError(TEXT("WritePrivateProfileString(utf16SearchCaseSensitive)"));

	if (!writeBool(L"utf16SearchModeContains", config.utf16SearchModeContains))
		DisplayError(TEXT("WritePrivateProfileString(utf16SearchModeContains)"));

	if (!writeBool(L"utf16MemoryEnabled", config.utf16MemoryEnabled))
		DisplayError(TEXT("WritePrivateProfileString(utf16MemoryEnabled)"));

	if (!WritePrivateProfileStringW(L"Config", L"utf16MemoryAddress", std::to_wstring(config.utf16MemoryAddress).c_str(), apiFile.c_str()))
		DisplayError(TEXT("WritePrivateProfileString(utf16MemoryAddress)"));

	if (!WritePrivateProfileStringW(L"Config", L"utf16MemorySize", std::to_wstring(config.utf16MemorySize).c_str(), apiFile.c_str()))
		DisplayError(TEXT("WritePrivateProfileString(utf16MemorySize)"));

	dprintf("Saved config: loggingEnabled: %d, utf16SearchEnabled: %d, utf16SearchText: %ls, utf16SearchRegistersEnabled: %d, utf16SearchStackEnabled: %d, utf16SearchCaseSensitive: %d, utf16SearchModeContains: %d, utf16MemoryEnabled: %d, utf16MemoryAddress: %p, utf16MemorySize: %p\n",
		config.loggingEnabled, config.utf16SearchEnabled, config.utf16SearchText.c_str(), config.utf16SearchRegistersEnabled, config.utf16SearchStackEnabled, config.utf16SearchCaseSensitive, config.utf16SearchModeContains, config.utf16MemoryEnabled, (void*)config.utf16MemoryAddress, (void*)config.utf16MemorySize);
}

Config loadConfig()
{
	std::wstring apiFile = StateManager::getInstance().getApiFile();

	if (GetFileAttributesW(apiFile.c_str()) == INVALID_FILE_ATTRIBUTES)
	{
		saveConfig(Config());
		return Config();
	}

	auto config = Config();
	config.loggingEnabled = GetPrivateProfileIntW(L"Config", L"loggingEnabled", 0, apiFile.c_str()) == 1;
	config.utf16SearchEnabled = GetPrivateProfileIntW(L"Config", L"utf16SearchEnabled", 0, apiFile.c_str()) == 1;

	wchar_t utf16SearchText[256];
	GetPrivateProfileStringW(L"Config", L"utf16SearchText", L"", utf16SearchText, 256, apiFile.c_str());
	config.utf16SearchText = std::wstring(utf16SearchText);

	config.utf16SearchRegistersEnabled = GetPrivateProfileIntW(L"Config", L"utf16SearchRegistersEnabled", 0, apiFile.c_str()) == 1;
	config.utf16SearchStackEnabled = GetPrivateProfileIntW(L"Config", L"utf16SearchStackEnabled", 0, apiFile.c_str()) == 1;
	config.utf16SearchCaseSensitive = GetPrivateProfileIntW(L"Config", L"utf16SearchCaseSensitive", 0, apiFile.c_str()) == 1;
	config.utf16SearchModeContains = GetPrivateProfileIntW(L"Config", L"utf16SearchModeContains", 1, apiFile.c_str()) == 1; // Default to contains
	config.utf16MemoryEnabled = GetPrivateProfileIntW(L"Config", L"utf16MemoryEnabled", 0, apiFile.c_str()) == 1;
	config.utf16MemoryAddress = GetPrivateProfileIntW(L"Config", L"utf16MemoryAddress", 0, apiFile.c_str());
	config.utf16MemorySize = GetPrivateProfileIntW(L"Config", L"utf16MemorySize", 0, apiFile.c_str());

	dprintf("Loaded config: loggingEnabled=%d, utf16SearchEnabled=%d, utf16SearchText=%ws, utf16SearchRegistersEnabled=%d, utf16SearchStackEnabled: %d, utf16SearchCaseSensitive=%d, utf16SearchModeContains=%d, utf16MemoryEnabled=%d, utf16MemoryAddress=%p, utf16MemorySize=%p\n",
		config.loggingEnabled, config.utf16SearchEnabled, config.utf16SearchText.c_str(), config.utf16SearchRegistersEnabled, config.utf16SearchStackEnabled, config.utf16SearchCaseSensitive, config.utf16SearchModeContains, config.utf16MemoryEnabled, (void*)config.utf16MemoryAddress, (void*)config.utf16MemorySize);

	StateManager::getInstance().setConfig(config);

	return config;
}
