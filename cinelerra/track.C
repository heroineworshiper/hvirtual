/*
 * CINELERRA
 * Copyright (C) 2010-2024 Adam Williams <broadcast at earthling dot net>
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
#include "automation.h"
#include "bcsignals.h"
#include "clip.h"
#include "edit.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "floatauto.h"
#include "floatautos.h"
#include "keyframe.h"
#include "keyframes.h"
#include "labels.h"
#include "localsession.h"
#include "mainsession.h"
#include "module.h"
#include "mwindow.h"
#include "patch.h"
#include "patchbay.h"
#include "plugin.h"
#include "pluginset.h"
#include "preferences.h"
#include "theme.h"
#include "intautos.h"
#include "track.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "transition.h"
#include "transportque.inc"
#include "vedit.h"
#include "vframe.h"
#include <string.h>


Track::Track(EDL *edl, Tracks *tracks) : ListItem<Track>()
{
	this->edl = edl;
	this->tracks = tracks;
	y_pixel = 0;
	expand_view = 0;
	draw = 1;
	gang = 1;
	title[0] = 0;
	record = 1;
	play = 1;
	nudge = 0;
	track_w = edl->session->output_w;
	track_h = edl->session->output_h;
	id = EDL::next_id();
}

Track::~Track()
{
	delete automation;
	delete edits;
	plugin_set.remove_all_objects();
}

void Track::create_objects()
{
}


int Track::copy_settings(Track *track)
{
	this->expand_view = track->expand_view;
	this->draw = track->draw;
	this->gang = track->gang;
	this->record = track->record;
	this->nudge = track->nudge;
	this->play = track->play;
	this->track_w = track->track_w;
	this->track_h = track->track_h;
	strcpy(this->title, track->title);
	return 0;
}

int Track::get_id()
{
	return id;
}


int Track::load_defaults(BC_Hash *defaults)
{
	return 0;
}

void Track::equivalent_output(Track *track, double *result)
{
	if(data_type != track->data_type ||
		track_w != track->track_w ||
		track_h != track->track_h ||
		play != track->play ||
		nudge != track->nudge)
		*result = 0;

// Convert result to track units
	int64_t result2 = -1;
	automation->equivalent_output(track->automation, &result2);
	edits->equivalent_output(track->edits, &result2);

	int plugin_sets = MIN(plugin_set.total, track->plugin_set.total);
// Test existing plugin sets
	for(int i = 0; i < plugin_sets; i++)
	{
		plugin_set.get(i)->equivalent_output(
			track->plugin_set.get(i), 
			&result2);
	}

// New EDL has more plugin sets.  Get starting plugin in new plugin sets
	for(int i = plugin_sets; i < plugin_set.total; i++)
	{
		Plugin *current = plugin_set.values[i]->get_first_plugin();
		if(current)
		{
			if(result2 < 0 || current->startproject < result2)
				result2 = current->startproject;
		}
	}

// New EDL has fewer plugin sets.  Get starting plugin in old plugin set
	for(int i = plugin_sets; i < track->plugin_set.total; i++)
	{
		Plugin *current = track->plugin_set.values[i]->get_first_plugin();
		if(current)
		{
			if(result2 < 0 || current->startproject < result2)
				result2 = current->startproject;
		}
	}

// Number of plugin sets differs but somehow we didn't find the start of the
// change.  Assume 0
	if(track->plugin_set.total != plugin_set.total && result2 < 0)
		result2 = 0;

	if(result2 >= 0 && 
		(*result < 0 || from_units(result2) < *result))
		*result = from_units(result2);
}

int Track::is_shared(int64_t position, int direction)
{
	for(int i = 0; i < plugin_set.total; i++)
	{
		Plugin *plugin = get_current_plugin(position,
			i,
			direction,
			0,
			0);
		if(plugin)
		{
			if(plugin->plugin_type == PLUGIN_SHAREDMODULE ||
                plugin->plugin_type == PLUGIN_SHAREDPLUGIN) 
				return 1;
		}
	}
    return 0;
}

// int Track::is_synthesis()
// {
// 	for(int i = 0; i < plugin_set.total; i++)
// 	{
// 		Plugin *plugin = (Plugin*)plugin_set.get(i)->first;
//         while(plugin)
//         {
//             if(plugin->on &&
//                 (plugin->plugin_type == PLUGIN_SHAREDMODULE ||
//                 plugin->is_synthesis(plugin->startproject, 
// 					PLAY_FORWARD,
//                     0)))
//             {
//                 return 1;
//             }
//             plugin = (Plugin*)plugin->next;
//         }
//     }
//     return 0;
// }

// int Track::is_synthesis(int64_t position, 
// 	int direction,
//     int depth)
// {
// 	int is_synthesis = 0;
// 	for(int i = 0; i < plugin_set.total; i++)
// 	{
// 		Plugin *plugin = get_current_plugin(position,
// 			i,
// 			direction,
// 			0,
// 			0);
// 		if(plugin)
// 		{
// // Assume data from a shared track is synthesized
// 			if(plugin->plugin_type == PLUGIN_SHAREDMODULE) 
// 				is_synthesis = 1;
// 			else
// 				is_synthesis = plugin->is_synthesis(position, 
// 					direction,
//                     depth);
// 
// //printf("Track::is_synthesis %d %d\n", __LINE__, is_synthesis);
// 			if(is_synthesis) break;
// 		}
// 	}
// 	return is_synthesis;
// }

void Track::copy_from(Track *track)
{
	copy_settings(track);
	edits->copy_from(track->edits);
	for(int i = 0; i < this->plugin_set.total; i++)
		delete this->plugin_set.values[i];
	this->plugin_set.remove_all_objects();

	for(int i = 0; i < track->plugin_set.total; i++)
	{
		PluginSet *new_plugin_set = plugin_set.append(new PluginSet(edl, this));
		new_plugin_set->copy_from(track->plugin_set.values[i]);
	}
	automation->copy_from(track->automation);
	this->track_w = track->track_w;
	this->track_h = track->track_h;
}

Track& Track::operator=(Track& track)
{
printf("Track::operator= 1\n");
	copy_from(&track);
	return *this;
}

int Track::vertical_span(Theme *theme)
{
	int result = 0;
    int plugin_h = theme->get_image("plugin_bg_data")->get_h();
    int title_h = theme->get_image("title_bg_data")->get_h();

// shift down if track has media or certain keyframes are visible
    int got_autos = 0;
    for(int i = 0; i < AUTOMATION_TOTAL; i++)
        if(edl->session->auto_conf->autos[i]) got_autos = 1;
//printf("Track::vertical_span %d expand_view=%d got_autos=%d\n", __LINE__, expand_view, got_autos);

// follows the greater of the plugin heights or the patchbay if expanded
	if(expand_view)
	{
// fixed vertical margin if no media or keyframes
// if the patch height is smaller than this + the plugin height, nothing 
// will show
        int plugin_margin = PLUGIN_MARGIN;
// height of the media
        if(edits->last || got_autos)
        {
	        if(edits->last && edl->session->show_titles)
    		{
                result += title_h;
                plugin_margin = 0;
            }

            if(edl->session->show_assets || got_autos)
            {
                result += edl->local_session->zoom_track;
                plugin_margin = 0;
            }
        }

    	result += plugin_margin + plugin_set.total * plugin_h;
        if(MWindow::theme->patch_h > result)
        {
            result = MWindow::theme->patch_h;
        }
	}
    else
	{
// fixed height if not expanded
        if(edl->session->show_assets || got_autos)
        	result += edl->local_session->zoom_track;
	    if(edits->last && edl->session->show_titles)
    		result += title_h;
        if(edl->local_session->zoom_track > result)
            result = edl->local_session->zoom_track;
    }


	return result;
}

double Track::get_length()
{
	double total_length = 0;
	double length = 0;

// Test edits
	if(edits->last)
	{
		length = from_units(edits->last->startproject + edits->last->length);
		if(length > total_length) total_length = length;
	}

// Test plugins
	for(int i = 0; i < plugin_set.total; i++)
	{
		if(plugin_set.values[i]->last)
		{
			length = from_units(plugin_set.values[i]->last->startproject + 
				plugin_set.values[i]->last->length);
			if(length > total_length) total_length = length;
		}
	}

// Test keyframes
	length = from_units(automation->get_length());
	if(length > total_length) total_length = length;
	

	return total_length;
}

int Track::has_speed()
{
	FloatAutos *autos = (FloatAutos*)automation->autos[AUTOMATION_SPEED];
	if(autos)
	{
		if(autos->first)
		{
			for(FloatAuto *current = (FloatAuto*)autos->first;
				current;
				current = (FloatAuto*)current->next)
			{
				if(!EQUIV(current->value, 1.0) ||
					!EQUIV(current->control_in_value, 0.0) ||
					!EQUIV(current->control_out_value, 0.0))
				{
					return 1;
				}
			}
		}
	}
	
	return 0;
}



void Track::get_source_dimensions(double position, int &w, int &h)
{
	int64_t native_position = to_units(position, 0);
	for(Edit *current = edits->first; current; current = NEXT)
	{
		if(current->startproject <= native_position &&
			current->startproject + current->length > native_position &&
			current->asset)
		{
			w = current->asset->width;
			h = current->asset->height;
			return;
		}
	}
}


int64_t Track::horizontal_span()
{
	return (int64_t)(get_length() * 
		edl->session->sample_rate / 
		edl->local_session->zoom_sample + 
		0.5);
}


int Track::load(FileXML *file, int track_offset)
{
	int result = 0;
	int current_channel = 0;
	int current_set = 0;
    int error = 0;

	record = file->tag.get_property("RECORD", record);
	play = file->tag.get_property("PLAY", play);
	gang = file->tag.get_property("GANG", gang);
	draw = file->tag.get_property("DRAW", draw);
	nudge = file->tag.get_property("NUDGE", nudge);
	expand_view = file->tag.get_property("EXPAND", expand_view);
	track_w = file->tag.get_property("TRACK_W", track_w);
	track_h = file->tag.get_property("TRACK_H", track_h);

	load_header(file);

	do{
		result = file->read_tag();
        if(result) break;
		if(file->tag.title_is("/TRACK"))
		{
			result = 1;
		}
		else
		if(file->tag.title_is("TITLE"))
		{
			file->read_text_until("/TITLE", title, BCTEXTLEN);
		}
		else
		if(automation->load(file))
		{
			;
		}
		else
		if(file->tag.title_is("EDITS"))
		{
// edits or transitions
			error |= edits->load(file, track_offset);
		}
		else
		if(file->tag.title_is("PLUGINSET"))
		{
// descend into the PLUGINSET tag
            PluginSet *plugin_set = 0;
			if(current_set < this->plugin_set.size())
			{
				plugin_set = this->plugin_set.get(current_set);
			}
            else
            {
				plugin_set = new PluginSet(edl, this);
				this->plugin_set.append(plugin_set);
            }
			plugin_set->load(file);
			current_set++;
		}
		else
        {
			load_derived(file);
        }
	}while(!result);

// delete leftovers
    while(current_set < this->plugin_set.size())
    {
        this->plugin_set.remove_object_number(this->plugin_set.size() - 1);
    }

	return error;
}

void Track::insert_asset(Asset *asset, 
	EDL *nested_edl,
	double length, 
	double position, 
	int track_number)
{
	edits->insert_asset(asset, 
		nested_edl,
		to_units(length, 1), 
		to_units(position, 0), 
		track_number);
}

// Insert data

// Default keyframes: We don't replace default keyframes in pasting but
// when inserting the first EDL of a load operation we need to replace
// the default keyframes.

// Plugins:  This is an arbitrary behavior
//
// 1) No plugin in source track: Paste silence into destination
// plugin sets.
// 2) Plugin in source track: plugin in source track is inserted into
// existing destination track plugin sets, new sets being added when
// necessary.

void Track::insert_track(Track *track, 
	double position, 
	int replace_default,
	int edit_plugins,
	int edit_autos,
	double edl_length)
{
// Calculate minimum length of data to pad.
	int64_t min_length = to_units(
		MAX(edl_length, track->get_length()),
		1);
//printf("Track::insert_track %d %s %lld\n", __LINE__, title, min_length);

// Decide whether to copy settings based on load_mode
	if(replace_default) copy_settings(track);

	edits->insert_edits(track->edits, 
		to_units(position, 0),
		min_length,
		edit_autos);

	if(edit_plugins)
		insert_plugin_set(track, 
			to_units(position, 0),
			min_length,
			edit_autos);

	if(edit_autos)
		automation->insert_track(track->automation, 
			to_units(position, 0), 
			min_length,
			replace_default);

	optimize();

}

// Called by insert_track
void Track::insert_plugin_set(Track *track, 
	int64_t position,
	int64_t min_length,
	int edit_autos)
{
// Extend plugins if no incoming plugins
	if(!track->plugin_set.total)
	{
		shift_effects(position, 
			min_length,
			edit_autos);
	}
	else
	for(int i = 0; i < track->plugin_set.total; i++)
	{
		if(i >= plugin_set.total)
			plugin_set.append(new PluginSet(edl, this));

		plugin_set.values[i]->insert_edits(track->plugin_set.values[i], 
			position,
			min_length,
			edit_autos);
	}
}


Plugin* Track::insert_effect(const char *title, 
		SharedLocation *shared_location, 
		KeyFrame *default_keyframe,
		PluginSet *plugin_set,
		double start,
		double length,
		int plugin_type)
{
	if(!plugin_set)
	{
		plugin_set = new PluginSet(edl, this);
		this->plugin_set.append(plugin_set);
	}

	Plugin *plugin = 0;

// Position is identical to source plugin
	if(plugin_type == PLUGIN_SHAREDPLUGIN)
	{
		Track *source_track = tracks->get_item_number(shared_location->module);
		if(source_track)
		{
			Plugin *source_plugin = source_track->get_current_plugin(
				edl->local_session->get_selectionstart(), 
				shared_location->plugin, 
				PLAY_FORWARD, 
				1,
				0);

// From an attach operation
			if(source_plugin)
			{
				plugin = plugin_set->insert_plugin(title, 
					source_plugin->startproject, 
					source_plugin->length,
					plugin_type, 
					shared_location,
					default_keyframe,
					1);
			}
			else
// From a drag operation
			{
				plugin = plugin_set->insert_plugin(title, 
					to_units(start, 0), 
					to_units(length, 0),
					plugin_type, 
					shared_location,
					default_keyframe,
					1);
			}
		}
	}
	else
	{
// This should be done in the caller
		if(EQUIV(length, 0))
		{
			if(edl->local_session->get_selectionend() > 
				edl->local_session->get_selectionstart())
			{
				start = edl->local_session->get_selectionstart();
				length = edl->local_session->get_selectionend() - start;
			}
			else
			{
				start = 0;
				length = get_length();
			}
		}
//printf("Track::insert_effect %f %f %d %d\n", start, length, to_units(start, 0), 
//			to_units(length, 0));

		plugin = plugin_set->insert_plugin(title, 
			to_units(start, 0), 
			to_units(length, 0),
			plugin_type, 
			shared_location,
			default_keyframe,
			1);
	}
//printf("Track::insert_effect 2 %f %f\n", start, length);

	expand_view = 1;
	return plugin;
}

void Track::move_plugins_up(PluginSet *plugin_set)
{
	for(int i = 0; i < this->plugin_set.total; i++)
	{
		if(this->plugin_set.values[i] == plugin_set)
		{
			if(i == 0) break;

			PluginSet *temp = this->plugin_set.values[i - 1];
			this->plugin_set.values[i - 1] = this->plugin_set.values[i];
			this->plugin_set.values[i] = temp;
			break;
		}
	}
}

void Track::move_plugins_down(PluginSet *plugin_set)
{
	for(int i = 0; i < this->plugin_set.total; i++)
	{
		if(this->plugin_set.values[i] == plugin_set)
		{
			if(i == this->plugin_set.total - 1) break;

			PluginSet *temp = this->plugin_set.values[i + 1];
			this->plugin_set.values[i + 1] = this->plugin_set.values[i];
			this->plugin_set.values[i] = temp;
			break;
		}
	}
}


void Track::remove_asset(Indexable *asset)
{
	for(Edit *edit = edits->first; edit; edit = edit->next)
	{
		if(asset->is_asset && 
			edit->asset && 
			edit->asset == (Asset*)asset)
		{
			edit->asset = 0;
		}
		else
		if(!asset->is_asset && 
			edit->nested_edl && 
			edit->nested_edl == (EDL*)asset)
		{
			edit->nested_edl = 0;
		}
	}
	optimize();
}

void Track::remove_pluginset(PluginSet *plugin_set)
{
	int i;
	for(i = 0; i < this->plugin_set.total; i++)
		if(plugin_set == this->plugin_set.values[i]) break;

	this->plugin_set.remove_object(plugin_set);
	for(i++ ; i < this->plugin_set.total; i++)
	{
		SharedLocation old_location, new_location;
		new_location.module = old_location.module = tracks->number_of(this);
		old_location.plugin = i;
		new_location.plugin = i - 1;
		tracks->change_plugins(old_location, new_location, 0);
	}
}

void Track::shift_keyframes(int64_t position, int64_t length)
{
	automation->paste_silence(position, 
		position + length);
// Effect keyframes are shifted in shift_effects
}

void Track::shift_effects(int64_t position, int64_t length, int edit_autos)
{
	for(int i = 0; i < plugin_set.total; i++)
	{
		plugin_set.values[i]->shift_effects(position, 
			length,
			edit_autos);
	}
}

void Track::detach_effect(Plugin *plugin)
{
//printf("Track::detach_effect 1\n");		
	for(int i = 0; i < plugin_set.total; i++)
	{
		PluginSet *plugin_set = this->plugin_set.values[i];
		for(Plugin *dest = (Plugin*)plugin_set->first; 
			dest; 
			dest = (Plugin*)dest->next)
		{
			if(dest == plugin)
			{
				int64_t start = plugin->startproject;
				int64_t end = plugin->startproject + plugin->length;

				plugin_set->clear(start, end, 1);
				plugin_set->paste_silence(start, end);

// Delete 0 length pluginsets	
				plugin_set->optimize();
//printf("Track::detach_effect 2 %d\n", plugin_set->length());
				if(!plugin_set->length()) 
					this->plugin_set.remove_object(plugin_set);

				return;
			}
		}
	}
}

void Track::resample(double old_rate, double new_rate)
{
	edits->resample(old_rate, new_rate);
	automation->resample(old_rate, new_rate);
	for(int i = 0; i < plugin_set.total; i++)
		plugin_set.values[i]->resample(old_rate, new_rate);
	nudge = (int64_t)(nudge * new_rate / old_rate);
}

void Track::detach_shared_effects(int module)
{
	for(int i = 0; i < plugin_set.size(); i++)
	{
		PluginSet *plugin_set = this->plugin_set.get(i);
		for(Plugin *dest = (Plugin*)plugin_set->first; 
			dest; 
			)
		{
			if ((dest->plugin_type == PLUGIN_SHAREDPLUGIN ||
				dest->plugin_type == PLUGIN_SHAREDMODULE)
			    &&
				dest->shared_location.module == module)
			{
				int64_t start = dest->startproject;
				int64_t end = dest->startproject + dest->length;

				plugin_set->clear(start, end, 1);
				plugin_set->paste_silence(start, end);

// Delete 0 length pluginsets
				plugin_set->optimize();
				if(!plugin_set->length())  
				{
					this->plugin_set.remove_object_number(i);
					dest = 0;
					--i;
				}
				
			}

			if(dest) dest = (Plugin*)dest->next;
		}
	}
}


void Track::optimize()
{
	edits->optimize();
	for(int i = 0; i < plugin_set.total; i++)
	{
		plugin_set.values[i]->optimize();
//printf("Track::optimize %d\n", plugin_set.values[i]->total());
		if(plugin_set.values[i]->total() <= 0)
		{
			remove_pluginset(plugin_set.values[i]);
			i--;
		}
	}
}

Plugin* Track::get_current_plugin(double position, 
	int plugin_set, 
	int direction, 
	int convert_units,
	int use_nudge)
{
	Plugin *current;
	if(convert_units) position = to_units(position, 0);
	if(use_nudge) position += nudge;
	
	if(plugin_set >= this->plugin_set.total || plugin_set < 0) return 0;

//printf("Track::get_current_plugin 1 %d %d %d\n", position, this->plugin_set.total, direction);
	if(direction == PLAY_FORWARD)
	{
		for(current = (Plugin*)this->plugin_set.values[plugin_set]->last; 
			current; 
			current = (Plugin*)PREVIOUS)
		{
// printf("Track::get_current_plugin 2 %d %ld %ld\n", 
// current->startproject, 
// current->startproject + current->length, 
// position);
			if(current->startproject <= position && 
				current->startproject + current->length > position)
			{
				return current;
			}
		}
	}
	else
	if(direction == PLAY_REVERSE)
	{
		for(current = (Plugin*)this->plugin_set.values[plugin_set]->first; 
			current; 
			current = (Plugin*)NEXT)
		{
			if(current->startproject < position && 
				current->startproject + current->length >= position)
			{
				return current;
			}
		}
	}

	return 0;
}

Plugin* Track::get_current_transition(double position, 
	int direction, 
	int convert_units,
	int use_nudge)
{
	Edit *current;
	Plugin *result = 0;
	if(convert_units) position = to_units(position, 0);
	if(use_nudge) position += nudge;

	if(direction == PLAY_FORWARD)
	{
		for(current = edits->last; current; current = PREVIOUS)
		{
			if(current->startproject <= position && current->startproject + current->length > position)
			{
//printf("Track::get_current_transition %p\n", current->transition);
				if(current->transition &&
                    current->transition->on && 
                    position < current->startproject + current->transition->length)
				{
					result = current->transition;
					break;
				}
			}
		}
	}
	else
	if(direction == PLAY_REVERSE)
	{
		for(current = edits->first; current; current = NEXT)
		{
			if(current->startproject < position && current->startproject + current->length >= position)
			{
				if(current->transition && 
                    current->transition->on && 
                    position <= current->startproject + current->transition->length)
				{
					result = current->transition;
					break;
				}
			}
		}
	}

	return result;
}

void Track::synchronize_params(Track *track)
{
	for(Edit *this_edit = edits->first, *that_edit = track->edits->first;
		this_edit && that_edit;
		this_edit = this_edit->next, that_edit = that_edit->next)
	{
		this_edit->synchronize_params(that_edit);
	}

	for(int i = 0; i < plugin_set.total && i < track->plugin_set.total; i++)
		plugin_set.values[i]->synchronize_params(track->plugin_set.values[i]);

	automation->copy_from(track->automation);
	this->nudge = track->nudge;
}





int Track::dump()
{
	printf("   Data type %d expand_view=%d\n", data_type, expand_view);
	printf("   Title %s\n", title);
	printf("   Edits:\n");
	for(Edit* current = edits->first; current; current = NEXT)
	{
		current->dump();
	}
	automation->dump();
	printf("   Plugin Sets: %d\n", plugin_set.total);

	for(int i = 0; i < plugin_set.total; i++)
		plugin_set.values[i]->dump();
//printf("Track::dump 2\n");
	return 0;
}





















Track::Track() : ListItem<Track>()
{
	y_pixel = 0;
}

// ======================================== accounting

int Track::number_of() 
{ 
	return tracks->number_of(this); 
}





	
	






// ================================================= editing

int Track::select_auto(AutoConf *auto_conf, int cursor_x, int cursor_y)
{
	return 0;
}

int Track::move_auto(AutoConf *auto_conf, int cursor_x, int cursor_y, int shift_down)
{
	return 0;
}

int Track::release_auto()
{
	return 0;
}

// used for copying automation alone
int Track::copy_automation(double selectionstart, 
	double selectionend, 
	FileXML *file,
	int default_only,
	int active_only)
{
	int64_t start = to_units(selectionstart, 0);
	int64_t end = to_units(selectionend, 0);

	file->tag.set_title("TRACK");
// Video or audio
    save_header(file);
	file->append_tag();
	file->append_newline();

	automation->copy(start, end, file, default_only, active_only);

	if(edl->session->auto_conf->autos[PLUGIN_KEYFRAMES])
	{
		for(int i = 0; i < plugin_set.total; i++)
		{
		
			plugin_set.get(i)->copy_keyframes(start, 
				end, 
				file, 
				default_only,
				active_only);
		}
	}

	file->tag.set_title("/TRACK");
	file->append_tag();
	file->append_newline();
	file->append_newline();
	file->append_newline();
	file->append_newline();

	return 0;
}

int Track::paste_automation(double selectionstart, 
	double total_length, 
	double frame_rate,
	int64_t sample_rate,
	FileXML *file,
	int default_only,
	int active_only)
{
// Only used for pasting automation alone.
	int64_t start;
	int64_t length;
	int result;
	double scale;
	int current_pluginset;

	if(data_type == TRACK_AUDIO)
		scale = edl->session->sample_rate / sample_rate;
	else
		scale = edl->session->frame_rate / frame_rate;

	total_length *= scale;
	start = to_units(selectionstart, 0);
	length = to_units(total_length, 0);
	result = 0;
	current_pluginset = 0;
//printf("Track::paste_automation 1\n");

	while(!result)
	{
		result = file->read_tag();

		if(!result)
		{
			if(file->tag.title_is("/TRACK"))
			{
            	result = 1;
			}
            else
			if(automation->paste(start, 
					length, 
					scale,
					file,
					default_only,
					active_only,
					0))
			{
				;
			}
			else
			if(file->tag.title_is("PLUGINSET"))
			{
				if(current_pluginset < plugin_set.total)
				{
					plugin_set.values[current_pluginset]->paste_keyframes(start, 
						length, 
						file,
						default_only,
						active_only);
					current_pluginset++;
				}
			}
		}
	}
	

	return 0;
}

void Track::clear_automation(double selectionstart, 
	double selectionend, 
	int shift_autos,
	int default_only)
{
	int64_t start = to_units(selectionstart, 0);
	int64_t end = to_units(selectionend, 0);

	automation->clear(start, end, edl->session->auto_conf, 0);

	if(edl->session->auto_conf->autos[PLUGIN_KEYFRAMES])
	{
		for(int i = 0; i < plugin_set.total; i++)
		{
			plugin_set.values[i]->clear_keyframes(start, end);
		}
	}

}

void Track::set_automation_mode(double selectionstart, 
	double selectionend,
	int mode)
{
	int64_t start = to_units(selectionstart, 0);
	int64_t end = to_units(selectionend, 0);

	automation->set_automation_mode(start, end, mode, edl->session->auto_conf);
}




int Track::copy(double start, 
	double end, 
	FileXML *file, 
	const char *output_path)
{
// Use a copy of the selection in converted units
// So copy_automation doesn't reconvert.
	int64_t start_unit = to_units(start, 0);
	int64_t end_unit = to_units(end, 1);




	file->tag.set_title("TRACK");
// save typing by not subclassing this
    if(data_type == TRACK_AUDIO)
        file->tag.set_property("TYPE", "AUDIO");
    else
        file->tag.set_property("TYPE", "VIDEO");
	file->tag.set_property("RECORD", record);
	file->tag.set_property("NUDGE", nudge);
	file->tag.set_property("PLAY", play);
	file->tag.set_property("GANG", gang);
	file->tag.set_property("DRAW", draw);
	file->tag.set_property("EXPAND", expand_view);
	file->tag.set_property("TRACK_W", track_w);
	file->tag.set_property("TRACK_H", track_h);
	file->append_tag();
	file->append_newline();

	file->tag.set_title("TITLE");
	file->append_tag();
	file->append_text(title);
	file->tag.set_title("/TITLE");
	file->append_tag();
	file->append_newline();


	edits->copy(start_unit, end_unit, file, output_path);

	AutoConf auto_conf;
	auto_conf.set_all(1);
	automation->copy(start_unit, end_unit, file, 0, 0);


	for(int i = 0; i < plugin_set.total; i++)
	{
		plugin_set.values[i]->copy(start_unit, end_unit, file);
	}

	copy_derived(start_unit, end_unit, file);

	file->tag.set_title("/TRACK");
	file->append_tag();
	file->append_newline();
	file->append_newline();
	file->append_newline();
	file->append_newline();

	return 0;
}

int Track::copy_assets(double start, 
	double end, 
	ArrayList<Asset*> *asset_list, 
	ArrayList<EDL*> *nested_list)
{
	int i, result = 0;

	start = to_units(start, 0);
	end = to_units(end, 0);

	Edit *current = edits->editof((int64_t)start, PLAY_FORWARD, 0);

// Search all edits
	while(current && current->startproject < end)
	{
// Check for duplicate assets
		if(current->asset)
		{
			for(i = 0, result = 0; i < asset_list->size(); i++)
			{
				if(asset_list->get(i) == current->asset)
                {
                    result = 1;
                }
			}
// append pointer to new asset
			if(!result) asset_list->append(current->asset);
		}

        if(current->nested_edl)
        {
            for(i = 0, result = 0; i < nested_list->size(); i++)
            {
                if(nested_list->get(i) == current->nested_edl)
                {
                    result = 1;
                }
            }
            if(!result)
            {
                nested_list->append(current->nested_edl);
            }
        }

		current = NEXT;
	}

	return 0;
}


int Track::clear(double start, 
	double end, 
	int edit_edits,
	int edit_labels,
	int edit_plugins,
	int edit_autos,
	int convert_units,
	Edits *trim_edits)
{
// Edits::move_auto calls this routine after the units are converted to the track
// format.
//printf("Track::clear 1 %d %d %d\n", edit_edits, edit_labels, edit_plugins);
	if(convert_units)
	{
		start = to_units(start, 0);
		end = to_units(end, 0);
	}


	if(edit_autos)
	{
		automation->clear((int64_t)start, (int64_t)end, 0, 1);
	}

	if(edit_plugins)
	{
		for(int i = 0; i < plugin_set.total; i++)
		{
			if(!trim_edits || trim_edits == (Edits*)plugin_set.values[i])
				plugin_set.values[i]->clear((int64_t)start, (int64_t)end, edit_autos);
		}
	}

	if(edit_edits)
	{
		edits->clear((int64_t)start, (int64_t)end);
	}
	
	return 0;
}

int Track::clear_handle(double start, 
	double end, 
	int clear_labels,
	int clear_plugins, 
	int edit_autos,
	double &distance)
{
	edits->clear_handle(start, end, clear_plugins, edit_autos, distance);
    return 0;
}

int Track::popup_transition(int cursor_x, int cursor_y)
{
	return 0;
}



int Track::modify_edithandles(double oldposition, 
	double newposition, 
	int currentend, 
	int handle_mode,
	int edit_labels,
	int edit_plugins,
	int edit_autos)
{
	edits->modify_handles(oldposition, 
		newposition, 
		currentend,
		handle_mode,
		1,
		edit_labels,
		edit_plugins,
		edit_autos,
		0);


	return 0;
}

int Track::modify_pluginhandles(double oldposition, 
	double newposition, 
	int currentend, 
	int handle_mode,
	int edit_labels,
	int edit_autos,
	Edits *trim_edits)
{
	for(int i = 0; i < plugin_set.total; i++)
	{
		if(!trim_edits || trim_edits == (Edits*)plugin_set.values[i])
			plugin_set.values[i]->modify_handles(oldposition, 
				newposition, 
				currentend, 
				handle_mode,
// Don't allow plugin tweeks to affect edits.
				0,
				edit_labels,
				1,
				edit_autos,
				trim_edits);
	}
	return 0;
}


int Track::paste_silence(double start, double end, int edit_plugins, int edit_autos)
{
	int64_t start_i = to_units(start, 0);
	int64_t end_i = to_units(end, 1);

	edits->paste_silence(start_i, end_i);
	if(edit_autos) shift_keyframes(start_i, end_i - start_i);
	if(edit_plugins) shift_effects(start_i, end_i - start_i, edit_autos);

	edits->optimize();
	return 0;
}

int Track::select_edit(int cursor_x, 
	int cursor_y, 
	double &new_start, 
	double &new_end)
{
	return 0;
}

int Track::scale_time(float rate_scale, int scale_edits, int scale_autos, int64_t start, int64_t end)
{
	return 0;
}

void Track::change_plugins(SharedLocation &old_location, SharedLocation &new_location, int do_swap)
{
	for(int i = 0; i < plugin_set.total; i++)
	{
		for(Plugin *plugin = (Plugin*)plugin_set.values[i]->first; 
			plugin; 
			plugin = (Plugin*)plugin->next)
		{
			if(plugin->plugin_type == PLUGIN_SHAREDPLUGIN)
			{
				if(plugin->shared_location == old_location)
					plugin->shared_location = new_location;
				else
				if(do_swap && plugin->shared_location == new_location)
					plugin->shared_location = old_location;
			}
		}
	}
}

void Track::change_modules(int old_location, int new_location, int do_swap)
{
	for(int i = 0; i < plugin_set.total; i++)
	{
		for(Plugin *plugin = (Plugin*)plugin_set.values[i]->first; 
			plugin; 
			plugin = (Plugin*)plugin->next)
		{
			if(plugin->plugin_type == PLUGIN_SHAREDPLUGIN ||
				plugin->plugin_type == PLUGIN_SHAREDMODULE)
			{
				if(plugin->shared_location.module == old_location)
					plugin->shared_location.module = new_location;
				else
				if(do_swap && plugin->shared_location.module == new_location)
					plugin->shared_location.module = old_location;
			}
		}
	}
}

int Track::playable_edit()
{
	for(Edit *current = edits->first; current; current = NEXT)
	{
		if((current->transition && current->transition->on) || 
			current->asset ||
			current->nested_edl) return 1;
	}

    return 0;
}

int Track::playable_edit(int64_t position, int direction)
{
	int result = 0;
	if(direction == PLAY_REVERSE) position--;
	for(Edit *current = edits->first; current && !result; current = NEXT)
	{
		if(current->startproject <= position && 
			current->startproject + current->length > position)
		{
//printf("Track::playable_edit %p %p\n", current->transition, current->asset);
			if((current->transition && current->transition->on) || 
				current->asset ||
				current->nested_edl) result = 1;
		}
	}
	return result;
}


int Track::need_edit(Edit *current /*, int test_transitions */)
{
	return ((/* test_transitions && */ current->transition && current->transition->on) ||
		(/* !test_transitions && */ !current->silence()));
}

