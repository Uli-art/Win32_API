#include "resource.h"

#include <windows.h>
#include <CommCtrl.h>
#include <strsafe.h>

#include <string>
#include <list>

#include "Helpers.h"

#pragma comment( lib, "comctl32.lib" )

enum
{
    WM_FILE_NOTIFY_EVENT = WM_APP + 1
};

struct FILE_NOTIFY_LIST_ITEM
{
    SLIST_ENTRY ItemEntry;
    DWORD Action;
    WCHAR FileName[MAX_PATH];
};


static HINSTANCE hAppInstance = NULL;
static HWND hMainWindow = NULL;

static std::wstring WatchedDirPath;
static HANDLE hWatchThread = NULL;
static HANDLE hStopWatchingEvent = NULL;
static PSLIST_HEADER pFileNotifyEventListHead;


INT_PTR CALLBACK MainDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

bool ReadDirectory(std::wstring const& dirPath, HWND hDirListView);
DWORD WINAPI WatchDirectoryProc(LPVOID);

void StartWatching();
void StopWatching();

std::wstring FileNotifyActionToString(DWORD action);

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PWSTR pCmdLine, _In_ int nCmdShow)
{
    hAppInstance = hInstance;

    INITCOMMONCONTROLSEX iCC = { sizeof(INITCOMMONCONTROLSEX), ICC_LISTVIEW_CLASSES | ICC_PROGRESS_CLASS };
    ::InitCommonControlsEx(&iCC);

    pFileNotifyEventListHead = (PSLIST_HEADER)_aligned_malloc(sizeof(SLIST_HEADER),
        MEMORY_ALLOCATION_ALIGNMENT);
    if (NULL == pFileNotifyEventListHead)
    {
        printf("Memory allocation failed.\n");
        return -1;
    }
    ::InitializeSListHead(pFileNotifyEventListHead);

    hMainWindow = ::CreateDialogParam(hInstance, MAKEINTRESOURCE(IDD_MAIN_DIALOG), NULL, MainDialogProc, 0);
    if (hMainWindow == NULL)
    {
        return -1;
    }

    ::ShowWindow(hMainWindow, nCmdShow);
    ::UpdateWindow(hMainWindow);

    MSG msg = { };
    while (::GetMessage(&msg, NULL, 0, 0) > 0)
    {
        if (!IsDialogMessage(hMainWindow, &msg))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
    }

return 0;
}

