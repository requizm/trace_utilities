#pragma once

#include <Windows.h>
#include <string>

class StateManager
{
private:
	HINSTANCE instance;

	LPSTR utf16Text = "";
	bool utf16Enabled = false;
public:
	static StateManager& getInstance();
	void setHInstance(HINSTANCE hInstance);
	HINSTANCE getHInstance();

	void setUtf16Text(LPSTR utf16Text);
	LPSTR getUtf16Text();

	void setUtf16Enabled(bool utf16Enabled);
	bool getUtf16Enabled();
};
