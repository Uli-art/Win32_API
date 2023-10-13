#pragma once

#include <windows.h>

#include <string>

namespace Helpers
{
    bool SetButtonIcon(HINSTANCE hInstance, HWND hDlg, int buttonID, int iconID);

    void SetControlEnabled(HWND hDlg, int controlID, bool enabled);
    std::wstring FileTimeToString(FILETIME ftime);
    time_t GetStartOfTheDay(SYSTEMTIME st);
    void Time_tToSystemTime(time_t t, SYSTEMTIME* st);
}

