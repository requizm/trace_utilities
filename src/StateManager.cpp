#include "StateManager.h"

StateManager& StateManager::getInstance()
{
	static StateManager instance;
	return instance;
}

void StateManager::setHInstance(HINSTANCE hInstance)
{
	instance = hInstance;
}

HINSTANCE StateManager::getHInstance()
{
	return instance;
}

void StateManager::setUtf16Text(LPSTR utf16Text)
{
	this->utf16Text = utf16Text;
}

LPSTR StateManager::getUtf16Text()
{
	return utf16Text;
}

void StateManager::setUtf16Enabled(bool utf16Enabled)
{
	this->utf16Enabled = utf16Enabled;
}

bool StateManager::getUtf16Enabled()
{
	return utf16Enabled;
}
