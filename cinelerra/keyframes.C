
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
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

#include "bchash.h"
#include "clip.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "keyframe.h"
#include "keyframes.h"
#include "localsession.h"
#include "track.h"
#include "transportque.inc"

KeyFrames::KeyFrames(EDL *edl, Track *track)
 : Autos(edl, track, Autos::AUTOMATION_TYPE_PLUGIN, PLUGIN_KEYFRAMES)
{
}

KeyFrames::~KeyFrames()
{
}


KeyFrame* KeyFrames::get_prev_keyframe(int64_t position,
	int direction)
{
	KeyFrame *current = 0;

// This doesn't work because edl->selectionstart doesn't change during
// playback at the same rate as PluginClient::source_position.
	if(position < 0)
	{
		position = track->to_units(edl->local_session->get_selectionstart(1), 0);
	}

// Get keyframe on or before current position
	for(current = (KeyFrame*)last;
		current;
		current = (KeyFrame*)PREVIOUS)
	{
		if(direction == PLAY_FORWARD && current->position <= position) break;
		else
		if(direction == PLAY_REVERSE && current->position < position) break;
	}

// Nothing before current position
	if(!current && first)
	{
		current = (KeyFrame*)first;
//printf("KeyFrames::get_prev_keyframe %d %p\n", __LINE__, current->get_data());
	}
	else
// No keyframes
	if(!current)
	{
		current = (KeyFrame*)default_auto;
//printf("KeyFrames::get_prev_keyframe %d %p\n", __LINE__, current->get_data());
	}

	return current;
}

KeyFrame* KeyFrames::get_keyframe()
{
// Search for keyframe on or before selection
	KeyFrame *result = 
		get_prev_keyframe(
			track->to_units(edl->local_session->get_selectionstart(1), 0), 
			PLAY_FORWARD);

// Return nearest keyframe if not in automatic keyframe generation
	if(!edl->session->auto_keyframes)
	{
		return result;
	}
	else
// Return new keyframe
	if(result == (KeyFrame*)default_auto || 
		result->position != track->to_units(edl->local_session->get_selectionstart(1), 0))
	{
		return (KeyFrame*)insert_auto(track->to_units(edl->local_session->get_selectionstart(1), 0));
	}
	else
// Return existing keyframe
	{
		return result;
	}

	return 0;
}


void KeyFrames::update_parameter(KeyFrame *src)
{
	const int debug = 0;
// Create new keyframe if auto keyframes or replace entire keyframe.
	double selection_start = edl->local_session->get_selectionstart(0);
	double selection_end = edl->local_session->get_selectionend(0);
	selection_start = edl->align_to_frame(selection_start, 0);
	selection_end = edl->align_to_frame(selection_end, 0);

//printf("KeyFrames::update_parameter %d\n", __LINE__);
	if(EQUIV(selection_start, selection_end))
	{
// Search for keyframe to write to
		KeyFrame *dst = get_keyframe();
		
		dst->copy_data(src);
	}
	else
// Replace changed parameter in all selected keyframes.
// TODO: if the changed parameter doesn't exist in the EDL because of a 
// software change, it'll never get saved unless the user selects a single point
// & issues another change.
	{
		BC_Hash *params = 0;
		char *text = 0;
		char *extra = 0;


// Search all keyframes in selection but don't create a new one.
		int64_t start = track->to_units(selection_start, 0);
		int64_t end = track->to_units(selection_end, 0);
		KeyFrame *current = get_prev_keyframe(
				start, 
				PLAY_FORWARD);

// The first one determines the changed parameters since it is the one displayed
// in the GUI.
		current->get_diff(src,
			&params, 
			&text,
			&extra);


if(debug) printf("KeyFrames::update_parameter %d params=%p position=%ld start=%ld\n", 
__LINE__,  
params,
(long)current->position,
(long)track->to_units(start, 0));

if(debug && params)
{
for(int i = 0; i < params->size(); i++)
printf("KeyFrames::update_parameter %d changed=%s %s\n", 
__LINE__,  
params->get_key(i),
params->get_value(i));
}

// Always update the first one
		current->update_parameter(params, 
			text,
			extra);
		for(current = (KeyFrame*)NEXT ; current && current->position < end; current = (KeyFrame*)NEXT)
		{
if(debug) printf("KeyFrames::update_parameter %d position=%lld\n", 
__LINE__,
(long long)current->position);
			current->update_parameter(params, 
				text,
				extra);
		}
		
		delete params;
		delete [] text,
		delete [] extra;
	}
}

Auto* KeyFrames::new_auto()
{
	return new KeyFrame(edl, this);
}


void KeyFrames::dump()
{
	printf("    DEFAULT_KEYFRAME\n");
	((KeyFrame*)default_auto)->dump();
	printf("    KEYFRAMES total=%d\n", total());
	for(KeyFrame *current = (KeyFrame*)first;
		current;
		current = (KeyFrame*)NEXT)
	{
		current->dump();
	}
}

