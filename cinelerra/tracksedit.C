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
#include "assets.h"
#include "atrack.h"
#include "automation.h"
#include "aedits.h"
#include "bcsignals.h"
#include "edit.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "intauto.h"
#include "intautos.h"
#include "keyframe.h"
#include "labels.h"
#include "localsession.h"
#include "mainundo.h"
#include "module.h"
#include "mainsession.h"
#include "mwindow.h"
#include "nestededls.h"
#include "pluginserver.h"
#include "pluginset.h"
#include "plugin.h"
#include "timebar.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "trackscroll.h"
#include "transition.h"
#include "transportque.h"
#include "vtrack.h"
#include <string.h>

int Tracks::clear(double start, double end, int clear_plugins, int edit_autos)
{
	Track *current_track;

	for(current_track = first; 
		current_track; 
		current_track = current_track->next)
	{
		if(current_track->record) 
		{
			current_track->clear(start, 
				end, 
				1, // edits
				1, // labels
				clear_plugins, // edit_plugins
				edit_autos,
				1, // convert_units
				0); // trim_edits
		}
	}
	return 0;
}

void Tracks::clear_automation(double selectionstart, double selectionend)
{
	Track* current_track;

	for(current_track = first; current_track; current_track = current_track->next)
	{
		if(current_track->record)
		{
			current_track->clear_automation(selectionstart, 
				selectionend, 
				0,
				0); 
		}
	}
}

void Tracks::clear_transitions(double start, double end)
{
	for(Track *current_track = first; 
		current_track; 
		current_track = current_track->next)
	{
		if(current_track->record)
		{
			int64_t start_units = current_track->to_units(start, 0);
			int64_t end_units = current_track->to_units(end, 0);

			for(Edit *current_edit = current_track->edits->first;
				current_edit;
				current_edit = current_edit->next)
			{
				if(current_edit->startproject >= start_units &&
					current_edit->startproject < end_units &&
					current_edit->transition)
				{
					current_edit->detach_transition();
				}
			}
		}
	}
}

void Tracks::razor(double start)
{
	for(Track *current_track = first; 
		current_track; 
		current_track = current_track->next)
	{
		if(current_track->record)
		{
			int64_t position = current_track->to_units(start, 0);

// do the edits
            current_track->edits->split_edit(position);

// do the plugins
            if(edl->session->plugins_follow_edits)
            {
                for(int i = 0; i < current_track->plugin_set.size(); i++)
                {
                    PluginSet *ptr = current_track->plugin_set.get(i);
                    ptr->split_edit(position);
                }
            }
		}
	}
}

void Tracks::shuffle_edits(double start, double end)
{
// This doesn't affect automation or effects
// Labels follow the first track.
	int first_track = 1;
	for(Track *current_track = first; 
		current_track; 
		current_track = current_track->next)
	{
		if(current_track->record)
		{
			current_track->shuffle_edits(start, end, first_track);

			first_track = 0;
		}
	}
}

void Tracks::reverse_edits(double start, double end)
{
// This doesn't affect automation or effects
// Labels follow the first track.
	int first_track = 1;
	for(Track *current_track = first; 
		current_track; 
		current_track = current_track->next)
	{
		if(current_track->record)
		{
			current_track->reverse_edits(start, end, first_track);

			first_track = 0;
		}
	}
}
void Tracks::align_edits(double start, double end)
{
// This doesn't affect automation or effects
	Track *master_track = 0;

	for(Track *current_track = first; 
		current_track; 
		current_track = current_track->next)
	{
		if(current_track->record)
		{
            if(!master_track)
            {
                master_track = current_track;
            }
            else
            {
    			current_track->align_edits(start, end, master_track);
            }
		}
	}
}

