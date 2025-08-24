#include "Config.h"
#include <windows.h>
#include <string>
#include "plugin.h"
#include "StateManager.h"

void saveConfig(const Config& config)
{
	std::wstring apiFile = StateManager::getInstance().getApiFile();

	bool loggingEnabled = config.loggingEnabled ? 1 : 0;
	if (!WritePrivateProfileStringW(L"Config", L"loggingEnabled", std::to_wstring(loggingEnabled).c_str(), apiFile.c_str()))
	{
		DisplayError(TEXT("WritePrivateProfileString(loggingEnabled)"));
		return;
	}

	bool utf16SearchEnabled = config.utf16SearchEnabled ? 1 : 0;
	if (!WritePrivateProfileStringW(L"Config", L"utf16SearchEnabled", std::to_wstring(utf16SearchEnabled).c_str(), apiFile.c_str()))
	{
		DisplayError(TEXT("WritePrivateProfileString(utf16SearchEnabled"));
		return;
	}

	if (!WritePrivateProfileStringW(L"Config", L"utf16SearchText", config.utf16SearchText.c_str(), apiFile.c_str()))
	{
		DisplayError(TEXT("WritePrivateProfileStringutf16Text(utf16SearchText)"));
		return;
	}

	bool utf16SearchRegistersEnabled = config.utf16SearchRegistersEnabled ? 1 : 0;
	if (!WritePrivateProfileStringW(L"Config", L"utf16SearchRegistersEnabled", std::to_wstring(utf16SearchRegistersEnabled).c_str(), apiFile.c_str()))
	{
		DisplayError(TEXT("WritePrivateProfileString(utf16SearchRegistersEnabled)"));
		return;
	}

	bool utf16MemoryEnabled = config.utf16MemoryEnabled ? 1 : 0;
	if (!WritePrivateProfileStringW(L"Config", L"utf16MemoryEnabled", std::to_wstring(utf16MemoryEnabled).c_str(), apiFile.c_str()))
	{
		DisplayError(TEXT("WritePrivateProfileString(utf16MemoryEnabled)"));
		return;
	}

	if (!WritePrivateProfileStringW(L"Config", L"utf16MemoryAddress", std::to_wstring(config.utf16MemoryAddress).c_str(), apiFile.c_str()))
	{
		DisplayError(TEXT("WritePrivateProfileString(utf16MemoryAddress)"));
		return;
	}

	if (!WritePrivateProfileStringW(L"Config", L"utf16MemorySize", std::to_wstring(config.utf16MemorySize).c_str(), apiFile.c_str()))
	{
		DisplayError(TEXT("WritePrivateProfileString(utf16MemorySize)"));
		return;
	}

	dprintf("Saved config: loggingEnabled: %d, utf16SearchEnabled: %d, utf16SearchText: %ls, utf16SearchRegistersEnabled: %d, utf16MemoryEnabled: %d, utf16MemoryAddress: %p, utf16MemorySize: %p\n",
		config.loggingEnabled, config.utf16SearchEnabled, config.utf16SearchText.c_str(), config.utf16SearchRegistersEnabled, config.utf16MemoryEnabled, config.utf16MemoryAddress, config.utf16MemorySize);
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
	config.utf16MemoryEnabled = GetPrivateProfileIntW(L"Config", L"utf16MemoryEnabled", 0, apiFile.c_str()) == 1;
	config.utf16MemoryAddress = GetPrivateProfileIntW(L"Config", L"utf16MemoryAddress", 0, apiFile.c_str());
	config.utf16MemorySize = GetPrivateProfileIntW(L"Config", L"utf16MemorySize", 0, apiFile.c_str());

	dprintf("Loaded config: loggingEnabled=%d, utf16SearchEnabled=%d, utf16SearchText=%ws, utf16SearchRegistersEnabled=%d, utf16MemoryEnabled=%d, utf16MemoryAddress=%p, utf16MemorySize=%p\n",
		config.loggingEnabled, config.utf16SearchEnabled, config.utf16SearchText.c_str(), config.utf16SearchRegistersEnabled, config.utf16MemoryEnabled, config.utf16MemoryAddress, config.utf16MemorySize);

	StateManager::getInstance().setConfig(config);

	return config;
}
