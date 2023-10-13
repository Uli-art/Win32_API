#include "Helpers.h"

#include <codecvt>
#include <shlobj.h>

#include <sstream>
#include <iomanip>
#include <ctime>

namespace Helpers
{
    bool SetButtonIcon(HINSTANCE hInstance, HWND hDlg, int buttonID, int iconID)
    {
        HICON hIcon = static_cast<HICON>(::LoadImage(hInstance, MAKEINTRESOURCE(iconID), IMAGE_ICON,
            16, 16, LR_DEFAULTCOLOR));
        if (hIcon == NULL)
        {
            return FALSE;
        }

        ::SendDlgItemMessage(hDlg, buttonID, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)hIcon);

        return TRUE;
    }

    void SetControlEnabled(HWND hDlg, int controlID, bool enabled)
    {
       :: EnableWindow(GetDlgItem(hDlg, controlID), enabled);
    }

    std::wstring FileTimeToString(FILETIME ftime)
    {
        SYSTEMTIME utc;
        ::FileTimeToSystemTime(std::addressof(ftime), std::addressof(utc));

        std::wostringstream os;
        os << std::setfill<wchar_t>('0') << std::setw(4) << utc.wYear << '-' << std::setw(2) << utc.wMonth
            << '-' << std::setw(2) << utc.wDay << ' ' << std::setw(2) << utc.wHour
            << ':' << std::setw(2) << utc.wMinute << ':' << std::setw(2) << utc.wSecond;

        return os.str();
    }

    time_t GetStartOfTheDay(SYSTEMTIME st)
    {
        std::tm tmDate;

        tmDate.tm_sec = 0;
        tmDate.tm_min = 0;
        tmDate.tm_hour = 0;
        tmDate.tm_mday = st.wDay;
        tmDate.tm_mon = st.wMonth - 1;
        tmDate.tm_year = st.wYear - 1900;
        tmDate.tm_isdst = 0;

        std::time_t t = std::mktime(&tmDate);
        return t;
    }

    void Time_tToSystemTime(time_t t, SYSTEMTIME* st)
    {
        std::tm tm;
        localtime_s(&tm, &t);

        st->wYear = tm.tm_year + 1900;
        st->wDay = tm.tm_mday;
        st->wDayOfWeek = tm.tm_wday;
        st->wMonth = tm.tm_mon + 1;
        st->wHour = tm.tm_hour;
        st->wMinute = tm.tm_min;
        st->wSecond = tm.tm_sec;
    }
}
