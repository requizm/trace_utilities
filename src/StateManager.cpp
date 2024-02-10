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

Config& StateManager::getConfig()
{
	return config;
}

void StateManager::setConfig(Config config)
{
	this->config = config;
}

void StateManager::setApiFile(std::wstring apiFile)
{
	this->apiFile = apiFile;
}

std::wstring StateManager::getApiFile()
{
	return apiFile;
}