INT_PTR CALLBACK MainDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return TRUE;

    case WM_CLOSE:
        ::DestroyWindow(hwnd);
        return TRUE;

    case WM_FILE_NOTIFY_EVENT:
    {
        std::list<FILE_NOTIFY_LIST_ITEM*> itemList;

        PSLIST_ENTRY nextEntry = ::InterlockedPopEntrySList(pFileNotifyEventListHead);
        while (NULL != nextEntry)
        {
            itemList.push_front((FILE_NOTIFY_LIST_ITEM*)nextEntry);
            nextEntry = ::InterlockedPopEntrySList(pFileNotifyEventListHead);
        }

        for (auto const& nextItem : itemList)
        {
            FILETIME actionTime;
            ::GetSystemTimeAsFileTime(&actionTime);

            std::wstring actionTimeString = Helpers::FileTimeToString(actionTime);
            
            LVITEMW lvi = { };
            lvi.mask = LVIF_TEXT;
            lvi.iItem = 0;
            lvi.pszText = &actionTimeString[0];
            
            HWND hLogListView = ::GetDlgItem(hwnd, IDC_LOG_LIST);
            ListView_InsertItem(hLogListView, &lvi);

            std::wstring fileName = nextItem->FileName;

            std::wstring logText = FileNotifyActionToString(nextItem->Action) + L" : " + fileName;
            ListView_SetItemText(hLogListView, lvi.iItem, 1, &logText[0]);

            if (fileName.find(L"\\")) {
                HWND hDirListView = ::GetDlgItem(hwnd, IDC_DIR_LIST);
                ListView_DeleteAllItems(hDirListView);
                ReadDirectory(WatchedDirPath + L"\\*", hDirListView);
            }
            _aligned_free(nextItem);
        }
    }
    break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
            case IDC_OPEN_DIR:
            {
                std::wstring dirPath = Helpers::BrowseFolder(hwnd, L"Select a folder to start monitoring", L"");
                if (!dirPath.empty())
                {
                    if (NULL != hWatchThread)
                    {
                        StopWatching();
                    }

                    HWND hDirListView = ::GetDlgItem(hwnd, IDC_DIR_LIST);
                    ListView_DeleteAllItems(hDirListView);

                    HWND hLogListView = ::GetDlgItem(hwnd, IDC_LOG_LIST);
                    ListView_DeleteAllItems(hLogListView);

                    ReadDirectory(dirPath + L"\\*", hDirListView);
                    WatchedDirPath = dirPath;
                    //StartWatching(dirPath.c_str());
                }

                return TRUE;
            }
            break;

            

            case IDC_START_STOP_MONITORING:
                if (NULL == hWatchThread)
                {
                    StartWatching();
                    Helpers::SetButtonIcon(hAppInstance, hwnd, IDC_START_STOP_MONITORING, IDI_STOP_ICON);
                }
                else
                {
                    StopWatching();
                    Helpers::SetButtonIcon(hAppInstance, hwnd, IDC_START_STOP_MONITORING, IDI_PLAY_ICON);
                }

                return TRUE;
            break;
        }
        break;

    case WM_NOTIFY:
    {
        switch (LOWORD(wParam))
        {
        case IDC_DIR_LIST:
        {
                if (((LPNMHDR)lParam)->code == NM_DBLCLK)
                {
                    if (WatchedDirPath.empty()) {
                        return FALSE;
                    }

                    if (NULL != hWatchThread)
                    {
                        StopWatching();
                        Helpers::SetButtonIcon(hAppInstance, hwnd, IDC_START_STOP_MONITORING, IDI_PLAY_ICON);
                    }

                    HWND hDirListView = ::GetDlgItem(hwnd, IDC_DIR_LIST);
                    int iPos = ListView_GetNextItem(hDirListView, -1, LVNI_SELECTED);
                    LVITEMW lvi = { };
                    if (iPos != -1) {
                        ListView_GetItem(hDirListView, &lvi);
                        if (lvi.lParam == 0) {
                            WCHAR buffer[MAX_PATH] = { 0 };
                            ListView_GetItemText(hDirListView, iPos, 0, buffer, MAX_PATH);
                            

                            HWND hDirListView = ::GetDlgItem(hwnd, IDC_DIR_LIST);
                            ListView_DeleteAllItems(hDirListView);

                            HWND hLogListView = ::GetDlgItem(hwnd, IDC_LOG_LIST);
                            ListView_DeleteAllItems(hLogListView);
                            std::wstring dirPath = WatchedDirPath + L"\\" + std::wstring(buffer);

                            ReadDirectory(dirPath + L"\\*", hDirListView);
                            WatchedDirPath = dirPath;
                            //StartWatching(dirPath.c_str());
                        }
                    }
                }            
            return TRUE;
        }
        break;
        }
    }
    break;

    case WM_INITDIALOG:
    {
        Helpers::SetButtonIcon(hAppInstance, hwnd, IDC_OPEN_DIR, IDI_OPEN_ICON);
        Helpers::SetButtonIcon(hAppInstance, hwnd, IDC_START_STOP_MONITORING, IDI_PLAY_ICON);

        LVCOLUMNW lvc;
        lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        lvc.cx = 120;
        lvc.fmt = LVCFMT_LEFT;

        static WCHAR szDirListColumnString[4][64] = { L"Name", L"Size", L"Created", L"Modified" };
        HWND hDirListView = ::GetDlgItem(hwnd, IDC_DIR_LIST);
        ListView_SetExtendedListViewStyle(hDirListView, LVS_EX_AUTOSIZECOLUMNS | LVS_EX_FULLROWSELECT);

        for (auto column = 0; column < 4; ++column)
        {
            lvc.pszText = szDirListColumnString[column];
            if (ListView_InsertColumn(hDirListView, column, &lvc) == -1)
            {
                return FALSE;
            }
        }

        static TCHAR szLogListColumnString[4][64] = { L"Time", L"Action" };
        HWND hLogListView = ::GetDlgItem(hwnd, IDC_LOG_LIST);
        ListView_SetExtendedListViewStyle(hLogListView, LVS_EX_AUTOSIZECOLUMNS | LVS_EX_FULLROWSELECT);

        for (auto column = 0; column < 2; ++column)
        {
            lvc.pszText = szLogListColumnString[column];
            if (ListView_InsertColumn(hLogListView, column, &lvc) == -1)
            {
                return FALSE;
            }
        }
    }
    break;
    }

    return FALSE;
}

