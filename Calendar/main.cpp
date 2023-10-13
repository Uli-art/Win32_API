#include "resource.h"

#include <windows.h>
#include <windowsx.h>
#include <CommCtrl.h>
#include <strsafe.h>

#include <chrono>
#include <string>
#include <map>

#include "Helpers.h"

#pragma comment( lib, "comctl32.lib" )

static HINSTANCE hAppInstance = NULL;
static HWND hMainWindow = NULL;
static HWND hCalendarWindow = NULL;

struct Event
{
    int StartTimeInMin;
    int EndTimeInMin;
    std::wstring Name;
    std::wstring Description;
};

typedef std::multimap<time_t, Event> EventContainer;
EventContainer Events;

INT_PTR CALLBACK MainDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK AddEventDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK EventListDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void OpenAddEventDialog();
void OpenEventListDialog();


int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PWSTR pCmdLine, _In_ int nCmdShow)
{
    hAppInstance = hInstance;

    INITCOMMONCONTROLSEX iCC = { sizeof(INITCOMMONCONTROLSEX), ICC_LISTVIEW_CLASSES | ICC_DATE_CLASSES };
    ::InitCommonControlsEx(&iCC);

    hMainWindow = ::CreateDialogParam(hInstance, MAKEINTRESOURCE(IDD_MAIN_DIALOG), NULL, MainDialogProc, 0);
    if (hMainWindow == NULL)
    {
        return -1;
    }

    HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CALENDAR));
    SendMessage(hMainWindow, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

    HWND hCalendarParent = ::GetDlgItem(hMainWindow, IDC_CALENDAR_STATIC);
    hCalendarWindow = CreateWindowEx(0, MONTHCAL_CLASS, L"", WS_BORDER | WS_CHILD | WS_VISIBLE /* | MCS_DAYSTATE*/,
        0, 0, 0, 0, hMainWindow, NULL, hAppInstance, NULL);

    if (hCalendarWindow == NULL)
        return -1;

    MonthCal_SetCurrentView(hCalendarWindow, MCMV_MONTH);

    RECT rc;
    ::GetWindowRect(hCalendarParent, &rc);
    ::MapWindowPoints(HWND_DESKTOP, hMainWindow, (LPPOINT)&rc, 2);
    ::SetWindowPos(hCalendarWindow, NULL, rc.left, rc.top - 4, rc.right - rc.left, rc.bottom - rc.top - 4, SWP_NOZORDER);
    
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
        
        case WM_NOTIFY:
        {
            static auto lastDaySelectTimeMs = std::chrono::system_clock::now();

            switch (((LPNMHDR)lParam)->code)
            {
            case MCN_GETDAYSTATE:
            {
                NMDAYSTATE* nmDayState = (NMDAYSTATE*)lParam;
                MONTHDAYSTATE rgMonths[12] = { 0 };
                int cMonths = ((NMDAYSTATE*)lParam)->cDayState;
            }
            break;

            case MCN_SELECT:
            {
                if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - lastDaySelectTimeMs).count() < ::GetDoubleClickTime())
                {
                    OpenEventListDialog();
                }

                lastDaySelectTimeMs = std::chrono::system_clock::now();
            }
            break;
            }
        }
        break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_ADD_TASK:
                {
                
                    return TRUE;
                }
                break;

                case IDC_ADD_EVENT:
                {
                    OpenAddEventDialog();

                    return TRUE;
                }
                break;
            }
            break;

        case WM_INITDIALOG:
        {
            //Helpers::SetButtonIcon(hAppInstance, hwnd, IDC_ADD_TASK, IDI_ICON1);
            Helpers::SetButtonIcon(hAppInstance, hwnd, IDC_ADD_EVENT, IDI_ADD_EVENT_ICON);
        }
        break;
    }

    return FALSE;
}

void OpenAddEventDialog()
{
    SYSTEMTIME selectedDate;
    MonthCal_GetCurSel(hCalendarWindow, &selectedDate);

    time_t startOfTheDay = Helpers::GetStartOfTheDay(selectedDate);

    INT_PTR result = ::DialogBoxParamW(hAppInstance, MAKEINTRESOURCE(IDD_ADD_EVENT_DIALOG), hMainWindow,
        AddEventDialogProc, (LPARAM)startOfTheDay);
    if (result == 1)
    {
    }
}

