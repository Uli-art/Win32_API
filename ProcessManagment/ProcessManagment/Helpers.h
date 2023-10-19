#pragma once


#include <windows.h>

#include <string>
#include <CommCtrl.h>

namespace Helpers
{
    bool SetButtonIcon(HINSTANCE hInstance, HWND hDlg, int buttonID, int iconID);

    std::wstring BrowseForFile(HINSTANCE hInstance, HWND hParentWindow, LPCTSTR title,
        LPCTSTR filter, LPCTSTR defExtention, bool existingOnl);
    std::wstring BrowseFolder(HWND hParentWindow, LPCWSTR title, LPCWSTR startFromPath);

    void SetControlEnabled(HWND hDlg, int controlID, bool enabled);
    std::wstring FileTimeToString(FILETIME ftime);
}