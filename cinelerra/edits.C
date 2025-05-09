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

#include "aedit.h"
#include "asset.h"
#include "assets.h"
#include "automation.h"
#include "bcsignals.h"
#include "cache.h"
#include "clip.h"
#include "edit.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "filexml.h"
#include "filesystem.h"
#include "localsession.h"
#include "mainsession.inc"
#include "mwindow.inc"
#include "nestededls.h"
#include "plugin.h"
#include "strategies.inc"
#include "track.h"
#include "transition.h"
#include "transportque.inc"

#include <string.h>

Edits::Edits(EDL *edl, Track *track)
 : List<Edit>()
{
	this->edl = edl;
	this->track = track;
}

Edits::~Edits()
{
}


void Edits::equivalent_output(Edits *edits, int64_t *result)
{
// For the case of plugin sets, a new plugin set may be created with
// plugins only starting after 0.  We only want to restart brender at
// the first plugin in this case.
	for(Edit *current = first, *that_current = edits->first; 
		current || that_current; 
		current = NEXT,
		that_current = that_current->next)
	{
//printf("Edits::equivalent_output 1 %d\n", *result);
		if(!current && that_current)
		{
			int64_t position1 = (last ? last->startproject + last->length : 0);
			int64_t position2 = that_current->startproject;
			if(*result < 0 || *result > MIN(position1, position2))
				*result = MIN(position1, position2);
			break;
		}
		else
		if(current && !that_current)
		{
			int64_t position1 = (edits->last ? edits->last->startproject + edits->last->length : 0);
			int64_t position2 = current->startproject;
			if(*result < 0 || *result > MIN(position1, position2))
				*result = MIN(position1, position2);
			break;
		}
		else
		{
//printf("Edits::equivalent_output 2 %d\n", *result);
			current->equivalent_output(that_current, result);
//printf("Edits::equivalent_output 3 %d\n", *result);
		}
	}
}

void Edits::copy_from(Edits *edits)
{
	while(last) delete last;
	for(Edit *current = edits->first; current; current = NEXT)
	{
		Edit *new_edit = append(create_edit());
		new_edit->copy_from(current);
	}
}


Edits& Edits::operator=(Edits& edits)
{
printf("Edits::operator= 1\n");
	copy_from(&edits);
	return *this;
}


void Edits::insert_asset(Asset *asset,
	EDL *nested_edl,
	int64_t length,
	int64_t position,
	int track_number)
{
	Edit *new_edit = insert_new_edit(position);

	new_edit->nested_edl = nested_edl;
	new_edit->asset = asset;
	new_edit->startsource = 0;
	new_edit->startproject = position;
	new_edit->length = length;

	if(nested_edl)
	{
		if(track->data_type == TRACK_AUDIO)
		{
        	new_edit->channel = track_number % nested_edl->session->audio_channels;
		}
        else
		{
        	new_edit->channel = 0;
        }
	}

	if(asset && !nested_edl)
	{
		if(asset->audio_data)
		{
        	new_edit->channel = track_number % asset->channels;
        }
		else
		if(asset->video_data)
		{
        	new_edit->channel = track_number % asset->layers;
        }
	}

//printf("Edits::insert_asset %d %d\n", new_edit->channel, new_edit->length);
	for(Edit *current = new_edit->next; current; current = NEXT)
	{
		current->startproject += length;
	}
}

void Edits::insert_edits(Edits *source_edits, 
	int64_t position,
	int64_t min_length,
	int edit_autos)
{
	int64_t clipboard_end = position + min_length;
// Length pasted so far
	int64_t source_len = 0;

// Fill region between end of edit table and beginning of pasted segment
// with silence.  Can't call from insert_new_edit because it's recursive.
	if(position > length())
	{
		paste_silence(length(), position);
	}


	for(Edit *source_edit = source_edits->first;
		source_edit;
		source_edit = source_edit->next)
	{
		EDL *dest_nested_edl = 0;
		if(source_edit->nested_edl)
		{
        	dest_nested_edl = edl->nested_edls->get_copy(source_edit->nested_edl);
        }

// Update Assets
		Asset *dest_asset = 0;
		if(source_edit->asset)
		{
        	dest_asset = edl->assets->update(source_edit->asset);
        }
// Open destination area
		Edit *dest_edit = insert_new_edit(position + source_edit->startproject);

		dest_edit->copy_from(source_edit);
		dest_edit->asset = dest_asset;
		dest_edit->nested_edl = dest_nested_edl;
		dest_edit->startproject = position + source_edit->startproject;



// Shift keyframes in source edit to their position in the
// destination edit for plugin case
		if(edit_autos) dest_edit->shift_keyframes(position);



// Shift following edits and keyframes in following edits by length
// in current source edit.
		for(Edit *future_edit = dest_edit->next;
			future_edit;
			future_edit = future_edit->next)
		{
			future_edit->startproject += dest_edit->length;
			future_edit->shift_keyframes(dest_edit->length);
		}
		
		source_len += source_edit->length;
	}




// Fill remaining clipboard length with silence
	if(source_len < min_length)
	{
//printf("Edits::insert_edits %d\n", __LINE__);
		paste_silence(position + source_len, position + min_length);
	}
}