void OpenEventListDialog()
{
    SYSTEMTIME selectedDate;
    MonthCal_GetCurSel(hCalendarWindow, &selectedDate);

    time_t startOfTheDay = Helpers::GetStartOfTheDay(selectedDate);

    INT_PTR result = ::DialogBoxParamW(hAppInstance, MAKEINTRESOURCE(IDD_LIST_EVENTS_DIALOG), hMainWindow,
        EventListDialogProc, (LPARAM)startOfTheDay);
    if (result == 1)
    {
    }
}

INT_PTR CALLBACK AddEventDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static time_t CurrentStartOfTheDay = 0;

    switch (uMsg)
    {
        case WM_CLOSE:
            return ::EndDialog(hwnd, 0);
        break;

        case WM_INITDIALOG:
        {
            CurrentStartOfTheDay = (time_t)lParam;

            

            HWND hStartTime = ::GetDlgItem(hwnd, IDC_STARTTIME);
            DateTime_SetFormat(hStartTime, L"hh:mm tt");
            SYSTEMTIME initStartTime;
            Helpers::Time_tToSystemTime(CurrentStartOfTheDay, &initStartTime);
            initStartTime.wHour = 8;
            DateTime_SetSystemtime(hStartTime, GDT_VALID, &initStartTime);

            HWND hEndTime = ::GetDlgItem(hwnd, IDC_ENDTIME);
            DateTime_SetFormat(hEndTime, L"hh:mm tt");
            SYSTEMTIME initEndTime;
            Helpers::Time_tToSystemTime(CurrentStartOfTheDay, &initEndTime);
            initEndTime.wHour = 8;
            initEndTime.wMinute = 30;
            DateTime_SetSystemtime(hEndTime, GDT_VALID, &initEndTime);

            HWND hEventName = ::GetDlgItem(hwnd, IDC_EVENT_NAME);
            Edit_SetText(hEventName, L"New Event");
        }
        break;

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    HWND hStartTime = ::GetDlgItem(hwnd, IDC_STARTTIME);
                    SYSTEMTIME startTime;
                    DateTime_GetSystemtime(hStartTime, &startTime);

                    SYSTEMTIME endTime;
                    HWND hEndTime = ::GetDlgItem(hwnd, IDC_ENDTIME);
                    DateTime_GetSystemtime(hEndTime, &endTime);

                    HWND hEventName = ::GetDlgItem(hwnd, IDC_EVENT_NAME);
                    WCHAR eventName[1024];
                    Edit_GetText(hEventName, eventName, 1024);

                    HWND hEventDescription = ::GetDlgItem(hwnd, IDC_EVENT_DESCRIPTION);
                    WCHAR eventDescription[1024];
                    Edit_GetText(hEventDescription, eventDescription, 1024);


                    Event newEvent;
                    newEvent.Name = eventName;
                    newEvent.Description = eventDescription;
                    newEvent.StartTimeInMin = startTime.wMinute + startTime.wHour * 60;
                    newEvent.EndTimeInMin = endTime.wMinute + endTime.wHour * 60;

                    Events.insert(EventContainer::value_type(CurrentStartOfTheDay, newEvent));

                    return ::EndDialog(hwnd, 1);
                }
                break;

                case IDCANCEL:
                {
                    return ::EndDialog(hwnd, 0);
                }
                break;
            }
            break;
        }
    }

    return FALSE;
}