void Tracks::set_edit_length(double start, double end, double length)
{
	int first_track = 1;
	for(Track *current_track = first; 
		current_track; 
		current_track = current_track->next)
	{
		if(current_track->record)
		{
#define USE_FLOATING_LENGTHS

#ifdef USE_FLOATING_LENGTHS


// The first edit anchors the length offsets.
// Round edits up & down so they end where they would if they all had floating point lengths.
			int first_edit = 1;
			int64_t start_units = current_track->to_units(start, 0);
			int64_t end_units = current_track->to_units(end, 0);
			int64_t length_units = current_track->to_units(length, 1);
// Total time of edits accumulated, in track units
			int64_t total_units = 0;
// Number of length offsets added so far
			int total_lengths = 0;

			for(Edit *current_edit = current_track->edits->last;
				current_edit;
				current_edit = current_edit->previous)
			{
				if(current_edit->startproject >= start_units &&
					current_edit->startproject + current_edit->length <= end_units)
				{
// Calculate true length based on number of length offsets & total time
					double end_time = (1 + total_lengths) * length;
					int64_t length_units = current_track->to_units(end_time, 0) -
						total_units;
					if(length_units < 1) length_units = 1;
printf("Tracks::set_edit_length %d %f %f\n", __LINE__, end_time, current_track->from_units(total_units));
					total_units += length_units;

// Go in using the edit handle interface
					int64_t starting_length = current_edit->length;

					if(length_units < current_edit->length)
					{
						current_edit->shift_end_in(MOVE_ALL_EDITS,
							current_edit->startproject + length_units,
							current_edit->startproject + current_edit->length,
							1,
							edl->session->labels_follow_edits,
							edl->session->plugins_follow_edits,
							edl->session->autos_follow_edits,
							0);
					}
					else
					{
						current_edit->shift_end_out(MOVE_ALL_EDITS,
							current_edit->startproject + length_units,
							current_edit->startproject + current_edit->length,
							1,
							edl->session->labels_follow_edits,
							edl->session->plugins_follow_edits,
							edl->session->autos_follow_edits,
							0);
					}

					int64_t ending_length = current_edit->length;

					if(edl->session->labels_follow_edits && first_track)
					{
// printf("Tracks::set_edit_length %d %f %f\n", 
// __LINE__, 
// current_track->from_units(current_edit->startproject + starting_length),
// current_track->from_units(current_edit->startproject + ending_length));
						 edl->labels->modify_handles(
							current_track->from_units(current_edit->startproject + starting_length),
							current_track->from_units(current_edit->startproject + ending_length),
							1,
							MOVE_ALL_EDITS,
							1);
					}
					
					
					first_edit = 0;
					total_lengths++;
				}
			}



#else // USE_FLOATING_LENGTHS

// The first edit anchors the length offsets.
// The idea was to round edits up & down so they end where they should
// if they all had floating point lengths.  It's easier just to make sure the framerate
// is divisible by the required length.
//			int first_edit = 1;
			int64_t start_units = current_track->to_units(start, 0);
			int64_t end_units = current_track->to_units(end, 0);
			int64_t length_units = current_track->to_units(length, 1);
// Starting time of the length offsets in seconds
//			double start_time = 0;
// Number of length offsets added so far
//			int total_lengths = 0;

			for(Edit *current_edit = current_track->edits->last;
				current_edit;
				current_edit = current_edit->previous)
			{
				if(current_edit->startproject >= start_units &&
					current_edit->startproject + current_edit->length <= end_units)
				{
// Calculate starting time of length offsets
//					if(first_edit)
//					{
//						start_time = current_track->from_units(current_edit->startproject);
//					}

// Calculate true length based on number of length offsets
//					double end_time = start_time + (1 + total_lengths) * length;
//					int64_t length_units = current_track->to_units(end_time, 0) -
//						current_edit->startproject;
//					if(length_units < 1) length_units = 1;

// Go in using the edit handle interface
					int64_t starting_length = current_edit->length;

					if(length_units < current_edit->length)
					{
						current_edit->shift_end_in(MOVE_ALL_EDITS,
							current_edit->startproject + length_units,
							current_edit->startproject + current_edit->length,
							1,
							edl->session->labels_follow_edits,
							edl->session->plugins_follow_edits,
							edl->session->autos_follow_edits,
							0);
					}
					else
					{
						current_edit->shift_end_out(MOVE_ALL_EDITS,
							current_edit->startproject + length_units,
							current_edit->startproject + current_edit->length,
							1,
							edl->session->labels_follow_edits,
							edl->session->plugins_follow_edits,
							edl->session->autos_follow_edits,
							0);
					}

					int64_t ending_length = current_edit->length;

					if(edl->session->labels_follow_edits && first_track)
					{
// printf("Tracks::set_edit_length %d %f %f\n", 
// __LINE__, 
// current_track->from_units(current_edit->startproject + starting_length),
// current_track->from_units(current_edit->startproject + ending_length));
						 edl->labels->modify_handles(
							current_track->from_units(current_edit->startproject + starting_length),
							current_track->from_units(current_edit->startproject + ending_length),
							1,
							MOVE_ALL_EDITS,
							1);
					}
					
					
//					first_edit = 0;
//					total_lengths++;
				}
			}
#endif // !USE_FLOATING_LENGTHS

			first_track = 0;
		}
	}
}

