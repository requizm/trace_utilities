#pragma once

#include <Windows.h>
#include <string>

struct Config
{
	bool utf16Enabled = false;
	std::wstring utf16Text = std::wstring(256, L'\0');
};

void saveConfig(Config config);
Config loadConfig();
