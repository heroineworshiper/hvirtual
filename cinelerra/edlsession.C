/*
 * CINELERRA
 * Copyright (C) 2008-2024 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at  your option) any later version.
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
#include "assets.h"
#include "autoconf.h"
#include "colormodels.h"
#include "bchash.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "overlayframe.inc"
#include "quicktime.h"
#include "tracks.h"
#include "workarounds.h"
#include <inttypes.h>

int EDLSession::current_id = 0;

EDLSession::EDLSession(EDL *edl)
{
    video_every_frame = 0;
    disable_muted = 0;
    only_top = 0;
	proxy_scale = 1;
	highlighted_track = 0;
	playback_cursor_visible = 0;
	interpolation_type = CUBIC_CUBIC;
	interpolate_raw = 1;
	white_balance_raw = 1;
	test_playback_edits = 1;
	brender_start = 0.0;
	brender_end = 0.0;
	mpeg4_deblock = 1;

	auto_conf = new AutoConf;
	strcpy(current_folder, "");
	strcpy(default_atransition, "");
	strcpy(default_vtransition, "");
	default_transition_length = 1.0;
	assetlist_format = ASSETS_TEXT;
	folderlist_format = FOLDERS_TEXT;
	frame_rate = 25; // just has to be something by default
    nested_frame_rate = -1;
	edit_handle_mode[0] = MOVE_ALL_EDITS;
	edit_handle_mode[1] = MOVE_ONE_EDIT;
	edit_handle_mode[2] = MOVE_NO_EDITS;
	autos_follow_edits = 1; // this is needed for predictability
	labels_follow_edits = 1;
	plugins_follow_edits = 1;
	single_standalone = 1;
	audio_tracks = -10;	// these insane values let us crash early if something is forgotten to be set
	audio_channels = -10;
	video_tracks = -10;
	video_channels = -10;
	sample_rate = -10;
    nested_sample_rate = -1;
	frame_rate = -10;
	frames_per_foot = -10;
	min_meter_db = -1000;
	max_meter_db = -1000;
	output_w = -1000;
	output_h = -1000;
	video_write_length = -1000;
	color_model = BC_RGB888;
	decode_subtitles = 0;
	subtitle_number = 0;
	cwindow_meter = 0;
}

EDLSession::~EDLSession()
{
	delete auto_conf;
}

double EDLSession::get_nested_frame_rate()
{
    if(nested_frame_rate < 0)
    {
        return frame_rate;
    }
    else
    {
        return nested_frame_rate;
    }
}

int64_t EDLSession::get_nested_sample_rate()
{
    if(nested_sample_rate < 0)
    {
        return sample_rate;
    }
    else
    {
        return nested_sample_rate;
    }
}

int EDLSession::need_rerender(EDLSession *ptr)
{
	return (playback_preload != ptr->playback_preload) ||
		(interpolation_type != ptr->interpolation_type) ||
		(test_playback_edits != ptr->test_playback_edits) ||
		(playback_buffer != ptr->playback_buffer) ||
		(decode_subtitles != ptr->decode_subtitles) ||
		(subtitle_number != ptr->subtitle_number) ||
		(interpolate_raw != ptr->interpolate_raw) ||
		(white_balance_raw != ptr->white_balance_raw) ||
		(proxy_scale != ptr->proxy_scale) ||
        (disable_muted != ptr->disable_muted) ||
        (only_top != ptr->only_top) ||
        (video_every_frame != ptr->video_every_frame);
}

void EDLSession::equivalent_output(EDLSession *session, double *result)
{
	if(session->output_w != output_w ||
		session->output_h != output_h ||
		session->frame_rate != frame_rate ||
		session->color_model != color_model ||
		session->interpolation_type != interpolation_type ||
		session->interpolate_raw != interpolate_raw ||
		session->white_balance_raw != white_balance_raw ||
		session->mpeg4_deblock != mpeg4_deblock ||
		session->decode_subtitles != decode_subtitles ||
		session->subtitle_number != subtitle_number ||
		session->proxy_scale != proxy_scale ||
        session->disable_muted != disable_muted ||
        session->only_top != only_top)
		*result = 0;

// If it's before the current brender_start, render extra data.
// If it's after brender_start, check brender map.
	if(brender_start != session->brender_start &&
		(*result < 0 || *result > brender_start))
	{
		*result = brender_start;
	}
}


int EDLSession::load_defaults(BC_Hash *defaults)
{
	char string[BCTEXTLEN];

//	proxy_scale = defaults->get("PROXY_SCALE", 1);
// Default channel positions
	for(int i = 0; i < MAXCHANNELS; i++)
	{
		sprintf(string, "ACHANNEL_ANGLE_%d", i);
		int default_position = i * 30;

		if(i == 0) default_position = 180;
		else
		if(i == 1) default_position = 0;
		else
		if(default_position == 90) default_position = 300;
		else
		if(default_position == 0) default_position = 330;

		achannel_positions[i] = defaults->get(string, default_position);
	}
//	assetlist_format = defaults->get("ASSETLIST_FORMAT", ASSETS_TEXT);
	aspect_w = defaults->get("ASPECTW", (float)4);
	aspect_h = defaults->get("ASPECTH", (float)3);
    auto_aspect = defaults->get("AUTOASPECT", 1);
	audio_channels = defaults->get("ACHANNELS", 2);
	audio_tracks = defaults->get("ATRACKS", 2);
	auto_conf->load_defaults(defaults);
	autos_follow_edits = defaults->get("AUTOS_FOLLOW_EDITS", 1);
	brender_start = defaults->get("BRENDER_START", brender_start);
	brender_end = defaults->get("BRENDER_END", brender_end);
	cmodel_to_text(string, BC_RGBA8888);
	color_model = cmodel_from_text(defaults->get("COLOR_MODEL", string));
	eyedrop_radius = defaults->get("EYEDROP_RADIUS", 0);
	eyedrop_x = defaults->get("EYEDROP_X", 0);
	eyedrop_y = defaults->get("EYEDROP_Y", 0);
	crop_x1 = defaults->get("CROP_X1", 0);
	crop_x2 = defaults->get("CROP_X2", 320);
	crop_y1 = defaults->get("CROP_Y1", 0);
	crop_y2 = defaults->get("CROP_Y2", 240);
	ruler_x1 = defaults->get("RULER_X1", 0.0);
	ruler_x2 = defaults->get("RULER_X2", 0.0);
	ruler_y1 = defaults->get("RULER_Y1", 0.0);
	ruler_y2 = defaults->get("RULER_Y2", 0.0);
	always_draw_ruler = defaults->get("ALWAYS_DRAW_RULER", 0);
	sprintf(current_folder, MEDIA_FOLDER);
	defaults->get("CURRENT_FOLDER", current_folder);
	cursor_on_frames = defaults->get("CURSOR_ON_FRAMES", 0);
	typeless_keyframes = defaults->get("TYPELESS_KEYFRAMES", 0);
	cwindow_dest = defaults->get("CWINDOW_DEST", 0);
	cwindow_mask = defaults->get("CWINDOW_MASK", 0);
	cwindow_meter = defaults->get("CWINDOW_METER", 0);
	cwindow_operation = defaults->get("CWINDOW_OPERATION", CWINDOW_NONE);
	cwindow_scrollbars = defaults->get("CWINDOW_SCROLLBARS", 0);
	cwindow_xscroll = defaults->get("CWINDOW_XSCROLL", 0);
	cwindow_yscroll = defaults->get("CWINDOW_YSCROLL", 0);
	cwindow_zoom = defaults->get("CWINDOW_ZOOM", (float)1);
	sprintf(default_atransition, "Crossfade");
	defaults->get("DEFAULT_ATRANSITION", default_atransition);
	sprintf(default_vtransition, "Dissolve");
	defaults->get("DEFAULT_VTRANSITION", default_vtransition);
	default_transition_length = defaults->get("DEFAULT_TRANSITION_LENGTH", (double)1);
//	edit_handle_mode[0] = defaults->get("EDIT_HANDLE_MODE0", MOVE_ALL_EDITS);
//	edit_handle_mode[1] = defaults->get("EDIT_HANDLE_MODE1", MOVE_ONE_EDIT);
//	edit_handle_mode[2] = defaults->get("EDIT_HANDLE_MODE2", MOVE_NO_EDITS);
	editing_mode = defaults->get("EDITING_MODE", EDITING_IBEAM);
	enable_duplex = defaults->get("ENABLE_DUPLEX", 1);
//	folderlist_format = defaults->get("FOLDERLIST_FORMAT", FOLDERS_TEXT);
	frame_rate = defaults->get("FRAMERATE", (double)30000.0/1001);
	frames_per_foot = defaults->get("FRAMES_PER_FOOT", (float)16);
	interpolation_type = defaults->get("INTERPOLATION_TYPE", interpolation_type);
	interpolate_raw = defaults->get("INTERPOLATE_RAW", interpolate_raw);
	white_balance_raw = defaults->get("WHITE_BALANCE_RAW", white_balance_raw);
	labels_follow_edits = defaults->get("LABELS_FOLLOW_EDITS", 1);
	plugins_follow_edits = defaults->get("PLUGINS_FOLLOW_EDITS", 1);
	single_standalone = defaults->get("SINGLE_STANDALONE", 1);
	auto_keyframes = defaults->get("AUTO_KEYFRAMES", 0);
	meter_format = defaults->get("METER_FORMAT", METER_DB);
	min_meter_db = defaults->get("MIN_METER_DB", -85);
	max_meter_db = defaults->get("MAX_METER_DB", 6);
	mpeg4_deblock = defaults->get("MPEG4_DEBLOCK", mpeg4_deblock);
	output_w = defaults->get("OUTPUTW", 720);
	output_h = defaults->get("OUTPUTH", 480);
	playback_buffer = defaults->get("PLAYBACK_BUFFER", 4096);
	playback_preload = defaults->get("PLAYBACK_PRELOAD", 0);
	real_time_record = defaults->get("REALTIME_RECORD", 0);
	record_software_position = defaults->get("RECORD_SOFTWARE_POSITION", 1);
	record_sync_drives = defaults->get("RECORD_SYNC_DRIVES", 0);
//	record_speed = defaults->get("RECORD_SPEED", 24);
	record_fragment_size = defaults->get("RECORD_FRAGMENT_SIZE", 2048);
	record_write_length = defaults->get("RECORD_WRITE_LENGTH", 131072);
	
	

//	safe_regions = defaults->get("SAFE_REGIONS", 0);
	sample_rate = defaults->get("SAMPLERATE", 48000);
	show_assets = defaults->get("SHOW_ASSETS", 1);
	show_titles = defaults->get("SHOW_TITLES", 1);
//	test_playback_edits = defaults->get("TEST_PLAYBACK_EDITS", 1);
	time_format = defaults->get("TIME_FORMAT", TIME_HMS);
	nudge_seconds = defaults->get("NUDGE_FORMAT", 1);
//	tool_window = defaults->get("TOOL_WINDOW", 0);
	for(int i = 0; i < MAXCHANNELS; i++)
	{
		int default_position = i * output_w;
		sprintf(string, "VCHANNEL_X_%d", i);
		vchannel_x[i] = defaults->get(string, default_position);
		sprintf(string, "VCHANNEL_Y_%d", i);
		vchannel_y[i] = defaults->get(string, 0);
	}
	video_channels = defaults->get("VCHANNELS", 1);
//	video_asynchronous = defaults->get("VIDEO_ASYNCHRONOUS", 0);
	video_tracks = defaults->get("VTRACKS", 1);
	video_write_length = defaults->get("VIDEO_WRITE_LENGTH", 30);
	vwindow_meter = defaults->get("VWINDOW_METER", 0);


	decode_subtitles = defaults->get("DECODE_SUBTITLES", decode_subtitles);
	subtitle_number = defaults->get("SUBTITLE_NUMBER", subtitle_number);
	video_every_frame = defaults->get("VIDEO_EVERY_FRAME", video_every_frame);
    disable_muted = defaults->get("DISABLE_MUTED", disable_muted);
    only_top = defaults->get("ONLY_TOP", only_top);

	vwindow_zoom = defaults->get("VWINDOW_ZOOM", (float)1);
	boundaries();

	return 0;
}

int EDLSession::save_defaults(BC_Hash *defaults)
{
	char string[BCTEXTLEN];

//	defaults->update("PROXY_SCALE", proxy_scale);

// Session
	for(int i = 0; i < MAXCHANNELS; i++)
	{
		sprintf(string, "ACHANNEL_ANGLE_%d", i);
		defaults->update(string, achannel_positions[i]);
	}
	defaults->update("ACHANNELS", audio_channels);
	auto_conf->save_defaults(defaults);
//    defaults->update("ASSETLIST_FORMAT", assetlist_format);
    defaults->update("ASPECTW", aspect_w);
    defaults->update("ASPECTH", aspect_h);
    defaults->update("AUTOASPECT", auto_aspect);
	defaults->update("ATRACKS", audio_tracks);
	defaults->update("AUTOS_FOLLOW_EDITS", autos_follow_edits);
	defaults->update("BRENDER_START", brender_start);
	defaults->update("BRENDER_END", brender_end);
	cmodel_to_text(string, color_model);
	defaults->update("COLOR_MODEL", string);
	defaults->update("EYEDROP_RADIUS", eyedrop_radius);
	defaults->update("EYEDROP_X", eyedrop_x);
	defaults->update("EYEDROP_Y", eyedrop_y);
	defaults->update("CROP_X1", crop_x1);
	defaults->update("CROP_X2", crop_x2);
	defaults->update("CROP_Y1", crop_y1);
	defaults->update("CROP_Y2", crop_y2);
	defaults->update("RULER_X1", ruler_x1);
	defaults->update("RULER_X2", ruler_x2);
	defaults->update("RULER_Y1", ruler_y1);
	defaults->update("RULER_Y2", ruler_y2);
	defaults->update("ALWAYS_DRAW_RULER", always_draw_ruler);
	defaults->update("CURRENT_FOLDER", current_folder);
	defaults->update("CURSOR_ON_FRAMES", cursor_on_frames);
	defaults->update("TYPELESS_KEYFRAMES", typeless_keyframes);
	defaults->update("CWINDOW_DEST", cwindow_dest);
	defaults->update("CWINDOW_MASK", cwindow_mask);
	defaults->update("CWINDOW_METER", cwindow_meter);
	defaults->update("CWINDOW_OPERATION", cwindow_operation);
	defaults->update("CWINDOW_SCROLLBARS", cwindow_scrollbars);
	defaults->update("CWINDOW_XSCROLL", cwindow_xscroll);
	defaults->update("CWINDOW_YSCROLL", cwindow_yscroll);
	defaults->update("CWINDOW_ZOOM", cwindow_zoom);
	defaults->update("DEFAULT_ATRANSITION", default_atransition);
	defaults->update("DEFAULT_VTRANSITION", default_vtransition);
	defaults->update("DEFAULT_TRANSITION_LENGTH", default_transition_length);
//     defaults->update("EDIT_HANDLE_MODE0", edit_handle_mode[0]);
//     defaults->update("EDIT_HANDLE_MODE1", edit_handle_mode[1]);
//     defaults->update("EDIT_HANDLE_MODE2", edit_handle_mode[2]);
	defaults->update("EDITING_MODE", editing_mode);
	defaults->update("ENABLE_DUPLEX", enable_duplex);
//    defaults->update("FOLDERLIST_FORMAT", folderlist_format);
	defaults->update("FRAMERATE", frame_rate);
	defaults->update("FRAMES_PER_FOOT", frames_per_foot);
	defaults->update("HIGHLIGHTED_TRACK", highlighted_track);
    defaults->update("INTERPOLATION_TYPE", interpolation_type);
    defaults->update("INTERPOLATE_RAW", interpolate_raw);
    defaults->update("WHITE_BALANCE_RAW", white_balance_raw);
	defaults->update("LABELS_FOLLOW_EDITS", labels_follow_edits);
	defaults->update("PLUGINS_FOLLOW_EDITS", plugins_follow_edits);
	defaults->update("SINGLE_STANDALONE", single_standalone);
	defaults->update("AUTO_KEYFRAMES", auto_keyframes);
    defaults->update("METER_FORMAT", meter_format);
    defaults->update("MIN_METER_DB", min_meter_db);
    defaults->update("MAX_METER_DB", max_meter_db);
	defaults->update("MPEG4_DEBLOCK", mpeg4_deblock);
	defaults->update("OUTPUTW", output_w);
	defaults->update("OUTPUTH", output_h);
    defaults->update("PLAYBACK_BUFFER", playback_buffer);
	defaults->update("PLAYBACK_PRELOAD", playback_preload);
	defaults->update("REALTIME_RECORD", real_time_record);
    defaults->update("RECORD_SOFTWARE_POSITION", record_software_position);
	defaults->update("RECORD_SYNC_DRIVES", record_sync_drives);
//	defaults->update("RECORD_SPEED", record_speed);  
	defaults->update("RECORD_FRAGMENT_SIZE", record_fragment_size); 
	defaults->update("RECORD_WRITE_LENGTH", record_write_length); // Heroine kernel 2.2 scheduling sucks.
//	defaults->update("SAFE_REGIONS", safe_regions);
	defaults->update("SAMPLERATE", sample_rate);
	defaults->update("SHOW_ASSETS", show_assets);
	defaults->update("SHOW_TITLES", show_titles);
//	defaults->update("TEST_PLAYBACK_EDITS", test_playback_edits);
	defaults->update("TIME_FORMAT", time_format);
	defaults->update("NUDGE_FORMAT", nudge_seconds);
//	defaults->update("TOOL_WINDOW", tool_window);
	for(int i = 0; i < MAXCHANNELS; i++)
	{
		sprintf(string, "VCHANNEL_X_%d", i);
		defaults->update(string, vchannel_x[i]);
		sprintf(string, "VCHANNEL_Y_%d", i);
		defaults->update(string, vchannel_y[i]);
	}
	defaults->update("VCHANNELS", video_channels);
//    defaults->update("VIDEO_ASYNCHRONOUS", video_asynchronous);
	defaults->update("VTRACKS", video_tracks);
	defaults->update("VIDEO_WRITE_LENGTH", video_write_length);
	defaults->update("VWINDOW_METER", vwindow_meter);
	defaults->update("VWINDOW_ZOOM", vwindow_zoom);

	defaults->update("DECODE_SUBTITLES", decode_subtitles);
	defaults->update("SUBTITLE_NUMBER", subtitle_number);
	defaults->update("VIDEO_EVERY_FRAME", video_every_frame);
	defaults->update("DISABLE_MUTED", disable_muted);
	defaults->update("ONLY_TOP", only_top);


	return 0;
}



// GCC 3.0 fails to compile
#define BC_INFINITY 65536


void EDLSession::boundaries()
{
	Workarounds::clamp(proxy_scale, 1, 32);
	Workarounds::clamp(audio_tracks, 0, (int)BC_INFINITY);
	Workarounds::clamp(audio_channels, 1, MAXCHANNELS - 1);
	Workarounds::clamp(sample_rate, 1, 1000000);
	Workarounds::clamp(nested_sample_rate, -1, 1000000);
	Workarounds::clamp(video_tracks, 0, (int)BC_INFINITY);
	Workarounds::clamp(video_channels, 1, MAXCHANNELS - 1);
	Workarounds::clamp(frame_rate, 1.0, (double)BC_INFINITY);
	Workarounds::clamp(nested_frame_rate, -1.0, (double)BC_INFINITY);
	Workarounds::clamp(min_meter_db, -80, -20);
	Workarounds::clamp(max_meter_db, 0, 10);
	Workarounds::clamp(frames_per_foot, 1, 32);
	Workarounds::clamp(output_w, 16, (int)BC_INFINITY);
	Workarounds::clamp(output_h, 16, (int)BC_INFINITY);
	Workarounds::clamp(video_write_length, 1, 1000);
//printf("EDLSession::boundaries 1\n");
// 	output_w /= 2;
// 	output_w *= 2;
// 	output_h /= 2;
// 	output_h *= 2;

	Workarounds::clamp(eyedrop_radius, 0, 255);
	Workarounds::clamp(eyedrop_x, 0, 65535);
	Workarounds::clamp(eyedrop_y, 0, 65535);
	Workarounds::clamp(crop_x1, 0, output_w);
	Workarounds::clamp(crop_x2, 0, output_w);
	Workarounds::clamp(crop_y1, 0, output_h);
	Workarounds::clamp(crop_y2, 0, output_h);
	Workarounds::clamp(ruler_x1, 0.0, output_w);
	Workarounds::clamp(ruler_x2, 0.0, output_w);
	Workarounds::clamp(ruler_y1, 0.0, output_h);
	Workarounds::clamp(ruler_y2, 0.0, output_h);
	if(brender_start < 0) brender_start = 0.0;
	if(brender_end < 0) brender_end = 0.0;

	Workarounds::clamp(subtitle_number, 0, 31);
	
// Correct framerates
	frame_rate = Units::fix_framerate(frame_rate);
	
	if(interpolation_type == CUBIC_CUBIC ||
		interpolation_type == CUBIC_LINEAR ||
		interpolation_type == LINEAR_LINEAR ||
		interpolation_type == LANCZOS_LANCZOS)
	{
		interpolation_type = CUBIC_CUBIC;
	}
	else
	{
		interpolation_type = NEAREST_NEIGHBOR;
	}
}



int EDLSession::load_video_config(FileXML *file, int append_mode, uint32_t load_flags)
{
	char string[BCTEXTLEN];
	if(append_mode) return 0;

	video_every_frame = file->tag.get_property("VIDEO_EVERY_FRAME", video_every_frame);
    disable_muted = file->tag.get_property("DISABLE_MUTED", disable_muted);
    only_top = file->tag.get_property("ONLY_TOP", only_top);
	proxy_scale = file->tag.get_property("PROXY_SCALE", proxy_scale);
	interpolation_type = file->tag.get_property("INTERPOLATION_TYPE", interpolation_type);
	interpolate_raw = file->tag.get_property("INTERPOLATE_RAW", interpolate_raw);
	white_balance_raw = file->tag.get_property("WHITE_BALANCE_RAW", white_balance_raw);
	cmodel_to_text(string, color_model);
	color_model = cmodel_from_text(file->tag.get_property("COLORMODEL", string));
	video_channels = file->tag.get_property("CHANNELS", video_channels);
	for(int i = 0; i < video_channels; i++)
	{
		int default_position = i * output_w;
		sprintf(string, "VCHANNEL_X_%d", i);
		vchannel_x[i] = file->tag.get_property(string, default_position);
		sprintf(string, "VCHANNEL_Y_%d", i);
		vchannel_y[i] = file->tag.get_property(string, 0);
	}

	frame_rate = file->tag.get_property("FRAMERATE", frame_rate);
	frames_per_foot = file->tag.get_property("FRAMES_PER_FOOT", frames_per_foot);
	output_w = file->tag.get_property("OUTPUTW", output_w);
	output_h = file->tag.get_property("OUTPUTH", output_h);
	aspect_w = file->tag.get_property("ASPECTW", aspect_w);
	aspect_h = file->tag.get_property("ASPECTH", aspect_h);
	auto_aspect = file->tag.get_property("AUTOASPECT", auto_aspect);
	return 0;
}

int EDLSession::load_audio_config(FileXML *file, int append_mode, uint32_t load_flags)
{
	char string[32];
// load channels setting
	if(append_mode) return 0;
	audio_channels = file->tag.get_property("CHANNELS", (int64_t)audio_channels);


	for(int i = 0; i < audio_channels; i++)
	{
		sprintf(string, "ACHANNEL_ANGLE_%d", i);
		achannel_positions[i] = file->tag.get_property(string, achannel_positions[i]);
//printf("EDLSession::load_audio_config 1 %d %d\n", i, achannel_positions[i]);
	}

	sample_rate = file->tag.get_property("SAMPLERATE", (int64_t)sample_rate);
	return 0;
}

int EDLSession::load_xml(FileXML *file, 
	int append_mode, 
	uint32_t load_flags)
{
	char string[BCTEXTLEN];

	if(append_mode)
	{
	}
	else
	{
//		assetlist_format = file->tag.get_property("ASSETLIST_FORMAT", assetlist_format);
// 		for(int i = 0; i < ASSET_COLUMNS; i++)
// 		{
// 			sprintf(string, "ASSET_COLUMN%d", i);
// 			asset_columns[i] = file->tag.get_property(string, asset_columns[i]);
// 		}
		auto_conf->load_xml(file);
		auto_keyframes = file->tag.get_property("AUTO_KEYFRAMES", auto_keyframes);
		autos_follow_edits = file->tag.get_property("AUTOS_FOLLOW_EDITS", autos_follow_edits);
		brender_start = file->tag.get_property("BRENDER_START", brender_start);
		brender_end = file->tag.get_property("BRENDER_END", brender_end);
		eyedrop_radius = file->tag.get_property("EYEDROP_RADIUS", eyedrop_radius);
		eyedrop_x = file->tag.get_property("EYEDROP_X", eyedrop_x);
		eyedrop_y = file->tag.get_property("EYEDROP_Y", eyedrop_y);
		crop_x1 = file->tag.get_property("CROP_X1", crop_x1);
		crop_y1 = file->tag.get_property("CROP_Y1", crop_y1);
		crop_x2 = file->tag.get_property("CROP_X2", crop_x2);
		crop_y2 = file->tag.get_property("CROP_Y2", crop_y2);
		ruler_x1 = file->tag.get_property("RULER_X1", ruler_x1);
		ruler_y1 = file->tag.get_property("RULER_Y1", ruler_y1);
		ruler_x2 = file->tag.get_property("RULER_X2", ruler_x2);
		ruler_y2 = file->tag.get_property("RULER_Y2", ruler_y2);
		always_draw_ruler = file->tag.get_property("ALWAYS_DRAW_RULER", always_draw_ruler);
		file->tag.get_property("CURRENT_FOLDER", current_folder);
		cursor_on_frames = file->tag.get_property("CURSOR_ON_FRAMES", cursor_on_frames);
		typeless_keyframes = file->tag.get_property("TYPELESS_KEYFRAMES", typeless_keyframes);
		cwindow_dest = file->tag.get_property("CWINDOW_DEST", cwindow_dest);
		cwindow_mask = file->tag.get_property("CWINDOW_MASK", cwindow_mask);
		cwindow_meter = file->tag.get_property("CWINDOW_METER", cwindow_meter);
		cwindow_operation = file->tag.get_property("CWINDOW_OPERATION", cwindow_operation);
		cwindow_scrollbars = file->tag.get_property("CWINDOW_SCROLLBARS", cwindow_scrollbars);
		cwindow_xscroll = file->tag.get_property("CWINDOW_XSCROLL", cwindow_xscroll);
		cwindow_yscroll = file->tag.get_property("CWINDOW_YSCROLL", cwindow_yscroll);
		cwindow_zoom = file->tag.get_property("CWINDOW_ZOOM", cwindow_zoom);
		editing_mode = file->tag.get_property("EDITING_MODE", editing_mode);
//		folderlist_format = file->tag.get_property("FOLDERLIST_FORMAT", folderlist_format);
		highlighted_track = file->tag.get_property("HIGHLIGHTED_TRACK", 0);
		labels_follow_edits = file->tag.get_property("LABELS_FOLLOW_EDITS", labels_follow_edits);
		mpeg4_deblock = file->tag.get_property("MPEG4_DEBLOCK", mpeg4_deblock);
		plugins_follow_edits = file->tag.get_property("PLUGINS_FOLLOW_EDITS", plugins_follow_edits);
		single_standalone = file->tag.get_property("SINGLE_STANDALONE", single_standalone);
		playback_preload = file->tag.get_property("PLAYBACK_PRELOAD", playback_preload);
//		safe_regions = file->tag.get_property("SAFE_REGIONS", safe_regions);
		show_assets = file->tag.get_property("SHOW_ASSETS", 1);
		show_titles = file->tag.get_property("SHOW_TITLES", 1);
//		test_playback_edits = file->tag.get_property("TEST_PLAYBACK_EDITS", test_playback_edits);
		time_format = file->tag.get_property("TIME_FORMAT", time_format);
		nudge_seconds = file->tag.get_property("NUDGE_FORMAT", nudge_seconds);
//		tool_window = file->tag.get_property("TOOL_WINDOW", tool_window);
		vwindow_meter = file->tag.get_property("VWINDOW_METER", vwindow_meter);
		vwindow_zoom = file->tag.get_property("VWINDOW_ZOOM", vwindow_zoom);

		decode_subtitles = file->tag.get_property("DECODE_SUBTITLES", decode_subtitles);
		subtitle_number = file->tag.get_property("subtitle_number", subtitle_number);
		boundaries();
	}
	
	return 0;
}

int EDLSession::save_xml(FileXML *file)
{
//printf("EDLSession::save_session 1\n");
	char string[BCTEXTLEN];
	file->tag.set_title("SESSION");
//	file->tag.set_property("ASSETLIST_FORMAT", assetlist_format);
// 	for(int i = 0; i < ASSET_COLUMNS; i++)
// 	{
// 		sprintf(string, "ASSET_COLUMN%d", i);
// 		file->tag.set_property(string, asset_columns[i]);
// 	}
	auto_conf->save_xml(file);
	file->tag.set_property("AUTO_KEYFRAMES", auto_keyframes);
	file->tag.set_property("AUTOS_FOLLOW_EDITS", autos_follow_edits);
	file->tag.set_property("BRENDER_START", brender_start);
	file->tag.set_property("BRENDER_END", brender_end);
	file->tag.set_property("EYEDROP_RADIUS", eyedrop_radius);
	file->tag.set_property("EYEDROP_X", eyedrop_x);
	file->tag.set_property("EYEDROP_Y", eyedrop_y);
	file->tag.set_property("CROP_X1", crop_x1);
	file->tag.set_property("CROP_Y1", crop_y1);
	file->tag.set_property("CROP_X2", crop_x2);
	file->tag.set_property("CROP_Y2", crop_y2);
	file->tag.set_property("RULER_X1", ruler_x1);
	file->tag.set_property("RULER_Y1", ruler_y1);
	file->tag.set_property("RULER_X2", ruler_x2);
	file->tag.set_property("RULER_Y2", ruler_y2);
	file->tag.set_property("ALWAYS_DRAW_RULER", always_draw_ruler);
	file->tag.set_property("CURRENT_FOLDER", current_folder);
	file->tag.set_property("CURSOR_ON_FRAMES", cursor_on_frames);
	file->tag.set_property("TYPELESS_KEYFRAMES", typeless_keyframes);
	file->tag.set_property("CWINDOW_DEST", cwindow_dest);
	file->tag.set_property("CWINDOW_MASK", cwindow_mask);
	file->tag.set_property("CWINDOW_METER", cwindow_meter);
	file->tag.set_property("CWINDOW_OPERATION", cwindow_operation);
	file->tag.set_property("CWINDOW_SCROLLBARS", cwindow_scrollbars);
	file->tag.set_property("CWINDOW_XSCROLL", cwindow_xscroll);
	file->tag.set_property("CWINDOW_YSCROLL", cwindow_yscroll);
	file->tag.set_property("CWINDOW_ZOOM", cwindow_zoom);
	file->tag.set_property("EDITING_MODE", editing_mode);
//	file->tag.set_property("FOLDERLIST_FORMAT", folderlist_format);
	file->tag.set_property("HIGHLIGHTED_TRACK", highlighted_track);
	file->tag.set_property("LABELS_FOLLOW_EDITS", labels_follow_edits);
	file->tag.set_property("MPEG4_DEBLOCK", mpeg4_deblock);
	file->tag.set_property("PLUGINS_FOLLOW_EDITS", plugins_follow_edits);
	file->tag.set_property("SINGLE_STANDALONE", single_standalone);
	file->tag.set_property("PLAYBACK_PRELOAD", playback_preload);
//	file->tag.set_property("SAFE_REGIONS", safe_regions);
	file->tag.set_property("SHOW_ASSETS", show_assets);
	file->tag.set_property("SHOW_TITLES", show_titles);
	file->tag.set_property("TEST_PLAYBACK_EDITS", test_playback_edits);
	file->tag.set_property("TIME_FORMAT", time_format);
	file->tag.set_property("NUDGE_SECONDS", nudge_seconds);
//	file->tag.set_property("TOOL_WINDOW", tool_window);
	file->tag.set_property("VWINDOW_METER", vwindow_meter);
	file->tag.set_property("VWINDOW_ZOOM", vwindow_zoom);


	file->tag.set_property("DECODE_SUBTITLES", decode_subtitles);
	file->tag.set_property("SUBTITLE_NUMBER", subtitle_number);



	file->append_tag();
	file->append_newline();
	file->append_newline();
//printf("EDLSession::save_session 3\n");
	return 0;
}

int EDLSession::save_video_config(FileXML *file)
{
	char string[BCTEXTLEN];
	file->tag.set_title("VIDEO");
	file->tag.set_property("VIDEO_EVERY_FRAME", video_every_frame);
	file->tag.set_property("DISABLE_MUTED", disable_muted);
	file->tag.set_property("ONLY_TOP", only_top);
	file->tag.set_property("PROXY_SCALE", proxy_scale);
	file->tag.set_property("INTERPOLATION_TYPE", interpolation_type);
	file->tag.set_property("INTERPOLATE_RAW", interpolate_raw);
	file->tag.set_property("WHITE_BALANCE_RAW", white_balance_raw);
	cmodel_to_text(string, color_model);
	file->tag.set_property("COLORMODEL", string);
    file->tag.set_property("CHANNELS", video_channels);
	for(int i = 0; i < video_channels; i++)
	{
		sprintf(string, "VCHANNEL_X_%d", i);
		file->tag.set_property(string, vchannel_x[i]);
		sprintf(string, "VCHANNEL_Y_%d", i);
		file->tag.set_property(string, vchannel_y[i]);
	}

	file->tag.set_property("FRAMERATE", frame_rate);
	file->tag.set_property("FRAMES_PER_FOOT", frames_per_foot);
	file->tag.set_property("OUTPUTW", output_w);
	file->tag.set_property("OUTPUTH", output_h);
	file->tag.set_property("ASPECTW", aspect_w);
	file->tag.set_property("ASPECTH", aspect_h);
	file->tag.set_property("AUTOASPECT", auto_aspect);
	file->append_tag();
	file->append_newline();
	file->append_newline();
	return 0;
}

int EDLSession::save_audio_config(FileXML *file)
{
	char string[1024];
	file->tag.set_title("AUDIO");
	file->tag.set_property("SAMPLERATE", (int64_t)sample_rate);
	file->tag.set_property("CHANNELS", (int64_t)audio_channels);
	
	for(int i = 0; i < audio_channels; i++)
	{
		sprintf(string, "ACHANNEL_ANGLE_%d", i);
		file->tag.set_property(string, achannel_positions[i]);
	}
	
	file->append_tag();
	file->append_newline();
	file->append_newline();
	return 0;
}

int EDLSession::copy(EDLSession *session)
{
// Audio channel positions
	for(int i = 0; i < MAXCHANNELS; i++)
	{
		achannel_positions[i] = session->achannel_positions[i];
	}
// 	for(int i = 0; i < ASSET_COLUMNS; i++)
// 	{
// 		asset_columns[i] = session->asset_columns[i];
// 	}
	assetlist_format = session->assetlist_format;
	auto_conf->copy_from(session->auto_conf);
	aspect_w = session->aspect_w;
	aspect_h = session->aspect_h;
    auto_aspect = session->auto_aspect;
	audio_channels = session->audio_channels;
	audio_tracks = session->audio_tracks;
	autos_follow_edits = session->autos_follow_edits;
	brender_start = session->brender_start;
	brender_end = session->brender_end;
	color_model = session->color_model;
	eyedrop_radius = session->eyedrop_radius;
	eyedrop_x = session->eyedrop_x;
	eyedrop_y = session->eyedrop_y;
	crop_x1 = session->crop_x1;
	crop_y1 = session->crop_y1;
	crop_x2 = session->crop_x2;
	crop_y2 = session->crop_y2;
	ruler_x1 = session->ruler_x1;
	ruler_y1 = session->ruler_y1;
	ruler_x2 = session->ruler_x2;
	ruler_y2 = session->ruler_y2;
	always_draw_ruler = session->always_draw_ruler;
	strcpy(current_folder, session->current_folder);
	cursor_on_frames = session->cursor_on_frames;
	typeless_keyframes = session->typeless_keyframes;
	cwindow_dest = session->cwindow_dest;
	cwindow_mask = session->cwindow_mask;
	cwindow_meter = session->cwindow_meter;
	cwindow_operation = session->cwindow_operation;
	cwindow_scrollbars = session->cwindow_scrollbars;
	cwindow_xscroll = session->cwindow_xscroll;
	cwindow_yscroll = session->cwindow_yscroll;
	cwindow_zoom = session->cwindow_zoom;
	strcpy(default_atransition, session->default_atransition);
	strcpy(default_vtransition, session->default_vtransition);
	default_transition_length = session->default_transition_length;
//	edit_handle_mode[0] = session->edit_handle_mode[0];
//	edit_handle_mode[1] = session->edit_handle_mode[1];
//	edit_handle_mode[2] = session->edit_handle_mode[2];
	editing_mode = session->editing_mode;
	enable_duplex = session->enable_duplex;
	folderlist_format = session->folderlist_format;
	frame_rate = session->frame_rate;
	nested_frame_rate = session->nested_frame_rate;
	frames_per_foot = session->frames_per_foot;
	highlighted_track = session->highlighted_track;
	interpolation_type = session->interpolation_type;
	interpolate_raw = session->interpolate_raw;
	white_balance_raw = session->white_balance_raw;
	labels_follow_edits = session->labels_follow_edits;
	plugins_follow_edits = session->plugins_follow_edits;
	single_standalone = session->single_standalone;
	auto_keyframes = session->auto_keyframes;
//	last_playback_position = session->last_playback_position;
	meter_format = session->meter_format;
	min_meter_db = session->min_meter_db;
	max_meter_db = session->max_meter_db;
	mpeg4_deblock = session->mpeg4_deblock;
	output_w = session->output_w;
	output_h = session->output_h;
	playback_buffer = session->playback_buffer;
	playback_cursor_visible = session->playback_cursor_visible;
	playback_preload = session->playback_preload;
	real_time_record = session->real_time_record;
//	record_speed = session->record_speed;
	record_sync_drives = session->record_sync_drives;
	record_fragment_size = session->record_fragment_size;
	record_write_length = session->record_write_length;
	record_software_position = session->record_software_position;
//	safe_regions = session->safe_regions;
	sample_rate = session->sample_rate;
    nested_sample_rate = session->nested_sample_rate;
	show_assets = session->show_assets;
	show_titles = session->show_titles;
	test_playback_edits = session->test_playback_edits;
	time_format = session->time_format;
	nudge_seconds = session->nudge_seconds;
//	tool_window = session->tool_window;
	for(int i = 0; i < MAXCHANNELS; i++)
	{
		vchannel_x[i] = session->vchannel_x[i];
		vchannel_y[i] = session->vchannel_y[i];
	}
	video_channels = session->video_channels;
//	video_asynchronous = session->video_asynchronous;
	video_tracks = session->video_tracks;
	video_write_length = session->video_write_length;
	vwindow_meter = session->vwindow_meter;
	vwindow_zoom = session->vwindow_zoom;
	proxy_scale = session->proxy_scale;

	subtitle_number = session->subtitle_number;
	decode_subtitles = session->decode_subtitles;
    video_every_frame = session->video_every_frame;
    disable_muted = session->disable_muted;
    only_top = session->only_top;
	
	return 0;
}


void EDLSession::dump()
{
	printf("EDLSession::dump\n");
	printf("    audio_tracks=%d\n"
            "    audio_channels=%d\n"
            "    sample_rate=%" PRId64 "\n"
            "    nested_sample_rate=%" PRId64 "\n"
			"    video_tracks=%d\n"
            "    frame_rate=%f\n"
            "    nested_frame_rate=%f\n"
            "    output_w=%d\n"
            "    output_h=%d\n"
			"    aspect_w=%f\n"
            "    aspect_h=%f\n"
            "    auto_aspect=%d\n"
            "    decode subtitles=%d\n"
            "    subtitle_number=%d\n"
			"    proxy_scale=%d\n", 
		audio_tracks, 
		audio_channels, 
		sample_rate, 
        nested_sample_rate,
		video_tracks, 
		frame_rate, 
        nested_frame_rate,
		output_w, 
		output_h, 
		aspect_w, 
		aspect_h,
        auto_aspect,
		decode_subtitles,
		subtitle_number,
		proxy_scale);
}