void Tracks::swap_assets(double start, 
    double end, 
    string *old_path, 
    string *new_path, 
    int old_is_silence,
    int new_is_silence)
{
printf("Tracks::swap_assets %d\n", __LINE__);
	for(Track *current_track = first; 
		current_track; 
		current_track = current_track->next)
	{
		if(current_track->record)
		{
			int64_t start_units = current_track->to_units(start, 0);
			int64_t end_units = current_track->to_units(end, 0);
    
			for(Edit *current_edit = current_track->edits->last;
				current_edit;
				current_edit = current_edit->previous)
			{
				if(current_edit->startproject >= start_units &&
					current_edit->startproject + current_edit->length <= end_units)
				{
                    if((old_is_silence && 
                            !current_edit->asset &&
                            !current_edit->nested_edl) ||
                        (!old_is_silence &&
                            current_edit->asset &&
                            !old_path->compare(current_edit->asset->path)) ||
                        (!old_is_silence &&
                            current_edit->nested_edl &&
                            !old_path->compare(current_edit->nested_edl->path)))
                    {
                        if(new_is_silence)
                        {
                            current_edit->asset = 0;
                            current_edit->nested_edl = 0;
                        }
                        else
                        {
                            Asset *asset = edl->assets->get_asset(new_path->c_str());
                            EDL *nested_edl = 0;
                            if(!asset)
                            {
                                nested_edl = edl->nested_edls->search(new_path->c_str());
                            }
                            current_edit->asset = asset;
                            current_edit->nested_edl = nested_edl;
                        }
                    }
                }
            }
        }
    }
}


void Tracks::set_transition_length(double start, double end, double length)
{
	for(Track *current_track = first; 
		current_track; 
		current_track = current_track->next)
	{
		if(current_track->record)
		{
			int64_t start_units = current_track->to_units(start, 0);
			int64_t end_units = current_track->to_units(end, 0);

			for(Edit *current_edit = current_track->edits->first;
				current_edit;
				current_edit = current_edit->next)
			{
				if(current_edit->startproject >= start_units &&
					current_edit->startproject < end_units &&
					current_edit->transition)
				{
					current_edit->transition->length = 
						current_track->to_units(length, 1);
				}
			}
		}
	}
}

void Tracks::set_transition_length(Transition *transition, double length)
{
// Must verify existence of transition
	int done = 0;
	if(!transition) return;
	for(Track *current_track = first; 
		current_track && !done; 
		current_track = current_track->next)
	{
		for(Edit *current_edit = current_track->edits->first;
			current_edit && !done;
			current_edit = current_edit->next)
		{
			if(current_edit->transition == transition)
			{
				transition->length = current_track->to_units(length, 1);
				done = 1;
			}
		}
	}
}

void Tracks::paste_transitions(double start, 
    double end, 
    int track_type, 
    char* title,
    KeyFrame *keyframe)
{
	for(Track *current_track = first; 
		current_track; 
		current_track = current_track->next)
	{
		if(current_track->record && current_track->data_type == track_type)
		{
			int64_t start_units = current_track->to_units(start, 0);
			int64_t end_units = current_track->to_units(end, 0);

			for(Edit *current_edit = current_track->edits->first;
				current_edit;
				current_edit = current_edit->next)
			{
				if(current_edit->startproject > 0 &&
					((end_units > start_units &&
					current_edit->startproject >= start_units &&
					current_edit->startproject < end_units) ||
					(end_units == start_units &&
					current_edit->startproject <= start_units &&
					current_edit->startproject + current_edit->length > start_units)))
				{
					current_edit->insert_transition(title, keyframe);
				}
			}
		}
	}
}

void Tracks::set_automation_mode(double selectionstart, 
	double selectionend,
	int mode)
{
	Track* current_track;

	for(current_track = first; current_track; current_track = current_track->next)
	{
		if(current_track->record)
		{
			current_track->set_automation_mode(selectionstart, 
				selectionend,
				mode); 
		}
	}
}

// int Tracks::clear_default_keyframe()
// {
// 	for(Track *current = first; current; current = NEXT)
// 	{
// 		if(current->record)
// 			current->clear_automation(0, 0, 0, 1);
// 	}
// 	return 0;
// }

int Tracks::clear_handle(double start, 
	double end,
	double &longest_distance,
	int clear_labels,
	int clear_plugins,
	int edit_autos)
{
	Track* current_track;
	double distance;

	for(current_track = first; current_track; current_track = current_track->next)
	{
		if(current_track->record)
		{
			current_track->clear_handle(start, 
				end, 
				clear_labels,
				clear_plugins, 
				edit_autos,
				distance);
			if(distance > longest_distance) longest_distance = distance;
		}
	}

	return 0;
}

