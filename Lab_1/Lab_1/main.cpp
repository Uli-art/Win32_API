#ifndef UNICODE
#define UNICODE
#endif


#include "MCIMediaPlayer.h"
#include "Helpers.h"
#include "resource.h"

#include <windows.h>
#include <CommCtrl.h>

#include <string>

static MCIMediaPlayer* pMediaPlayer = NULL;
static HINSTANCE hAppInstance = NULL;
static HWND hMainWindow = NULL;

LRESULT CALLBACK MainDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void UpdatePlayerControls();
void UpdatePlayPosition();


int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PWSTR pCmdLine, _In_ int nCmdShow)
{
    hAppInstance = hInstance;

    HWND hMainDialog = ::CreateDialog(hInstance, MAKEINTRESOURCE(IDD_MAINWND), NULL, MainDialogProc);
    if (hMainDialog == NULL)
    {
        return -1;
    }

    hMainWindow = hMainDialog;

    ::ShowWindow(hMainDialog, nCmdShow);
    ::UpdateWindow(hMainDialog);

    pMediaPlayer = new MCIMediaPlayer(hMainDialog);
    UpdatePlayerControls();

    MSG msg = { };
    while (::GetMessage(&msg, NULL, 0, 0) > 0)
    {
        if (!IsDialogMessage(hMainDialog, &msg))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
    }

    delete pMediaPlayer;

    return 0;
}

