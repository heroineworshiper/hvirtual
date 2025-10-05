/*
 * CINELERRA
 * Copyright (C) 2010-2024 Adam Williams <broadcast at earthling dot net>
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

#include "adeviceprefs.h"
#include "audioconfig.h"
#include "audiodevice.inc"
#include "bcsignals.h"
#include "clip.h"
#include "bchash.h"
#include "edl.h"
#include "edlsession.h"
#include "language.h"
#include "mainsession.h"
#include "mwindow.h"
#include "overlayframe.inc"
#include "playbackprefs.h"
#include "preferences.h"
#include "theme.h"
#include "vdeviceprefs.h"
#include "videodevice.inc"



PlaybackPrefs::PlaybackPrefs(MWindow *mwindow, PreferencesWindow *pwindow)
 : PreferencesDialog(mwindow, pwindow)
{
	audio_device = 0;
	video_device = 0;
}

PlaybackPrefs::~PlaybackPrefs()
{
	delete audio_device;
	delete video_device;
}

void PlaybackPrefs::create_objects()
{
	int x, y, x1, x2;
	char string[BCTEXTLEN];
	BC_PopupTextBox *popup;
	BC_Resources *resources = BC_WindowBase::get_resources();


	playback_config = pwindow->thread->preferences->playback_config;

	x = mwindow->theme->preferencesoptions_x;
	y = mwindow->theme->preferencesoptions_y;
	int margin = mwindow->theme->widget_border;

// Audio
	BC_Title *title1, *title2;
	add_subwindow(title1 = new BC_Title(x, 
		y, 
		_("Audio Out"), 
		LARGEFONT));


	y += title1->get_h() + margin;


	add_subwindow(title2 = new BC_Title(x, y, _("Playback buffer samples:"), MEDIUMFONT));
	x2 = title2->get_x() + title2->get_w() + margin;

SET_TRACE
	sprintf(string, "%d", playback_config->aconfig->fragment_size);
	PlaybackModuleFragment *menu;
	add_subwindow(menu = new PlaybackModuleFragment(x2, 
		y, 
		pwindow, 
		this, 
		string));
	menu->add_item(new BC_MenuItem("1024"));
	menu->add_item(new BC_MenuItem("2048"));
	menu->add_item(new BC_MenuItem("4096"));
	menu->add_item(new BC_MenuItem("8192"));
	menu->add_item(new BC_MenuItem("16384"));
	menu->add_item(new BC_MenuItem("32768"));
	menu->add_item(new BC_MenuItem("65536"));
	menu->add_item(new BC_MenuItem("131072"));
	menu->add_item(new BC_MenuItem("262144"));

	y += menu->get_h() + margin;
	add_subwindow(title1 = new BC_Title(x, y, _("Audio offset (sec):")));
	x1 = x + title1->get_w() + margin;
	PlaybackAudioOffset *audio_offset = new PlaybackAudioOffset(pwindow,
		this,
		x1,
		y);
	audio_offset->create_objects();
	y += audio_offset->get_h() + margin;

SET_TRACE
	add_subwindow(new PlaybackViewFollows(pwindow, pwindow->thread->preferences->view_follows_playback, y));
	y += DP(30);
	add_subwindow(new PlaybackSoftwareTimer(pwindow, pwindow->thread->preferences->playback_software_position, y));
	y += DP(30);
	add_subwindow(new PlaybackRealTime(pwindow, pwindow->thread->preferences->real_time_playback, y));
	y += DP(40);
	add_subwindow(title1 = new BC_Title(x, y, _("Audio Driver:")));
	audio_device = new ADevicePrefs(title1->get_x() + title1->get_w() + margin, 
		y, 
		pwindow, 
		this, 
		playback_config->aconfig, 
		0,
		MODEPLAY);
	audio_device->initialize(0);

SET_TRACE



// Video
 	y += audio_device->get_h(0) + margin;

SET_TRACE
	add_subwindow(new BC_Bar(x, y, 	get_w() - x * 2));
	y += margin;

SET_TRACE
	add_subwindow(title1 = new BC_Title(x, y, _("Video Out"), LARGEFONT));
	y += title1->get_h() + margin;

//	add_subwindow(title1 = new BC_Title(x, y, _("Framerate achieved:")));
//  x1 = title1->get_x() + title1->get_w() + margin;
//	add_subwindow(framerate_title = new BC_Title(x1, y, _("--"), MEDIUMFONT, RED));
//	draw_framerate(0);
//	y += framerate_title->get_h() + margin;

//	add_subwindow(asynchronous = new VideoAsynchronous(pwindow, x, y));
//	y += asynchronous->get_h() + 10;

SET_TRACE
	BC_Title *title;


SET_TRACE
//	y += 30;
//	add_subwindow(new PlaybackDeblock(pwindow, 10, y));

	add_subwindow(vdevice_title = new BC_Title(x, y, _("Video Driver:")));
	video_device = new VDevicePrefs(x + vdevice_title->get_w() + margin, 
		y, 
		pwindow, 
		this, 
		playback_config->vconfig, 
		0,
		MODEPLAY);
	video_device->initialize(0);

SET_TRACE	

}


int PlaybackPrefs::get_buffer_bytes()
{
//	return pwindow->thread->edl->aconfig->oss_out_bits / 8 * pwindow->thread->preferences->aconfig->oss_out_channels * pwindow->thread->preferences->playback_buffer;
    return 0;
}

// int PlaybackPrefs::draw_framerate(int flush)
// {
// //printf("PlaybackPrefs::draw_framerate 1 %f\n", mwindow->session->actual_frame_rate);
// 	char string[BCTEXTLEN];
// 	sprintf(string, "%.4f", mwindow->session->actual_frame_rate);
// 	framerate_title->update(string, flush);
// 	return 0;
// }



PlaybackAudioOffset::PlaybackAudioOffset(PreferencesWindow *pwindow, 
	PlaybackPrefs *playback, 
	int x, 
	int y)
 : BC_TumbleTextBox(playback,
 	playback->playback_config->aconfig->audio_offset,
	-10.0,
	10.0,
	x,
	y,
	DP(100))
{
	this->pwindow = pwindow;
	this->playback = playback;
	set_precision(2);
	set_increment(0.1);
}

int PlaybackAudioOffset::handle_event()
{
	playback->playback_config->aconfig->audio_offset = atof(get_text());
	return 1;
}




PlaybackModuleFragment::PlaybackModuleFragment(int x, 
	int y, 
	PreferencesWindow *pwindow, 
	PlaybackPrefs *playback, 
	char *text)
 : BC_PopupMenu(x, 
 	y, 
	DP(130), 
	text,
	1)
{ 
	this->pwindow = pwindow;
	this->playback = playback;
}

int PlaybackModuleFragment::handle_event() 
{
	playback->playback_config->aconfig->fragment_size = atol(get_text()); 
	return 1;
}




PlaybackViewFollows::PlaybackViewFollows(PreferencesWindow *pwindow, int value, int y)
 : BC_CheckBox(10, y, value, _("View follows playback"))
{ 
	this->pwindow = pwindow; 
}

int PlaybackViewFollows::handle_event() 
{ 
	pwindow->thread->preferences->view_follows_playback = get_value(); 
	return 1;
}




PlaybackSoftwareTimer::PlaybackSoftwareTimer(PreferencesWindow *pwindow, int value, int y)
 : BC_CheckBox(10, y, value, _("Use software for positioning information"))
{ 
	this->pwindow = pwindow; 
}

int PlaybackSoftwareTimer::handle_event() 
{ 
	pwindow->thread->preferences->playback_software_position = get_value(); 
	return 1;
}




PlaybackRealTime::PlaybackRealTime(PreferencesWindow *pwindow, int value, int y)
 : BC_CheckBox(10, y, value, _("Audio playback in real time priority (root only)"))
{ 
	this->pwindow = pwindow; 
}

int PlaybackRealTime::handle_event() 
{ 
	pwindow->thread->preferences->real_time_playback = get_value(); 
	return 1;
}


PlaybackHWDecode::PlaybackHWDecode(
	int x, 
	int y, 
	PreferencesWindow *pwindow, 
	PlaybackPrefs *playback)
 : BC_CheckBox(x, 
 	y, 
	pwindow->thread->preferences->use_hardware_decoding, 
	_("Decode in hardware (restart required)"))
{
	this->pwindow = pwindow;
	this->playback = playback;
}

int PlaybackHWDecode::handle_event()
{
	pwindow->thread->preferences->use_hardware_decoding = get_value();
	return 1;
}


PlaybackFFmpegMov::PlaybackFFmpegMov(
	int x, 
	int y, 
	PreferencesWindow *pwindow, 
	PlaybackPrefs *playback)
 : BC_CheckBox(x, 
 	y, 
	pwindow->thread->preferences->use_ffmpeg_mov, 
	_("Use ffmpeg to decode quicktime/mp4 (restart required)"))
{
	this->pwindow = pwindow;
	this->playback = playback;
}

int PlaybackFFmpegMov::handle_event()
{
	pwindow->thread->preferences->use_ffmpeg_mov = get_value();
	return 1;
}











