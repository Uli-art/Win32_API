#include "Helpers.h"


#include <codecvt>
#include <shlobj.h>

#include <sstream>
#include <iomanip>

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

    std::wstring BrowseForFile(HINSTANCE hInstance, HWND hParentWindow, LPCTSTR title,
        LPCTSTR filter, LPCTSTR defExtention, bool existingOnly)
    {
        static const int MaxFilePath = 1024;

        OPENFILENAME of = {};
        of.lStructSize = sizeof(OPENFILENAME);
        of.hwndOwner = hParentWindow;
        of.hInstance = hInstance;
        of.lpstrFilter = filter;
        of.lpstrDefExt = defExtention;
        of.lpstrTitle = title;
        of.Flags = OFN_EXPLORER;


        if (existingOnly)
        {
            of.Flags |= OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
        }

        TCHAR selectedFilePath[MaxFilePath] = { 0 };
        of.lpstrFile = selectedFilePath;
        of.nMaxFile = MaxFilePath;

        BOOL ok = ::GetOpenFileName(&of);
        if (ok)
        {
            return selectedFilePath;
        }

        return std::wstring();
    }


    static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
    {

        if (uMsg == BFFM_INITIALIZED)
        {
            SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
        }

        return 0;
    }

    std::wstring BrowseFolder(HWND hParentWindow, LPCWSTR title, LPCWSTR startFromPath)
    {
        WCHAR path[MAX_PATH];

        BROWSEINFO bi = { 0 };
        bi.hwndOwner = hParentWindow;
        bi.lpszTitle = title;
        bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
        bi.lpfn = BrowseCallbackProc;
        bi.lParam = (LPARAM)startFromPath;

        LPITEMIDLIST pidl = SHBrowseForFolder(&bi);

        if (pidl != 0)
        {
            SHGetPathFromIDListW(pidl, path);

            IMalloc* imalloc = 0;
            if (SUCCEEDED(SHGetMalloc(&imalloc)))
            {
                imalloc->Free(pidl);
                imalloc->Release();
            }

            return std::wstring(path);
        }

        return std::wstring();
    }

    void SetControlEnabled(HWND hDlg, int controlID, bool enabled)
    {
        ::EnableWindow(GetDlgItem(hDlg, controlID), enabled);
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
}