bool ReadDirectory(std::wstring const& dirPath, HWND hDirListView)
{
    WIN32_FIND_DATAW ffd;
    HANDLE hFind = INVALID_HANDLE_VALUE;

    HIMAGELIST hLarge = ImageList_Create(GetSystemMetrics(SM_CXICON),
        GetSystemMetrics(SM_CYICON),
        ILC_MASK, 1, 1);
    HIMAGELIST hSmall = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON),
        ILC_MASK, 1, 1);

    hFind = FindFirstFileW(dirPath.c_str(), &ffd);

    if (INVALID_HANDLE_VALUE == hFind)
    {
        return false;
    }

    LVITEMW lvi = { };
    lvi.mask = LVIF_PARAM | LVIF_IMAGE | LVIF_TEXT;
    lvi.iSubItem = 0;
    lvi.pszText = (LPWSTR)"1";

    Helpers::AddIconToListView(hAppInstance, hLarge, hSmall, IDI_FOLDER_ICON);
    Helpers::AddIconToListView(hAppInstance, hLarge, hSmall, IDI_FILE_ICON);

    ListView_SetImageList(hDirListView, hLarge, LVSIL_NORMAL);
    ListView_SetImageList(hDirListView, hSmall, LVSIL_SMALL);

    do
    {
        if (::CompareStringW(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, ffd.cFileName, -1, L".", -1) == CSTR_EQUAL)
            {
                continue;
            }
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            lvi.iImage = 0;
            lvi.lParam = 0;
        }
        else 
        {
            lvi.iImage = 1;
            lvi.lParam = 1;
        }

        lvi.pszText = ffd.cFileName;
        ListView_InsertItem(hDirListView, &lvi);

        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            static WCHAR emptyFileSize[2] = { L"-" };
            ListView_SetItemText(hDirListView, lvi.iItem, 1, emptyFileSize);
        }
        else
        {
            LARGE_INTEGER fileSize = { 0, 0 };

            fileSize.LowPart = ffd.nFileSizeLow;
            fileSize.HighPart = ffd.nFileSizeHigh;

            WCHAR fileSizeString[256];
            wsprintfW(fileSizeString, L"%ld", fileSize.QuadPart);

            ListView_SetItemText(hDirListView, lvi.iItem, 1, fileSizeString);
        }

        std::wstring fileCreatedString = Helpers::FileTimeToString(ffd.ftCreationTime);
        ListView_SetItemText(hDirListView, lvi.iItem, 2, &fileCreatedString[0]);

        std::wstring fileModifiedString = Helpers::FileTimeToString(ffd.ftLastWriteTime);
        ListView_SetItemText(hDirListView, lvi.iItem, 3, &fileModifiedString[0]);

        ++lvi.iItem;
    }
    while (FindNextFileW(hFind, &ffd) != 0);

    DWORD dwError = GetLastError();
    if (dwError != ERROR_NO_MORE_FILES)
    {
        return false;
    }

    ::FindClose(hFind);
    return true;
}

