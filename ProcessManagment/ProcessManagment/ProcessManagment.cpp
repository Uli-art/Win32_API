#include "resource.h"

#include <windows.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <winternl.h>

#include <strsafe.h>

#include <string>
#include <list>

#include "Helpers.h"
#include "SystemInformation.h"

#pragma comment(lib,"ntdll.lib")


#define IDT_REFRESH_TIMER	1001


struct CurrentState {
    int CurrentPosition = 0;
    DWORD CurrentPID = NULL;
    BOOL IsSuspended = 0;
};

static HINSTANCE hAppInstance = NULL;
static HWND hMainWindow = NULL;

static CurrentState CurrentProcess;
static HANDLE hWatchThread = NULL;
static HANDLE hStopWatchingEvent = NULL;


INT_PTR CALLBACK MainDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
//  Forward declarations:
BOOL GetProcessList(HWND hDirListView);
BOOL ListProcessModules(DWORD dwPID);
BOOL ListProcessThreads(DWORD dwOwnerPID);
BOOL PopulateProcessListAndStatus(HWND hDirListView);
void printError(const TCHAR* msg);

void SuspendProcess(DWORD PID);
void ResumeProcess(DWORD PID);
void TerminateProcess(DWORD PID);
void CreateProcessFromPath(std::wstring path);

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PWSTR pCmdLine, _In_ int nCmdShow)
{
    hAppInstance = hInstance;


    hMainWindow = ::CreateDialogParam(hInstance, MAKEINTRESOURCE(IDD_MAIN_DIALOG), NULL, MainDialogProc, 0);
    if (hMainWindow == NULL)
    {
        return -1;
    }

    HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PROCESSMANAGMENT));
    SendMessage(hMainWindow, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);


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
        ::KillTimer(hwnd, IDT_REFRESH_TIMER);
        ::PostQuitMessage(0);
        return TRUE;

    case WM_CLOSE:
        ::DestroyWindow(hwnd);
        return TRUE;

    case WM_TIMER:
        if (wParam == IDT_REFRESH_TIMER)
        {
            HWND hDirListView = ::GetDlgItem(hwnd, IDC_PROCESS_LIST);
            ListView_DeleteAllItems(hDirListView);
            PopulateProcessListAndStatus(hDirListView);
        }
        return TRUE;

    case WM_NOTIFY:
    {
        switch (LOWORD(wParam))
        {
        case IDC_PROCESS_LIST:
            if (((LPNMHDR)lParam)->code == NM_CLICK)
            {
                HWND hDirListView = ::GetDlgItem(hwnd, IDC_PROCESS_LIST);
                int iPos = ListView_GetNextItem(hDirListView, -1, LVNI_SELECTED);
                LVITEMW lvi = { };
                if (iPos != -1) {
                    ListView_GetItem(hDirListView, &lvi);
                    WCHAR PID[MAX_PATH] = { 0 };
                    ListView_GetItemText(hDirListView, iPos, 1, PID, MAX_PATH);
                    DWORD pid = _wtoi(std::wstring(PID).c_str());

                    if (pid >= 0) {
                        CurrentProcess.CurrentPID = pid;
                        CurrentProcess.CurrentPosition = iPos;
                        WCHAR watingReason[MAX_PATH] = { 0 };
                        ListView_GetItemText(hDirListView, iPos, 4, watingReason, MAX_PATH);
                        if (wcscmp(watingReason, ThreadWaitReasonValueNames[5]) == 0) {
                            Helpers::SetButtonIcon(hAppInstance, hwnd, IDC_PLAY_PAUSE_BUTTON, IDI_PLAY_ICON);
                        }
                        else {
                            Helpers::SetButtonIcon(hAppInstance, hwnd, IDC_PLAY_PAUSE_BUTTON, IDI_PAUSE_ICON);
                        }
                    }
                }
            }
            return TRUE;
        }
        break;
    }

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {

        case IDC_PLAY_PAUSE_BUTTON:
        {
            HWND hDirListView = ::GetDlgItem(hwnd, IDC_PROCESS_LIST);
            LVITEMW lvi = { };
            ListView_GetItem(hDirListView, &lvi);

            if (CurrentProcess.IsSuspended) {
                ResumeProcess(CurrentProcess.CurrentPID);
                Helpers::SetButtonIcon(hAppInstance, hwnd, IDC_PLAY_PAUSE_BUTTON, IDI_PAUSE_ICON);
            }
            else {
                SuspendProcess(CurrentProcess.CurrentPID);
                Helpers::SetButtonIcon(hAppInstance, hwnd, IDC_PLAY_PAUSE_BUTTON, IDI_PLAY_ICON);
            }
            CurrentProcess.IsSuspended = !CurrentProcess.IsSuspended;
            PopulateProcessListAndStatus(::GetDlgItem(hwnd, IDC_PROCESS_LIST));
        }
        break;

        case IDC_STOP_BUTTON:
        {
            TerminateProcess(CurrentProcess.CurrentPID);
            CurrentProcess = {};
            PopulateProcessListAndStatus(::GetDlgItem(hwnd, IDC_PROCESS_LIST));

        }
        break;

        case IDC_ADD_BUTTON:
        {
            std::wstring filePath = Helpers::BrowseForFile(hAppInstance, hwnd, TEXT("Open exe"),
                TEXT("Exe Files\0*.exe\0"), TEXT("Exe files"), false);
            CreateProcessFromPath(filePath);
            PopulateProcessListAndStatus(::GetDlgItem(hwnd, IDC_PROCESS_LIST));
        }
        break;

        case IDC_REFRESH_BUTTON:
        {
            PopulateProcessListAndStatus(::GetDlgItem(hwnd, IDC_PROCESS_LIST));
           // GetProcessList(::GetDlgItem(hwnd, IDC_PROCESS_LIST));
        }
        break;
        }
    }
    break;

    case WM_INITDIALOG:
    {
        Helpers::SetButtonIcon(hAppInstance, hwnd, IDC_PLAY_PAUSE_BUTTON, IDI_PAUSE_ICON);
        Helpers::SetButtonIcon(hAppInstance, hwnd, IDC_ADD_BUTTON, IDI_ADD_ICON);
        Helpers::SetButtonIcon(hAppInstance, hwnd, IDC_STOP_BUTTON, IDI_STOP_ICON);
        Helpers::SetButtonIcon(hAppInstance, hwnd, IDC_REFRESH_BUTTON, IDI_REFRESH_ICON);

        
        LVCOLUMNW lvc;
        lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        lvc.cx = 120;
        lvc.fmt = LVCFMT_LEFT;


        static WCHAR szDirListColumnString[5][64] = { L"Name", L"PID", L"Thread count", L"State", L"Wait reason"};
        HWND hDirListView = ::GetDlgItem(hwnd, IDC_PROCESS_LIST);
        ListView_SetExtendedListViewStyle(hDirListView, LVS_EX_AUTOSIZECOLUMNS | LVS_EX_FULLROWSELECT);

        for (auto column = 0; column < 5; ++column)
        {
            lvc.pszText = szDirListColumnString[column];
            if (ListView_InsertColumn(hDirListView, column, &lvc) == -1)
            {
                return FALSE;
            }
        }

        PopulateProcessListAndStatus(hDirListView);
        //::SetTimer(hwnd, IDT_REFRESH_TIMER, 5000, NULL);
    }
    break;
    }
    return FALSE;
}