INT_PTR MainDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_DESTROY:
            ::PostQuitMessage(0);
            return TRUE;

        case WM_CLOSE:
            ::DestroyWindow(hwnd);
            return TRUE;

        case WM_TIMER:
            if (wParam == IDT_TIMER)
            {
                UpdatePlayPosition();
            }
            return TRUE;

        case MM_MCINOTIFY:
        case MM_MCISIGNAL:
            pMediaPlayer->handleMCINotify(hwnd, uMsg, wParam, lParam);
            return TRUE;

        case WM_HSCROLL:
        {
            int position = ::SendDlgItemMessage(hMainWindow, IDC_PROGRESS_SLIDER, TBM_GETPOS, 0, 0);
            pMediaPlayer->setPlayPostionInMs(position);

            return TRUE;
        }
        break;

        case MCIMediaPlayer::MCI_MP_TRACK_ADDED:
        {
            int trackIndex = static_cast<int>(wParam);
            MCIMediaPlayer::TrackInfo* trackInfo = reinterpret_cast<MCIMediaPlayer::TrackInfo*>(lParam);

            char trackTitle[1024];
            int trackLengthInSec = trackInfo->LengthInMs / 1000;
            std::snprintf(trackTitle, sizeof(trackTitle), "%s [%02d:%02d:%02d]", 
                trackInfo->Title.c_str(), trackLengthInSec / 3600, (trackLengthInSec % 3600) / 60, (trackLengthInSec % 3600) % 60);

            LRESULT index = ::SendDlgItemMessage(hMainWindow, IDC_PLAYLIST, LB_ADDSTRING, 0,
                (LPARAM)Helpers::StringToWideString(std::string(trackTitle)).c_str());
            ::SendDlgItemMessage(hMainWindow, IDC_PLAYLIST, LB_SETITEMDATA, index, (LPARAM)trackInfo);

            UpdatePlayerControls();
        }
        break;

        case MCIMediaPlayer::MCI_MP_TRACK_REMOVED:
        {
            int trackIndex = static_cast<int>(wParam);
            ::SendDlgItemMessage(hMainWindow, IDC_PLAYLIST, LB_DELETESTRING, trackIndex, 0);
            UpdatePlayerControls();
        }
        break;

        case MCIMediaPlayer::MCI_MP_PLAYLIST_LOADED:
        {
            ::SendDlgItemMessage(hMainWindow, IDC_PLAYLIST, LB_SETCURSEL, 0, 0);
            UpdatePlayerControls();
        }
        break;

        case MCIMediaPlayer::MCI_MP_PLAYLIST_CLEAR:
        {
            ::SendDlgItemMessage(hMainWindow, IDC_PLAYLIST, LB_RESETCONTENT, 0, 0);
            UpdatePlayerControls();
        }
        break;

        case MCIMediaPlayer::MCI_MP_PLAY_TRACK:
        {
            int trackIndex = static_cast<int>(wParam);
            ::SendDlgItemMessage(hMainWindow, IDC_PLAYLIST, LB_SETCURSEL, trackIndex, 0);
            Helpers::SetButtonIcon(hAppInstance, hwnd, IDC_PLAY_BUTTON, IDI_PAUSE_ICON);

            UpdatePlayerControls();

            ::SendDlgItemMessage(hMainWindow, IDC_PLAYLIST, LB_SETCURSEL, trackIndex, 0);

            MCIMediaPlayer::TrackInfo const& trackInfo = pMediaPlayer->getTrackInfo(trackIndex);

            ::SendDlgItemMessage(hMainWindow, IDC_PROGRESS_SLIDER, TBM_SETRANGEMIN, TRUE, 0);
            ::SendDlgItemMessage(hMainWindow, IDC_PROGRESS_SLIDER, TBM_SETRANGEMAX, TRUE, trackInfo.LengthInMs);
            ::SendDlgItemMessage(hMainWindow, IDC_PROGRESS_SLIDER, TBM_SETPAGESIZE, 0, 10000);

            UpdatePlayPosition();

            ::SetTimer(hwnd, IDT_TIMER, 100, NULL);
        }
        break;

        case MCIMediaPlayer::MCI_MP_STOP_TRACK:
        case MCIMediaPlayer::MCI_MP_PAUSE_TRACK:
        {
            ::KillTimer(hwnd, IDT_TIMER);

            Helpers::SetButtonIcon(hAppInstance, hwnd, IDC_PLAY_BUTTON, IDI_PLAY_ICON);
            UpdatePlayerControls();
        }
        break;

        case MCIMediaPlayer::MCI_MP_TRACK_CHANGED:
        {
            int trackIndex = static_cast<int>(wParam);
            ::SendDlgItemMessage(hMainWindow, IDC_PLAYLIST, LB_SETCURSEL, trackIndex, 0);

            UpdatePlayerControls();
        }
        break;

        case WM_COMMAND:        
            switch (LOWORD(wParam))
            {
                case IDC_OPEN_PLAYLIST_BUTTON:
                {
                    std::string playListFilePath = Helpers::BrowseForFile(hAppInstance, hwnd, TEXT("Open Playlist"), 
                        TEXT("Playlist Files\0*.playlist\0"), TEXT("playlist"), false);
                    if (playListFilePath.empty())
                    {
                        return TRUE;
                    }

                    if (!pMediaPlayer->openPlayList(playListFilePath))
                    {
                        ::MessageBox(hMainWindow, TEXT("Failed to open playlist"), TEXT("Open Playlist"), MB_OK);
                    }
                    
                    return TRUE;
                }
                break;

                case IDC_PLAY_BUTTON:
                    if (pMediaPlayer->isPlaying())
                    {
                        pMediaPlayer->pause();
                    }
                    else
                    {
                        pMediaPlayer->play();
                    }
                   
                    return TRUE;

                case IDC_PREV_BUTTON:
                    pMediaPlayer->prev();
                    return TRUE;

                case IDC_NEXT_BUTTON:
                    pMediaPlayer->next();
                    return TRUE;

                case IDC_PLAYLIST:
                {
                    switch HIWORD(wParam)
                    {
                        case LBN_SELCHANGE:
                        {
                            int trackIndex = static_cast<int>(::SendDlgItemMessage(hMainWindow, IDC_PLAYLIST, LB_GETCURSEL, 0, 0));
                            if (trackIndex != LB_ERR)
                            {
                                pMediaPlayer->moveToTrack(trackIndex);

                                if (pMediaPlayer->isPlaying())
                                {
                                    pMediaPlayer->play();
                                }

                                UpdatePlayerControls();
                            }
                        }
                        break;

                        case LBN_DBLCLK:
                        {
                            int trackIndex = static_cast<int>(::SendDlgItemMessage(hMainWindow, IDC_PLAYLIST, LB_GETCURSEL, 0, 0));
                            if (trackIndex != LB_ERR)
                            {
                                pMediaPlayer->moveToTrack(trackIndex);
                                pMediaPlayer->play();

                                UpdatePlayerControls();
                            }
                        }
                        break;
                    }

                    return TRUE;
                }
                break;

                case IDC_ADDSONG_BUTTON:
                {
                    std::string trackFilePath = Helpers::BrowseForFile(hAppInstance, hwnd, TEXT("Add Track"),
                        TEXT("Audio Files\0*.mp3;*.wav;*.aiff;*.wma\0"), NULL, true);
                    if (trackFilePath.empty())
                    {
                        return TRUE;
                    }

                    if (!pMediaPlayer->addTrackToPlayList(trackFilePath))
                    {
                        ::MessageBox(hMainWindow, TEXT("Failed to add track"), TEXT("Add Track"), MB_OK);
                    }

                    pMediaPlayer->savePlayList();

                    return TRUE;

                }
                break;

                case IDC_REMOVESONG_BUTTON:
                {
                    int trackIndex = static_cast<int>(::SendDlgItemMessage(hMainWindow, IDC_PLAYLIST, LB_GETCURSEL, 0, 0));
                    if (trackIndex != LB_ERR)
                    {
                        pMediaPlayer->removeTrackFromPlayList(trackIndex);
                        pMediaPlayer->savePlayList();
                    }

                    return TRUE;
                }
                break;
            }
            break;
        
        case WM_INITDIALOG:
            Helpers::SetButtonIcon(hAppInstance, hwnd, IDC_NEW_PLAYLIST_BUTTON, IDI_NEW_PLAYLIST_ICON);
            Helpers::SetButtonIcon(hAppInstance, hwnd, IDC_OPEN_PLAYLIST_BUTTON, IDI_OPEN_PLAYLIST_ICON);
            Helpers::SetButtonIcon(hAppInstance, hwnd, IDC_SAVE_PLAYLIST_BUTTON, IDI_SAVE_PLAYLIST_ICON);
            Helpers::SetButtonIcon(hAppInstance, hwnd, IDC_ADDSONG_BUTTON, IDI_ADDSONG_ICON);
            Helpers::SetButtonIcon(hAppInstance, hwnd, IDC_REMOVESONG_BUTTON, IDI_REMOVESONG_ICON);
            Helpers::SetButtonIcon(hAppInstance, hwnd, IDC_PLAY_BUTTON, IDI_PLAY_ICON);
            Helpers::SetButtonIcon(hAppInstance, hwnd, IDC_NEXT_BUTTON, IDI_NEXT_ICON);
            Helpers::SetButtonIcon(hAppInstance, hwnd, IDC_PREV_BUTTON, IDI_PREV_ICON);

            return TRUE;
    }

    return FALSE;
}

