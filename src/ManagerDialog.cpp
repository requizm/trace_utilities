#include "ManagerDialog.h"
#include "StateManager.h"
#include "resource.h"
#include <stdio.h>
#include "pluginmain.h"
#include "Config.h"

INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CLOSE:
	{
		DestroyWindow(hwndDlg);
		return true;
	}
	case WM_INITDIALOG:
	{
		StateManager& instance = StateManager::getInstance();
		SetDlgItemTextW(hwndDlg, TRACE_T_UTF16, instance.getConfig().utf16Text.c_str());
		CheckDlgButton(hwndDlg, TRACE_C_ENABLED, instance.getConfig().utf16Enabled ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, TRACE_C_LOG, instance.getConfig().loggingEnabled ? BST_CHECKED : BST_UNCHECKED);
		return true;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case TRACE_B_OK:
			StateManager& instance = StateManager::getInstance();
			wchar_t utf16Text[256];
			GetDlgItemTextW(hwndDlg, TRACE_T_UTF16, utf16Text, 256);
			instance.getConfig().utf16Text = std::wstring(utf16Text);
			instance.getConfig().utf16Enabled = IsDlgButtonChecked(hwndDlg, TRACE_C_ENABLED) == BST_CHECKED;
			instance.getConfig().loggingEnabled = IsDlgButtonChecked(hwndDlg, TRACE_C_LOG) == BST_CHECKED;
			saveConfig(instance.getConfig());
			DestroyWindow(hwndDlg);
			return true;
			break;
		}
		break;
	default:
		break;
	}
	return false;
}

ManagerDialog::ManagerDialog()
{
	this->hwnd = CreateDialog(StateManager::getInstance().getHInstance(), MAKEINTRESOURCE(IDD_DIALOG1), GuiGetWindowHandle(), DialogProc);
	char title[100];
	snprintf(title, sizeof(title), "Manager");
	SetWindowText(this->hwnd, title);

	SetDlgItemText(hwnd, TRACE_C_ENABLED, "Searcn utf16 on all registers");
	SetDlgItemText(hwnd, TRACE_L_UTF16, "Utf-16 text");
	SetDlgItemText(hwnd, TRACE_C_LOG, "Logging(Will affect performance)");

	ShowWindow(this->hwnd, SW_SHOW);
}
