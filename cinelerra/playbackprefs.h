/*
 * CINELERRA
 * Copyright (C) 2008-2022 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#ifndef PLAYBACKPREFS_H
#define PLAYBACKPREFS_H

class PlaybackBicubicBicubic;
class PlaybackBicubicBilinear;
class PlaybackBilinearBilinear;
class PlaybackLanczos;
class PlaybackBufferBytes;
class PlaybackBufferSize;
class PlaybackDeblock;
class PlaybackDisableNoEdits;
class PlaybackHead;
class PlaybackHeadCount;
class PlaybackHost;
class PlaybackInterpolateRaw;
class PlaybackHWDecode;
class PlaybackFFmpegMov;
class PlaybackModuleFragment;
class PlaybackNearest;
class PlaybackOutBits;
class PlaybackOutChannels;
class PlaybackOutPath;
class PlaybackPreload;
class PlaybackReadLength;
class PlaybackRealTime;
class PlaybackSoftwareTimer;
class PlaybackViewFollows;
class PlaybackWhiteBalanceRaw;
class DisableMutedTracks;
//class VideoAsynchronous;

#include "adeviceprefs.h"
#include "guicast.h"
#include "mwindow.inc"
#include "preferencesthread.h"
#include "vdeviceprefs.h"

class PlaybackPrefs : public PreferencesDialog
{
public:
	PlaybackPrefs(MWindow *mwindow, PreferencesWindow *pwindow);
	~PlaybackPrefs();

	void create_objects();
//	int set_strategy(int strategy);
	int get_buffer_bytes();

	static char* strategy_to_string(int strategy);
	void delete_strategy();

//	int draw_framerate(int flush /* = 1 */);

	ADevicePrefs *audio_device;
	VDevicePrefs *video_device;
	ArrayList<BC_ListBoxItem*> strategies;

	PlaybackConfig *playback_config;
//	BC_Title *framerate_title;

	BC_Title *vdevice_title;
	PlaybackHWDecode *hw_decode;
    PlaybackFFmpegMov *ffmpeg_mov;
};

class PlaybackModuleFragment : public BC_PopupMenu
{
public:
	PlaybackModuleFragment(int x, 
		int y, 
		PreferencesWindow *pwindow, 
		PlaybackPrefs *playback, 
		char *text);
	int handle_event();
	PreferencesWindow *pwindow;
	PlaybackPrefs *playback;
};

class PlaybackAudioOffset : public BC_TumbleTextBox
{
public:
	PlaybackAudioOffset(PreferencesWindow *pwindow, 
		PlaybackPrefs *subwindow,
		int x, 
		int y);
	int handle_event();
	PreferencesWindow *pwindow;
	PlaybackPrefs *playback;
};


class PlaybackViewFollows : public BC_CheckBox
{
public:
	PlaybackViewFollows(PreferencesWindow *pwindow, int value, int y);
	int handle_event();
	PreferencesWindow *pwindow;
};

class PlaybackSoftwareTimer : public BC_CheckBox
{
public:
	PlaybackSoftwareTimer(PreferencesWindow *pwindow, int value, int y);
	int handle_event();
	PreferencesWindow *pwindow;
};

class PlaybackRealTime : public BC_CheckBox
{
public:
	PlaybackRealTime(PreferencesWindow *pwindow, int value, int y);
	int handle_event();
	PreferencesWindow *pwindow;
};


class PlaybackHWDecode : public BC_CheckBox
{
public:
	PlaybackHWDecode(int x, 
		int y, 
		PreferencesWindow *pwindow, 
	    PlaybackPrefs *playback);
	int handle_event();
	PreferencesWindow *pwindow;
	PlaybackPrefs *playback;
};

class PlaybackFFmpegMov : public BC_CheckBox
{
public:
	PlaybackFFmpegMov(int x, 
		int y, 
		PreferencesWindow *pwindow, 
	    PlaybackPrefs *playback);
	int handle_event();
	PreferencesWindow *pwindow;
	PlaybackPrefs *playback;
};


#endif
