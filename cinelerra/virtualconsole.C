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

#include "auto.h"
#include "automation.h"
#include "autos.h"
#include "bcsignals.h"
#include "commonrender.h"
#include "condition.h"
#include "edl.h"
#include "edlsession.h"
#include "intautos.h"
#include "module.h"
#include "mutex.h"
#include "playabletracks.h"
#include "renderengine.h"
#include "tracks.h"
#include "transportque.h"
#include "virtualconsole.h"
#include "virtualnode.h"


VirtualConsole::VirtualConsole(RenderEngine *renderengine, 
	CommonRender *commonrender,
	int data_type)
{
	this->renderengine = renderengine;
	this->commonrender = commonrender;
	this->data_type = data_type;
	total_exit_nodes = 0;
	playable_tracks = 0;
	entry_nodes = 0;
	debug_tree = 0;
//printf("VirtualConsole::VirtualConsole\n");
}


VirtualConsole::~VirtualConsole()
{
	delete_virtual_console();

	delete playable_tracks;
//printf("VirtualConsole::~VirtualConsole\n");
}


void VirtualConsole::create_objects()
{
	interrupt = 0;
	done = 0;

	get_playable_tracks();
	total_exit_nodes = playable_tracks->size();
	build_virtual_console(1);
//dump();
}

void VirtualConsole::start_playback()
{
	interrupt = 0;
	done = 0;
}

void VirtualConsole::get_playable_tracks()
{
	if(!playable_tracks)
		playable_tracks = new PlayableTracks(renderengine->get_edl(), 
			commonrender->current_position, 
			renderengine->command->get_direction(),
			data_type /*,
			1 */);
}

Module* VirtualConsole::module_of(Track *track)
{
	for(int i = 0; i < commonrender->total_modules; i++)
	{
		if(commonrender->modules[i]->track == track) 
			return commonrender->modules[i];
	}
	return 0;
}

Module* VirtualConsole::module_number(int track_number)
{
// The track number is an absolute number of the track independant of
// the tracks with matching data type but virtual modules only exist for
// the matching data type.
// Convert from absolute track number to data type track number.
	Track *current = renderengine->get_edl()->tracks->first;
	int data_type_number = 0, number = 0;

	for( ; current; current = NEXT, number++)
	{
		if(current->data_type == data_type)
		{
			if(number == track_number)
				return commonrender->modules[data_type_number];
			else
				data_type_number++;
		}
	}


	return 0;
}

void VirtualConsole::build_virtual_console(int persistent_plugins)
{
// allocate the entry nodes
	if(!entry_nodes)
	{
		entry_nodes = new VirtualNode*[total_exit_nodes];

// printf("VirtualConsole::build_virtual_console %d total_exit_nodes=%d\n", 
// __LINE__,
// total_exit_nodes);
		for(int i = 0; i < total_exit_nodes; i++)
		{
// printf("VirtualConsole::build_virtual_console %d track=%p module=%p\n",
// __LINE__,
// playable_tracks->get(i),
// module_of(playable_tracks->get(i)));
			entry_nodes[i] = new_entry_node(playable_tracks->get(i), 
				module_of(playable_tracks->get(i)), 
				i);

// Expand the trees
			entry_nodes[i]->expand(persistent_plugins, 
				commonrender->current_position);
		}
		commonrender->restart_plugins = 1;
	}
//dump();
}

VirtualNode* VirtualConsole::new_entry_node(Track *track, 
	Module *module, 
	int track_number)
{
	printf("VirtualConsole::new_entry_node should not be called\n");
	return 0;
}

void VirtualConsole::append_exit_node(VirtualNode *node)
{
	node->is_exit = 1;
	exit_nodes.append(node);
}

void VirtualConsole::reset_attachments()
{
	for(int i = 0; i < commonrender->total_modules; i++)
	{
		commonrender->modules[i]->reset_attachments();
	}
}

void VirtualConsole::dump()
{
	printf("VirtualConsole\n");
	printf(" Modules\n");
	for(int i = 0; i < commonrender->total_modules; i++)
		commonrender->modules[i]->dump();
	printf(" Nodes\n");
	for(int i = 0; i < total_exit_nodes; i++)
		entry_nodes[i]->dump(0);
}