// Native units
// Can't paste silence in here because it's used by paste_silence.
Edit* Edits::insert_new_edit(int64_t position)
{
	Edit *current = 0;
//printf("Edits::insert_new_edit 1\n");
	current = split_edit(position);
	if(current) current = PREVIOUS;

//printf("Edits::insert_new_edit 1\n");
	Edit *new_edit = create_edit();
//printf("Edits::insert_new_edit 1\n");
	insert_after(current, new_edit);
	new_edit->startproject = position;
//printf("Edits::insert_new_edit 2\n");
	return new_edit;
}


Edit* Edits::split_edit(int64_t position)
{
// Get edit containing position
	Edit *edit = editof(position, PLAY_FORWARD, 0);

// No edit found
	if(!edit)
	{
		return 0;
	}
// Split would have created a 0 length
//	if(edit->startproject == position) return edit;
// Create anyway so the return value comes before position

	Edit *new_edit = create_edit();
	insert_after(edit, new_edit);
	new_edit->copy_from(edit);
	new_edit->length = new_edit->startproject + new_edit->length - position;
	edit->length = position - edit->startproject;
	new_edit->startproject = edit->startproject + edit->length;
	new_edit->startsource += edit->length;


// Decide what to do with the transition
	if(edit->length && edit->transition)
	{
		delete new_edit->transition;
		new_edit->transition = 0;
	}

	if(edit->transition && edit->transition->length > edit->length) 
		edit->transition->length = edit->length;
	if(new_edit->transition && new_edit->transition->length > new_edit->length)
		new_edit->transition->length = new_edit->length;
	return new_edit;
}

int Edits::save(FileXML *xml, const char *output_path)
{
	copy(0, length(), xml, output_path);
	return 0;
}

void Edits::resample(double old_rate, double new_rate)
{
	for(Edit *current = first; current; current = NEXT)
	{
		current->startproject = Units::to_int64((double)current->startproject / 
			old_rate * 
			new_rate);
		if(PREVIOUS) PREVIOUS->length = current->startproject - PREVIOUS->startproject;
		current->startsource = Units::to_int64((double)current->startsource /
			old_rate *
			new_rate);
		if(!NEXT) current->length = Units::to_int64((double)current->length /
			old_rate *
			new_rate);
		if(current->transition)
		{
			current->transition->length = Units::to_int64(
				(double)current->transition->length /
				old_rate *
				new_rate);
		}
		current->resample(old_rate, new_rate);
	}
}













int Edits::optimize(int silence_only)
{
	int result = 1;
	Edit *current;
    if(silence_only < 0)
    {
#ifdef ENABLE_RAZOR
        silence_only = 1;
#else
        silence_only = 0;
#endif
    }

//printf("Edits::optimize %d\n", __LINE__);
// Sort edits by starting point
	while(result)
	{
		result = 0;
		
		for(current = first; current; current = NEXT)
		{
			Edit *next_edit = NEXT;
			
			if(next_edit && next_edit->startproject < current->startproject)
			{
				swap(next_edit, current);
				result = 1;
			}
		}
	}

// Insert silence between edits which aren't consecutive
	for(current = last; current; current = current->previous)
	{
		if(current->previous)
		{
			Edit *previous_edit = current->previous;
			if(current->startproject - 
				previous_edit->startproject -
				previous_edit->length > 0)
			{
				Edit *new_edit = create_edit();
				insert_before(current, new_edit);
				new_edit->startproject = previous_edit->startproject + previous_edit->length;
				new_edit->length = current->startproject - 
					previous_edit->startproject -
					previous_edit->length;
			}
		}
		else
		if(current->startproject > 0)
		{
			Edit *new_edit = create_edit();
			insert_before(current, new_edit);
			new_edit->length = current->startproject;
		}
	}

	result = 1;
	while(result)
	{
		result = 0;


// delete 0 length edits
		for(current = first; 
			current && !result; )
		{
			if(current->length == 0)
			{
				Edit* next = current->next;
				delete current;
				result = 1;
				current = next;
			}
			else
				current = current->next;
		}

//printf("Edits::optimize %d result=%d\n", __LINE__, result);
// merge same files or transitions
		for(current = first; 
			current && current->next && !result; )
		{
			Edit *next_edit = current->next;

// printf("Edits::optimize %d %lld=%lld %d=%d %p=%p %p=%p\n", 
// __LINE__,
// current->startsource + current->length,
// next_edit->startsource,
// current->channel,
// next_edit->channel,
// current->asset,
// next_edit->asset,
// current->nested_edl,
// next_edit->nested_edl);


            result = join(current, next_edit, silence_only);

    		current = current->next;
		}

// delete last edit of 0 length or silence
		if(last && 
			(last->silence() || 
			!last->length))
		{
			delete last;
			result = 1;
		}
	}

//track->dump();
	return 0;
}

