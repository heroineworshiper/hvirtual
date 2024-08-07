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
#include "assets.h"
#include "bcsignals.h"
#include "clip.h"
#include "edit.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "filesystem.h"
#include "keyframe.h"
#include "localsession.h"
#include "plugin.h"
#include "mainsession.h"
#include "nestededls.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "transition.h"
#include <string.h>


Edit::Edit()
{
	reset();
}

Edit::Edit(EDL *edl, Track *track)
{
	reset();
	this->edl = edl;
	this->track = track;
	if(track) this->edits = track->edits;
	id = EDL::next_id();
}

Edit::Edit(EDL *edl, Edits *edits)
{
	reset();
	this->edl = edl;
	this->edits = edits;
	if(edits) this->track = edits->track;
	id = EDL::next_id();
}

Edit::~Edit()
{
//printf("Edit::~Edit 1\n");
	if(transition) delete transition;
//printf("Edit::~Edit 2\n");
}

void Edit::reset()
{
	edl = 0;
	track = 0;
	edits = 0;
	startsource = 0;  
	startproject = 0;	 
	length = 0;  
	asset = 0;
	transition = 0;
	channel = 0;
	user_title[0] = 0;
	nested_edl = 0;
	is_plugin = 0;
}

Indexable* Edit::get_source()
{
	if(asset) return asset;
	if(nested_edl) return nested_edl;
	return 0;
}

int Edit::copy(int64_t start, 
	int64_t end, 
	FileXML *file, 
	const char *output_path)
{
// variables
//printf("Edit::copy 1\n");

	int64_t endproject = startproject + length;
	int result;

	if((startproject >= start && startproject <= end) ||  // startproject in range
		 (endproject <= end && endproject >= start) ||	   // endproject in range
		 (startproject <= start && endproject >= end))    // range in project
	{   
// edit is in range
		int64_t startproject_in_selection = startproject; // start of edit in selection in project
		int64_t startsource_in_selection = startsource; // start of source in selection in source
		int64_t endsource_in_selection = startsource + length; // end of source in selection
		int64_t length_in_selection = length;             // length of edit in selection
//printf("Edit::copy 2\n");

		if(startproject < start)
		{         // start is after start of edit in project
			int64_t length_difference = start - startproject;

			startsource_in_selection += length_difference;
			startproject_in_selection += length_difference;
			length_in_selection -= length_difference;
		}
//printf("Edit::copy 3\n");

		if(endproject > end)
		{         // end is before end of edit in project
			length_in_selection = end - startproject_in_selection;
		}
		
//printf("Edit::copy 4\n");
		if(file)    // only if not counting
		{
			file->tag.set_title("EDIT");
			file->tag.set_property("STARTSOURCE", startsource_in_selection);
			file->tag.set_property("CHANNEL", (int64_t)channel);
			file->tag.set_property("LENGTH", length_in_selection);
			if(user_title[0]) file->tag.set_property("USER_TITLE", user_title);
//printf("Edit::copy 5\n");

			copy_properties_derived(file, length_in_selection);

			file->append_tag();
//			file->append_newline();
//printf("Edit::copy 6\n");

			if(nested_edl)
			{
				file->tag.set_title("NESTED_EDL");
				file->tag.set_property("SRC", nested_edl->path);
				file->append_tag();
			}

			if(asset)
			{
//printf("Edit::copy 6 %s\n", asset->path);
				char stored_path[BCTEXTLEN];
				char asset_directory[BCTEXTLEN];
				char output_directory[BCTEXTLEN];
				FileSystem fs;

//printf("Edit::copy %d %s\n", __LINE__, asset->path);
				fs.extract_dir(asset_directory, asset->path);
//printf("Edit::copy %d %s\n", __LINE__, asset->path);

				if(output_path)
					fs.extract_dir(output_directory, output_path);
				else
					output_directory[0] = 0;
//printf("Edit::copy %s, %s %s, %s\n", asset->path, asset_directory, output_path, output_directory);

				if(output_path && !strcmp(asset_directory, output_directory))
					fs.extract_name(stored_path, asset->path);
				else
					strcpy(stored_path, asset->path);

				file->tag.set_title("FILE");
				file->tag.set_property("SRC", stored_path);
				file->append_tag();
			}

			if(transition)
			{
				transition->save_xml(file);
			}

//printf("Edit::copy 7\n");
			file->tag.set_title("/EDIT");
			file->append_tag();
			file->append_newline();	
//printf("Edit::copy 8\n");
		}
//printf("Edit::copy 9\n");
		result = 1;
	}
	else
	{
		result = 0;
	}
//printf("Edit::copy 10\n");
	return result;
}


