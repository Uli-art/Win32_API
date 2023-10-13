#pragma once

#include <string>
#include <vector>

#include <windows.h>
#include <mciapi.h>

class MCIMediaPlayer
{
public:
	struct TrackInfo
	{
		MCIDEVICEID MciDeviceId;

		std::string Uri;
		std::string Title;
		std::string Artist;
		unsigned long long LengthInMs;
	};

	typedef std::vector<TrackInfo> TrackList;

	enum HWNDMessage
	{
		MCI_MP_PLAYLIST_LOADED = WM_APP + 1,
		MCI_MP_PLAYLIST_SAVED,
		MCI_MP_PLAYLIST_CLEAR,
		MCI_MP_TRACK_ADDED,
		MCI_MP_TRACK_REMOVED,
		MCI_MP_PLAY_TRACK,
		MCI_MP_PAUSE_TRACK,
		MCI_MP_STOP_TRACK,
		MCI_MP_TRACK_CHANGED
	};

	MCIMediaPlayer(HWND mciNotifyHandle);

	bool play();
	bool pause();
	bool stop();
	bool next();
	bool prev();
	bool moveToTrack(int trackIndex);

	bool hasNext() const;
	bool hasPrev() const;

	bool isPlaylistLoaded() const;
	bool isPlaying() const;

	bool openPlayList(std::string const& filePath);
	bool savePlayList();
	bool savePlayListAs(std::string const& filePath);
	bool addTrackToPlayList(std::string const& uri);
	bool removeTrackFromPlayList(int trackIndex);

	TrackInfo const& getTrackInfo(int trackIndex) const;
	TrackList::const_iterator beginTrack() const;
	TrackList::const_iterator endTrack() const;
	int trackCount() const;

	unsigned long long getPlayPostionInMs() const;
	bool setPlayPostionInMs(unsigned long long position);

	LRESULT handleMCINotify(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	bool mciOpenMedia(std::string const& uri, TrackInfo& trackInfo);
	bool mciPlayMedia(TrackInfo& trackInfo);
	bool mciPauseMedia(TrackInfo& trackInfo);
	bool mciCloseMedia(TrackInfo& trackInfo);
	
	bool addTrackPrivate(std::string const& uri);
	void clearPlayList();

	void notifyWindow(HWNDMessage msg);
	void notifyWindow(HWNDMessage msg,WPARAM wParam);
	void notifyWindow(HWNDMessage msg, WPARAM wParam, LPARAM lParam);

private:
	enum Status
	{
		PLAY,
		STOP,
		PAUSE
	};
	Status _currentStatus;

	TrackList _trackList;
	int _currentTrackIndex;

	std::string _playListFilePath;

	HWND _notifyHandle;
};