int Edits::join(Edit *prev, Edit *next, int silence_only)
{
    if(!prev || !next) return 0;

	if(
// both edits are silence & not a plugin
		(prev->silence() && next->silence() && !prev->is_plugin) 
        ||
        (!silence_only &&
// different edits run into each other
			(prev->startsource + prev->length == next->startsource &&
// source channels are identical
	       	prev->channel == next->channel &&
// assets are identical
			prev->asset == next->asset && 
    		prev->nested_edl == next->nested_edl)
        ))
	{
//printf("Edits::join %d\n", __LINE__);
        prev->length += next->length;
        remove(next);
        return 1;
    }
    return 0;
}




















// ===================================== file operations

int Edits::load(FileXML *file, int track_offset)
{
	int result = 0;
	int64_t startproject = 0;
    int error = 0;
    Edit *current = first;

	do{
		result = file->read_tag();

		if(!result)
		{
			if(!strcmp(file->tag.get_title(), "EDIT"))
			{
				error |= load_edit(file, current, startproject, track_offset);
			}
			else
			if(!strcmp(file->tag.get_title(), "/EDITS"))
			{
				result = 1;
			}
		}
	}while(!result);

// delete leftovers
    while(current)
    {
        Edit *edit2 = NEXT;
        delete current;
        current = edit2;
    }

//track->dump();
	optimize();

    return error;
}

int Edits::load_edit(FileXML *file, 
    Edit* &current,
    int64_t &startproject, 
    int track_offset)
{
    int error = 0;

    if(!current)
    {
        current = append_new_edit();
    }

// reset the current edit to silence
    current->nested_edl = 0;
    current->asset = 0;

	current->load_properties(file, startproject);

    startproject += current->length;

	int result = 0;
    int got_transition = 0;

	do{
		result = file->read_tag();
        if(result) break;
		if(file->tag.title_is("NESTED_EDL"))
		{
			char path[BCTEXTLEN];
			path[0] = 0;
			file->tag.get_property("SRC", path);


			if(path[0] != 0)
			{
                current->nested_edl = edl->nested_edls->get(path, &error);
			}
// printf("Edits::load_edit %d path=%s nested_edl=%p\n", 
// __LINE__, 
// path,
// current->nested_edl);
		}
		else
		if(file->tag.title_is("FILE"))
		{
			char filename[BCTEXTLEN];
			filename[0] = 0;
			file->tag.get_property("SRC", filename);
// Extend path
			if(filename[0] != 0)
			{
				char directory[BCTEXTLEN], edl_directory[BCTEXTLEN];
				FileSystem fs;
				fs.set_current_dir("");
				fs.extract_dir(directory, filename);
				if(!strlen(directory))
				{
					fs.extract_dir(edl_directory, file->filename);
					fs.join_names(directory, edl_directory, filename);
					strcpy(filename, directory);
				}
				current->asset = edl->assets->get_asset(filename);
			}
			else
			{
				current->asset = 0;
			}
//printf("Edits::load_edit 5\n");
		}
		else
		if(file->tag.title_is("TRANSITION"))
		{
            if(!current->transition)
				current->transition = new Transition(edl,
					current, 
					"",
					track->to_units(edl->session->default_transition_length, 1));

            current->transition->load_xml(file);
            got_transition = 1;
		}
		else
		if(file->tag.title_is("/EDIT"))
		{
			result = 1;
		}
	}while(!result);

// delete leftovers
    if(!got_transition)
    {
        delete current->transition;
        current->transition = 0;
    }

    current = NEXT;
//printf("Edits::load_edit %d\n", __LINE__);
//track->dump();
//printf("Edits::load_edit %d\n", __LINE__);
	return error;
}