int64_t Edit::get_source_end(int64_t default_)
{
	return default_;
}

void Edit::insert_transition(char *title, KeyFrame *keyframe)
{
//printf("Edit::insert_transition this=%p title=%p title=%s\n", this, title, title);
// preserve the length of an existing transition
    int64_t length = track->to_units(edl->session->default_transition_length, 1);
    if(transition) length = transition->length;
	detach_transition();
	transition = new Transition(edl, 
		this, 
		title, 
		length);
// replace the keyframe
    transition->get_keyframe()->copy_from(keyframe);
}

void Edit::detach_transition()
{
	if(transition) delete transition;
	transition = 0;
}

int Edit::silence()
{
	if(asset || nested_edl)
	{
    	return 0;
	}
    else
	{
    	return 1;
    }
}


void Edit::copy_from(Edit *edit)
{
	this->nested_edl = edl->nested_edls->get_copy(edit->nested_edl);
	this->asset = edl->assets->update(edit->asset);
	this->startsource = edit->startsource;
	this->startproject = edit->startproject;
	this->length = edit->length;
	strcpy (this->user_title, edit->user_title);

	if(edit->transition)
	{
		if(!transition) transition = new Transition(edl, 
			this, 
			edit->transition->title,
			edit->transition->length);
		*this->transition = *edit->transition;
	}
	this->channel = edit->channel;
}

void Edit::equivalent_output(Edit *edit, int64_t *result)
{
// End of edit changed
	if(startproject + length != edit->startproject + edit->length)
	{
		int64_t new_length = MIN(startproject + length, 
			edit->startproject + edit->length);
		if(*result < 0 || new_length < *result) 
			*result = new_length;
	}

	if(
// Different nested EDLs
		edit->nested_edl && !nested_edl ||
		!edit->nested_edl && nested_edl ||
// Different assets
		edit->asset == 0 && asset != 0 ||
		edit->asset != 0 && asset == 0 ||
// different transitions
		edit->transition == 0 && transition != 0 ||
		edit->transition != 0 && transition == 0 ||
// Position changed
		startproject != edit->startproject ||
		startsource != edit->startsource ||
// Transition changed
		(transition && edit->transition && 
			!transition->identical(edit->transition)) ||
// Asset changed
		(asset && edit->asset &&
			!asset->equivalent(*edit->asset, 1, 1)) ||
// Nested EDL changed
		(nested_edl && edit->nested_edl &&
			strcmp(nested_edl->path, edit->nested_edl->path))
		)
	{
// Start of edit changed
		if(*result < 0 || startproject < *result) *result = startproject;
	}
}


Edit& Edit::operator=(Edit& edit)
{
//printf("Edit::operator= called\n");
	copy_from(&edit);
	return *this;
}

void Edit::synchronize_params(Edit *edit)
{
	copy_from(edit);
}


// Comparison for ResourcePixmap drawing
int Edit::identical(Edit &edit)
{
	int result = (this->nested_edl == edit.nested_edl &&
		this->asset == edit.asset &&
		this->startsource == edit.startsource &&
		this->startproject == edit.startproject &&
		this->length == edit.length &&
		this->transition == edit.transition &&
		this->channel == edit.channel);
	return result;
}

int Edit::operator==(Edit &edit)
{
	return identical(edit);
}

double Edit::frames_per_picon()
{
	return Units::round(picon_w()) / frame_w();
}

