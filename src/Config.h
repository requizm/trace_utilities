#pragma once

#include <Windows.h>
#include <string>

struct Config
{
	bool utf16Enabled = false;
	char utf16Text[256] = "";
};

void saveConfig(Config config);
Config loadConfig();