int64_t Track::plugin_change_duration(int64_t input_position,
	int64_t input_length,
	int reverse,
	int use_nudge)
{
	if(use_nudge) input_position += nudge;
	for(int i = 0; i < plugin_set.total; i++)
	{
		int64_t new_duration = plugin_set.values[i]->plugin_change_duration(
			input_position, 
			input_length, 
			reverse);
		if(new_duration < input_length) input_length = new_duration;
	}
	return input_length;
}

int64_t Track::edit_change_duration(int64_t input_position, 
	int64_t input_length, 
	int reverse, 
//	int test_transitions,
	int use_nudge)
{
	Edit *current;
	int64_t edit_length = input_length;
	if(use_nudge) input_position += nudge;

	if(reverse)
	{
// ================================= Reverse playback

// former method of avoiding a lockup
//        const int fudge = 1;
        const int fudge = 0;
// Get first edit on or after position
		for(current = edits->first; 
			current && current->startproject + current->length < input_position;
			current = NEXT)
			;

		if(current)
		{
			if(current->startproject >= input_position)
			{
// Before this edit
				;
			}
			else
			if(need_edit(current /*, test_transitions */))
			{
// Over an edit with data.
				if(input_position - current->startproject < input_length)
					edit_length = input_position - current->startproject + fudge;

                if(/* test_transitions && */
                    current->transition && 
                    current->transition->on)
                {
                    int64_t transition_end = current->startproject + current->transition->length;
                    if(input_position - transition_end > 0 &&
                        input_position - transition_end < edit_length)
                        edit_length = input_position - transition_end;
                }
// printf("Track::edit_change_duration %d test_transitions=%d input_position=%d edit_length=%d\n",
// __LINE__,
// test_transitions,
// (int)input_position,
// (int)edit_length);
			}
			else
			{
// Over an edit with no data
// Search for next edit with data.
				for(current = PREVIOUS ; 
					current && 
					current->startproject + current->length > input_position - input_length &&
					!need_edit(current /*, test_transitions */);
					current = PREVIOUS)
					;

				if(current && need_edit(current /*, test_transitions */))
                {
                    int64_t edit_end = current->startproject + current->length;
					if(edit_end > input_position - input_length)
                        edit_length = input_position - edit_end + fudge;
                }
			}
		}
		else
		{
// Not over an edit.  Clamp to the last edit.
			current = edits->last;
			if(current)
            {
                int64_t edit_end = edits->last->startproject + edits->last->length;

                if(input_position == edit_length)
                    edit_length = input_position = edits->last->startproject;
                else
    				edit_length = input_position - edit_end + fudge;
// printf("Track::edit_change_duration %d edit_length=%d\n",
// __LINE__,
// (int)edit_length);
            }
		}
	}
	else
	{
// =================================== forward playback
// Get first edit on or before position
		for(current = edits->last; 
			current && current->startproject > input_position;
			current = PREVIOUS)
			;

		if(current)
		{
			if(current->startproject + current->length <= input_position)
			{
// After this edit.
				;
			}
			else
			if(need_edit(current /*, test_transitions */))
			{
// Over an edit with data.
// Next edit is going to require a change.
                int64_t edit_end = current->length + current->startproject;
				if(edit_end - input_position < input_length)
					edit_length = edit_end - input_position;


                if(/* test_transitions && */
                    current->transition &&
                    current->transition->on)
                {
                    int64_t transition_end = current->startproject + current->transition->length;
                    if(transition_end - input_position > 0 &&
                        transition_end - input_position < edit_length)
                        edit_length = transition_end - input_position;
                }

			}
			else
			{
// Over an edit with no data.
// Search for next edit with data.
				for(current = NEXT ; 
					current && 
					current->startproject < input_position + input_length &&
					!need_edit(current /*, test_transitions */);
					current = NEXT)
					;

				if(current && 
					need_edit(current /*, test_transitions */) &&
					current->startproject < input_position + input_length)
					edit_length = current->startproject - input_position;
			}
		}
		else
		{
// Not over an edit.  Clamp to the first edit.
			current = edits->first;
            if(current)
    			edit_length = edits->first->startproject - input_position;
		}
	}

	if(edit_length < input_length)
		return edit_length;
	else
		return input_length;
}