double Edit::frame_w()
{
	return track->from_units(1) * 
		edl->session->sample_rate / 
		edl->local_session->zoom_sample;
}

double Edit::picon_w()
{
	if(asset)
		return (double)edl->local_session->zoom_track * 
			asset->width / 
			asset->height;
	if(nested_edl)
		return (double)edl->local_session->zoom_track * 
			nested_edl->session->output_w / 
			nested_edl->session->output_h;

	return 0;
}

int Edit::picon_h()
{
	return edl->local_session->zoom_track;
}


int Edit::dump()
{
	printf("     EDIT %p\n", this); fflush(stdout);
	printf("      nested_edl=%p %s asset=%p %s\n", 
		nested_edl, 
		nested_edl ? nested_edl->path : "", 
		asset,
		asset ? asset->path : "");
	fflush(stdout);
	printf("      channel %d\n", channel);
	if(transition) 
	{
		printf("      TRANSITION %p\n", transition);
		transition->dump();
	}
	printf("      startsource %lld startproject %lld length %lld\n", (long long)startsource, (long long)startproject, (long long)length); 
	fflush(stdout);
	return 0;
}

int Edit::load_properties(FileXML *file, int64_t &startproject)
{
	startsource = file->tag.get_property("STARTSOURCE", (int64_t)0);
	length = file->tag.get_property("LENGTH", (int64_t)0);
	user_title[0] = 0;
	file->tag.get_property("USER_TITLE", user_title);
	this->startproject = startproject;
	load_properties_derived(file);
	return 0;
}

void Edit::shift(int64_t difference)
{
//printf("Edit::shift 1 %p %lld %lld\n", this, startproject, difference);
	startproject += difference;
//printf("Edit::shift 2 %lld %lld\n", startproject, difference);
}

int Edit::shift_start_in(int edit_mode, 
	int64_t newposition, 
	int64_t oldposition,
	int edit_edits,
	int edit_labels,
	int edit_plugins,
	int edit_autos,
	Edits *trim_edits)
{
	int64_t cut_length = newposition - oldposition;
	int64_t end_previous_source, end_source;

	if(edit_mode == MOVE_ALL_EDITS)
	{
		if(cut_length < length)
		{        // clear partial 
			edits->clear_recursive(oldposition, 
				newposition,
				edit_edits,
				edit_labels,
				edit_plugins,
				edit_autos,
				trim_edits);
		}
		else
		{        // clear entire
			edits->clear_recursive(oldposition, 
				startproject + length,
				edit_edits,
				edit_labels,
				edit_plugins,
				edit_autos,
				trim_edits);
		}
	}
	else
	if(edit_mode == MOVE_ONE_EDIT)
	{
// Paste silence and cut
//printf("Edit::shift_start_in 1\n");
		if(!previous)
		{
			Edit *new_edit = edits->create_edit();
			new_edit->startproject = this->startproject;
			new_edit->length = 0;
			edits->insert_before(this, 
				new_edit);
		}
//printf("Edit::shift_start_in 2 %p\n", previous);

		end_previous_source = previous->get_source_end(previous->startsource + previous->length + cut_length);
		if(end_previous_source > 0 && 
			previous->startsource + previous->length + cut_length > end_previous_source)
			cut_length = end_previous_source - previous->startsource - previous->length;

		if(cut_length < length)
		{		// Move in partial
			startproject += cut_length;
			startsource += cut_length;
			length -= cut_length;
			previous->length += cut_length;
//printf("Edit::shift_start_in 2\n");
		}
		else
		{		// Clear entire edit
			cut_length = length;
			previous->length += cut_length;
			for(Edit* current_edit = this; current_edit; current_edit = current_edit->next)
			{
				current_edit->startproject += cut_length;
			}
			edits->clear_recursive(oldposition + cut_length, 
				startproject + cut_length,
				edit_edits,
				edit_labels,
				edit_plugins,
				edit_autos,
				trim_edits);
		}
//printf("Edit::shift_start_in 3\n");
	}
	else
	if(edit_mode == MOVE_NO_EDITS)
	{
		end_source = get_source_end(startsource + length + cut_length);
		if(end_source > 0 && startsource + length + cut_length > end_source)
			cut_length = end_source - startsource - length;
		
		startsource += cut_length;
	}
	return 0;
}

