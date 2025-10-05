/*
 * CINELERRA
 * Copyright (C) 2008-2025 Adam Williams <broadcast at earthling dot net>
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

#ifndef SETFORMAT_H
#define SETFORMAT_H


#include "edl.inc"
#include "formatpresets.h"
#include "guicast.h"
#include "mutex.inc"
#include "mwindow.inc"
#include "setformat.inc"
#include "thread.h"


class SetFormatPresets;



class SetFormat : public BC_MenuItem
{
public:
	SetFormat(MWindow *mwindow);
	int handle_event();
	SetFormatThread *thread;
	MWindow *mwindow;
};


class SetFormatThread : public Thread
{
public:
	SetFormatThread(MWindow *mwindow);

	void run();

	void apply_changes();
// Update image size based on ratio of dimensions to original.
	void update_window();
// Update automatic aspect ratio based in image size
	void update_aspect();
// Update all parameters from preset menu
	void update();
    void update_interpolation(int interpolation);


	Mutex *window_lock;
	SetFormatWindow *window;
	MWindow *mwindow;
	EDL *new_settings;
	float ratio[2];
	int dimension[2];
	int orig_dimension[2];
	int constrain_ratio;
};


class SetSampleRateTextBox : public BC_TextBox
{
public:
	SetSampleRateTextBox(SetFormatThread *thread, int x, int y);
	int handle_event();
	SetFormatThread *thread;
};

class SetChannelsTextBox : public BC_TextBox
{
public:
	SetChannelsTextBox(SetFormatThread *thread, int x, int y);
	
	int handle_event();
	
	SetFormatThread *thread;
	MWindow *mwindow;
};


class SetChannelsCanvas : public BC_SubWindow
{
public:
	SetChannelsCanvas(MWindow *mwindow, 
		SetFormatThread *thread, 
		int x, 
		int y);
	~SetChannelsCanvas();

	int initialize();
	int draw(int angle = -1);
	int get_dimensions(int channel_position, int &x, int &y, int &w, int &h);
	int button_press_event();
	int button_release_event();
	int cursor_motion_event();

private:	
	int active_channel;   // for selection
	int degree_offset;
	int box_r;
	
	int poltoxy(int &x, int &y, int r, int d);
	int xytopol(int &d, int x, int y);
	MWindow *mwindow;
	SetFormatThread *thread;
	VFrame *temp_picon;
	RotateFrame *rotater;
};


class SetFrameRateTextBox : public BC_TextBox
{
public:
	SetFrameRateTextBox(SetFormatThread *thread, int x, int y);
	int handle_event();
	SetFormatThread *thread;
};

class ScaleSizeText : public BC_TextBox
{
public:
	ScaleSizeText(int x, int y, SetFormatThread *thread, int *output);
	~ScaleSizeText();
	int handle_event();
	SetFormatThread *thread;
	int *output;
};


class ScaleRatioText : public BC_TextBox
{
public:
	ScaleRatioText(int x, int y, SetFormatThread *thread, float *output);
	~ScaleRatioText();
	int handle_event();
	SetFormatThread *thread;
	float *output;
};

class ScaleAspectAuto : public BC_CheckBox
{
public:
	ScaleAspectAuto(int x, int y, SetFormatThread *thread);
	~ScaleAspectAuto();
	int handle_event();
	SetFormatThread *thread;
};

class ScaleAspectText : public BC_TextBox
{
public:
	ScaleAspectText(int x, int y, SetFormatThread *thread, float *output);
	~ScaleAspectText();
	int handle_event();
	SetFormatThread *thread;
	float *output;
};

class SetFormatApply : public BC_GenericButton
{
public:
	SetFormatApply(int x, int y, SetFormatThread *thread);
	int handle_event();
	SetFormatThread *thread;
};

class SetFormatPresets : public FormatPresets
{
public:
	SetFormatPresets(MWindow *mwindow, SetFormatWindow *gui, int x, int y);
	~SetFormatPresets();
	int handle_event();
	EDL* get_edl();
};

class FormatSwapExtents : public BC_Button
{
public:
	FormatSwapExtents(MWindow *mwindow, 
		SetFormatThread *thread,
		SetFormatWindow *gui, 
		int x, 
		int y);
	int handle_event();
	MWindow *mwindow;
	SetFormatThread *thread;
	SetFormatWindow *gui;
};


class LabelText : public BC_TextBox
{
public:
	LabelText(SetFormatThread *thread, int x, int y, int color);
	int handle_event();
	SetFormatThread *thread;
    int color;
};


/*
 * class VideoAsynchronous : public BC_CheckBox
 * {
 * public:
 * 	VideoAsynchronous(PreferencesWindow *pwindow, int x, int y);
 * 	int handle_event();
 * 	PreferencesWindow *pwindow;
 * };
 */

class VideoEveryFrame : public BC_CheckBox
{
public:
	VideoEveryFrame(SetFormatThread *thread,
		int x, 
		int y);
	int handle_event();
	SetFormatThread *thread;
};