INT_PTR CALLBACK EventListDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static auto startOfTheDay = 0;
    switch (uMsg)
    {
        case WM_CLOSE:
            return ::EndDialog(hwnd, 0);
            break;

        case WM_INITDIALOG:
        {
            Helpers::SetButtonIcon(hAppInstance, hwnd, IDC_DELETE_EVENT, IDI_DELETE_ICON);
            startOfTheDay = (time_t)lParam;
            LVCOLUMNW lvc;
            lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
            lvc.cx = 120;
            lvc.fmt = LVCFMT_LEFT;

            static WCHAR szEventListColumnString[4][64] = { L"Start Date", L"End Date", L"Name", L"Description" };
            HWND hEventListView = ::GetDlgItem(hwnd, IDC_EVENT_LIST);
            ListView_SetExtendedListViewStyle(hEventListView, LVS_EX_AUTOSIZECOLUMNS | LVS_EX_FULLROWSELECT);

            for (auto column = 0; column < 4; ++column)
            {
                lvc.pszText = szEventListColumnString[column];
                if (ListView_InsertColumn(hEventListView, column, &lvc) == -1)
                {
                    return FALSE;
                }
            }

            auto eventList = Events.equal_range(startOfTheDay);
            for (auto it = eventList.first; it != eventList.second; ++it)
            {
                LVITEMW lvi = { };
                lvi.mask = LVIF_TEXT;
                ListView_InsertItem(hEventListView, &lvi);

                WCHAR startTimeString[16];
                wsprintfW(startTimeString, L"%02d:%02d", it->second.StartTimeInMin / 60, it->second.StartTimeInMin % 60);
                ListView_SetItemText(hEventListView, lvi.iItem, 0, startTimeString);

                WCHAR endTimeString[16];
                wsprintfW(endTimeString, L"%02d:%02d", it->second.EndTimeInMin / 60, it->second.EndTimeInMin % 60);
                ListView_SetItemText(hEventListView, lvi.iItem, 1, endTimeString);

                ListView_SetItemText(hEventListView, lvi.iItem, 2, &it->second.Name[0]);
                ListView_SetItemText(hEventListView, lvi.iItem, 3, &it->second.Description[0]);
            }

        }
        break;

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_DELETE_EVENT:
                {
                    HWND hDirListView = ::GetDlgItem(hwnd, IDC_EVENT_LIST);
                    int iPos = ListView_GetNextItem(hDirListView, -1, LVNI_SELECTED);
                    LVITEMW lvi = { };
                    if (iPos != -1) {
                        ListView_GetItem(hDirListView, &lvi);
                            WCHAR buffer[MAX_PATH] = { 0 };
                            ListView_GetItemText(hDirListView, iPos, 2, buffer, MAX_PATH);

                            HWND hDirListView = ::GetDlgItem(hwnd, IDC_EVENT_LIST);
                            ListView_DeleteAllItems(hDirListView);

                            auto eventList = Events.equal_range(startOfTheDay);
                            for (auto it = eventList.first; it != eventList.second; ++it)
                            {
                                if (it->second.Name == buffer) {
                                    Events.erase(it);
                                    break;
                                }
                            }
                    }

                    HWND hEventListView = ::GetDlgItem(hwnd, IDC_EVENT_LIST);
                    ListView_SetExtendedListViewStyle(hEventListView, LVS_EX_AUTOSIZECOLUMNS | LVS_EX_FULLROWSELECT);
                    auto eventList = Events.equal_range(startOfTheDay);
                    for (auto it = eventList.first; it != eventList.second; ++it)
                    {
                        LVITEMW lvi = { };
                        lvi.mask = LVIF_TEXT;
                        ListView_InsertItem(hEventListView, &lvi);

                        WCHAR startTimeString[16];
                        wsprintfW(startTimeString, L"%02d:%02d", it->second.StartTimeInMin / 60, it->second.StartTimeInMin % 60);
                        ListView_SetItemText(hEventListView, lvi.iItem, 0, startTimeString);

                        WCHAR endTimeString[16];
                        wsprintfW(endTimeString, L"%02d:%02d", it->second.EndTimeInMin / 60, it->second.EndTimeInMin % 60);
                        ListView_SetItemText(hEventListView, lvi.iItem, 1, endTimeString);

                        ListView_SetItemText(hEventListView, lvi.iItem, 2, &it->second.Name[0]);
                        ListView_SetItemText(hEventListView, lvi.iItem, 3, &it->second.Description[0]);
                    }
                }
                break;
                case IDOK:
                {
                    return ::EndDialog(hwnd, 0);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}