// ============================================= accounting

int64_t Edits::length()
{
	if(last) 
		return last->startproject + last->length;
	else 
		return 0;
}



Edit* Edits::editof(int64_t position, int direction, int use_nudge)
{
	Edit *current = 0;
	if(use_nudge && track) position += track->nudge;

	if(direction == PLAY_FORWARD)
	{
		for(current = last; current; current = PREVIOUS)
		{
			if(current->startproject <= position && current->startproject + current->length > position)
				return current;
		}
	}
	else
	if(direction == PLAY_REVERSE)
	{
		for(current = first; current; current = NEXT)
		{
			if(current->startproject < position && current->startproject + current->length >= position)
				return current;
		}
	}

	return 0;     // return 0 on failure
}

Edit* Edits::get_playable_edit(int64_t position, int use_nudge)
{
	Edit *current;
	if(track && use_nudge) position += track->nudge;

// Get the current edit
	for(current = first; current; current = NEXT)
	{
		if(current->startproject <= position && 
			current->startproject + current->length > position)
			break;
	}

// Get the edit's asset
// TODO: descend into nested EDLs
	if(current)
	{
		if(!current->asset)
			current = 0;
	}

	return current;     // return 0 on failure
}

// ================================================ editing



int Edits::copy(int64_t start, int64_t end, FileXML *file, const char *output_path)
{
	Edit *current_edit;

	file->tag.set_title("EDITS");
	file->append_tag();
	file->append_newline();

	for(current_edit = first; current_edit; current_edit = current_edit->next)
	{
		current_edit->copy(start, end, file, output_path);
	}

	file->tag.set_title("/EDITS");
	file->append_tag();
	file->append_newline();
    return 0; 
}



void Edits::clear(int64_t start, int64_t end)
{
	Edit* edit1 = editof(start, PLAY_FORWARD, 0);
	Edit* edit2 = editof(end, PLAY_FORWARD, 0);
	Edit* current_edit;

	if(end == start) return;        // nothing selected
	if(!edit1 && !edit2) return;       // nothing selected


// end point beyond end of track
	if(!edit2)
	{                
		edit2 = last;
		end = this->length();
	}

// starting point beyond start of track.  Unusual case caused only by edit info.
    if(!edit1)
    {
        edit1 = first;
        start = first->startproject;
    }

	if(edit1 != edit2)
	{
// in different edits

//printf("Edits::clear 3.5 %d %d %d %d\n", edit1->startproject, edit1->length, edit2->startproject, edit2->length);
		edit1->length = start - edit1->startproject;
		edit2->length -= end - edit2->startproject;
		edit2->startsource += end - edit2->startproject;
		edit2->startproject += end - edit2->startproject;

// delete
		for(current_edit = edit1->next; current_edit && current_edit != edit2;)
		{
			Edit *next = current_edit->next;
			remove(current_edit);
			current_edit = next;
		}
// shift
		for(current_edit = edit2; current_edit; current_edit = current_edit->next)
		{
			current_edit->startproject -= end - start;
		}
	}
	else
	{
// in same edit. paste_edit depends on this
// create a new edit
		current_edit = split_edit(start);

		current_edit->length -= end - start;
		current_edit->startsource += end - start;

// shift
		for(current_edit = current_edit->next; 
			current_edit; 
			current_edit = current_edit->next)
		{            
			current_edit->startproject -= end - start;
		}
	}

	optimize();
}

// Used by edit handle and plugin handle movement but plugin handle movement
// can only effect other plugins.
void Edits::clear_recursive(int64_t start, 
	int64_t end, 
	int edit_edits,
	int edit_labels, 
	int edit_plugins,
	int edit_autos,
	Edits *trim_edits)
{
//printf("Edits::clear_recursive 1\n");
	track->clear(start, 
		end, 
		edit_edits,
		edit_labels,
		edit_plugins,
		edit_autos,
		0,
		trim_edits);
// join any plugins that were split in this operation, 
// to preserve razor tool operations.
// TODO: It might be smarter to always do this for plugins in the optimize function.
// note: optimize specifically doesn't join contiguous plugins & razor doesn't
// split plugins
    if(edit_plugins)
    {
		for(int i = 0; i < track->plugin_set.size(); i++)
		{
            Edits *plugins = (Edits*)track->plugin_set.get(i);
			for(Edit *plugin = plugins->first; plugin; plugin = plugin->next)
            {
                Edit *next_plugin = plugin->next;
                if(next_plugin && 
                    plugin->startproject + plugin->length == start &&
                    next_plugin->startproject == start)
                {
                    plugin->length += next_plugin->length;
                    remove(next_plugin);
                    break;
                }
            }
		}
    }
}


