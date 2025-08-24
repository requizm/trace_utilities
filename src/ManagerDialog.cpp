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
		const auto& config = instance.getConfig();
		CheckDlgButton(hwndDlg, TRACE_C_LOG, config.loggingEnabled ? BST_CHECKED : BST_UNCHECKED);

		SetDlgItemTextW(hwndDlg, TRACE_T_SEARCH, config.utf16SearchText.c_str());
		CheckDlgButton(hwndDlg, TRACE_C_ENABLED, config.utf16SearchEnabled ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, TRACE_C_REGISTERS, config.utf16SearchRegistersEnabled ? BST_CHECKED : BST_UNCHECKED);

		CheckDlgButton(hwndDlg, TRACE_C_MEMORY, config.utf16MemoryEnabled ? BST_CHECKED : BST_UNCHECKED);
		wchar_t utf16MemoryAddress[256];
		wchar_t utf16MemorySize[256];
		swprintf_s(utf16MemoryAddress, L"%p", (void*)config.utf16MemoryAddress);
		swprintf_s(utf16MemorySize, L"%p", (void*)config.utf16MemorySize);
		SetDlgItemTextW(hwndDlg, TRACE_T_MEMORYADDRESS, utf16MemoryAddress);
		SetDlgItemTextW(hwndDlg, TRACE_T_MEMORYSIZE, utf16MemorySize);
		return true;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case TRACE_B_OK:
			StateManager& instance = StateManager::getInstance();
			auto& config = instance.getConfig();
			config.loggingEnabled = IsDlgButtonChecked(hwndDlg, TRACE_C_LOG) == BST_CHECKED;
			wchar_t utf16Text[256];
			GetDlgItemTextW(hwndDlg, TRACE_T_SEARCH, utf16Text, 256);
			config.utf16SearchText = std::wstring(utf16Text);
			config.utf16SearchEnabled = IsDlgButtonChecked(hwndDlg, TRACE_C_ENABLED) == BST_CHECKED;
			config.utf16SearchRegistersEnabled = IsDlgButtonChecked(hwndDlg, TRACE_C_REGISTERS) == BST_CHECKED;

			config.utf16MemoryEnabled = IsDlgButtonChecked(hwndDlg, TRACE_C_MEMORY) == BST_CHECKED;
			wchar_t utf16MemoryAddress[256];
			wchar_t utf16MemorySize[256];
			GetDlgItemTextW(hwndDlg, TRACE_T_MEMORYADDRESS, utf16MemoryAddress, 256);
			GetDlgItemTextW(hwndDlg, TRACE_T_MEMORYSIZE, utf16MemorySize, 256);
			config.utf16MemoryAddress = wcstoull(utf16MemoryAddress, NULL, 16);
			config.utf16MemorySize = wcstoull(utf16MemorySize, NULL, 16);
			saveConfig(config);
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
	ShowWindow(this->hwnd, SW_SHOW);
}
