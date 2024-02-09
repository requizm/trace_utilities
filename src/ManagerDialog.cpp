#include "ManagerDialog.h"
#include "StateManager.h"
#include "resource.h"
#include <stdio.h>
#include "pluginmain.h"

ManagerDialog::ManagerDialog()
{
    this->hwnd = CreateDialog(StateManager::getInstance().getHInstance(), MAKEINTRESOURCE(IDD_DIALOG111), GuiGetWindowHandle(), NULL);
    char title[100];
    snprintf(title, sizeof(title), "Manager");
    SetWindowText(this->hwnd, title);
    ShowWindow(this->hwnd, SW_SHOW);
}
