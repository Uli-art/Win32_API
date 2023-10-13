#include "MCIMediaPlayer.h"
#include "Helpers.h"

#ifndef UNICODE
#define UNICODE
#endif

#include <cassert>
#include <fstream>

MCIMediaPlayer::MCIMediaPlayer(HWND mciNotifyHandle) :
	_currentStatus(STOP),
	_trackList(),
	_currentTrackIndex(-1),
	_playListFilePath(),
	_notifyHandle(mciNotifyHandle)
{
}

bool MCIMediaPlayer::play()
{
	if (_currentTrackIndex < 0)
	{
		return false;
	}

	TrackInfo& trackInfo = _trackList[_currentTrackIndex];
	if (mciPlayMedia(trackInfo))
	{
		_currentStatus = PLAY;
		notifyWindow(MCI_MP_PLAY_TRACK, _currentTrackIndex);

		return true;
	}
	
	return false;
}

bool MCIMediaPlayer::stop()
{
	if (_currentTrackIndex < 0)
	{
		return false;
	}

	TrackInfo& trackInfo = _trackList[_currentTrackIndex];
	if (mciCloseMedia(trackInfo))
	{
		_currentStatus = STOP;
		notifyWindow(MCI_MP_STOP_TRACK);

		return true;
	}

	return false;
}

bool MCIMediaPlayer::pause()
{
	if (_currentTrackIndex < 0)
	{
		return false;
	}

	TrackInfo& trackInfo = _trackList[_currentTrackIndex];
	if (mciPauseMedia(trackInfo))
	{
		_currentStatus = PAUSE;
		notifyWindow(MCI_MP_PAUSE_TRACK);

		return true;
	}
	
	return false;;
}

bool MCIMediaPlayer::next()
{
	if (!hasNext())
	{
		return false;
	}

	moveToTrack(_currentTrackIndex + 1);

	if (isPlaying())
	{
		play();
	}
	return true;
}

bool MCIMediaPlayer::prev()
{
	if (!hasPrev())
	{
		return false;
	}

	moveToTrack(_currentTrackIndex - 1);

	if (isPlaying())
	{
		play();
	}
	return true;
}

bool MCIMediaPlayer::moveToTrack(int trackIndex)
{
	if (trackIndex < 0 || trackIndex >= _trackList.size())
	{
		return false;
	}

	if (isPlaying() && _currentTrackIndex != -1)
	{
		mciCloseMedia(_trackList[_currentTrackIndex]);
	}

	_currentTrackIndex = trackIndex;
	notifyWindow(MCI_MP_TRACK_CHANGED, _currentTrackIndex);
	return true;
}

bool MCIMediaPlayer::hasNext() const
{
	if (_currentTrackIndex + 1 >= _trackList.size())
	{
		return false;
	}

	return true;
}

bool MCIMediaPlayer::hasPrev() const
{

	if (_currentTrackIndex - 1 < 0)
	{
		return false;
	}

	return true;
}

bool MCIMediaPlayer::isPlaylistLoaded() const
{
	return !_playListFilePath.empty();
}

bool MCIMediaPlayer::isPlaying() const
{
	return _currentStatus == PLAY;
}

bool MCIMediaPlayer::openPlayList(std::string const& filePath)
{
	std::ifstream playListStream(filePath);
	std::string nextTrackPath;

	stop();
	clearPlayList();

	while (std::getline(playListStream, nextTrackPath))
	{
		if (nextTrackPath.empty())
		{
			continue;
		}

		addTrackPrivate(nextTrackPath);
	}

	_playListFilePath = filePath;
	_currentTrackIndex = _trackList.empty() ? -1 : 0;

	notifyWindow(MCI_MP_PLAYLIST_LOADED);

	return true;
}

bool MCIMediaPlayer::savePlayList()
{
	if (_playListFilePath.empty())
	{
		return false;
	}

	std::ofstream playListStream(_playListFilePath, std::ofstream::out | std::ofstream::trunc);
	for(auto nextTrackInfo : _trackList)
	{
		playListStream << nextTrackInfo.Uri << std::endl;
	}

	notifyWindow(MCI_MP_PLAYLIST_SAVED);
	return true;
}

