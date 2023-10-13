#pragma once

#include <windows.h>

#include <string>

namespace Helpers
{
    std::string WideStringToString(std::wstring const& wideStr);
    std::wstring StringToWideString(std::string const& str);

    bool SetButtonIcon(HINSTANCE hInstance, HWND hDlg, int buttonID, int iconID);
    std::string BrowseForFile(HINSTANCE hInstance, HWND hParentWindow, LPCTSTR title, 
        LPCTSTR filter, LPCTSTR defExtention, bool existingOnly);

    void SetControlEnabled(HWND hDlg, int controlID, bool enabled);
}