int VirtualConsole::test_reconfigure(int64_t position, 
	int64_t &length)
{
	int result = 0;
	Track *current_track;
	Module *module;
	int direction = renderengine->command->get_direction();


// Test playback status against virtual console for current position.
// Create a new PlayableTracks & compare the arrays
    PlayableTracks new_playable_tracks(renderengine->get_edl(), 
		position, 
		direction,
		data_type);
    if(new_playable_tracks.compare(playable_tracks)) result = 1;

//     int disable_lower = 0;
// 	for(current_track = renderengine->get_edl()->tracks->first;
// 		current_track && !result;
// 		current_track = current_track->next)
// 	{
// 		if(current_track->data_type == data_type)
// 		{
// // Playable status changed
// 			if(playable_tracks->is_playable(renderengine->get_edl(),
//                 current_track, 
// 				commonrender->current_position,
// 				direction /*,
// 				1 */) &&
//                 !disable_lower)
// 			{
// 				if(!playable_tracks->is_listed(current_track))
// 					result = 1;
// // disable all tracks below the 1st unmuted
//                 if(renderengine->get_edl()->session->disable_muted &&
//                     data_type == TRACK_VIDEO) disable_lower = 1;
// 			}
// 			else
// 			if(playable_tracks->is_listed(current_track))
// 			{
// 				result = 1;
// 			}
// 		}
// 	}

// Test plugins against virtual console at current position
	for(int i = 0; i < commonrender->total_modules && !result; i++)
		result = commonrender->modules[i]->test_plugins();


// Length of time until next plugin or edit change.

// It has to fragment the buffer if it's crossing between silence & an edit 
// or any plugin change.  It doesn't need to fragment if it's crossing 
// between 2 playable edits.  



// Now get the length of time until next reconfiguration.
// This part is not concerned with result.
// Don't clip input length if only rendering 1 frame.
	if(length == 1) return result;





// GCC 3.2 requires this or optimization error results.
//	int64_t longest_duration1;
	int64_t longest_duration2;
	int64_t longest_duration3;


// Length of time until next change between playable & unplayable.
// It doesn't need to fragment if it's crossing between 2 playable edits.  
	for(current_track = renderengine->get_edl()->tracks->first;
		current_track;
		current_track = current_track->next)
	{
		if(current_track->data_type == data_type)
		{
// // Test the transitions.  
// // Can't know the transition state because of random access effects.
// 			longest_duration1 = current_track->edit_change_duration(
// 				commonrender->current_position, 
// 				length, 
// 				direction == PLAY_REVERSE, 
// 				1,
// 				1);
// // printf("VirtualConsole::test_reconfigure %d current_track=%p current_position=%d length=%d longest_duration1=%d\n",
// // __LINE__,
// // current_track,
// // (int)commonrender->current_position,
// // (int)length,
// // (int)longest_duration1);


// Test the edits.  Playable status changes if it goes between silence & an edit.
			longest_duration2 = current_track->edit_change_duration(
				commonrender->current_position, 
				length, 
				direction, 
//				0,
				1);

// // printf("VirtualConsole::test_reconfigure %d current_track=%p current_position=%d length=%d longest_duration1=%d\n",
// // __LINE__,
// // current_track,
// // (int)commonrender->current_position,
// // (int)length,
// // (int)longest_duration1);

// Test the plugins.  Playable status changes if there's a plugin.
			longest_duration3 = current_track->plugin_change_duration(
				commonrender->current_position,
				length,
				direction == PLAY_REVERSE,
				1);

// printf("VirtualConsole::test_reconfigure %d current_position=%d length=%d duration1=%d duration2=%d duration3=%d\n",
// __LINE__,
// (int)commonrender->current_position,
// (int)length,
// (int)longest_duration1,
// (int)longest_duration2,
// (int)longest_duration3);

// 			if(longest_duration1 < length)
// 			{
// 				length = longest_duration1;
// 			}
			if(longest_duration2 < length)
			{
				length = longest_duration2;
			}
			if(longest_duration3 < length)
			{
				length = longest_duration3;
			}

		}
	}

	return result;
}





int VirtualConsole::delete_virtual_console()
{
// delete the virtual node tree
	for(int i = 0; i < total_exit_nodes; i++)
	{
		delete entry_nodes[i];
	}
// Seems to get allocated even if new[0].
	if(entry_nodes) delete [] entry_nodes;
	entry_nodes = 0;
	exit_nodes.remove_all();
    return 0;
}