void UpdatePlayerControls()
{
    Helpers::SetControlEnabled(hMainWindow, IDC_ADDSONG_BUTTON, pMediaPlayer->isPlaylistLoaded());
    Helpers::SetControlEnabled(hMainWindow, IDC_REMOVESONG_BUTTON, pMediaPlayer->isPlaylistLoaded());
    Helpers::SetControlEnabled(hMainWindow, IDC_PLAYLIST, pMediaPlayer->isPlaylistLoaded());

    Helpers::SetControlEnabled(hMainWindow, IDC_PROGRESS_SLIDER, pMediaPlayer->isPlaying());
    Helpers::SetControlEnabled(hMainWindow, IDC_CURRENT_TIME_STATIC, pMediaPlayer->isPlaying());

    Helpers::SetControlEnabled(hMainWindow, IDC_PLAY_BUTTON, pMediaPlayer->trackCount() > 0);
    Helpers::SetControlEnabled(hMainWindow, IDC_PREV_BUTTON, pMediaPlayer->hasPrev());
    Helpers::SetControlEnabled(hMainWindow, IDC_NEXT_BUTTON, pMediaPlayer->hasNext());

    UpdatePlayPosition();
}

void UpdatePlayPosition()
{
    unsigned long long position = pMediaPlayer->getPlayPostionInMs();
    ::SendDlgItemMessage(hMainWindow, IDC_PROGRESS_SLIDER, TBM_SETPOS, TRUE, position);

    position /= 1000;
    char positionTime[64];
    std::snprintf(positionTime, sizeof(positionTime), "%02d:%02d:%02d", position / 3600, (position % 3600) / 60, (position % 3600) % 60);

    ::SendDlgItemMessage(hMainWindow, IDC_CURRENT_TIME_STATIC, WM_SETTEXT, NULL, (LPARAM)Helpers::StringToWideString(positionTime).c_str());
}