BOOL PopulateProcessListAndStatus(HWND hDirListView) 
{
    ListView_DeleteAllItems(hDirListView);

    NTSTATUS status;
    PVOID buffer;
    PSYSTEM_PROCESS_INFO spi;

    buffer = VirtualAlloc(NULL, 1024 * 1024, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    if (!buffer)
    {
        printf("\nError: Unable to allocate memory for process list (%d)\n", GetLastError());
        return -1;
    }

    spi = (PSYSTEM_PROCESS_INFO)buffer;

    if (!NT_SUCCESS(status = NtQuerySystemInformation(SystemProcessInformation, spi, 1024 * 1024, NULL)))
    {
        printf("\nError: Unable to query process list (%#x)\n", status);

        VirtualFree(buffer, 0, MEM_RELEASE);
        return -1;
    }

    LVITEMW lvi = { };
    lvi.mask = LVIF_PARAM | LVIF_IMAGE | LVIF_TEXT;
    lvi.iSubItem = 0;
    WCHAR pszText[] = L"Idle";
    lvi.pszText = pszText;

    while (spi->pi.NextEntryOffset) 
    {
        if (spi->pi.ImageName.Buffer != NULL)
            lvi.pszText = spi->pi.ImageName.Buffer;
        ListView_InsertItem(hDirListView, &lvi);

        WCHAR PIDString[256];
        wsprintfW(PIDString, L"%d", spi->pi.UniqueProcessId);

        ListView_SetItemText(hDirListView, lvi.iItem, 1, PIDString);

        WCHAR ThreandString[256];
        wsprintfW(ThreandString, L"%d", spi->pi.NumberOfThreads);

        ListView_SetItemText(hDirListView, lvi.iItem, 2, ThreandString);
        
        if (spi->ti->ThreadState < 8) {
            ListView_SetItemText(hDirListView, lvi.iItem, 3, (LPWSTR)ThreadStateValueNames[spi->ti->ThreadState]);
        }
        else {
            WCHAR StateString[256];
            wsprintfW(StateString, L"%d", spi->ti->ThreadState);
            ListView_SetItemText(hDirListView, lvi.iItem, 3, StateString);
        }
        if (spi->ti->WaitReason < 38) {
            ListView_SetItemText(hDirListView, lvi.iItem, 4, (LPWSTR)ThreadWaitReasonValueNames[spi->ti->WaitReason]);
        }
        else {
            WCHAR ReasonString[256];
            wsprintfW(ReasonString, L"%d", spi->ti->WaitReason);
            ListView_SetItemText(hDirListView, lvi.iItem, 4, ReasonString);
        }

        ++lvi.iItem;
        spi = (PSYSTEM_PROCESS_INFO)((LPBYTE)spi + spi->pi.NextEntryOffset);
    }

    VirtualFree(buffer, 0, MEM_RELEASE);
    return 0;
}

void SuspendProcess(DWORD PID)
{
    ::DebugActiveProcess(PID);
}

void ResumeProcess(DWORD PID) {
    ::DebugActiveProcessStop(PID);
}

void TerminateProcess(DWORD PID) {
    const auto explorer = OpenProcess(PROCESS_TERMINATE, false, PID);
    TerminateProcess(explorer, 0);
    CloseHandle(explorer);
}

void CreateProcessFromPath(std::wstring path) {
    STARTUPINFO info = { sizeof(info) };
    PROCESS_INFORMATION processInfo;
    if (CreateProcess(path.data(), NULL, NULL, NULL, TRUE, 0, NULL, NULL, &info, &processInfo)) {
        CloseHandle(processInfo.hProcess); 
        CloseHandle(processInfo.hThread);
    }
}




















BOOL GetProcessList(HWND hDirListView)
{
    HANDLE hProcessSnap;
    HANDLE hProcess;
    PROCESSENTRY32 pe32;
    DWORD dwPriorityClass;

    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE)
    {
        printError(TEXT("CreateToolhelp32Snapshot (of processes)"));
        return(FALSE);
    }

    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hProcessSnap, &pe32))
    {
        printError(TEXT("Process32First"));
        CloseHandle(hProcessSnap);         
        return(FALSE);
    }

    LVITEMW lvi = { };
    lvi.mask = LVIF_PARAM | LVIF_IMAGE | LVIF_TEXT;
    lvi.iSubItem = 0;
    lvi.pszText = (LPWSTR)"1";

    do
    {
        dwPriorityClass = 0;
        hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);
        if (hProcess == NULL)
            printError(TEXT("OpenProcess"));
        else
        {
            dwPriorityClass = GetPriorityClass(hProcess);
            if (!dwPriorityClass)
                printError(TEXT("GetPriorityClass"));
            CloseHandle(hProcess);
        }

        lvi.pszText = pe32.szExeFile;
        ListView_InsertItem(hDirListView, &lvi);

        WCHAR PIDString[256];
        wsprintfW(PIDString, L"%d", pe32.th32ProcessID);

        ListView_SetItemText(hDirListView, lvi.iItem, 1, PIDString);

        WCHAR ThreandString[256];
        wsprintfW(ThreandString, L"%d", pe32.cntThreads);

        ListView_SetItemText(hDirListView, lvi.iItem, 2, ThreandString);


        WCHAR StatusString[256] = L"Active";
        lvi.lParam = 0;
        ListView_SetItemText(hDirListView, lvi.iItem, 3, StatusString);

        ++lvi.iItem;
    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
    return(TRUE);
}



