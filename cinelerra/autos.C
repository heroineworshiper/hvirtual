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

#include "automation.h"
#include "autos.h"
#include "clip.h"
#include "edl.h"
#include "edlsession.h"
#include "localsession.h"
#include "filexml.h"
#include "track.h"
#include "transportque.inc"

#include <string.h>


Autos::Autos(EDL *edl, 
        Track *track, 
        int type, // data type below
        int overlay_type) // overlay type from automation.inc
 : List<Auto>()
{
	this->edl = edl;
	this->track = track;
	this->type = type;
    this->overlay_type = overlay_type;
}



Autos::~Autos()
{
	while(last) delete last;
	delete default_auto;
}

void Autos::create_objects()
{
// Default
	default_auto = new_auto();
	default_auto->is_default = 1;
}

int Autos::get_type()
{
	return type;
}

Auto* Autos::append_auto()
{
	return append(new_auto());
}


Auto* Autos::new_auto()
{
	return new Auto(edl, this);
}

void Autos::resample(double old_rate, double new_rate)
{
	for(Auto *current = first; current; current = NEXT)
	{
		current->position = (int64_t)((double)current->position * 
			new_rate / 
			old_rate + 
			0.5);
	}
}

void Autos::equivalent_output(Autos *autos, int64_t startproject, int64_t *result)
{
// Default keyframe differs
	if(!total() && !(*default_auto == *autos->default_auto))
	{
		if(*result < 0 || *result > startproject) *result = startproject;
	}
	else
// Search for difference
	{
		for(Auto *current = first, *that_current = autos->first; 
			current || that_current; 
			current = NEXT,
			that_current = that_current->next)
		{
// Total differs
			if(current && !that_current)
			{
				int64_t position1 = (autos->last ? autos->last->position : startproject);
				int64_t position2 = current->position;
				if(*result < 0 || *result > MIN(position1, position2))
					*result = MIN(position1, position2);
				break;
			}
			else
			if(!current && that_current)
			{
				int64_t position1 = (last ? last->position : startproject);
				int64_t position2 = that_current->position;
				if(*result < 0 || *result > MIN(position1, position2))
					*result = MIN(position1, position2);
				break;
			}
			else
// Keyframes differ
			if(!(*current == *that_current) || 
				current->position != that_current->position)
			{
				int64_t position1 = (current->previous ? 
					current->previous->position : 
					startproject);
				int64_t position2 = (that_current->previous ? 
					that_current->previous->position : 
					startproject);
				if(*result < 0 || *result > MIN(position1, position2))
					*result = MIN(position1, position2);
				break;
			}
		}
	}
}

void Autos::copy_from(Autos *autos)
{
	Auto *current = autos->first, *this_current = first;

	default_auto->copy_from(autos->default_auto);

// Detect common memory leak bug
	if(autos->first && !autos->last)
	{
		printf("Autos::copy_from inconsistent pointers\n");
		exit(1);
	}

	for(current = autos->first; current; current = NEXT)
	{
//printf("Autos::copy_from 1 %p\n", current);
//sleep(1);
		if(!this_current)
		{
			append(this_current = new_auto());
		}
		this_current->copy_from(current);
		this_current = this_current->next;
	}

	for( ; this_current; )
	{
		Auto *next_current = this_current->next;
		delete this_current;
		this_current = next_current;
	}
}


// We don't replace it in pasting but
// when inserting the first EDL of a load operation we need to replace
// the default keyframe.
void Autos::insert_track(Autos *automation, 
	int64_t start_unit, 
	int64_t length_units,
	int replace_default)
{
// Insert silence
	insert(start_unit, start_unit + length_units);

	if(replace_default) default_auto->copy_from(automation->default_auto);
	for(Auto *current = automation->first; current; current = NEXT)
	{
		Auto *new_auto = insert_auto(start_unit + current->position);
		new_auto->copy_from(current);
// Override copy_from
		new_auto->position = current->position + start_unit;
	}
}

Auto* Autos::get_prev_auto(int64_t position, 
	int direction, 
	Auto* &current, 
	int use_default)
{
// Get on or before position
	if(direction == PLAY_FORWARD)
	{
// Try existing result
		if(current)
		{
			while(current && current->position < position) current = NEXT;
			while(current && current->position > position) current = PREVIOUS;
		}

		if(!current)
		{
			for(current = last; 
				current && current->position > position; 
				current = PREVIOUS) ;
		}
		if(!current && use_default) current = (first ? first : default_auto);
	}
	else
// Get on or after position
	if(direction == PLAY_REVERSE)
	{
		if(current)
		{
			while(current && current->position > position) current = PREVIOUS;
			while(current && current->position < position) current = NEXT;
		}

		if(!current)
		{
			for(current = first; 
				current && current->position < position; 
				current = NEXT) ;
		}

		if(!current && use_default) current = (last ? last : default_auto);
	}

	return current;
}