void Track::shuffle_edits(double start, double end, int first_track)
{
	ArrayList<Edit*> new_edits;
	ArrayList<Label*> new_labels;
	int64_t start_units = to_units(start, 0);
	int64_t end_units = to_units(end, 0);
// Edit before range
	Edit *start_edit = 0;
	int have_start_edit = 0;

// Move all edit pointers to list
	for(Edit *current = edits->first; 
		current; )
	{
		if(current->startproject >= start_units &&
			current->startproject + current->length <= end_units)
		{
			if(!have_start_edit) start_edit = current->previous;
			have_start_edit = 1;
			new_edits.append(current);

// Move label pointers
			if(first_track && edl->session->labels_follow_edits)
			{
				double start_seconds = from_units(current->startproject);
				double end_seconds = from_units(current->startproject +
					current->length);
				for(Label *label = edl->labels->first;
					label;
					label = label->next)
				{
					if(label->position >= start_seconds &&
						label->position < end_seconds)
					{
						new_labels.append(label);
						edl->labels->remove_pointer(label);
					}
				}
			}

// Remove edit pointer
			Edit *previous = current;
			current = NEXT;
			edits->remove_pointer(previous);
		}
		else
		{
			current = NEXT;
		}
	}

// Insert pointers in random order
	while(new_edits.size())
	{
		int index = rand() % new_edits.size();
		Edit *edit = new_edits.get(index);
		new_edits.remove_number(index);
        if(!start_edit)
            edits->insert_before(edits->first, edit);
		else
            edits->insert_after(start_edit, edit);
		start_edit = edit;

// Recalculate start position
// Save old position for moving labels
		int64_t startproject1 = edit->startproject;
		int64_t startproject2 = 0;
		if(edit->previous)
		{
			edit->startproject = 
				startproject2 =
				edit->previous->startproject + edit->previous->length;
		}
		else
		{
			edit->startproject = startproject2 = 0;
		}


// Insert label pointers
		if(first_track && edl->session->labels_follow_edits)
		{
			double start_seconds1 = from_units(startproject1);
			double start_seconds2 = from_units(startproject2);
			double end_seconds1 = from_units(edit->startproject +
				edit->length);
			for(int i = new_labels.size() - 1; i >= 0; i--)
			{
				Label *label = new_labels.get(i);
// Was in old edit position
				if(label->position >= start_seconds1 &&
					label->position < end_seconds1)
				{
// Move to new edit position
					double position = label->position - 
						start_seconds1 + 
						start_seconds2;
					edl->labels->insert_label(position);
					new_labels.remove_object_number(i);
				}
			}
		}


	}
	
	optimize();
	
	if(first_track && edl->session->labels_follow_edits)
	{
		edl->labels->optimize();
	}
}