int Tracks::copy_automation(double selectionstart, 
	double selectionend, 
	FileXML *file,
	int default_only,
	int autos_only)
{
// called by MWindow::copy_automation for copying automation alone
	Track* current_track;


    edl->start_auto_copy(file, selectionstart, selectionend);
	for(current_track = first; 
		current_track; 
		current_track = current_track->next)
	{
		if(current_track->record)
		{
			current_track->copy_automation(selectionstart, 
				selectionend, 
				file,
				default_only,
				autos_only);
		}
	}
    edl->end_auto_copy(file);
	return 0;
}

// int Tracks::copy_default_keyframe(FileXML *file)
// {
// 	copy_automation(0, 0, file, 1, 0);
// 	return 0;
// }

int Tracks::delete_tracks()
{
	int total_deleted = 0;
	int done = 0;

	while(!done)
	{
		done = 1;
		Track *next_track = 0;
		for (Track* current = first;
			current && done;
			current = next_track)
		{
			next_track = current->next;
			if(current->record)
			{
				delete_track(current);
				total_deleted++;
				done = 0;
			}
		}
	}
	return total_deleted;
}

void Tracks::move_edits(ArrayList<Edit*> *edits, 
	Track *track,
	double position,
	int edit_labels,  // Ignored
	int edit_plugins, 
	int edit_autos)
{
    int overwrite_mode = (MWindow::session->drag_button != LEFT_BUTTON);
//printf("Tracks::move_edits %d overwrite_mode=%d\n", __LINE__, overwrite_mode);
	for(Track *dest_track = track; dest_track; dest_track = dest_track->next)
	{
		if(dest_track->record)
		{
// Need a local copy of the source edit since the original source edit may
// change in the editing operation.
			Edit *source_edit = 0;
			Track *source_track = 0;


// Get source track & edit
			if(dest_track->data_type == TRACK_AUDIO)
			{
				int current_aedit = 0;

				while(current_aedit < edits->size() &&
					edits->get(current_aedit)->track->data_type != TRACK_AUDIO)
					current_aedit++;

				if(current_aedit < edits->size())
				{
					source_edit = edits->get(current_aedit);
					source_track = source_edit->track;
// take it off the source list
					edits->remove_number(current_aedit);
				}
			}
			else
			if(dest_track->data_type == TRACK_VIDEO)
			{
				int current_vedit = 0;
				while(current_vedit < edits->size() &&
					edits->get(current_vedit)->track->data_type != TRACK_VIDEO)
					current_vedit++;

				if(current_vedit < edits->size())
				{
					source_edit = edits->get(current_vedit);
					source_track = source_edit->track;
// take it off the source list
					edits->remove_number(current_vedit);
				}
			}

//printf("Tracks::move_edits 2 %s %s %d\n", source_track->title, dest_track->title, source_edit->length);
			if(source_edit)
			{
// align the positions to frames
                double aligned_position = edl->align_to_frame(position, 0);

// convert the positions into media units
 				int64_t position_i = source_track->to_units(aligned_position, 0);
 				int64_t source_length = source_edit->length;

// handle free dragging into the same track & with insert mode
                if(MWindow::session->free_drag &&
                    source_track == dest_track &&
                    !overwrite_mode)
                {
                    int64_t length = 0;
                    int64_t src_start;
                    int64_t dst_start;
                    int is_before = 0;
// move stuff from the right to the left of the source edit
                    if(position_i > source_edit->startproject)
                    {
                        length = position_i - source_edit->startproject;
                        src_start = source_edit->startproject + source_edit->length;
                        dst_start = source_edit->startproject;
                        is_before = 1;
                    }
                    else
// move stuff from the left to the right of the source edit
                    if(position_i < source_edit->startproject)
                    {
                        length = source_edit->startproject - position_i;
                        src_start = source_edit->startproject - length;
                        dst_start = position_i + source_edit->length;
                    }

// do nothing if we didn't move
                    if(length > 0)
                    {
                        if(edit_autos)
                        {
                            FileXML temp;
                            AutoConf temp_autoconf;
                            temp_autoconf.set_all(1);
// cut automation from source position
                            source_track->automation->copy(src_start, 
    					        src_start + length, 
    					        &temp, 
    					        0,
    					        1);
    				        temp.terminate_string();
    				        temp.rewind();
				            source_track->automation->clear(src_start,
					            src_start + length, 
					            &temp_autoconf,
					            1);

// paste automation in dest position
                            dest_track->automation->paste_silence(dst_start, 
					            dst_start + length);
// each automation type is pasted separately
                            while(!temp.read_tag())
					            dest_track->automation->paste(dst_start, 
						            length, 
						            1.0, 
						            &temp, 
						            0,
						            1,
						            &temp_autoconf);
                        }

// cut edits from source position
                        FileXML temp;
                        source_track->edits->copy(src_start, 
                            src_start + length, 
                            &temp, 
                            0); // output_path
                        temp.terminate_string();
    				    temp.rewind();
                        source_track->edits->clear(src_start, 
                            src_start + length);
//                        temp.dump();
// paste edits in dest position
                        Edits *source_edits;
                        source_edits = dest_track->new_edits();
                        source_edits->load(&temp, 0);
                        dest_track->edits->insert_edits(source_edits, 
	                        dst_start,
	                        length,
	                        0); // edit_autos

// join newly created edits.  This also joins all the razor tools but
// it's far simpler than optimizing only the affected range.
                        dest_track->edits->optimize(0);
                    }
                }
                else
// Handle constrained dragging, 
// free dragging into a different track,
// or all free dragging with overwrite mode.
// Do nothing if we're constrained dragging into the same track, 
// within 1 edit after the source edit
                if(!(!MWindow::session->free_drag &&
                    source_track == dest_track &&
                    position_i >= source_edit->startproject &&
                        (source_edit->next == 0 ||
                        position_i < source_edit->next->startproject + 
                            source_edit->next->length)))
                {
                    if(edit_autos)
                    {
				        int64_t position_a = position_i;
// Copy keyframes from source edit
    				    FileXML temp;
    				    AutoConf temp_autoconf;
    				    temp_autoconf.set_all(1);
    				    source_track->automation->copy(source_edit->startproject, 
    					    source_edit->startproject + source_length, 
    					    &temp, 
    					    0,
    					    1);
    				    temp.terminate_string();
    				    temp.rewind();

// delete source keyframes if not free dragging with overwrite
				        if(!(MWindow::session->free_drag && overwrite_mode))
                        {
                            source_track->automation->clear(source_edit->startproject,
					            source_edit->startproject + source_length, 
					            &temp_autoconf,
					            1); // shift_autos
// shift destination position if it's later in the source track
				            if(dest_track == source_track &&
                                position_a > source_edit->startproject + source_length)
                            {
                                position_a -= source_length;
                            }
                        }
                        else
                        {
// mute source keyframes if free dragging with overwrite
                            source_track->automation->clear(source_edit->startproject,
					            source_edit->startproject + source_length, 
					            &temp_autoconf,
					            0); // shift_autos
                        }

// clear destination keyframes if overwriting
                        if(overwrite_mode &&
                            dest_track != source_track)
                        {
                            dest_track->automation->clear(position_a,
                                position_a + source_length,
                                &temp_autoconf,
					            1);
                        }

// paste automation
				        dest_track->automation->paste_silence(position_a, 
					        position_a + source_length);
				        while(!temp.read_tag())
					        dest_track->automation->paste(position_a, 
						        source_length, 
						        1.0, 
						        &temp, 
						        0,
						        1,
						        &temp_autoconf);
                    }

// Remove source edit from source track
                    source_track->edits->remove_pointer(source_edit);

// shift positions of edits in the source track
                    for(Edit *current = source_edit->next; current; current = current->next)
                    {
                        current->startproject -= source_length;
                    }

                    if(!(MWindow::session->free_drag && overwrite_mode))
                    {
// join the neighbors in the source track if they're contiguous
                        source_track->edits->join(source_edit->previous, 
                            source_edit->next,
                            0);

// shift destination position if it's later in the source track
                        if(source_track == dest_track &&
                            position_i > source_edit->startproject + source_length)
                            position_i -= source_length;
                    }
                    else
// replace source edit with silence if free dragging with overwrite
                    {
                        int64_t source_start = source_edit->startproject;
                        source_track->edits->paste_silence(source_start, 
                            source_start + source_length);
                    }

// printf("Tracks::move_edits %d drag_button=%d\n", 
// __LINE__, MWindow::session->drag_button);
// clear the destination if we're overwriting
                    if(overwrite_mode /* &&
                        dest_track != source_track */)
                    {
                        dest_track->edits->clear(position_i,
                            position_i + source_length);
                    }

// insert the source edit into the destination track
                    Edit *dest_edit = dest_track->edits->paste_silence(position_i,
                        position_i + source_length);
                    dest_edit->copy_from(source_edit);
                    dest_edit->startproject = position_i;
                    delete source_edit;
                    source_track->optimize();
                    dest_track->optimize();
                }
			}
		}
	}
}

