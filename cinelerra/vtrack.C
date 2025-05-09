
/*
 * CINELERRA
 * Copyright (C) 2008-2024 Adam Williams <broadcast at earthling dot net>
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
#include "autoconf.h"
#include "cache.h"
#include "clip.h"
#include "datatype.h"
#include "edit.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "floatauto.h"
#include "floatautos.h"
#include "language.h"
#include "localsession.h"
#include "patch.h"
#include "mainsession.h"
#include "theme.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "transportque.inc"
#include "units.h"
#include "vautomation.h"
#include "vedit.h"
#include "vedits.h"
#include "vframe.h"
#include "vmodule.h"
//#include "vpluginset.h"
#include "vtrack.h"

VTrack::VTrack(EDL *edl, Tracks *tracks)
 : Track(edl, tracks)
{
	data_type = TRACK_VIDEO;
	draw = 1;
}

VTrack::~VTrack()
{
}

Edits* VTrack::new_edits()
{
    return new VEdits(edl, this);
}

void VTrack::create_objects()
{
	Track::create_objects();
	automation = new VAutomation(edl, this);
	automation->create_objects();
}

// Used by PlaybackEngine
void VTrack::synchronize_params(Track *track)
{
	Track::synchronize_params(track);

	VTrack *vtrack = (VTrack*)track;
}

// Used by EDL::operator=
int VTrack::copy_settings(Track *track)
{
	Track::copy_settings(track);

	VTrack *vtrack = (VTrack*)track;
	return 0;
}

int VTrack::vertical_span(Theme *theme)
{
	int track_h = Track::vertical_span(theme);
	int patch_h = 0;
	if(expand_view)
	{
		patch_h += theme->title_h + theme->play_h + theme->fade_h + theme->mode_h;
	}
	return MAX(track_h, patch_h);
}


// PluginSet* VTrack::new_plugins()
// {
// 	return new VPluginSet(edl, this);
// }

int VTrack::load_defaults(BC_Hash *defaults)
{
	Track::load_defaults(defaults);
	return 0;
}

void VTrack::set_default_title()
{
	Track *current = ListItem<Track>::owner->first;
	int i;
	for(i = 0; current; current = NEXT)
	{
		if(current->data_type == TRACK_VIDEO) i++;
	}
	sprintf(title, _("Video %d"), i);
}

int64_t VTrack::to_units(double position, int round)
{
	if(round)
	{
		return Units::round(position * edl->session->frame_rate);
	}
	else
	{
// Kludge for rounding errors, just on a smaller scale than formal rounding
		position *= edl->session->frame_rate;
		return Units::to_int64(position);
	}
}

double VTrack::to_doubleunits(double position)
{
	return position * edl->session->frame_rate;
}


double VTrack::from_units(int64_t position)
{
	return (double)position / edl->session->frame_rate;
}




int VTrack::identical(int64_t sample1, int64_t sample2)
{
// Units of frames
	if(labs(sample1 - sample2) <= 1) return 1; else return 0;
}



int VTrack::direct_copy_possible(int64_t start, int direction, int use_nudge)
{
	int i;
	if(use_nudge) start += nudge;

// Track size must equal output size
	if(track_w != edl->session->output_w || track_h != edl->session->output_h)
		return 0;
		
	if(has_speed()) return 0;

// No automation must be present in the track
	if(!automation->direct_copy_possible(start, direction))
		return 0;

// No plugin must be present
	if(plugin_used(start, direction)) 
		return 0;

// No transition
	if(get_current_transition(start, direction, 0, 0))
		return 0;

	return 1;
}

















int VTrack::create_derived_objs(int flash)
{
	int i;
	edits = new VEdits(edl, this);
	return 0;
}


// int VTrack::get_dimensions(double &view_start, 
// 	double &view_units, 
// 	double &zoom_units)
// {
// 	view_start = edl->local_session->view_start * edl->session->frame_rate;
// 	view_units = 0;
// 	zoom_units = edl->local_session->zoom_sample / edl->session->sample_rate * edl->session->frame_rate;
// }

int VTrack::copy_derived(int64_t start, int64_t end, FileXML *xml)
{
// automation is copied in the Track::copy
	return 0;
}

int VTrack::copy_automation_derived(AutoConf *auto_conf, int64_t start, int64_t end, FileXML *file)
{
	return 0;
}

int VTrack::paste_derived(int64_t start, int64_t end, int64_t total_length, FileXML *xml, int &current_channel)
{
	return 0;
}

int VTrack::paste_output(int64_t startproject, int64_t endproject, int64_t startsource, int64_t endsource, int layer, Asset *asset)
{
	return 0;
}

int VTrack::clear_derived(int64_t start, int64_t end)
{
	return 0;
}

int VTrack::paste_automation_derived(int64_t start, int64_t end, int64_t total_length, FileXML *xml, int shift_autos, int &current_pan)
{
	return 0;
}

int VTrack::clear_automation_derived(AutoConf *auto_conf, int64_t start, int64_t end, int shift_autos)
{
	return 0;
}

int VTrack::draw_autos_derived(float view_start, float zoom_units, AutoConf *auto_conf)
{
	return 0;
}


int VTrack::select_auto_derived(float zoom_units, float view_start, AutoConf *auto_conf, int cursor_x, int cursor_y)
{
	return 0;
}


int VTrack::move_auto_derived(float zoom_units, float view_start, AutoConf *auto_conf, int cursor_x, int cursor_y, int shift_down)
{
	return 0;
}

int VTrack::draw_floating_autos_derived(float view_start, float zoom_units, AutoConf *auto_conf, int flash)
{
	return 0;
}

// int VTrack::is_playable(int64_t position, int direction)
// {
// 	int result = 0;
// 	float in_x, in_y, in_w, in_h;
// 	float out_x, out_y, out_w, out_h;
// 
// 	calculate_output_transfer(position, 
// 		direction, 
// 		in_x, in_y, in_w, in_h,
// 		out_x, out_y, out_w, out_h);
// 
// //printf("VTrack::is_playable %0.0f %0.0f %0.0f %0.0f %0.0f %0.0f %0.0f %0.0f\n", 
// //in_x, in_y, in_w, in_h, out_x, out_y, out_w, out_h);
// 	if(out_w > 0 && out_h > 0) 
// 		result = 1;
// 	return result;
// }

void VTrack::calculate_input_transfer(int asset_w, 
	int asset_h,
	int64_t position, 
	int direction, 
	float &in_x, 
	float &in_y, 
	float &in_w, 
	float &in_h,
	float &out_x, 
	float &out_y, 
	float &out_w, 
	float &out_h)
{
	float auto_x, auto_y, auto_z;
	float camera_z = 1;
	float camera_x = asset_w / 2;
	float camera_y = asset_h / 2;
// camera and output coords
	float z[6], x[6], y[6];        

//printf("VTrack::calculate_input_transfer %lld\n", position);

// get camera center in asset
	automation->get_camera(&auto_x, 
		&auto_y, 
		&auto_z, 
		position, 
		direction);

	camera_z *= auto_z;
	camera_x += auto_x;
	camera_y += auto_y;

// get camera coords on asset
	x[0] = camera_x - (float)track_w / 2 / camera_z;
	y[0] = camera_y - (float)track_h / 2 / camera_z;
	x[1] = x[0] + (float)track_w / camera_z;
	y[1] = y[0] + (float)track_h / camera_z;

// get asset coords on camera
	x[2] = 0;
	y[2] = 0;
	x[3] = track_w;
	y[3] = track_h;

// crop asset coords on camera
	if(x[0] < 0)
	{
		x[2] -= x[0] * camera_z;
		x[0] = 0;
	}
	if(y[0] < 0)
	{
		y[2] -= y[0] * camera_z;
		y[0] = 0;
	}
	if(x[1] > asset_w)
	{
		x[3] -= (x[1] - asset_w) * camera_z;
		x[1] = asset_w;
	}
	if(y[1] > asset_h)
	{
		y[3] -= (y[1] - asset_h) * camera_z;
		y[1] = asset_h;
	}

// get output bounding box
	out_x = x[2];
	out_y = y[2];
	out_w = x[3] - x[2];
	out_h = y[3] - y[2];

	in_x = x[0];
	in_y = y[0];
	in_w = x[1] - x[0];
	in_h = y[1] - y[0];
// printf("VTrack::calculate_input_transfer %f %f %f %f -> %f %f %f %f\n", 
// in_x, in_y, in_w, in_h,
// out_x, out_y, out_w, out_h);
}

void VTrack::calculate_output_transfer(int64_t position, 
	int direction, 
	float &in_x, 
	float &in_y, 
	float &in_w, 
	float &in_h,
	float &out_x, 
	float &out_y, 
	float &out_w, 
	float &out_h)
{
	float center_x, center_y, center_z;
	float x[4], y[4];

	x[0] = 0;
	y[0] = 0;
	x[1] = track_w;
	y[1] = track_h;

	automation->get_projector(&center_x, 
		&center_y, 
		&center_z, 
		position, 
		direction);

	center_x += edl->session->output_w / 2;
	center_y += edl->session->output_h / 2;

	x[2] = center_x - (track_w / 2) * center_z;
	y[2] = center_y - (track_h / 2) * center_z;
	x[3] = x[2] + track_w * center_z;
	y[3] = y[2] + track_h * center_z;

// Clip to boundaries of output
	if(x[2] < 0)
	{
		x[0] -= (x[2] - 0) / center_z;
		x[2] = 0;
	}
	if(y[2] < 0)
	{
		y[0] -= (y[2] - 0) / center_z;
		y[2] = 0;
	}
	if(x[3] > edl->session->output_w)
	{
		x[1] -= (x[3] - edl->session->output_w) / center_z;
		x[3] = edl->session->output_w;
	}
	if(y[3] > edl->session->output_h)
	{
		y[1] -= (y[3] - edl->session->output_h) / center_z;
		y[3] = edl->session->output_h;
	}

	in_x = x[0];
	in_y = y[0];
	in_w = x[1] - x[0];
	in_h = y[1] - y[0];
	out_x = x[2];
	out_y = y[2];
	out_w = x[3] - x[2];
	out_h = y[3] - y[2];
// printf("VTrack::calculate_output_transfer %f %f %f %f -> %f %f %f %f\n", 
// in_x, in_y, in_w, in_h,
// out_x, out_y, out_w, out_h);
}



int VTrack::get_projection(float &in_x1, 
	float &in_y1, 
	float &in_x2, 
	float &in_y2, 
	float &out_x1, 
	float &out_y1, 
	float &out_x2, 
	float &out_y2, 
	int frame_w, 
	int frame_h, 
	int64_t real_position, 
	int direction)
{
	float center_x, center_y, center_z;
	float x[4], y[4];

	automation->get_projector(&center_x, 
		&center_y, 
		&center_z, 
		real_position, 
		direction);

	x[0] = y[0] = 0;
	x[1] = frame_w;
	y[1] = frame_h;

	center_x += edl->session->output_w / 2;
	center_y += edl->session->output_h / 2;

	x[2] = center_x - (frame_w / 2) * center_z;
	y[2] = center_y - (frame_h / 2) * center_z;
	x[3] = x[2] + frame_w * center_z;
	y[3] = y[2] + frame_h * center_z;

	if(x[2] < 0)
	{
		x[0] -= x[2] / center_z;
		x[2] = 0;
	}
	if(y[2] < 0)
	{
		y[0] -= y[2] / center_z;
		y[2] = 0;
	}
	if(x[3] > edl->session->output_w)
	{
		x[1] -= (x[3] - edl->session->output_w) / center_z;
		x[3] = edl->session->output_w;
	}
	if(y[3] > edl->session->output_h)
	{
		y[1] -= (y[3] - edl->session->output_h) / center_z;
		y[3] = edl->session->output_h;
	}

	in_x1 = x[0];
	in_y1 = y[0];
	in_x2 = x[1];
	in_y2 = y[1];
	out_x1 = x[2];
	out_y1 = y[2];
	out_x2 = x[3];
	out_y2 = y[3];
	return 0;
}


void VTrack::translate(float offset_x, float offset_y, int do_camera)
{
	int subscript;
	if(do_camera) 
		subscript = AUTOMATION_CAMERA_X;
	else
		subscript = AUTOMATION_PROJECTOR_X;
	
// Translate default keyframe
	((FloatAuto*)automation->autos[subscript]->default_auto)->value += offset_x;
	((FloatAuto*)automation->autos[subscript + 1]->default_auto)->value += offset_y;

// Translate everyone else
	for(Auto *current = automation->autos[subscript]->first; 
		current; 
		current = NEXT)
	{
		((FloatAuto*)current)->value += offset_x;
	}

	for(Auto *current = automation->autos[subscript + 1]->first; 
		current; 
		current = NEXT)
	{
		((FloatAuto*)current)->value += offset_y;
	}
}