Auto* Autos::get_prev_auto(int direction, Auto* &current)
{
	double position_double = edl->local_session->get_selectionstart(1);
	position_double = edl->align_to_frame(position_double, 0);
	int64_t position = track->to_units(position_double, 0);

	return get_prev_auto(position, direction, current);

	return current;
}

int Autos::auto_exists_for_editing(double position)
{
	int result = 0;
	
	if(edl->session->auto_keyframes)
	{
		double unit_position = position;
		unit_position = edl->align_to_frame(unit_position, 0);
		if (get_auto_at_position(unit_position))
			result = 1;
	}
	else
	{
		result = 1;
	}

	return result;
}

Auto* Autos::get_auto_at_position(double position)
{
	int64_t unit_position = track->to_units(position, 0);

	for(Auto *current = first; 
		current; 
		current = NEXT)
	{
		if(edl->equivalent(current->position, unit_position))
		{
			return current;
		}
	}
	return 0;
}


Auto* Autos::get_auto_for_editing(double position)
{
	if(position < 0)
	{
		position = edl->local_session->get_selectionstart(1);
	}

	Auto *result = 0;
	position = edl->align_to_frame(position, 0);




//printf("Autos::get_auto_for_editing %p %p\n", first, default_auto);

	if(edl->session->auto_keyframes)
	{
		result = insert_auto(track->to_units(position, 0));
	}
	else
		result = get_prev_auto(track->to_units(position, 0), 
			PLAY_FORWARD, 
			result);

//printf("Autos::get_auto_for_editing %p %p %p\n", default_auto, first, result);
	return result;
}


Auto* Autos::get_next_auto(int64_t position, int direction, Auto* &current, int use_default)
{
	if(direction == PLAY_FORWARD)
	{
		if(current)
		{
			while(current && current->position > position) current = PREVIOUS;
			while(current && current->position < position) current = NEXT;
		}

		if(!current)
		{
			for(current = first;
				current && current->position <= position;
				current = NEXT)
				;
		}

		if(!current && use_default) current = (last ? last : default_auto);
	}
	else
	if(direction == PLAY_REVERSE)
	{
		if(current)
		{
			while(current && current->position < position) current = NEXT;
			while(current && current->position > position) current = PREVIOUS;
		}

		if(!current)
		{
			for(current = last;
				current && current->position > position;
				current = PREVIOUS)
				;
		}

		if(!current && use_default) current = (first ? first : default_auto);
	}

	return current;
}

Auto* Autos::insert_auto(int64_t position)
{
	Auto *current, *result;

// Test for existence
	for(current = first; 
		current && !edl->equivalent(current->position, position); 
		current = NEXT)
	{
		;
	}

// Insert new
	if(!current)
	{
// Get first one on or before as a template
		for(current = last; 
			current && current->position > position; 
			current = PREVIOUS)
		{
			;
		}

		if(current)
		{
			insert_after(current, result = new_auto());
			result->copy_from(current);
		}
		else
		{
			current = first;
			if(!current) current = default_auto;

			insert_before(first, result = new_auto());
			if(current) result->copy_from(current);
		}

		result->position = position;
// Set curve type
		result->mode = edl->local_session->floatauto_type;
	}
	else
	{
		result = current;
	}

	return result;
}

int Autos::clear_all()
{
	Auto *current_, *current;
	
	for(current = first; current; current = current_)
	{
		current_ = NEXT;
		remove(current);
	}
	append_auto();
	return 0;
}

int Autos::insert(int64_t start, int64_t end)
{
	int64_t length;
	Auto *current = first;

	for( ; current && current->position < start; current = NEXT)
		;

	length = end - start;

	for(; current; current = NEXT)
	{
		current->position += length;
	}
	return 0;
}

void Autos::paste(int64_t start, 
	int64_t length, 
	double scale, 
	FileXML *file, 
	int default_only,
	int active_only)
{
	int total = 0;
	int result = 0;

//printf("Autos::paste %d start=%lld\n", __LINE__, start);
	do{
		result = file->read_tag();

		if(!result)
		{
// End of list
			if(file->tag.get_title()[0] == '/')
			{
				result = 1;
			}
			else
			if(!strcmp(file->tag.get_title(), "AUTO"))
			{
				Auto *current = 0;

// Paste first auto into default				
				if(default_only && total == 0)
				{
					current = default_auto;
				}
				else
// Paste default auto into default
				if(!default_only)
				{
					int64_t position = Units::to_int64(
						(double)file->tag.get_property("POSITION", 0) *
							scale + 
							start);
// Paste active auto into track
					current = insert_auto(position);
				}

				if(current)
				{
					current->load(file);
				}
				total++;
			}
		}
	}while(!result);
	
}


