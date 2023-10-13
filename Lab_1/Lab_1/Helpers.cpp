#include "Helpers.h"

#include <locale>
#include <codecvt>

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

    std::string BrowseForFile(HINSTANCE hInstance, HWND hParentWindow, LPCTSTR title,
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
            std::wstring fileName(selectedFilePath);
            return WideStringToString(fileName);
        }

        return std::string();
    }

    std::string WideStringToString(std::wstring const& wideStr)
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
        return converter.to_bytes(wideStr);
    }

    std::wstring StringToWideString(std::string const& str)
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
        return converter.from_bytes(str);
    }

    void SetControlEnabled(HWND hDlg, int controlID, bool enabled)
    {
       :: EnableWindow(GetDlgItem(hDlg, controlID), enabled);
    }
}