void Tracks::move_effect(Plugin *plugin,
	PluginSet *dest_plugin_set,
	Track *dest_track, 
	int64_t dest_position)
{
	Track *source_track = plugin->track;
	Plugin *result = 0;

// Insert on an existing plugin set
	if(!dest_track && dest_plugin_set)
	{
		Track *dest_track = dest_plugin_set->track;


// Assume this operation never splits a plugin
// Shift destination plugins back
		dest_plugin_set->shift(dest_position, plugin->length);

// Insert new plugin
		Plugin *current = 0;
		for(current = (Plugin*)dest_plugin_set->first; current; current = (Plugin*)NEXT)
			if(current->startproject >= dest_position) break;

		result = (Plugin*)dest_plugin_set->insert_before(current, 
			new Plugin(edl, dest_plugin_set, ""));
	}
	else
// Create a new plugin set
	{
		double length = 0;
		double start = 0;
		if(edl->local_session->get_selectionend() > 
			edl->local_session->get_selectionstart())
		{
			start = edl->local_session->get_selectionstart();
			length = edl->local_session->get_selectionend() - 
				start;
		}
		else
		if(dest_track->get_length() > 0)
		{
			start = 0;
			length = dest_track->get_length();
		}
		else
		{
			start = 0;
			length = dest_track->from_units(plugin->length);
		}


		result = dest_track->insert_effect("", 
				&plugin->shared_location, 
				0,
				0,
				start,
				length,
				plugin->plugin_type);
	}



	result->copy_from(plugin);
	result->shift(dest_position - plugin->startproject);

// Clear new plugin from old set
	plugin->plugin_set->clear(plugin->startproject, 
		plugin->startproject + plugin->length,
		edl->session->autos_follow_edits);


	source_track->optimize();
}