void StartWatching()
{
    hStopWatchingEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    hWatchThread = ::CreateThread(NULL, 0, WatchDirectoryProc, 0, 0, NULL);
}

void StopWatching()
{
    if (NULL != hWatchThread)
    {
        ::SetEvent(hStopWatchingEvent);
        ::WaitForSingleObject(hWatchThread, INFINITE);
    }

    ::InterlockedFlushSList(pFileNotifyEventListHead);

    hStopWatchingEvent = NULL;
    hWatchThread = NULL;
}

DWORD WINAPI WatchDirectoryProc(LPVOID)
{
    static const UINT CHANGES_BUFFER_SIZE = 1024;

    LPCWSTR dirPath = WatchedDirPath.c_str();

    HANDLE hDirHandle = CreateFile(dirPath, FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);

    if (INVALID_HANDLE_VALUE == hDirHandle)
    {
        return ::GetLastError();
    }

    OVERLAPPED overlapped;
    overlapped.hEvent = CreateEvent(NULL, FALSE, 0, NULL);

    if (INVALID_HANDLE_VALUE == overlapped.hEvent)
    {
        return ::GetLastError();
    }

    while (true)
    {
        DWORD changesBuffer[CHANGES_BUFFER_SIZE];
        if (!::ReadDirectoryChangesW(hDirHandle, changesBuffer, sizeof(changesBuffer), TRUE,
            FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_LAST_WRITE, NULL, &overlapped, NULL))
        {
            return ::GetLastError();
        }

        HANDLE events[2] = { overlapped.hEvent, hStopWatchingEvent };
        DWORD result = ::WaitForMultipleObjects(2, events, FALSE, INFINITE);

        if (result == WAIT_OBJECT_0)
        {
            DWORD bytesTransferred;
            ::GetOverlappedResult(hDirHandle, &overlapped, &bytesTransferred, FALSE);

            FILE_NOTIFY_INFORMATION* event = (FILE_NOTIFY_INFORMATION*)changesBuffer;           

            for (;;) {

                FILE_NOTIFY_LIST_ITEM* pNotifyItem = (FILE_NOTIFY_LIST_ITEM*)_aligned_malloc(sizeof(FILE_NOTIFY_LIST_ITEM),
                    MEMORY_ALLOCATION_ALIGNMENT);
                ::ZeroMemory(pNotifyItem, sizeof(FILE_NOTIFY_LIST_ITEM));

                if (NULL == pNotifyItem)
                {
                    return -1;
                }

                pNotifyItem->Action = event->Action;
                ::StringCchCopyN(pNotifyItem->FileName, MAX_PATH, event->FileName, event->FileNameLength / sizeof(WCHAR));
                ::InterlockedPushEntrySList(pFileNotifyEventListHead, &pNotifyItem->ItemEntry);

                if (event->NextEntryOffset)
                {
                    *((uint8_t**)&event) += event->NextEntryOffset;
                }
                else
                {
                    break;
                }
            }

            ::PostMessage(hMainWindow, WM_FILE_NOTIFY_EVENT, 0, 0);
        }
        else if (result == WAIT_OBJECT_0 + 1)
        {
            break;
        }
    }

    ::CloseHandle(hStopWatchingEvent);

    hStopWatchingEvent = NULL;
    hWatchThread = NULL;

    return 0;
}

std::wstring FileNotifyActionToString(DWORD action)
{
    switch (action)
    {
        case FILE_ACTION_ADDED:
        {
            return L"File added";
        }
        break;

        case FILE_ACTION_REMOVED:
        {
            return L"File removed";
        }
        break;

        case FILE_ACTION_MODIFIED:
        {
            return L"File modified";
        }
        break;

        case FILE_ACTION_RENAMED_OLD_NAME:
        {
            return L"File renamed [old name]";
        }
        break;

        case FILE_ACTION_RENAMED_NEW_NAME:
        {
            return L"File renamed [new name]";
        }
        break;

        default:
        {
            return L"Unknown action";
        }
        break;
    }
}