// exactly the same as shuffle_edits except for 1 line
void Track::reverse_edits(double start, double end, int first_track)
{
	ArrayList<Edit*> new_edits;
	ArrayList<Label*> new_labels;
	int64_t start_units = to_units(start, 0);
	int64_t end_units = to_units(end, 0);
// Edit before range
	Edit *start_edit = 0;
	int have_start_edit = 0;

// Move all edit pointers to list
	for(Edit *current = edits->first; 
		current; )
	{
		if(current->startproject >= start_units &&
			current->startproject + current->length <= end_units)
		{
			if(!have_start_edit) start_edit = current->previous;
			have_start_edit = 1;
			new_edits.append(current);

// Move label pointers
			if(first_track && edl->session->labels_follow_edits)
			{
				double start_seconds = from_units(current->startproject);
				double end_seconds = from_units(current->startproject +
					current->length);
				for(Label *label = edl->labels->first;
					label;
					label = label->next)
				{
					if(label->position >= start_seconds &&
						label->position < end_seconds)
					{
						new_labels.append(label);
						edl->labels->remove_pointer(label);
					}
				}
			}

// Remove edit pointer
			Edit *previous = current;
			current = NEXT;
			edits->remove_pointer(previous);
		}
		else
		{
			current = NEXT;
		}
	}

// Insert pointers in reverse order
	while(new_edits.size())
	{
		int index = new_edits.size() - 1;
		Edit *edit = new_edits.get(index);
		new_edits.remove_number(index);
		if(!start_edit)
            edits->insert_before(edits->first, edit);
        else
            edits->insert_after(start_edit, edit);
		start_edit = edit;

// Recalculate start position
// Save old position for moving labels
		int64_t startproject1 = edit->startproject;
		int64_t startproject2 = 0;
		if(edit->previous)
		{
			edit->startproject = 
				startproject2 =
				edit->previous->startproject + edit->previous->length;
		}
		else
		{
			edit->startproject = startproject2 = 0;
		}


// Insert label pointers
		if(first_track && edl->session->labels_follow_edits)
		{
			double start_seconds1 = from_units(startproject1);
			double start_seconds2 = from_units(startproject2);
			double end_seconds1 = from_units(edit->startproject +
				edit->length);
			for(int i = new_labels.size() - 1; i >= 0; i--)
			{
				Label *label = new_labels.get(i);
// Was in old edit position
				if(label->position >= start_seconds1 &&
					label->position < end_seconds1)
				{
// Move to new edit position
					double position = label->position - 
						start_seconds1 + 
						start_seconds2;
					edl->labels->insert_label(position);
					new_labels.remove_object_number(i);
				}
			}
		}


	}
	
	optimize();
	
	if(first_track && edl->session->labels_follow_edits)
	{
		edl->labels->optimize();
	}
}