int Tracks::concatenate_tracks(int edit_plugins, int edit_autos)
{
	Track *output_track, *first_output_track, *input_track;
	int i, data_type = TRACK_AUDIO;
	double output_start;
	FileXML *clipboard;
	int result = 0;
	IntAuto *play_keyframe = 0;

// Relocate tracks
	for(i = 0; i < 2; i++)
	{
// Get first output track
		for(output_track = first; 
			output_track; 
			output_track = output_track->next)
			if(output_track->data_type == data_type && 
				output_track->record) break;

		first_output_track = output_track;

// Get first input track
		for(input_track = first;
			input_track;
			input_track = input_track->next)
		{
			if(input_track->data_type == data_type &&
				input_track->play && 
				!input_track->record) break;
		}


		if(output_track && input_track)
		{
// Transfer input track to end of output track one at a time
			while(input_track)
			{
				output_start = output_track->get_length();
				output_track->insert_track(input_track, 
					output_start, 
					0,
					edit_plugins,
					edit_autos,
					0);

// Get next source and destination
				for(input_track = input_track->next; 
					input_track; 
					input_track = input_track->next)
				{

					if(input_track->data_type == data_type && 
						!input_track->record && 
						input_track->play) break;
				}

				for(output_track = output_track->next; 
					output_track; 
					output_track = output_track->next)
				{
					if(output_track->data_type == data_type && 
						output_track->record) break;
				}

				if(!output_track)
				{
					output_track = first_output_track;
				}
			}
			result = 1;
		}

		if(data_type == TRACK_AUDIO) data_type = TRACK_VIDEO;
	}

	return result;
}

int Tracks::delete_all_tracks()
{
	while(last) delete last;
	return 0;
}


void Tracks::change_modules(int old_location, int new_location, int do_swap)
{
	for(Track* current = first ; current; current = current->next)
	{
		current->change_modules(old_location, new_location, do_swap);
	}
}

void Tracks::change_plugins(SharedLocation &old_location, SharedLocation &new_location, int do_swap)
{
	for(Track* current = first ; current; current = current->next)
	{
		current->change_plugins(old_location, new_location, do_swap);
	}
}



// =========================================== EDL editing


int Tracks::copy(double start, 
	double end, 
	int all, 
	FileXML *file, 
	const char *output_path)
{
// nothing selected
	if(start == end && !all) return 1;

	Track* current;

	for(current = first; 
		current; 
		current = NEXT)
	{
		if(current->record || all)
		{
			current->copy(start, end, file,output_path);
		}
	}

	return 0;
}



