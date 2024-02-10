#pragma once

#include <Windows.h>
#include <string>

class StateManager
{
private:
	HINSTANCE instance;

	std::wstring utf16Text = std::wstring(256, L'\0');
	bool utf16Enabled = false;

	std::wstring apiFile = std::wstring(256, L'\0');
public:
	static StateManager& getInstance();
	void setHInstance(HINSTANCE hInstance);
	HINSTANCE getHInstance();

	void setUtf16Text(std::wstring utf16Text);
	std::wstring getUtf16Text();

	void setUtf16Enabled(bool utf16Enabled);
	bool getUtf16Enabled();

	void setApiFile(std::wstring apiFile);
	std::wstring getApiFile();
};