void Track::align_edits(double start, 
	double end, 
	Track *master_track)
{
	int64_t start_units = to_units(start, 0);
	int64_t end_units = to_units(end, 0);

// All other tracks get silence or cuts to align the edits to the master track.
	Edit *master = master_track->edits->first;
	for(Edit *current = edits->first; 
		current && master; )
	{
// edit is in highlighted region
		if(current->startproject >= start_units &&
			current->startproject + current->length <= end_units)
		{
            int64_t master_length_units = to_units(master_track->from_units(master->length), 0);
// starting times of master edit in the current track units
            int64_t master_start_units = to_units(master_track->from_units(master->startproject), 0);
            int64_t master_startsource = to_units(master_track->from_units(master->startsource), 0);
// starting times of current edit in the current track units
			int64_t current_startunits = current->startproject;
			int64_t current_startsource = current->startsource;

// the following occur if multiple aligns are performed
// master edit is not silence but current edit is silence
            if(!master->silence() &&
                current->silence())
            {
// try again with next edit
                current = NEXT;
                continue;
            }
            else
// master edit is silence but current edit is not silence
            if(master->silence() &&
                !current->silence())
            {
                master = master->next;
                continue;
            }
            else
// current edit is a glitch edit between 2 required edits
            if(current->length < master_length_units / 2 &&
                MWindow::preferences->align_deglitch)
            {
                current = NEXT;
                continue;
            }


// synchronize source position to the master edit
//             printf("Track::align_edits %d master=%ld current=%ld\n",
//                 __LINE__,
//                 (long)master_startsource,
//                 (long)current_startsource);
            if(MWindow::preferences->align_synchronize)
            {
                current->startsource = master_startsource;
            }

// advance it before we add silence
            Edit *prev = current;
            Edit *prev2 = prev->previous;
			current = NEXT;

printf("Track::align_edits %d prev2=%p prev=%p current=%p prev->start=%ld current_startunits=%ld master_start_units=%ld\n",
__LINE__,
prev2,
prev,
current,
(int64_t)prev->startproject,
(int64_t)current_startunits,
(int64_t)master_start_units);

// Current edit starts before master edit
			if(current_startunits < master_start_units)
			{
// shift the keyframes by the full amount
				if(edl->session->autos_follow_edits)
				    shift_keyframes(current_startunits,
					    master_start_units - current_startunits);
                if(edl->session->plugins_follow_edits)
                    shift_effects(current_startunits, 
                        master_start_units - current_startunits, 
                        edl->session->autos_follow_edits);

// extend the previous edit as far as possible into the full amount
                if(prev2 && MWindow::preferences->align_extend)
                {
                    int64_t need = prev2->startsource + 
                        prev2->length + 
                        (master_start_units - 
                        current_startunits);
                    int64_t have = prev2->get_source_end(need);
// printf("Track::align_edits %d need=%ld have=%ld\n",
// __LINE__,
// (int64_t)need,
// (int64_t)have);
                    if(have >= need)
                    {
                        int64_t diff = master_start_units - 
                            prev2->startproject -
                            prev2->length;
                        prev2->length += diff;
// shift the rest down
                        prev2 = prev2->next;
                        while(prev2)
                        {
                            prev2->startproject += diff;
                            prev2 = prev2->next;
                        }
                    }
                    else
                    {
                        int64_t diff = have - 
                            prev2->startsource -
                            prev2->length;
                        prev2->length += diff;
                        current_startunits += diff;
// shift the rest down
                        prev2 = prev2->next;
                        while(prev2)
                        {
                            prev2->startproject += diff;
                            prev2 = prev2->next;
                        }

    				    edits->paste_silence(current_startunits,
	    				    master_start_units);
                    }
                }
                else
                {
    				edits->paste_silence(current_startunits,
	    				master_start_units);
                }
			}
			else
// cut ending if current edit starts after master edit
			if(current_startunits > master_start_units)
			{
				edits->clear(master_start_units,
					current_startunits);
				if(edl->session->autos_follow_edits)
				{
                	shift_keyframes(master_start_units,
						master_start_units - current_startunits);
                }
                if(edl->session->plugins_follow_edits)
                {
// printf("Track::align_edits %d start=%ld diff=%ld\n",
// __LINE__,
// (int64_t)master_start_units,
// (int64_t)current_startunits - master_start_units);
                    shift_effects(master_start_units, 
                        master_start_units - current_startunits, 
                        edl->session->autos_follow_edits);
                }
			}

			master = master->next;
		}
		else
		{
			current = NEXT;
            master = master->next;
		}
	}

	optimize();
}

