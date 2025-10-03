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

#include "asset.h"
#include "auto.h"
#include "bcdisplayinfo.h"
#include "clip.h"
#include "bchash.h"
#include "editinfo.inc"
#include "editpopup.inc"
#include "edl.h"
#include "edlsession.h"
#include "file.inc"
#include "guicast.h"
#include "indexable.h"
#include "mainsession.h"
#include "meterpanel.h"
#include "mwindow.h"
#include "mwindowgui.h"

MainSession::MainSession(MWindow *mwindow)
{
	this->mwindow = mwindow;
    reset();
	changes_made = 0;
	filename[0] = 0;
//	playback_cursor_visible = 0;
//	is_playing_back = 0;
	track_highlighted = 0;
	plugin_highlighted = 0;
	pluginset_highlighted = 0;
	vcanvas_highlighted = 0;
	ccanvas_highlighted = 0;
	edit_highlighted = 0;
	current_operation = NO_OPERATION;
	drag_pluginservers = new ArrayList<PluginServer*>;
	drag_plugin = 0;
	drag_assets = new ArrayList<Indexable*>;
	drag_auto_gang = new ArrayList<Auto*>;
	drag_clips = new ArrayList<EDL*>;
	drag_edits = new ArrayList<Edit*>;
	drag_edit = 0;
    drag_transition = 0;
	clip_number = 1;
	brender_end = 0;
	cwindow_controls = 1;
	trim_edits = 0;
	gwindow_x = 0;
	gwindow_y = 0;
	show_gwindow = 0;
	current_tip = 0;
	cwindow_fullscreen = 0;
	rwindow_fullscreen = 0;
	vwindow_fullscreen = 0;
	actual_frame_rate = 0;
	record_scope = 0;
    timebar_position = -1;
}

MainSession::~MainSession()
{
	delete drag_pluginservers;
	delete drag_assets;
	delete drag_auto_gang;
	delete drag_clips;
	delete drag_edits;
}


void MainSession::boundaries()
{
	lwindow_x = MAX(0, lwindow_x);
	lwindow_y = MAX(0, lwindow_y);
	mwindow_x = MAX(0, mwindow_x);
	mwindow_y = MAX(0, mwindow_y);
	cwindow_x = MAX(0, cwindow_x);
	cwindow_y = MAX(0, cwindow_y);
	vwindow_x = MAX(0, vwindow_x);
	vwindow_y = MAX(0, vwindow_y);
	awindow_x = MAX(0, awindow_x);
	awindow_y = MAX(0, awindow_y);
	gwindow_x = MAX(0, gwindow_x);
	gwindow_y = MAX(0, gwindow_y);
	rwindow_x = MAX(0, rwindow_x);
	rwindow_y = MAX(0, rwindow_y);
	rmonitor_x = MAX(0, rmonitor_x);
	rmonitor_y = MAX(0, rmonitor_y);
    edit_info_w = MAX(0, edit_info_w);
    edit_info_h = MAX(0, edit_info_h);
    swap_asset_w = MAX(0, swap_asset_w);
    swap_asset_h = MAX(0, swap_asset_h);
	cwindow_controls = CLIP(cwindow_controls, 0, 1);
    transitiondialog_w = MAX(100, transitiondialog_w);
    transitiondialog_h = MAX(100, transitiondialog_h);
    label_color = CLIP(label_color, 0, LABEL_COLORS - 1);
    eyedrop_radius = CLIP(eyedrop_radius, 0, 255);
	eyedrop_x = CLIP(eyedrop_x, 0, 65535);
	eyedrop_y = CLIP(eyedrop_y, 0, 65535);
}

void MainSession::reset()
{
	record_scope = 0;
	use_hist = 1;
	use_wave = 1;
	use_vector = 1;
	use_hist_parade = 1;
	use_wave_parade = 1;
    edit_info_format = TIME_HMS;
    label_color = 3;
    eyedrop_radius = 0;
    eyedrop_x = 0;
    eyedrop_y = 0;

    default_window_positions();
}