bool MCIMediaPlayer::savePlayListAs(std::string const& filePath)
{
	return false;
}

bool MCIMediaPlayer::addTrackToPlayList(std::string const& uri)
{
	assert(!_playListFilePath.empty());

	return addTrackPrivate(uri);
}

bool MCIMediaPlayer::removeTrackFromPlayList(int trackIndex)
{
	assert(trackIndex >= 0 && trackIndex < _trackList.size());

	if (trackIndex == _currentTrackIndex && isPlaying())
	{
		stop();
	}

	_trackList.erase(std::next(_trackList.begin(), trackIndex));
	notifyWindow(MCI_MP_TRACK_REMOVED, trackIndex);

	if (trackIndex < _trackList.size())
	{
		moveToTrack(trackIndex);
	}
	else if (!_trackList.empty())
	{
		moveToTrack(trackIndex - 1);
	}

	return false;
}

bool MCIMediaPlayer::mciOpenMedia(std::string const& uri, TrackInfo& trackInfo)
{
	std::wstring wUri = Helpers::StringToWideString(uri);

	MCI_OPEN_PARMS mciOpenParams = { };

	mciOpenParams.lpstrDeviceType = L"mpegvideo";
	mciOpenParams.lpstrElementName = wUri.c_str();

	MCIERROR mciErroCode = mciSendCommand(0, MCI_OPEN,
		MCI_OPEN_TYPE | MCI_OPEN_ELEMENT, (LONG_PTR)(LPMCI_OPEN_PARMS)&mciOpenParams);

	if (mciErroCode != 0)
	{
		return false;
	}

	trackInfo.Uri = uri;
	trackInfo.MciDeviceId = mciOpenParams.wDeviceID;

	MCI_SET_PARMS mciSet = { NULL, MCI_FORMAT_MILLISECONDS, 0 };
	mciErroCode = mciSendCommand(trackInfo.MciDeviceId, MCI_SET, (DWORD)MCI_SET_TIME_FORMAT,
		(LONG_PTR)(LPMCI_SET_PARMS)&mciSet);
	if (mciErroCode != 0)
	{
		mciCloseMedia(trackInfo);
		return false;
	}

	MCI_STATUS_PARMS mciStatus = { NULL, 0, MCI_STATUS_LENGTH, 1 };
	mciErroCode = mciSendCommand(trackInfo.MciDeviceId, MCI_STATUS, (DWORD)MCI_STATUS_ITEM,
		(LONG_PTR)(LPMCI_STATUS_PARMS)&mciStatus);
	if (mciErroCode == 0)
	{
		trackInfo.LengthInMs = mciStatus.dwReturn;
	}

	return true;
}

bool MCIMediaPlayer::mciPlayMedia(TrackInfo& trackInfo)
{
	if (trackInfo.MciDeviceId == 0 && !mciOpenMedia(trackInfo.Uri, trackInfo))
	{
		return false;
	}

	MCI_PLAY_PARMS mciPlayParams = { };
	mciPlayParams.dwCallback = (LONG_PTR)_notifyHandle;

	MCIERROR mciErroCode = mciSendCommand(trackInfo.MciDeviceId, MCI_PLAY, MCI_NOTIFY,
		(LONG_PTR)(LPMCI_PLAY_PARMS)&mciPlayParams);

	if (mciErroCode != 0)
	{
		return false;
	}

	return true;
}

bool MCIMediaPlayer::mciPauseMedia(TrackInfo& trackInfo)
{
	MCI_GENERIC_PARMS mciGeneric;

	MCIERROR mciErroCode = mciSendCommand(trackInfo.MciDeviceId, MCI_PAUSE, MCI_WAIT,
		(LONG_PTR)(LPMCI_GENERIC_PARMS)&mciGeneric);

	if (mciErroCode != 0)
	{
		return false;
	}
	return true;
}

bool MCIMediaPlayer::mciCloseMedia(TrackInfo& trackInfo)
{
	MCI_GENERIC_PARMS mciGeneric;

	MCIERROR mciErroCode = mciSendCommand(trackInfo.MciDeviceId, MCI_CLOSE, MCI_WAIT,
		(LONG_PTR)(LPMCI_GENERIC_PARMS)&mciGeneric);
	
	if (mciErroCode != 0)
	{
		return false;
	}

	trackInfo.MciDeviceId = 0;

	return true;
}