int Track::purge_asset(Asset *asset)
{
	return 0;
}

int Track::asset_used(Asset *asset)
{
	Edit* current_edit;
	int result = 0;

	for(current_edit = edits->first; current_edit; current_edit = current_edit->next)
	{
		if(current_edit->asset == asset)
		{
			result++;
		}
	}
	return result;
}


int Track::plugin_used(int64_t position, int64_t direction)
{
//printf("Track::plugin_used 1 %d\n", this->plugin_set.total);
	for(int i = 0; i < this->plugin_set.total; i++)
	{
		Plugin *current_plugin = get_current_plugin(position, 
			i, 
			direction, 
			0,
			0);

//printf("Track::plugin_used 2 %p %d %d\n", current_plugin, current_plugin->on, current_plugin->plugin_type);
		if(current_plugin && 
			current_plugin->on && 
			current_plugin->plugin_type != PLUGIN_NONE)
		{
			return 1;
		}
	}
//printf("Track::plugin_used 3 %p\n", current_plugin);
	return 0;
}

// Audio is always rendered through VConsole
int Track::direct_copy_possible(int64_t start, int direction, int use_nudge)
{
	return 1;
}

int64_t Track::to_units(double position, int round)
{
	return (int64_t)position;
}

double Track::to_doubleunits(double position)
{
	return position;
}