int Edits::clear_handle(double start, 
	double end, 
	int edit_plugins, 
	int edit_autos,
	double &distance)
{
	Edit *current_edit;
    Edit *next_edit;
    

	distance = 0.0; // if nothing is found, distance is 0!
	for(current_edit = first; 
		current_edit && current_edit->next; 
		current_edit = current_edit->next)
	{
// test if the files are the same
        int equiv_file = 0;
        next_edit = current_edit->next;
        
        if(current_edit->asset && 
			next_edit->asset &&
            current_edit->asset->equivalent(*next_edit->asset, 0, 0))
        {
            equiv_file = 1;
        }
        else
        if(current_edit->nested_edl &&
            next_edit->nested_edl &&
            current_edit->nested_edl == next_edit->nested_edl)
        {
            equiv_file = 1;
        }

		if(equiv_file)
		{

// Got two consecutive edits in same source
			if(edl->equivalent(track->from_units(next_edit->startproject), 
				start))
			{
// handle selected
				int length = -current_edit->length;
				current_edit->length = next_edit->startsource - current_edit->startsource;
				length += current_edit->length;

// Lengthen automation
				if(edit_autos)
				{
                	track->automation->paste_silence(next_edit->startproject, 
						next_edit->startproject + length);
                }

// Lengthen effects
				if(edit_plugins)
				{
                	track->shift_effects(next_edit->startproject, 
						length,
						edit_autos);
                }

				for(Edit *current_edit2 = next_edit; current_edit2; current_edit2 = current_edit2->next)
				{
					current_edit2->startproject += length;
				}

				distance = track->from_units(length);

// have to merge the edits here if we're supporting razor mode
#ifdef ENABLE_RAZOR
//printf("Edits::clear_handle %d %p %p\n", __LINE__, current_edit, next_edit);

                current_edit->length += next_edit->length;
                remove(next_edit);
#endif

				optimize();
				break;
			}
		}
	}

	return 0;
}

int Edits::modify_handles(double oldposition, 
	double newposition, 
	int currentend,
	int edit_mode, 
	int edit_edits,
	int edit_labels,
	int edit_plugins,
	int edit_autos,
	Edits *trim_edits)
{
	int result = 0;
	Edit *current_edit;

//printf("Edits::modify_handles %d: %d %f %f\n", __LINE__, currentend, newposition, oldposition);
	if(currentend == LEFT_HANDLE)
	{
// left handle
		for(current_edit = first; current_edit && !result;)
		{
			if(edl->equivalent(track->from_units(current_edit->startproject), 
				oldposition))
			{
// edit matches selection
//printf("Edits::modify_handles %d: %f %f\n", __LINE__, newposition, oldposition);
				oldposition = track->from_units(current_edit->startproject);
				result = 1;

				if(newposition >= oldposition)
				{
//printf("Edits::modify_handle %d: %s %f %f\n", __LINE__, track->title, oldposition, newposition);
// shift start of edit in
					current_edit->shift_start_in(edit_mode, 
						track->to_units(newposition, 0), 
						track->to_units(oldposition, 0),
						edit_edits,
						edit_labels,
						edit_plugins,
						edit_autos,
						trim_edits);
				}
				else
				{
//printf("Edits::modify_handle %d: %s\n", __LINE__, track->title);
// move start of edit out
					current_edit->shift_start_out(edit_mode, 
						track->to_units(newposition, 0), 
						track->to_units(oldposition, 0),
						edit_edits,
						edit_labels,
						edit_plugins,
						edit_autos,
						trim_edits);
				}
			}

			if(!result) current_edit = current_edit->next;
		}
	}
	else
	{
// right handle selected
		for(current_edit = first; current_edit && !result;)
		{
			if(edl->equivalent(track->from_units(current_edit->startproject) + 
				track->from_units(current_edit->length), oldposition))
			{
            	oldposition = track->from_units(current_edit->startproject) + 
					track->from_units(current_edit->length);
				result = 1;

//printf("Edits::modify_handle 3\n");
				if(newposition <= oldposition)
				{     
// shift end of edit in
//printf("Edits::modify_handle 4\n");
					current_edit->shift_end_in(edit_mode, 
						track->to_units(newposition, 0), 
						track->to_units(oldposition, 0),
						edit_edits,
						edit_labels,
						edit_plugins,
						edit_autos,
						trim_edits);
//printf("Edits::modify_handle 5\n");
				}
				else
				{     
// move end of edit out
//printf("Edits::modify_handle %d edit_mode=%d\n", __LINE__, edit_mode);
					current_edit->shift_end_out(edit_mode, 
						track->to_units(newposition, 0), 
						track->to_units(oldposition, 0),
						edit_edits,
						edit_labels,
						edit_plugins,
						edit_autos,
						trim_edits);
//printf("Edits::modify_handle 7\n");
				}
			}

			if(!result) current_edit = current_edit->next;
//printf("Edits::modify_handle 8\n");
		}
	}

	optimize();
	return 0;
}