int Edit::shift_start_out(int edit_mode, 
	int64_t newposition, 
	int64_t oldposition,
	int edit_edits,
	int edit_labels,
	int edit_plugins,
	int edit_autos,
	Edits *trim_edits)
{
	int64_t cut_length = oldposition - newposition;

//printf("Edit::shift_start_out %d: edit_mode=%d startsource=%ld cut_length=%ld\n", 
//__LINE__, edit_mode, startsource, cut_length);

	if(asset || nested_edl)
	{
		int64_t end_source = get_source_end(1);

//printf("Edit::shift_start_out 1 %lld %lld\n", startsource, cut_length);
		if(end_source > 0 && startsource < cut_length)
		{
			cut_length = startsource;
		}
	}

	if(edit_mode == MOVE_ALL_EDITS)
	{
//printf("Edit::shift_start_out %d: %lld\n", __LINE__, cut_length);
		startsource -= cut_length;
		length += cut_length;

		if(edit_autos)
			edits->shift_keyframes_recursive(startproject, 
				cut_length);
		if(edit_plugins)
			edits->shift_effects_recursive(startproject, 
				cut_length,
				edit_autos);

		for(Edit* current_edit = next; current_edit; current_edit = current_edit->next)
		{
			current_edit->startproject += cut_length;
		}
	}
	else
	if(edit_mode == MOVE_ONE_EDIT)
	{
//printf("Edit::shift_start_out %d: previous=%p\n", __LINE__, previous);
		if(previous)
		{
			if(cut_length < previous->length)
			{   // Cut into previous edit
				previous->length -= cut_length;
				startproject -= cut_length;
				startsource -= cut_length;
				length += cut_length;
//printf("Edit::shift_start_out 2\n");
			}
			else
			{   // Clear entire previous edit
				cut_length = previous->length;
				previous->length = 0;
				length += cut_length;
				startsource -= cut_length;
				startproject -= cut_length;
			}
		}
	}
	else
	if(edit_mode == MOVE_NO_EDITS)
	{
//printf("Edit::shift_start_out %d: cut_length=%ld startsource=%ld\n",
//__LINE__,
//cut_length,
//startsource);
		startsource -= cut_length;
	}

// Fix infinite length files
	if(startsource < 0) startsource = 0;
	return 0;
}

int Edit::shift_end_in(int edit_mode, 
	int64_t newposition, 
	int64_t oldposition,
	int edit_edits,
	int edit_labels,
	int edit_plugins,
	int edit_autos,
	Edits *trim_edits)
{
	int64_t cut_length = oldposition - newposition;

	if(edit_mode == MOVE_ALL_EDITS)
	{
//printf("Edit::shift_end_in 1\n");
		if(newposition > startproject)
		{        // clear partial edit
//printf("Edit::shift_end_in %p %p\n", track->edits, edits);
			edits->clear_recursive(newposition, 
				oldposition,
				edit_edits,
				edit_labels,
				edit_plugins,
				edit_autos,
				trim_edits);
		}
		else
		{        // clear entire edit
			edits->clear_recursive(startproject, 
				oldposition,
				edit_edits,
				edit_labels,
				edit_plugins,
				edit_autos,
				trim_edits);
		}
	}
	else
	if(edit_mode == MOVE_ONE_EDIT)
	{
		if(next)
		{
			if(next->asset)
			{
				int64_t end_source = next->get_source_end(1);

				if(end_source > 0 && next->startsource - cut_length < 0)
				{
					cut_length = next->startsource;
				}
			}

			if(cut_length < length)
			{
				length -= cut_length;
				next->startproject -= cut_length;
				next->startsource -= cut_length;
				next->length += cut_length;
//printf("Edit::shift_end_in 2 %d\n", cut_length);
			}
			else
			{
				cut_length = length;
				next->length += cut_length;
				next->startsource -= cut_length;
				next->startproject -= cut_length;
				length -= cut_length;
			}
		}
		else
		{
			if(cut_length < length)
			{
				length -= cut_length;
			}
			else
			{
				cut_length = length;
				edits->clear_recursive(startproject, 
					oldposition,
					edit_edits,
					edit_labels,
					edit_plugins,
					edit_autos,
					trim_edits);
			}
		}
	}
	else
// Does nothing for plugins
	if(edit_mode == MOVE_NO_EDITS)
	{
//printf("Edit::shift_end_in 3\n");
		int64_t end_source = get_source_end(1);
		if(end_source > 0 && startsource < cut_length)
		{
			cut_length = startsource;
		}
		startsource -= cut_length;
	}
	return 0;
}

