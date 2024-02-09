#pragma once

#include <Windows.h>

class StateManager
{
private:
    HINSTANCE instance;
public:
    static StateManager& getInstance();
    void setHInstance(HINSTANCE hInstance);
    HINSTANCE getHInstance();
};