class DisableMutedTracks : public BC_CheckBox
{
public:
	DisableMutedTracks(SetFormatThread *thread,
		int x, 
		int y);
	int handle_event();
	SetFormatThread *thread;
};

class OnlyTop : public BC_CheckBox
{
public:
	OnlyTop(SetFormatThread *thread,
		int x, 
		int y);
	int handle_event();
	SetFormatThread *thread;
};

// class PlaybackDeblock : public BC_CheckBox
// {
// public:
// 	PlaybackDeblock(PreferencesWindow *pwindow, int x, int y);
// 	int handle_event();
// 	PreferencesWindow *pwindow;
// };

class PlaybackNearest : public BC_Radial
{
public:
	PlaybackNearest(SetFormatThread *thread, int value, int x, int y);
	int handle_event();
	SetFormatThread *thread;
};

class PlaybackBicubicBicubic : public BC_Radial
{
public:
	PlaybackBicubicBicubic(SetFormatThread *thread, int value, int x, int y);
	int handle_event();
    SetFormatThread *thread;
};

class PlaybackBicubicBilinear : public BC_Radial
{
public:
	PlaybackBicubicBilinear(SetFormatThread *thread, 
		int value, 
		int x, 
		int y);
	int handle_event();
    SetFormatThread *thread;
};

class PlaybackLanczos : public BC_Radial
{
public:
	PlaybackLanczos(SetFormatThread *thread, 
		int value, 
		int x, 
		int y);
	int handle_event();
    SetFormatThread *thread;
};

class PlaybackBilinearBilinear : public BC_Radial
{
public:
	PlaybackBilinearBilinear(SetFormatThread *thread, 
		int value, 
		int x, 
		int y);
	int handle_event();
    SetFormatThread *thread;
};

class PlaybackPreload : public BC_TextBox
{
public:
	PlaybackPreload(int x, 
		int y, 
		SetFormatThread *thread, 
		char *text);
	int handle_event();
	SetFormatThread *thread;
};

class PlaybackInterpolateRaw : public BC_CheckBox
{
public:
	PlaybackInterpolateRaw(int x, 
		int y, 
		SetFormatThread *thread);
	int handle_event();
	SetFormatThread *thread;
};

class PlaybackWhiteBalanceRaw : public BC_CheckBox
{
public:
	PlaybackWhiteBalanceRaw(int x, 
		int y, 
		SetFormatThread *thread);
	int handle_event();
	SetFormatThread *thread;
};

class PlaybackSubtitle : public BC_CheckBox
{
public:
	PlaybackSubtitle(int x, 
		int y, 
		SetFormatThread *thread);
	int handle_event();
	SetFormatThread *thread;
};

class PlaybackSubtitleNumber : public BC_TumbleTextBox
{
public:
	PlaybackSubtitleNumber(int x, 
		int y, 
		SetFormatThread *thread);
	int handle_event();
	SetFormatThread *thread;
};


class TimeFormatFeetSetting : public BC_TextBox
{
public:
	TimeFormatFeetSetting(SetFormatThread *thread, int x, int y, char *string);
	int handle_event();
	SetFormatThread *thread;
};



class MeterMinDB : public BC_TextBox
{
public:
	MeterMinDB(SetFormatThread *thread, char *text, int x, int y);
	int handle_event();
	SetFormatThread *thread;
};


class MeterMaxDB : public BC_TextBox
{
public:
	MeterMaxDB(SetFormatThread *thread, char *text, int x, int y);
	int handle_event();
	SetFormatThread *thread;
};

class SetFormatWindow : public BC_Window
{
public:
	SetFormatWindow(MWindow *mwindow, 
		SetFormatThread *thread,
		int x,
		int y);

	void create_objects();
	const char* get_preset_text();

	MWindow *mwindow;
	SetFormatThread *thread;
	SetChannelsCanvas *canvas;
// Screen size width, height
	ScaleSizeText* dimension[2];
	SetFormatPresets *presets;
// Size ratio width, height
	ScaleRatioText* ratio[2];
// Aspect ratio
	ScaleAspectText *aspect_w;
	ScaleAspectText *aspect_h;
	SetSampleRateTextBox *sample_rate;
	SetChannelsTextBox *channels;
	SetFrameRateTextBox *frame_rate;
	BC_TextBox *color_model;
	ScaleAspectAuto *auto_aspect;
	PlaybackNearest *nearest_neighbor;
	PlaybackBicubicBicubic *cubic_cubic;
//	PlaybackBicubicBilinear *cubic_linear;
//	PlaybackBilinearBilinear *linear_linear;
//	PlaybackLanczos *lanczos;
//	PlaybackDeblock *mpeg4_deblock;
	PlaybackInterpolateRaw *interpolate_raw;
	PlaybackWhiteBalanceRaw *white_balance_raw;
//	VideoAsynchronous *asynchronous;
	MeterMinDB *min_db;
	MeterMaxDB *max_db;
};

	
	
	




#endif