int Autos::paste_silence(int64_t start, int64_t end)
{
	insert(start, end);
	return 0;
}

int Autos::copy(int64_t start, 
	int64_t end, 
	FileXML *file, 
	int default_only,
	int active_only)
{
// First auto always loaded with default
//printf("Autos::copy %d %d %d\n", __LINE__, default_only, active_only);
	if(default_only || (!active_only && !default_only))
	{
		default_auto->copy(0, 0, file, default_only);
	}

//printf("Autos::copy 10 %d %d %p\n", default_only, start, autoof(start));
	if(active_only || (!default_only && !active_only))
	{
		for(Auto* current = autoof(start); 
			current && current->position <= end; 
			current = NEXT)
		{
// Want to copy single keyframes by putting the cursor on them
			if(current->position >= start && current->position <= end)
			{
				current->copy(start, end, file, default_only);
			}
		}
	}
// Copy default auto again to make it the active auto on the clipboard
//	else
//	{
// Need to force position to 0 for the case of plugins
// and default status to 0.
//		default_auto->copy(0, 0, file, default_only);
//	}
//printf("Autos::copy 20\n");

	return 0;
}

// Remove 3 consecutive autos with the same value
// Remove autos which are out of order
void Autos::optimize()
{
	int done = 0;


// Default auto should always be at 0
	default_auto->position = 0;
	while(!done)
	{
		int consecutive = 0;
		done = 1;
		
		
		for(Auto *current = first; current; current = NEXT)
		{
// Get 3rd consecutive auto of equal value
			if(current != first)
			{
				if(*current == *PREVIOUS)
				{
					consecutive++;
					if(consecutive >= 3)
					{
						delete PREVIOUS;
						break;
					}
				}
				else
					consecutive = 0;
				
				if(done && current->position <= PREVIOUS->position)
				{
					delete current;
					break;
				}
			}
		}
	}
}


void Autos::remove_nonsequential(Auto *keyframe)
{
	if((keyframe->next && keyframe->next->position <= keyframe->position) ||
		(keyframe->previous && keyframe->previous->position >= keyframe->position))
	{
		delete keyframe;
	}
}


void Autos::set_automation_mode(int64_t start, int64_t end, int mode)
{
}

void Autos::clear(int64_t start, 
	int64_t end, 
	int shift_autos)
{
	int64_t length;
	Auto *next, *current;
	length = end - start;


	current = autoof(start);

// If a range is selected don't delete the ending keyframe but do delete
// the beginning keyframe because shifting end handle forward shouldn't
// delete the first keyframe of the next edit.

	while(current && 
		((end != start && current->position < end) ||
		(end == start && current->position <= end)))
	{
		next = NEXT;
		remove(current);
		current = next;
	}

	while(current && shift_autos)
	{
		current->position -= length;
		current = NEXT;
	}
}

int Autos::clear_auto(int64_t position)
{
	Auto *current;
	current = autoof(position);
	if(current->position == position) remove(current);
    return 0;
}


int Autos::load(FileXML *file)
{
//	while(last)
//		remove(last);    // remove any existing autos

	int result = 0, first_auto = 1;
	Auto *current = first;
	char end_tag[BCTEXTLEN];
    sprintf(end_tag, "/%s", Automation::get_save_title(overlay_type));
    
    
    
	do{
		result = file->read_tag();
		if(result) break;
		if(!strcasecmp(file->tag.get_title(), end_tag))
		{
			result = 1;
		}
		else
		if(!strcmp(file->tag.get_title(), "AUTO"))
		{
			if(first_auto)
			{
				default_auto->load(file);
				default_auto->position = 0;
				first_auto = 0;
			}
			else
			{
                Auto *dst = 0;
                if(current)
                {
                    dst = current;
                    current = NEXT;
                }
                else
    				dst = append(new_auto());

				dst->position = file->tag.get_property("POSITION", (int64_t)0);
				dst->load(file);
			}
		}
	}while(!result);

// delete unused keyframes
    while(current)
    {
        Auto *dst = current;
        current = NEXT;
        delete dst;
    }
	return 0;
}






int Autos::slope_adjustment(int64_t ax, double slope)
{
	return (int)(ax * slope);
}


int Autos::scale_time(float rate_scale, int scale_edits, int scale_autos, int64_t start, int64_t end)
{
	Auto *current;
	
	for(current = first; current && scale_autos; current = NEXT)
	{
//		if(current->position >= start && current->position <= end)
//		{
			current->position = (int64_t)((current->position - start) * rate_scale + start + 0.5);
//		}
	}
	return 0;
}