double Track::from_units(int64_t position)
{
	return (double)position;
}

int Track::plugin_exists(Plugin *plugin)
{
	for(int number = 0; number < plugin_set.size(); number++)
	{
		PluginSet *ptr = plugin_set.get(number);
		for(Plugin *current_plugin = (Plugin*)ptr->first;
			current_plugin;
			current_plugin = (Plugin*)current_plugin->next)
		{
			if(current_plugin == plugin) return 1;
		}
	}

	for(Edit *current = edits->first; current; current = NEXT)
	{
		if(current->transition &&
            current->transition->on &&
			(Plugin*)current->transition == plugin) return 1;
	}


	return 0;
}


int Track::keyframe_exists(Auto *auto_)
{
    for(int i = 0; i < AUTOMATION_TOTAL; i++)
    {
        Autos *autos = automation->autos[i];
//        if(!autos) printf("Track::keyframe_exists %d: automation type %d doesn't exist\n", __LINE__, i);
// not every track type has every automation type
        if(autos && autos->auto_exists(auto_)) return 1;
    }

	for(int number = 0; number < plugin_set.size(); number++)
	{
		PluginSet *ptr = plugin_set.get(number);
		for(Plugin *current_plugin = (Plugin*)ptr->first;
			current_plugin;
			current_plugin = (Plugin*)current_plugin->next)
		{
			if(current_plugin->keyframes->auto_exists(auto_)) return 1;
		}
	}
    return 0;
}



		
