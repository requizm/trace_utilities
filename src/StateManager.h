#pragma once

#include <Windows.h>
#include <string>
#include "Config.h"

class StateManager
{
private:
	HINSTANCE instance;

	Config config;

	std::wstring apiFile = std::wstring(256, L'\0');
public:
	static StateManager& getInstance();
	void setHInstance(HINSTANCE hInstance);
	HINSTANCE getHInstance();

	Config& getConfig();
	void setConfig(Config config);

	void setApiFile(std::wstring apiFile);
	std::wstring getApiFile();
};