int Tracks::move_track_up(Track *track)
{
	Track *next_track = track->previous;
	if(!next_track) next_track = last;

	change_modules(number_of(track), number_of(next_track), 1);

// printf("Tracks::move_track_up 1 %p %p\n", track, next_track);
// int count = 0;
// for(Track *current = first; current && count < 5; current = NEXT, count++)
// 	printf("Tracks::move_track_up %p %p %p\n", current->previous, current, current->next);
// printf("Tracks::move_track_up 2\n");
// 
	swap(track, next_track);

// count = 0;
// for(Track *current = first; current && count < 5; current = NEXT, count++)
// 	printf("Tracks::move_track_up %p %p %p\n", current->previous, current, current->next);
// printf("Tracks::move_track_up 3\n");

	return 0;
}

int Tracks::move_track_down(Track *track)
{
	Track *next_track = track->next;
	if(!next_track) next_track = first;

	change_modules(number_of(track), number_of(next_track), 1);
	swap(track, next_track);
	return 0;
}


int Tracks::move_tracks_up()
{
	Track *track, *next_track;
	int result = 0;

	for(track = first;
		track; 
		track = next_track)
	{
		next_track = track->next;

		if(track->record)
		{
			if(track->previous)
			{
				change_modules(number_of(track->previous), number_of(track), 1);

				swap(track->previous, track);
				result = 1;
			}
		}
	}

	return result;
}

int Tracks::move_tracks_down()
{
	Track *track, *previous_track;
	int result = 0;
	
	for(track = last;
		track; 
		track = previous_track)
	{
		previous_track = track->previous;

		if(track->record)
		{
			if(track->next)
			{
				change_modules(number_of(track), number_of(track->next), 1);

				swap(track, track->next);
				result = 1;
			}
		}
	}
	
	return result;
}



void Tracks::paste_automation(double selectionstart, 
	FileXML *file,
	int default_only,
	int active_only,
	int typeless)
{
	Track* current_track = 0;
	Track* current_atrack = 0;
	Track* current_vtrack = 0;
	Track* dst_track = 0;
	int src_type;
	int result = 0;
	double length;
	double frame_rate = edl->session->frame_rate;
	int64_t sample_rate = edl->session->sample_rate;
	char string[BCTEXTLEN];
	string[0] = 0;

// Search for start
	do{
	  result = file->read_tag();
	}while(!result && 
		!file->tag.title_is("AUTO_CLIPBOARD"));

	if(!result)
	{
		length = file->tag.get_property("LENGTH", 0);
		frame_rate = file->tag.get_property("FRAMERATE", frame_rate);
		sample_rate = file->tag.get_property("SAMPLERATE", sample_rate);


		do
		{
			result = file->read_tag();

			if(!result)
			{
				if(file->tag.title_is("/AUTO_CLIPBOARD"))
				{
					result = 1;
				}
				else
				if(file->tag.title_is("TRACK"))
				{
					file->tag.get_property("TYPE", string);
					if(!strcmp(string, "AUDIO"))
					{
						src_type = TRACK_AUDIO;
					}
					else
					{
						src_type = TRACK_VIDEO;
					}

// paste to any media type
					if(typeless)
					{
						if(!current_track) current_track = first;
						while(current_track && !current_track->record)
							current_track = current_track->next;
						dst_track = current_track;
					}
					else
					if(!strcmp(string, "AUDIO"))
					{
// Get next audio track
						if(!current_atrack)
							current_atrack = first;
						else
							current_atrack = current_atrack->next;

						while(current_atrack && 
							(current_atrack->data_type != TRACK_AUDIO ||
							!current_atrack->record))
							current_atrack = current_atrack->next;
						dst_track = current_atrack;
					}
					else
					{
// Get next video track
						if(!current_vtrack)
							current_vtrack = first;
						else
							current_vtrack = current_vtrack->next;

						while(current_vtrack && 
							(current_vtrack->data_type != TRACK_VIDEO ||
							!current_vtrack->record))
							current_vtrack = current_vtrack->next;

						dst_track = current_vtrack;
					}

					if(dst_track)
					{
						double frame_rate2 = frame_rate;
						double sample_rate2 = sample_rate;
						
						if(src_type != dst_track->data_type)
						{
							frame_rate2 = sample_rate;
							sample_rate2 = frame_rate;
						}
						
						dst_track->paste_automation(selectionstart,
							length,
							frame_rate2,
							sample_rate2,
							file,
							default_only,
							active_only);
					}
				}
			}
		}while(!result);
	}
}