BOOL ListProcessModules(DWORD dwPID)
{
    HANDLE hModuleSnap = INVALID_HANDLE_VALUE;
    MODULEENTRY32 me32;
    hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwPID);
    if (hModuleSnap == INVALID_HANDLE_VALUE)
    {
        printError(TEXT("CreateToolhelp32Snapshot (of modules)"));
        return(FALSE);
    }

    me32.dwSize = sizeof(MODULEENTRY32);

    if (!Module32First(hModuleSnap, &me32))
    {
        printError(TEXT("Module32First"));  
        CloseHandle(hModuleSnap);          
        return(FALSE);
    }

    do
    {
        _tprintf(TEXT("\n\n     MODULE NAME:     %s"), me32.szModule);
        _tprintf(TEXT("\n     Executable     = %s"), me32.szExePath);
        _tprintf(TEXT("\n     Process ID     = 0x%08X"), me32.th32ProcessID);
        _tprintf(TEXT("\n     Ref count (g)  = 0x%04X"), me32.GlblcntUsage);
        _tprintf(TEXT("\n     Ref count (p)  = 0x%04X"), me32.ProccntUsage);
        _tprintf(TEXT("\n     Base address   = 0x%08X"), (DWORD)me32.modBaseAddr);
        _tprintf(TEXT("\n     Base size      = %d"), me32.modBaseSize);

    } while (Module32Next(hModuleSnap, &me32));

    CloseHandle(hModuleSnap);
    return(TRUE);
}