void MainSession::default_window_positions()
{
// Get defaults based on root window size
	BC_DisplayInfo display_info;

	int root_x = 0;
	int root_y = 0;
	int root_w = display_info.get_root_w();
	int root_h = display_info.get_root_h();
	int border_left = 0;
	int border_right = 0;
	int border_top = 0;
	int border_bottom = 0;

	border_left = display_info.get_left_border();
	border_top = display_info.get_top_border();
	border_right = display_info.get_right_border();
	border_bottom = display_info.get_bottom_border();

// Wider than 16:9, narrower than dual head
	if((float)root_w / root_h > 1.8) root_w /= 2;



	vwindow_x = root_x;
	vwindow_y = root_y;
	vwindow_w = root_w / 2 - border_left - border_right;
	vwindow_h = root_h * 6 / 10 - border_top - border_bottom;

	cwindow_x = root_x + root_w / 2;
	cwindow_y = root_y;
	cwindow_w = vwindow_w;
	cwindow_h = vwindow_h;

	ctool_x = cwindow_x + cwindow_w / 2;
	ctool_y = cwindow_y + cwindow_h / 2;

	mwindow_x = root_x;
	mwindow_y = vwindow_y + vwindow_h + border_top + border_bottom;
	mwindow_w = root_w * 2 / 3 - border_left - border_right;
	mwindow_h = root_h - mwindow_y - border_top - border_bottom;

	awindow_x = mwindow_x + border_left + border_right + mwindow_w;
	awindow_y = mwindow_y;
	awindow_w = root_x + root_w - awindow_x - border_left - border_right;
	awindow_h = mwindow_h;
    asset_columns[0] = DP(100);
    asset_columns[1] = DP(100);

	ewindow_w = DP(640);
	ewindow_h = DP(240);

	channels_x = 0;
	channels_y = 0;
	picture_x = 0;
	picture_y = 0;
	scope_x = 0;
	scope_y = 0;
	scope_w = DP(640);
	scope_h = DP(320);
	histogram_x = 0;
	histogram_y = 0;
	histogram_w = DP(320);
	histogram_h = DP(480);

	if(mwindow->edl)
	{
		lwindow_w = MeterPanel::get_meters_width(mwindow->theme, 
			mwindow->edl->session->audio_channels, 
			1);
	}
	else
	{
		lwindow_w = DP(100);
	}

	lwindow_y = 0;
	lwindow_x = root_w - lwindow_w;
	lwindow_h = mwindow_y;

	rwindow_x = 0;
	rwindow_y = 0;
	rwindow_h = DP(500);
	rwindow_w = DP(650);

	rmonitor_x = rwindow_x + rwindow_w + 10;
	rmonitor_y = rwindow_y;
	rmonitor_w = root_w - rmonitor_x;
	rmonitor_h = rwindow_h;

	batchrender_w = DP(540);
	batchrender_h = DP(340);
	batchrender_x = root_w / 2 - batchrender_w / 2;
	batchrender_y = root_h / 2 - batchrender_h / 2;
    edit_info_w = DP(500);
    edit_info_h = DP(400);
    swap_asset_w = DP(500);
    swap_asset_h = DP(400);
}