MCIMediaPlayer::TrackInfo const& MCIMediaPlayer::getTrackInfo(int trackIndex) const
{
	assert(trackIndex >= 0 && trackIndex < _trackList.size());
	return _trackList[trackIndex];
}

MCIMediaPlayer::TrackList::const_iterator MCIMediaPlayer::beginTrack() const
{
	return _trackList.begin();
}

MCIMediaPlayer::TrackList::const_iterator MCIMediaPlayer::endTrack() const
{
	return _trackList.end();
}

int MCIMediaPlayer::trackCount() const
{
	return static_cast<int>(_trackList.size());
}

unsigned long long MCIMediaPlayer::getPlayPostionInMs() const
{
	if (!isPlaying())
	{
		return 0;
	}

	unsigned long long position = 0;
	TrackInfo const& trackInfo = _trackList[_currentTrackIndex];

	MCI_STATUS_PARMS mciStatus;

	mciStatus.dwTrack = 1;
	mciStatus.dwCallback = NULL;
	mciStatus.dwItem = MCI_STATUS_POSITION;

	MCIERROR mciErroCode = mciSendCommand(trackInfo.MciDeviceId, MCI_STATUS, (DWORD)MCI_STATUS_ITEM,
		(LONG_PTR)(LPMCI_STATUS_PARMS)&mciStatus);
	if (mciErroCode == 0)
	{
		return mciStatus.dwReturn;
	}

	return 0;
}

bool MCIMediaPlayer::setPlayPostionInMs(unsigned long long position)
{
	if (!isPlaying())
	{
		return false;
	}

	TrackInfo const& trackInfo = _trackList[_currentTrackIndex];

	MCI_SEEK_PARMS mciSeek = { NULL, position };

	MCIERROR mciErroCode = mciSendCommand(trackInfo.MciDeviceId, MCI_SEEK, MCI_WAIT | MCI_TO,
		(LONG_PTR)(LPMCI_SEEK_PARMS)&mciSeek);

	if (mciErroCode != 0)
	{
		return false;
	}

	play();

	return true;
}

LRESULT MCIMediaPlayer::handleMCINotify(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	assert(hwnd == _notifyHandle);
	
	switch (uMsg)
	{
		case MM_MCINOTIFY:
			if (wParam == MCI_NOTIFY_SUCCESSFUL)
			{
				if (hasNext())
				{
					next();
				}
				else
				{
					stop();
				}
			}
			break;

		case MM_MCISIGNAL:
			break;
	}

	return 0;
}

bool MCIMediaPlayer::addTrackPrivate(std::string const& uri)
{
	std::string baseFilename = uri.substr(uri.find_last_of("/\\") + 1);
	std::string::size_type const p(baseFilename.find_last_of('.'));
	std::string trackName = baseFilename.substr(0, p);

	TrackInfo trackInfo = { 0, uri, trackName, std::string(), 0 };

	if (mciOpenMedia(uri, trackInfo))
	{
		mciCloseMedia(trackInfo);

		_trackList.push_back(trackInfo);
		notifyWindow(MCI_MP_TRACK_ADDED, _trackList.size() - 1, reinterpret_cast<LPARAM>(&_trackList.back()));

		return true;
	}

	return false;
}

void MCIMediaPlayer::clearPlayList()
{
	notifyWindow(MCI_MP_PLAYLIST_CLEAR);

	_trackList.clear();
	_currentTrackIndex = -1;
}

void MCIMediaPlayer::notifyWindow(HWNDMessage msg)
{
	notifyWindow(msg, 0);
}

void MCIMediaPlayer::notifyWindow(HWNDMessage msg, WPARAM wParam)
{
	notifyWindow(msg, wParam, 0);
}

void  MCIMediaPlayer::notifyWindow(HWNDMessage msg, WPARAM wParam, LPARAM lParam)
{
	::SendMessage(_notifyHandle, (UINT)msg, wParam, lParam);

}