// int Tracks::paste_default_keyframe(FileXML *file)
// {
// 	paste_automation(0, file, 1, 0);
// 	return 0;
// }

// void Tracks::paste_transition(PluginServer *server, 
//     Edit *dest_edit,
//     KeyFrame *keyframe)
// {
// 	dest_edit->insert_transition(server->title, keyframe);
// }


void Tracks::paste_transition(PluginServer *server, 
    int data_type,
    int first_track,
    KeyFrame *keyframe)
{
	for(Track *current = first; current; current = NEXT)
	{
		if(current->data_type == data_type &&
			current->record)
		{
			int64_t position = current->to_units(
				edl->local_session->get_selectionstart(), 0);
			Edit *current_edit = current->edits->editof(position, 
				PLAY_FORWARD,
				0);
			if(current_edit)
			{
                current_edit->insert_transition(server->title, keyframe);
//				paste_transition(server, current_edit, keyframe);
			}
			if(first_track) break;
		}
	}
}

// void Tracks::paste_video_transition(PluginServer *server, 
//     int first_track,
//     KeyFrame *keyframe)
// {
// 	for(Track *current = first; current; current = NEXT)
// 	{
// 		if(current->data_type == TRACK_VIDEO &&
// 			current->record)
// 		{
// 			int64_t position = current->to_units(
// 				edl->local_session->get_selectionstart(), 0);
// 			Edit *current_edit = current->edits->editof(position, 
// 				PLAY_FORWARD,
// 				0);
// 			if(current_edit)
// 			{
//                 current_edit->insert_transition(server->title, keyframe);
// //				paste_transition(server, current_edit, keyframe);
// 			}
// 			if(first_track) break;
// 		}
// 	}
// }


int Tracks::paste_silence(double start, 
	double end, 
	int edit_plugins, 
	int edit_autos)
{
	Track* current_track;

	for(current_track = first; 
		current_track; 
		current_track = current_track->next)
	{
		if(current_track->record) 
		{ 
			current_track->paste_silence(start, 
				end, 
				edit_plugins, 
				edit_autos); 
		}
	}
	return 0;
}



int Tracks::select_auto(int cursor_x, int cursor_y)
{
	int result = 0;
	for(Track* current = first; current && !result; current = NEXT) { result = current->select_auto(&auto_conf, cursor_x, cursor_y); }
	return result;
}

int Tracks::move_auto(int cursor_x, int cursor_y, int shift_down)
{
	int result = 0;

	for(Track* current = first; current && !result; current = NEXT) 
	{
		result = current->move_auto(&auto_conf, cursor_x, cursor_y, shift_down); 
	}
	return 0;
}

int Tracks::modify_edithandles(double &oldposition, 
	double &newposition, 
	int currentend, 
	int handle_mode,
	int edit_labels,
	int edit_plugins,
	int edit_autos)
{
	Track *current;

	for(current = first; current; current = NEXT)
	{
		if(current->record)
		{
			current->modify_edithandles(oldposition, 
				newposition, 
				currentend,
				handle_mode,
				edit_labels,
				edit_plugins,
				edit_autos);
		}
	}
	return 0;
}

int Tracks::modify_pluginhandles(double &oldposition, 
	double &newposition, 
	int currentend, 
	int handle_mode,
	int edit_labels,
	int edit_autos,
	Edits *trim_edits)
{
	Track *current;

	for(current = first; current; current = NEXT)
	{
		if(current->record)
		{
			current->modify_pluginhandles(oldposition, 
				newposition, 
				currentend, 
				handle_mode,
				edit_labels,
				edit_autos,
				trim_edits);
		}
	}
	return 0;
}



int Tracks::purge_asset(Asset *asset)
{
	Track *current_track;
	int result = 0;
	
	for(current_track = first; current_track; current_track = current_track->next)
	{
		result += current_track->purge_asset(asset); 
	}
	return result;
}

int Tracks::asset_used(Asset *asset)
{
	Track *current_track;
	int result = 0;
	
	for(current_track = first; current_track; current_track = current_track->next)
	{
		result += current_track->asset_used(asset); 
	}
	return result;
}

int Tracks::scale_time(float rate_scale, int ignore_record, int scale_edits, int scale_autos, int64_t start, int64_t end)
{
	Track *current_track;

	for(current_track = first; 
		current_track; 
		current_track = current_track->next)
	{
		if((current_track->record || ignore_record) && 
			current_track->data_type == TRACK_VIDEO)
		{
			current_track->scale_time(rate_scale, scale_edits, scale_autos, start, end);
		}
	}
	return 0;
}