int MainSession::load_defaults(BC_Hash *defaults)
{
	char string[BCTEXTLEN];
// Setup main windows
	reset();
	vwindow_x = defaults->get("VWINDOW_X", vwindow_x);
	vwindow_y = defaults->get("VWINDOW_Y", vwindow_y);
	vwindow_w = defaults->get("VWINDOW_W", vwindow_w);
	vwindow_h = defaults->get("VWINDOW_H", vwindow_h);


	cwindow_x = defaults->get("CWINDOW_X", cwindow_x);
	cwindow_y = defaults->get("CWINDOW_Y", cwindow_y);
	cwindow_w = defaults->get("CWINDOW_W", cwindow_w);
	cwindow_h = defaults->get("CWINDOW_H", cwindow_h);

	ctool_x = defaults->get("CTOOL_X", ctool_x);
	ctool_y = defaults->get("CTOOL_Y", ctool_y);

	gwindow_x = defaults->get("GWINDOW_X", gwindow_x);
	gwindow_y = defaults->get("GWINDOW_Y", gwindow_y);

	mwindow_x = defaults->get("MWINDOW_X", mwindow_x);
	mwindow_y = defaults->get("MWINDOW_Y", mwindow_y);
	mwindow_w = defaults->get("MWINDOW_W", mwindow_w);
	mwindow_h = defaults->get("MWINDOW_H", mwindow_h);

	lwindow_x = defaults->get("LWINDOW_X", lwindow_x);
	lwindow_y = defaults->get("LWINDOW_Y", lwindow_y);
	lwindow_w = defaults->get("LWINDOW_W", lwindow_w);
	lwindow_h = defaults->get("LWINDOW_H", lwindow_h);


	awindow_x = defaults->get("AWINDOW_X", awindow_x);
	awindow_y = defaults->get("AWINDOW_Y", awindow_y);
	awindow_w = defaults->get("AWINDOW_W", awindow_w);
	awindow_h = defaults->get("AWINDOW_H", awindow_h);
	for(int i = 0; i < ASSET_COLUMNS; i++)
	{
		sprintf(string, "ASSET_COLUMN%d", i);
		asset_columns[i] = defaults->get(string, DP(100));
	}

	ewindow_w = defaults->get("EWINDOW_W", ewindow_w);
	ewindow_h = defaults->get("EWINDOW_H", ewindow_h);

	edit_info_w = defaults->get("EDIT_INFO_W", edit_info_w);
	edit_info_h = defaults->get("EDIT_INFO_H", edit_info_h);
	swap_asset_w = defaults->get("SWAP_ASSET_W", swap_asset_w);
	swap_asset_h = defaults->get("SWAP_ASSET_H", swap_asset_h);

	channels_x = defaults->get("CHANNELS_X", channels_x);
	channels_y = defaults->get("CHANNELS_Y", channels_y);
	picture_x = defaults->get("PICTURE_X", picture_x);
	picture_y = defaults->get("PICTURE_Y", picture_y);
	scope_x = defaults->get("SCOPE_X", scope_x);
	scope_y = defaults->get("SCOPE_Y", scope_y);
	scope_w = defaults->get("SCOPE_W", scope_w);
	scope_h = defaults->get("SCOPE_H", scope_h);
	histogram_x = defaults->get("HISTOGRAM_X", histogram_x);
	histogram_y = defaults->get("HISTOGRAM_Y", histogram_y);
	histogram_w = defaults->get("HISTOGRAM_W", histogram_w);
	histogram_h = defaults->get("HISTOGRAM_H", histogram_h);
	record_scope = defaults->get("RECORD_SCOPE", record_scope);
	use_hist = defaults->get("USE_HIST", use_hist);
	use_wave = defaults->get("USE_WAVE", use_wave);
	use_vector = defaults->get("USE_VECTOR", use_vector);
	use_hist_parade = defaults->get("USE_HIST_PARADE", use_hist_parade);
	use_wave_parade = defaults->get("USE_WAVE_PARADE", use_wave_parade);
    edit_info_format = defaults->get("EDIT_INFO_FORMAT", edit_info_format);

//printf("MainSession::load_defaults 1\n");

// Other windows
	afolders_w = defaults->get("ABINS_W", DP(100));
	rwindow_x = defaults->get("RWINDOW_X", rwindow_x);
	rwindow_y = defaults->get("RWINDOW_Y", rwindow_y);
	rwindow_w = defaults->get("RWINDOW_W", rwindow_w);
	rwindow_h = defaults->get("RWINDOW_H", rwindow_h);

	rmonitor_x = defaults->get("RMONITOR_X", rmonitor_x);
	rmonitor_y = defaults->get("RMONITOR_Y", rmonitor_y);
	rmonitor_w = defaults->get("RMONITOR_W", rmonitor_w);
	rmonitor_h = defaults->get("RMONITOR_H", rmonitor_h);

	batchrender_x = defaults->get("BATCHRENDER_X", batchrender_x);
	batchrender_y = defaults->get("BATCHRENDER_Y", batchrender_y);
	batchrender_w = defaults->get("BATCHRENDER_W", batchrender_w);
	batchrender_h = defaults->get("BATCHRENDER_H", batchrender_h);

	show_vwindow = defaults->get("SHOW_VWINDOW", 0);
	show_awindow = defaults->get("SHOW_AWINDOW", 1);
	show_cwindow = defaults->get("SHOW_CWINDOW", 1);
	show_lwindow = defaults->get("SHOW_LWINDOW", 0);
	show_gwindow = defaults->get("SHOW_GWINDOW", 0);

	cwindow_controls = defaults->get("CWINDOW_CONTROLS", cwindow_controls);

	plugindialog_w = defaults->get("PLUGINDIALOG_W", DP(510));
	plugindialog_h = defaults->get("PLUGINDIALOG_H", DP(415));
	presetdialog_w = defaults->get("PRESETDIALOG_W", DP(510));
	presetdialog_h = defaults->get("PRESETDIALOG_H", DP(415));
	keyframedialog_w = defaults->get("KEYFRAMEDIALOG_W", DP(510));
	keyframedialog_h = defaults->get("KEYFRAMEDIALOG_H", DP(415));
	keyframedialog_column1 = defaults->get("KEYFRAMEDIALOG_COLUMN1", DP(150));
	keyframedialog_column2 = defaults->get("KEYFRAMEDIALOG_COLUMN2", DP(100));
	keyframedialog_all = defaults->get("KEYFRAMEDIALOG_ALL", 0);
	menueffect_w = defaults->get("MENUEFFECT_W", DP(580));
	menueffect_h = defaults->get("MENUEFFECT_H", DP(350));
	transitiondialog_w = defaults->get("TRANSITIONDIALOG_W", DP(320));
	transitiondialog_h = defaults->get("TRANSITIONDIALOG_H", DP(512));

	current_tip = defaults->get("CURRENT_TIP", current_tip);
	actual_frame_rate = defaults->get("ACTUAL_FRAME_RATE", (float)-1);

    label_color = defaults->get("LABEL_COLOR", label_color);

	eyedrop_radius = defaults->get("EYEDROP_RADIUS", eyedrop_radius);
	eyedrop_x = defaults->get("EYEDROP_X", eyedrop_x);
	eyedrop_y = defaults->get("EYEDROP_Y", eyedrop_y);

	boundaries();
	return 0;
}

