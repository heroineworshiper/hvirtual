/*
 * CINELERRA
 * Copyright (C) 2009-2024 Adam Williams <broadcast at earthling dot net>
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
#include "bcsignals.h"
#include "cache.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "mwindow.h"
#include "patch.h"
#include "playabletracks.h"
#include "preferences.h"
#include "mainsession.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "transportque.h"
#include "units.h"
#include "vedit.h"
#include "vedits.h"
#include "vframe.h"
#include "vtrack.h"

VEdit::VEdit(EDL *edl, Edits *edits)
 : Edit(edl, edits)
{
}


VEdit::~VEdit() { }

int VEdit::load_properties_derived(FileXML *xml)
{
	channel = xml->tag.get_property("CHANNEL", (int64_t)0);
	return 0;
}





Asset* VEdit::get_nested_asset(int64_t *source_position,
	int64_t position,
	int direction)
{
	const int debug = 0;
	Asset *result = 0;
// Make position relative to edit
	*source_position = position - startproject + startsource;

if(debug) printf("VEdit::get_nested_asset %d %ld %ld %ld %ld\n", 
__LINE__, 
*source_position, 
position,
startproject,
startsource);


// Descend into nested EDLs
	if(nested_edl)
	{
// Convert position to nested EDL rate
if(debug) printf("VEdit::get_nested_asset %d\n", 
__LINE__);
		if(direction == PLAY_REVERSE) (*source_position)--;
		*source_position = Units::to_int64(*source_position *
			nested_edl->session->get_nested_frame_rate() /
			edl->session->frame_rate);
		if(direction == PLAY_REVERSE) (*source_position)++;
		PlayableTracks *playable_tracks = new PlayableTracks(
			nested_edl, 
			*source_position, 
			direction,
			TRACK_VIDEO /*,
			1 */);
		if(playable_tracks->size())
		{
			VTrack *nested_track = (VTrack*)playable_tracks->get(0);
			VEdit* nested_edit = (VEdit*)nested_track->edits->editof(
				*source_position, 
				direction,
				1);
			if(nested_edit)
			{
				result = nested_edit->get_nested_asset(
					source_position,
					*source_position,
					direction);
			}
		}

		delete playable_tracks;
if(debug) printf("VEdit::get_nested_asset %d\n", 
__LINE__);
		return result;
	}
	else
	{
// Convert position to asset rate
if(debug) printf("VEdit::get_nested_asset %d %ld %f %f\n", 
__LINE__, 
*source_position, 
asset->frame_rate,
edl->session->frame_rate);
		if(direction == PLAY_REVERSE) (*source_position)--;

		(*source_position) = Units::to_int64((double)(*source_position) * 
			asset->frame_rate / 
			edl->session->frame_rate);


		if(direction == PLAY_REVERSE) (*source_position)++;

		return asset;
	}
}


int VEdit::read_frame(VFrame *video_out, 
	int64_t input_position, 
	int direction,
	CICache *cache,
	int use_nudge,
	int use_cache,
	int use_asynchronous,
    int use_opengl,
    VDeviceX11 *device)
{
	int64_t source_position = 0;
	const int debug = 0;

	if(use_nudge) input_position += track->nudge;
// printf("VEdit::read_frame %d source_position=%lld input_position=%lld cmodel=%d rows=%p\n", 
// __LINE__, 
// (long long)source_position,
// (long long)input_position,
// video_out->get_color_model(),
// video_out->get_rows()[0]);

	Asset *asset = get_nested_asset(&source_position,
		input_position,
		direction);

// printf("VEdit::read_frame %d source_position=%ld input_position=%ld\n", 
// __LINE__, 
// (long)source_position,
// (long)input_position);

	File *file = cache->check_out(asset,
		edl);
	int result = 0;

if(debug) printf("VEdit::read_frame %d path=%s source_position=%lld\n", 
__LINE__, 
asset->path, 
(long long)source_position);

	if(file)
	{

if(debug) printf("VEdit::read_frame %d\n", __LINE__);
		source_position = (direction == PLAY_FORWARD) ? 
			source_position : 
			(source_position - 1);
if(debug) printf("VEdit::read_frame %d %lld %lld\n", 
__LINE__,
(long long)input_position,
(long long)source_position);

// 		if(use_asynchronous)
// 			file->start_video_decode_thread();
// 		else
			file->stop_video_thread();
if(debug) printf("VEdit::read_frame %d\n", __LINE__);

		file->set_layer(channel);
//printf("VEdit::read_frame %d %lld\n", __LINE__, source_position);
		file->set_video_position(source_position, 0);

		if(use_cache) file->set_cache_frames(use_cache);
		result = file->read_frame(video_out, 0, use_opengl, device);

if(debug) printf("VEdit::read_frame %d\n", __LINE__);
		if(use_cache) file->set_cache_frames(0);

if(debug) printf("VEdit::read_frame %d\n", __LINE__);
		cache->check_in(asset);
if(debug) printf("VEdit::read_frame %d\n", __LINE__);
	}
	else
		result = 1;

	return result;
}

int VEdit::copy_properties_derived(FileXML *xml, int64_t length_in_selection)
{
	return 0;
}

int VEdit::dump_derived()
{
	printf("	VEdit::dump_derived\n");
	printf("		startproject %ld\n", startproject);
	printf("		length %ld\n", length);
    return 0;
}

int64_t VEdit::get_source_end(int64_t default_)
{
	if(!nested_edl && !asset) return default_;   // Infinity

	if(nested_edl)
	{
		return (int64_t)(nested_edl->tracks->total_playable_length() *
            nested_edl->session->frame_rate / 
            nested_edl->session->get_nested_frame_rate() *
			edl->session->frame_rate + 0.5);
	}

	return (int64_t)((double)asset->video_length / 
		asset->frame_rate * 
		edl->session->frame_rate + 0.5);
}