// Used by other editing commands so don't optimize
Edit* Edits::paste_silence(int64_t start, int64_t end)
{
	Edit *new_edit = insert_new_edit(start);
	new_edit->length = end - start;
	for(Edit *current = new_edit->next; current; current = NEXT)
	{
		current->startproject += end - start;
	}
	return new_edit;
}
				     
Edit* Edits::shift(int64_t position, int64_t difference)
{
	Edit *new_edit = split_edit(position);

	for(Edit *current = first; 
		current; 
		current = NEXT)
	{
		if(current->startproject >= position)
		{
			current->shift(difference);
		}
	}
	return new_edit;
}


void Edits::shift_keyframes_recursive(int64_t position, int64_t length)
{
	track->shift_keyframes(position, length);
}

void Edits::shift_effects_recursive(int64_t position, int64_t length, int edit_autos)
{
	track->shift_effects(position, length, edit_autos);
}

// only used for audio but also used for plugins which inherit from Edits
void Edits::deglitch(int64_t position)
{
// range from the splice junk appears
	int64_t threshold = (int64_t)((double)edl->session->sample_rate / 
		edl->session->frame_rate) / 2;
	Edit *current = 0;

// the last edit before the splice
	Edit *edit1 = 0;
	if(first)
	{
		for(current = first; current; current = NEXT)
		{
			if(current->startproject + current->length >= position - threshold)
			{
				edit1 = current;
				break;
			}
		}

// ignore if it ends after the splice
		if(current && current->startproject + current->length >= position)
		{
			edit1 = 0;
		}
	}

// the first edit after the splice
	Edit *edit2 = 0;
	if(last)
	{
		for(current = last; current; current = PREVIOUS)
		{
			if(current->startproject < position + threshold)
			{
				edit2 = current;
				break;
			}
		}

	// ignore if it starts before the splice
		if(current && current->startproject < position)
		{
			edit2 = 0;
		}
	}




// printf("Edits::deglitch %d position=%ld edit1=%p edit2=%p\n", __LINE__,
// position, 
// edit1, 
// edit2);
// delete junk between the edits
	if(edit1 != edit2)
	{
		if(edit1 != 0)
		{
// end the starting edit later
			current = edit1->next;
			while(current != 0 &&
				current != edit2 &&
				current->startproject < position)
			{
				Edit* next = NEXT;

				edit1->length += current->length;
				remove(current);

				current = next;
			}
		}
		
		if(edit2 != 0)
		{
// start the ending edit earlier
			current = edit2->previous;
			while(current != 0 && 
				current != edit1 &&
				current->startproject >= position)
			{
				Edit *previous = PREVIOUS;

				int64_t length = current->length;
//printf("Edits::deglitch %d length=%ld\n", __LINE__, length);
				if(!edit2->silence() && 
					length > edit2->startsource)
				{
					length = edit2->startsource;
				}

				// shift edit2 by using material from its source
				edit2->startproject -= length;
				edit2->startsource -= length;
				// assume enough is at the end
				edit2->length += length;

				// shift edit2 & its source earlier by remainder
				if(length < current->length)
				{
					int64_t remainder = current->length - length;
					edit2->startproject -= remainder;
					// assume enough is at the end
					edit2->length += remainder;
				}

				remove(current);


				current = previous;
			}
		}
	}
	
}