BOOL ListProcessThreads(DWORD dwOwnerPID)
{
    HANDLE hThreadSnap = INVALID_HANDLE_VALUE;
    THREADENTRY32 te32;

    hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hThreadSnap == INVALID_HANDLE_VALUE)
        return(FALSE);

    te32.dwSize = sizeof(THREADENTRY32);

    if (!Thread32First(hThreadSnap, &te32))
    {
        printError(TEXT("Thread32First")); 
        CloseHandle(hThreadSnap);          
        return(FALSE);
    }

    do
    {
        if (te32.th32OwnerProcessID == dwOwnerPID)
        {

            _tprintf(TEXT("\n\n     THREAD ID      = 0x%08X"), te32.th32ThreadID);
            _tprintf(TEXT("\n     Base priority  = %d"), te32.tpBasePri);
            _tprintf(TEXT("\n     Delta priority = %d"), te32.tpDeltaPri);
            _tprintf(TEXT("\n"));
        }
    } while (Thread32Next(hThreadSnap, &te32));

    CloseHandle(hThreadSnap);
    return(TRUE);
}

void printError(const TCHAR* msg)
{
    DWORD eNum;
    TCHAR sysMsg[256];
    TCHAR* p;

    eNum = GetLastError();
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, eNum,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
        sysMsg, 256, NULL);

    p = sysMsg;
    while ((*p > 31) || (*p == 9))
        ++p;
    do { *p-- = 0; } while ((p >= sysMsg) &&
        ((*p == '.') || (*p < 33)));

    _tprintf(TEXT("\n  WARNING: %s failed with error %d (%s)"), msg, eNum, sysMsg);
}