int MainSession::save_defaults(BC_Hash *defaults)
{
	char string[BCTEXTLEN];

// Window positions
	defaults->update("MWINDOW_X", mwindow_x);
	defaults->update("MWINDOW_Y", mwindow_y);
	defaults->update("MWINDOW_W", mwindow_w);
	defaults->update("MWINDOW_H", mwindow_h);

	defaults->update("LWINDOW_X", lwindow_x);
	defaults->update("LWINDOW_Y", lwindow_y);
	defaults->update("LWINDOW_W", lwindow_w);
	defaults->update("LWINDOW_H", lwindow_h);

	defaults->update("VWINDOW_X", vwindow_x);
	defaults->update("VWINDOW_Y", vwindow_y);
	defaults->update("VWINDOW_W", vwindow_w);
	defaults->update("VWINDOW_H", vwindow_h);

	defaults->update("CWINDOW_X", cwindow_x);
	defaults->update("CWINDOW_Y", cwindow_y);
	defaults->update("CWINDOW_W", cwindow_w);
	defaults->update("CWINDOW_H", cwindow_h);

	defaults->update("CTOOL_X", ctool_x);
	defaults->update("CTOOL_Y", ctool_y);

	defaults->update("GWINDOW_X", gwindow_x);
	defaults->update("GWINDOW_Y", gwindow_y);

	defaults->update("AWINDOW_X", awindow_x);
	defaults->update("AWINDOW_Y", awindow_y);
	defaults->update("AWINDOW_W", awindow_w);
	defaults->update("AWINDOW_H", awindow_h);
	for(int i = 0; i < ASSET_COLUMNS; i++)
	{
		sprintf(string, "ASSET_COLUMN%d", i);
		defaults->update(string, asset_columns[i]);
	}

	defaults->update("EWINDOW_W", ewindow_w);
	defaults->update("EWINDOW_H", ewindow_h);

	defaults->update("EDIT_INFO_W", edit_info_w);
	defaults->update("EDIT_INFO_H", edit_info_h);
	defaults->update("SWAP_ASSET_W", swap_asset_w);
	defaults->update("SWAP_ASSET_H", swap_asset_h);

	defaults->update("CHANNELS_X", channels_x);
	defaults->update("CHANNELS_Y", channels_y);
	defaults->update("PICTURE_X", picture_x);
	defaults->update("PICTURE_Y", picture_y);
	defaults->update("SCOPE_X", scope_x);
	defaults->update("SCOPE_Y", scope_y);
	defaults->update("SCOPE_W", scope_w);
	defaults->update("SCOPE_H", scope_h);
	defaults->update("HISTOGRAM_X", histogram_x);
	defaults->update("HISTOGRAM_Y", histogram_y);
	defaults->update("HISTOGRAM_W", histogram_w);
	defaults->update("HISTOGRAM_H", histogram_h);
	defaults->update("RECORD_SCOPE", record_scope);
	defaults->update("USE_HIST", use_hist);
	defaults->update("USE_WAVE", use_wave);
	defaults->update("USE_VECTOR", use_vector);
	defaults->update("USE_HIST_PARADE", use_hist_parade);
	defaults->update("USE_WAVE_PARADE", use_wave_parade);
    defaults->update("EDIT_INFO_FORMAT", edit_info_format);

 	defaults->update("ABINS_W", afolders_w);

	defaults->update("RMONITOR_X", rmonitor_x);
	defaults->update("RMONITOR_Y", rmonitor_y);
	defaults->update("RMONITOR_W", rmonitor_w);
	defaults->update("RMONITOR_H", rmonitor_h);

	defaults->update("RWINDOW_X", rwindow_x);
	defaults->update("RWINDOW_Y", rwindow_y);
	defaults->update("RWINDOW_W", rwindow_w);
	defaults->update("RWINDOW_H", rwindow_h);

	defaults->update("BATCHRENDER_X", batchrender_x);
	defaults->update("BATCHRENDER_Y", batchrender_y);
	defaults->update("BATCHRENDER_W", batchrender_w);
	defaults->update("BATCHRENDER_H", batchrender_h);

	defaults->update("SHOW_VWINDOW", show_vwindow);
	defaults->update("SHOW_AWINDOW", show_awindow);
	defaults->update("SHOW_CWINDOW", show_cwindow);
	defaults->update("SHOW_LWINDOW", show_lwindow);
	defaults->update("SHOW_GWINDOW", show_gwindow);

	defaults->update("CWINDOW_CONTROLS", cwindow_controls);

	defaults->update("PLUGINDIALOG_W", plugindialog_w);
	defaults->update("PLUGINDIALOG_H", plugindialog_h);
	defaults->update("PRESETDIALOG_W", presetdialog_w);
	defaults->update("PRESETDIALOG_H", presetdialog_h);
	defaults->update("KEYFRAMEDIALOG_W", keyframedialog_w);
	defaults->update("KEYFRAMEDIALOG_H", keyframedialog_h);
	defaults->update("KEYFRAMEDIALOG_COLUMN1", keyframedialog_column1);
	defaults->update("KEYFRAMEDIALOG_COLUMN2", keyframedialog_column2);
	defaults->update("KEYFRAMEDIALOG_ALL", keyframedialog_all);

	defaults->update("MENUEFFECT_W", menueffect_w);
	defaults->update("MENUEFFECT_H", menueffect_h);

	defaults->update("TRANSITIONDIALOG_W", transitiondialog_w);
	defaults->update("TRANSITIONDIALOG_H", transitiondialog_h);

    defaults->update("ACTUAL_FRAME_RATE", actual_frame_rate);
	defaults->update("CURRENT_TIP", current_tip);

	defaults->update("LABEL_COLOR", label_color);
	defaults->update("EYEDROP_RADIUS", eyedrop_radius);
	defaults->update("EYEDROP_X", eyedrop_x);
	defaults->update("EYEDROP_Y", eyedrop_y);

	return 0;
}