Auto* Autos::autoof(int64_t position)
{
	Auto *current;

	for(current = first; 
		current && current->position < position; 
		current = NEXT)
	{ 
		;
	}
	return current;     // return 0 on failure
}

Auto* Autos::nearest_before(int64_t position)
{
	Auto *current;

	for(current = last; current && current->position >= position; current = PREVIOUS)
	{ ; }


	return current;     // return 0 on failure
}

Auto* Autos::nearest_after(int64_t position)
{
	Auto *current;

	for(current = first; current && current->position <= position; current = NEXT)
	{ ; }


	return current;     // return 0 on failure
}

int Autos::get_neighbors(int64_t start, int64_t end, Auto **before, Auto **after)
{
	if(*before == 0) *before = first;
	if(*after == 0) *after = last; 

	while(*before && (*before)->next && (*before)->next->position <= start)
		*before = (*before)->next;
	
	while(*after && (*after)->previous && (*after)->previous->position >= end)
		*after = (*after)->previous;

	while(*before && (*before)->position > start) *before = (*before)->previous;
	
	while(*after && (*after)->position < end) *after = (*after)->next;
	return 0;
}

int Autos::automation_is_constant(int64_t start, int64_t end)
{
	return 0;
}

double Autos::get_automation_constant(int64_t start, int64_t end)
{
	return 0;
}


int Autos::init_automation(int64_t &buffer_position,
				int64_t &input_start, 
				int64_t &input_end, 
				int &automate, 
				double &constant, 
				int64_t input_position,
				int64_t buffer_len,
				Auto **before, 
				Auto **after,
				int reverse)
{
	buffer_position = 0;

// set start and end boundaries for automation info
	input_start = reverse ? input_position - buffer_len : input_position;
	input_end = reverse ? input_position : input_position + buffer_len;

// test automation for constant value
// and set up *before and *after
	if(automate)
	{
		if(automation_is_constant(input_start, input_end))
		{
			constant += get_automation_constant(input_start, input_end);
			automate = 0;
		}
	}
	return automate;
}


int Autos::init_slope(Auto **current_auto, 
				double &slope_start, 
				double &slope_value,
				double &slope_position, 
				int64_t &input_start, 
				int64_t &input_end, 
				Auto **before, 
				Auto **after,
				int reverse)
{
// apply automation
	*current_auto = reverse ? *after : *before;
// no auto before start so use first auto in range
// already know there is an auto since automation isn't constant
	if(!*current_auto)
	{
		*current_auto = reverse ? last : first;
//		slope_value = (*current_auto)->value;
		slope_start = input_start;
		slope_position = 0;
	}
	else
	{
// otherwise get the first slope point and advance auto
//		slope_value = (*current_auto)->value;
		slope_start = (*current_auto)->position;
		slope_position = reverse ? slope_start - input_end : input_start - slope_start;
		(*current_auto) = reverse ? (*current_auto)->previous : (*current_auto)->next;
	}
	return 0;
}


int Autos::get_slope(Auto **current_auto, 
				double &slope_start, 
				double &slope_end, 
				double &slope_value,
				double &slope, 
				int64_t buffer_len, 
				int64_t buffer_position,
				int reverse)
{
// get the slope
	if(*current_auto)
	{
		slope_end = reverse ? slope_start - (*current_auto)->position : (*current_auto)->position - slope_start;
		if(slope_end) 
//			slope = ((*current_auto)->value - slope_value) / slope_end;
//		else
			slope = 0;
	}
	else
	{
		slope = 0;
		slope_end = buffer_len - buffer_position;
	}
	return 0;
}

int Autos::advance_slope(Auto **current_auto, 
				double &slope_start, 
				double &slope_value,
				double &slope_position, 
				int reverse)
{
	if(*current_auto) 
	{
		slope_start = (*current_auto)->position;
//		slope_value = (*current_auto)->value;
		(*current_auto) = reverse ? (*current_auto)->previous : (*current_auto)->next;
		slope_position = 0;
	}
	return 0;
}

int64_t Autos::get_length()
{
	if(last) 
		return last->position + 1;
	else
		return 0;
}

void Autos::get_extents(float *min, 
	float *max,
	int *coords_undefined,
	int64_t unit_start,
	int64_t unit_end)
{
	
}

int Autos::auto_exists(Auto *auto_)
{
    if(default_auto == auto_) return 1;
	for(Auto *current = first; current; current = NEXT)
	{
        if(current == auto_) return 1;
    }
    return 0;
}


void Autos::dump()
{
}