int Edit::shift_end_out(int edit_mode, 
	int64_t newposition, 
	int64_t oldposition,
	int edit_edits,
	int edit_labels,
	int edit_plugins,
	int edit_autos,
	Edits *trim_edits)
{
	int64_t cut_length = newposition - oldposition;
	int64_t endsource = get_source_end(startsource + length + cut_length);

// check end of edit against end of source file
	if(endsource > 0 && startsource + length + cut_length > endsource)
		cut_length = endsource - startsource - length;

//printf("Edit::shift_end_out 1 %lld %d %d %d\n", oldposition, newposition, this->length, cut_length);
	if(edit_mode == MOVE_ALL_EDITS)
	{
// Extend length
		this->length += cut_length;

// Effects are shifted in length extension
		if(edit_plugins)
			edits->shift_effects_recursive(oldposition /* startproject */, 
				cut_length,
				edit_autos);
		if(edit_autos)
			edits->shift_keyframes_recursive(oldposition /* startproject */, 
				cut_length);

		for(Edit* current_edit = next; current_edit; current_edit = current_edit->next)
		{
			current_edit->startproject += cut_length;
		}
	}
	else
	if(edit_mode == MOVE_ONE_EDIT)
	{
		if(next)
		{
			if(cut_length < next->length)
			{
				length += cut_length;
				next->startproject += cut_length;
				next->startsource += cut_length;
				next->length -= cut_length;
//printf("Edit::shift_end_out %d cut_length=%d\n", __LINE__, cut_length);
			}
			else
			{
//printf("Edit::shift_end_out %d cut_length=%d next->length=%d\n", __LINE__, cut_length, next->length);
				cut_length = next->length;
				next->startproject += next->length;
				next->startsource += next->length;
				next->length = 0;
				length += cut_length;
//track->dump();
			}
		}
		else
		{
			length += cut_length;
		}
	}
	else
	if(edit_mode == MOVE_NO_EDITS)
	{
		startsource += cut_length;
	}
	return 0;
}





























int Edit::popup_transition(float view_start, float zoom_units, int cursor_x, int cursor_y)
{
	int64_t left, right, left_unit, right_unit;
	if(!transition) return 0;
	get_handle_parameters(left, right, left_unit, right_unit, view_start, zoom_units);

	if(cursor_x > left && cursor_x < right)
	{
//		transition->popup_transition(cursor_x, cursor_y);
		return 1;
	}
	return 0;
}

int Edit::select_handle(float view_start, float zoom_units, int cursor_x, int cursor_y, int64_t &selection)
{
	int64_t left, right, left_unit, right_unit;
	get_handle_parameters(left, right, left_unit, right_unit, view_start, zoom_units);

	int64_t pixel1, pixel2;
	pixel1 = left;
	pixel2 = pixel1 + 10;

// test left edit
// cursor_x is faked in acanvas
	if(cursor_x >= pixel1 && cursor_x <= pixel2)
	{
		selection = left_unit;
		return 1;     // left handle
	}

	int64_t endproject = startproject + length;
	pixel2 = right;
	pixel1 = pixel2 - 10;

// test right edit	
	if(cursor_x >= pixel1 && cursor_x <= pixel2)
	{
		selection = right_unit;
		return 2;     // right handle
	}
	return 0;
}

