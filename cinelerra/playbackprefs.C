
/*
 * CINELERRA
 * Copyright (C) 2010 Adam Williams <broadcast at earthling dot net>
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
	BC_WindowBase *window;
	BC_Resources *resources = BC_WindowBase::get_resources();


	playback_config = pwindow->thread->edl->session->playback_config;

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

	y += menu->get_h() + 5;
	add_subwindow(title1 = new BC_Title(x, y, _("Audio offset (sec):")));
	x1 = x + title1->get_w() + margin;
	PlaybackAudioOffset *audio_offset = new PlaybackAudioOffset(pwindow,
		this,
		x1,
		y);
	audio_offset->create_objects();
	y += audio_offset->get_h() + 5;

SET_TRACE
	add_subwindow(new PlaybackViewFollows(pwindow, pwindow->thread->edl->session->view_follows_playback, y));
	y += 30;
	add_subwindow(new PlaybackSoftwareTimer(pwindow, pwindow->thread->edl->session->playback_software_position, y));
	y += 30;
	add_subwindow(new PlaybackRealTime(pwindow, pwindow->thread->edl->session->real_time_playback, y));
	y += 40;
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
	y += 5;

SET_TRACE
	add_subwindow(title1 = new BC_Title(x, y, _("Video Out"), LARGEFONT));
	y += title1->get_h() + margin;

SET_TRACE
	add_subwindow(window = new VideoEveryFrame(pwindow, this, x, y));
	y += window->get_h() + margin;
	
	add_subwindow(title1 = new BC_Title(x, y, _("Framerate achieved:")));
	x1 = title1->get_x() + title1->get_w() + margin;
	add_subwindow(framerate_title = new BC_Title(x1, y, _("--"), MEDIUMFONT, RED));
	draw_framerate(0);
	y += framerate_title->get_h() + margin;

//	add_subwindow(asynchronous = new VideoAsynchronous(pwindow, x, y));
//	y += asynchronous->get_h() + 10;

SET_TRACE
	BC_Title *title;
 	add_subwindow(title = new BC_Title(x, y, _("Scaling equation:")));
	y += title->get_h() + margin;
	add_subwindow(nearest_neighbor = new PlaybackNearest(pwindow, 
		this, 
		pwindow->thread->edl->session->interpolation_type == NEAREST_NEIGHBOR, 
		10, 
		y));
	y += nearest_neighbor->get_h();
	add_subwindow(cubic_cubic = new PlaybackBicubicBicubic(pwindow, 
		this, 
		pwindow->thread->edl->session->interpolation_type == CUBIC_CUBIC ||
			pwindow->thread->edl->session->interpolation_type == CUBIC_LINEAR ||
			pwindow->thread->edl->session->interpolation_type == LINEAR_LINEAR ||
			pwindow->thread->edl->session->interpolation_type == LANCZOS_LANCZOS, 
		10, 
		y));
	y += cubic_cubic->get_h() + margin;
// 	add_subwindow(cubic_linear = new PlaybackBicubicBilinear(pwindow, 
// 		this, 
// 		pwindow->thread->edl->session->interpolation_type == CUBIC_LINEAR, 
// 		10, 
// 		y));
// 	y += nearest_neighbor->get_h();
// 	add_subwindow(linear_linear = new PlaybackBilinearBilinear(pwindow, 
// 		this, 
// 		pwindow->thread->edl->session->interpolation_type == LINEAR_LINEAR, 
// 		10, 
// 		y));

SET_TRACE
//	y += nearest_neighbor->get_h() + margin;
	add_subwindow(title1 = new BC_Title(x, y, _("Preload buffer for Quicktime:"), MEDIUMFONT));
	sprintf(string, "%d", (int)pwindow->thread->edl->session->playback_preload);
	PlaybackPreload *preload;
	x1 = title1->get_x() + title1->get_w() + margin;
	add_subwindow(preload = new PlaybackPreload(x1, y, pwindow, this, string));

	y += preload->get_h() + 5;
	add_subwindow(title1 = new BC_Title(x, y, _("DVD Subtitle to display:")));
	PlaybackSubtitleNumber *subtitle_number;
	x1 = x + title1->get_w() + margin;
	subtitle_number = new PlaybackSubtitleNumber(x1, 
		y, 
		pwindow, 
		this);
	subtitle_number->create_objects();

	PlaybackSubtitle *subtitle_toggle;
	x1 += subtitle_number->get_w() + margin;
	add_subwindow(subtitle_toggle = new PlaybackSubtitle(
		x1, 
		y, 
		pwindow, 
		this));
	y += subtitle_number->get_h();


	add_subwindow(interpolate_raw = new PlaybackInterpolateRaw(
		x, 
		y,
		pwindow,
		this));
	y += interpolate_raw->get_h() + margin;

// 	add_subwindow(white_balance_raw = new PlaybackWhiteBalanceRaw(
// 		x, 
// 		y,
// 		pwindow,
// 		this));
// 	y += white_balance_raw->get_h() + margin;
// 	if(!pwindow->thread->edl->session->interpolate_raw) 
// 		white_balance_raw->disable();


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


void PlaybackPrefs::update(int interpolation)
{
	pwindow->thread->edl->session->interpolation_type = interpolation;
	nearest_neighbor->update(interpolation == NEAREST_NEIGHBOR);
	cubic_cubic->update(interpolation == CUBIC_CUBIC ||
		interpolation == CUBIC_LINEAR ||
		interpolation == LINEAR_LINEAR ||
		interpolation == LANCZOS_LANCZOS);
//	cubic_linear->update(interpolation == CUBIC_LINEAR);
//	linear_linear->update(interpolation == LINEAR_LINEAR);
}


int PlaybackPrefs::get_buffer_bytes()
{
//	return pwindow->thread->edl->aconfig->oss_out_bits / 8 * pwindow->thread->preferences->aconfig->oss_out_channels * pwindow->thread->preferences->playback_buffer;
}

int PlaybackPrefs::draw_framerate(int flush)
{
//printf("PlaybackPrefs::draw_framerate 1 %f\n", mwindow->session->actual_frame_rate);
	char string[BCTEXTLEN];
	sprintf(string, "%.4f", mwindow->session->actual_frame_rate);
	framerate_title->update(string, flush);
	return 0;
}



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
	100)
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
	100, 
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
	pwindow->thread->edl->session->view_follows_playback = get_value(); 
	return 1;
}




PlaybackSoftwareTimer::PlaybackSoftwareTimer(PreferencesWindow *pwindow, int value, int y)
 : BC_CheckBox(10, y, value, _("Use software for positioning information"))
{ 
	this->pwindow = pwindow; 
}

int PlaybackSoftwareTimer::handle_event() 
{ 
	pwindow->thread->edl->session->playback_software_position = get_value(); 
	return 1;
}




PlaybackRealTime::PlaybackRealTime(PreferencesWindow *pwindow, int value, int y)
 : BC_CheckBox(10, y, value, _("Audio playback in real time priority (root only)"))
{ 
	this->pwindow = pwindow; 
}

int PlaybackRealTime::handle_event() 
{ 
	pwindow->thread->edl->session->real_time_playback = get_value(); 
	return 1;
}







PlaybackNearest::PlaybackNearest(PreferencesWindow *pwindow, PlaybackPrefs *prefs, int value, int x, int y)
 : BC_Radial(x, y, value, _("Low quality"))
{
	this->pwindow = pwindow;
	this->prefs = prefs;
}
int PlaybackNearest::handle_event()
{
	prefs->update(NEAREST_NEIGHBOR);
	return 1;
}





PlaybackBicubicBicubic::PlaybackBicubicBicubic(PreferencesWindow *pwindow, PlaybackPrefs *prefs, int value, int x, int y)
 : BC_Radial(x, y, value, _("High quality"))
{
	this->pwindow = pwindow;
	this->prefs = prefs;
}
int PlaybackBicubicBicubic::handle_event()
{
	prefs->update(CUBIC_CUBIC);
	return 1;
}




PlaybackBicubicBilinear::PlaybackBicubicBilinear(PreferencesWindow *pwindow, PlaybackPrefs *prefs, int value, int x, int y)
 : BC_Radial(x, y, value, _("Bicubic enlarge and bilinear reduce"))
{
	this->pwindow = pwindow;
	this->prefs = prefs;
}
int PlaybackBicubicBilinear::handle_event()
{
	prefs->update(CUBIC_LINEAR);
	return 1;
}


PlaybackBilinearBilinear::PlaybackBilinearBilinear(PreferencesWindow *pwindow, 
	PlaybackPrefs *prefs, 
	int value, 
	int x, 
	int y)
 : BC_Radial(x, y, value, _("Bilinear enlarge and bilinear reduce"))
{
	this->pwindow = pwindow;
	this->prefs = prefs;
}
int PlaybackBilinearBilinear::handle_event()
{
	prefs->update(LINEAR_LINEAR);
	return 1;
}

PlaybackLanczos::PlaybackLanczos(PreferencesWindow *pwindow, 
	PlaybackPrefs *prefs, 
	int value, 
	int x, 
	int y)
 : BC_Radial(x, y, value, _("Lanczos"))
{
	this->pwindow = pwindow;
	this->prefs = prefs;
}
int PlaybackLanczos::handle_event()
{
	prefs->update(LANCZOS_LANCZOS);
	return 1;
}


PlaybackPreload::PlaybackPreload(int x, 
	int y, 
	PreferencesWindow *pwindow, 
	PlaybackPrefs *playback, 
	char *text)
 : BC_TextBox(x, y, 100, 1, text)
{ 
	this->pwindow = pwindow; 
	this->playback = playback; 
}

int PlaybackPreload::handle_event() 
{ 
	pwindow->thread->edl->session->playback_preload = atol(get_text()); 
	return 1;
}


PlaybackInterpolateRaw::PlaybackInterpolateRaw(
	int x, 
	int y, 
	PreferencesWindow *pwindow, 
	PlaybackPrefs *playback)
 : BC_CheckBox(x, 
 	y, 
	pwindow->thread->edl->session->interpolate_raw, 
	_("Interpolate CR2 images"))
{
	this->pwindow = pwindow;
	this->playback = playback;
}

int PlaybackInterpolateRaw::handle_event()
{
	pwindow->thread->edl->session->interpolate_raw = get_value();
// 	if(!pwindow->thread->edl->session->interpolate_raw)
// 	{
// 		playback->white_balance_raw->update(0, 0);
// 		playback->white_balance_raw->disable();
// 	}
// 	else
// 	{
// 		playback->white_balance_raw->update(pwindow->thread->edl->session->white_balance_raw, 0);
// 		playback->white_balance_raw->enable();
// 	}
	return 1;
}




// PlaybackWhiteBalanceRaw::PlaybackWhiteBalanceRaw(
// 	int x, 
// 	int y, 
// 	PreferencesWindow *pwindow, 
// 	PlaybackPrefs *playback)
//  : BC_CheckBox(x, 
//  	y, 
// 	pwindow->thread->edl->session->interpolate_raw &&
// 		pwindow->thread->edl->session->white_balance_raw, 
// 	_("White balance CR2 images"))
// {
// 	this->pwindow = pwindow;
// 	this->playback = playback;
// 	if(!pwindow->thread->edl->session->interpolate_raw) disable();
// }
// 
// int PlaybackWhiteBalanceRaw::handle_event()
// {
// 	pwindow->thread->edl->session->white_balance_raw = get_value();
// 	return 1;
// }






// VideoAsynchronous::VideoAsynchronous(PreferencesWindow *pwindow, int x, int y)
//  : BC_CheckBox(x, 
//  	y, 
// 	pwindow->thread->edl->session->video_every_frame &&
// 		pwindow->thread->edl->session->video_asynchronous, 
// 	_("Decode frames asynchronously"))
// {
// 	this->pwindow = pwindow;
// 	if(!pwindow->thread->edl->session->video_every_frame)
// 		disable();
// }
// 
// int VideoAsynchronous::handle_event()
// {
// 	pwindow->thread->edl->session->video_asynchronous = get_value();
// 	return 1;
// }




VideoEveryFrame::VideoEveryFrame(PreferencesWindow *pwindow, 
	PlaybackPrefs *playback_prefs,
	int x, 
	int y)
 : BC_CheckBox(x, y, pwindow->thread->edl->session->video_every_frame, _("Play every frame"))
{
	this->pwindow = pwindow;
	this->playback_prefs = playback_prefs;
}

int VideoEveryFrame::handle_event()
{
	pwindow->thread->edl->session->video_every_frame = get_value();
// 	if(!pwindow->thread->edl->session->video_every_frame)
// 	{
// 		playback_prefs->asynchronous->update(0, 0);
// 		playback_prefs->asynchronous->disable();
// 	}
// 	else
// 	{
// 		playback_prefs->asynchronous->update(pwindow->thread->edl->session->video_asynchronous, 0);
// 		playback_prefs->asynchronous->enable();
// 	}
	return 1;
}







PlaybackSubtitle::PlaybackSubtitle(int x, 
	int y, 
	PreferencesWindow *pwindow, 
	PlaybackPrefs *playback)
 : BC_CheckBox(x, 
 	y, 
	pwindow->thread->edl->session->decode_subtitles,
	_("Enable subtitles"))
{
	this->pwindow = pwindow;
	this->playback = playback;
}

int PlaybackSubtitle::handle_event()
{
	pwindow->thread->edl->session->decode_subtitles = get_value();
	return 1;
}










PlaybackSubtitleNumber::PlaybackSubtitleNumber(int x, 
	int y, 
	PreferencesWindow *pwindow, 
	PlaybackPrefs *playback)
 : BC_TumbleTextBox(playback,
 	pwindow->thread->edl->session->subtitle_number,
 	0,
	31,
	x, 
 	y, 
	50)
{
	this->pwindow = pwindow;
	this->playback = playback;
}

int PlaybackSubtitleNumber::handle_event()
{
	pwindow->thread->edl->session->subtitle_number = atoi(get_text());
	return 1;
}








