#include "StateManager.h"

StateManager &StateManager::getInstance()
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
