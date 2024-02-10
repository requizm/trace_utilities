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
		SetDlgItemTextW(hwndDlg, TRACE_T_UTF16, StateManager::getInstance().getUtf16Text().c_str());
		CheckDlgButton(hwndDlg, TRACE_C_ENABLED, StateManager::getInstance().getUtf16Enabled() ? BST_CHECKED : BST_UNCHECKED);
		return true;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case TRACE_B_OK:
			std::wstring utf16Text = std::wstring(256, L'\0');
			if (GetDlgItemTextW(hwndDlg, TRACE_T_UTF16, &utf16Text[0], 256)) {
				StateManager::getInstance().setUtf16Text(utf16Text);
				StateManager::getInstance().setUtf16Enabled(IsDlgButtonChecked(hwndDlg, TRACE_C_ENABLED) == BST_CHECKED);
				Config config;
				config.utf16Enabled = StateManager::getInstance().getUtf16Enabled();
				config.utf16Text = StateManager::getInstance().getUtf16Text();
				saveConfig(config);
				DestroyWindow(hwndDlg);
				return true;
			}
			_plugin_logprintf("GetDlgItemText failed\n");
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

	SetDlgItemText(hwnd, TRACE_C_ENABLED, "Enable");
	SetDlgItemText(hwnd, TRACE_L_UTF16, "Utf-16 text");

	ShowWindow(this->hwnd, SW_SHOW);
}
