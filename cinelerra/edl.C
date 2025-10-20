/*
 * CINELERRA
 * Copyright (C) 1997-2022 Adam Williams <broadcast at earthling dot net>
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
#include "autoconf.h"
#include "automation.h"
#include "awindowgui.inc"
#include "bcsignals.h"
#include "clip.h"
#include "colormodels.h"
#include "bchash.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "filesystem.h"
#include "guicast.h"
#include "indexstate.h"
#include "labels.h"
#include "localsession.h"
#include "mainsession.h"
#include "mutex.h"
#include "mwindow.h"
#include "nestededls.h"
#include "panauto.h"
#include "panautos.h"
#include "playbackconfig.h"
#include "playabletracks.h"
#include "plugin.h"
#include "preferences.h"
#include "recordconfig.h"
#include "recordlabel.h"
#include "sharedlocation.h"
#include "theme.h"
#include "tracks.h"
#include "transition.h"
#include "transportque.inc"
#include "vedit.h"
#include "vtrack.h"




Mutex* EDL::id_lock = 0;



EDL::EDL(EDL *parent_edl)
 : Indexable(0)
{
	this->parent_edl = parent_edl;
    nested_depth = 0;
	tracks = 0;
	labels = 0;
	local_session = 0;
//	vwindow_edl = 0;
//	vwindow_edl_shared = 0;

	folders.set_array_delete();

	new_folder(CLIP_FOLDER);

	new_folder(MEDIA_FOLDER);

	id = next_id();
	path[0] = 0;
}


EDL::~EDL()
{

	if(tracks)
	{
		delete tracks;
	}
	if(labels)
	{
		delete labels;
	}

	if(local_session)
	{
		delete local_session;
	}


//	remove_vwindow_edls();

//	if(vwindow_edl && !vwindow_edl_shared)
//		vwindow_edl->Garbage::remove_user();

	if(!parent_edl)
	{
		delete assets;
		delete session;
	}


	folders.remove_all_objects();
	for(int i = 0; i < clips.size(); i++)
		clips.get(i)->Garbage::remove_user();
	clips.remove_all();
	delete nested_edls;
}



void EDL::create_objects()
{
	tracks = new Tracks(this);
	if(!parent_edl)
	{
		assets = new Assets(this);
		session = new EDLSession(this);
	}
	else
	{
		assets = parent_edl->assets;
		session = parent_edl->session;
	}
	
	local_session = new LocalSession(this);
	labels = new Labels(this, "LABELS");
	nested_edls = new NestedEDLs(this);
//	last_playback_position = 0;
}

EDL& EDL::operator=(EDL &edl)
{
printf("EDL::operator= 1\n");
	copy_all(&edl);
	return *this;
}

int EDL::load_defaults(BC_Hash *defaults)
{
	if(!parent_edl)
    {
		session->load_defaults(defaults);
    }

	local_session->load_defaults(defaults);
	return 0;
}

int EDL::save_defaults(BC_Hash *defaults)
{
	if(!parent_edl)
    {
		session->save_defaults(defaults);
    }
	
	local_session->save_defaults(defaults);
	return 0;
}

void EDL::boundaries()
{
	session->boundaries();
	local_session->boundaries();
}

int EDL::create_default_tracks()
{

	for(int i = 0; i < session->video_tracks; i++)
	{
		tracks->add_video_track(0, 0);
	}
	for(int i = 0; i < session->audio_tracks; i++)
	{
		tracks->add_audio_track(0, 0);
	}
	return 0;
}

int EDL::load_xml(FileXML *file, uint32_t load_flags)
{
	int result = 0;
// Track numbering offset for replacing undo data.
	int track_offset = 0;
    int error = 0;


// Clear objects
	folders.remove_all_objects();

// 	if((load_flags & LOAD_ALL) == LOAD_ALL)	
// 	{
// 		remove_vwindow_edls();
// 	}


// Search for start of master EDL.

// The parent_edl test caused clip creation to fail since those XML files
// contained an EDL tag.

// The parent_edl test is required to make EDL loading work because
// when loading an EDL the EDL tag is already read by the parent.

	if(!parent_edl)
	{
		do{
		  result = file->read_tag();
//printf("EDL::load_xml %d %s\n", __LINE__, file->tag.get_title());
		}while(!result && 
			!file->tag.title_is("XML") && 
			!file->tag.title_is("EDL"));
	}

	if(!result)
	{
// Get path for the case of restoring a backup
//		path[0] = 0;
		file->tag.get_property("PATH", path);

// Erase everything
// 		if((load_flags & LOAD_ALL) == LOAD_ALL ||
// 			(load_flags & LOAD_EDITS) == LOAD_EDITS)
// 		{
// 			while(tracks->last) delete tracks->last;
// 		}

// Move all the tracks to pools of 1 type for reuse
        Track *current = tracks->first;
        Tracks atracks;
        Tracks vtracks;
        while(current)
        {
            Track *track2 = NEXT;
            tracks->remove_pointer(current);

            if(current->data_type == TRACK_AUDIO)
                atracks.append(current);
            else
                vtracks.append(current);

            current = track2;
        }
        Track *current_atrack = atracks.first;
        Track *current_vtrack = vtracks.first;

		if((load_flags & LOAD_ALL) == LOAD_ALL)
		{
			for(int i = 0; i < clips.size(); i++)
				clips.get(i)->Garbage::remove_user();
			clips.remove_all();
		}

		if(load_flags & LOAD_TIMEBAR)
		{
			while(labels->last) delete labels->last;
			local_session->unset_inpoint();
			local_session->unset_outpoint();
		}

// This was originally in LocalSession::load_xml
		if(load_flags & LOAD_SESSION)
		{
			local_session->clipboard_length = 0;
		}

		do{
			result = file->read_tag();
            if(result) break;

			if(file->tag.title_is("/XML") ||
				file->tag.title_is("/EDL") ||
				file->tag.title_is("/CLIP_EDL") ||
				file->tag.title_is("/VWINDOW_EDL"))
			{
				result = 1;
			}
			else
			if(file->tag.title_is("CLIPBOARD"))
			{
				local_session->clipboard_length = 
					file->tag.get_property("LENGTH", (double)0);
			}
			else
			if(file->tag.title_is("VIDEO"))
			{
				if((load_flags & LOAD_VCONFIG) &&
					(load_flags & LOAD_SESSION))
					session->load_video_config(file, 0, load_flags);
			}
			else
			if(file->tag.title_is("AUDIO"))
			{
				if((load_flags & LOAD_ACONFIG) &&
					(load_flags & LOAD_SESSION))
					session->load_audio_config(file, 0, load_flags);
			}
			else
			if(file->tag.title_is("FOLDER"))
			{
				char folder[BCTEXTLEN];
				strcpy(folder, file->read_text());
				new_folder(folder);
			}
			else
			if(file->tag.title_is("ASSETS"))
			{
				assets->load(file);
			}
			else
			if(file->tag.title_is("NESTED_EDLS"))
			{
				error |= nested_edls->load(file);
			}
			else
			if(file->tag.title_is(labels->xml_tag))
			{
				if(load_flags & LOAD_TIMEBAR)
				{
                    labels->load(file, load_flags);
                }
			}
			else
			if(file->tag.title_is("LOCALSESSION"))
			{
				if((load_flags & LOAD_SESSION) ||
					(load_flags & LOAD_TIMEBAR))
                {
					local_session->load_xml(file, load_flags);
                }
			}
			else
			if(file->tag.title_is("SESSION"))
			{
				if((load_flags & LOAD_SESSION) &&
					!parent_edl)
                {
					session->load_xml(file, 0, load_flags);
                }
			}
			else
			if(file->tag.title_is("TRACK"))
			{
                char string[BCTEXTLEN];
                file->tag.get_property("TYPE", string);

// reuse a previous track before creating a new one
		        if(!strcasecmp(string, "VIDEO"))
		        {
                    if(current_vtrack)
                    {
                        Track *track2 = current_vtrack->next;
                        vtracks.remove_pointer(current_vtrack);
                        tracks->append(current_vtrack);
                        current_vtrack = track2;
			        }
                    else
                        tracks->add_video_track(0, 0);
		        }
		        else
		        {
                    if(current_atrack)
                    {
                        Track *track2 = current_atrack->next;
                        atracks.remove_pointer(current_atrack);
                        tracks->append(current_atrack);
                        current_atrack = track2;
                    }
                    else
    			        tracks->add_audio_track(0, 0);
		        }

// load it
				error |= tracks->last->load(file, track_offset);
			}
			else
// Sub EDL.
// Causes clip creation to fail because that involves an opening EDL tag.
			if(file->tag.title_is("CLIP_EDL") && !parent_edl)
			{
				EDL *new_edl = new EDL(this);
				new_edl->create_objects();
				new_edl->load_xml(file, LOAD_ALL);

				if((load_flags & LOAD_ALL) == LOAD_ALL)
				{
                    clips.append(new_edl);
				}
                else
				{
                    new_edl->Garbage::remove_user();
                }
			}
			else
			if(file->tag.title_is("VWINDOW_EDL") && !parent_edl)
			{
				EDL *new_edl = new EDL(this);
				new_edl->create_objects();
				new_edl->load_xml(file, LOAD_ALL);


				if((load_flags & LOAD_ALL) == LOAD_ALL)
				{
//						if(vwindow_edl && !vwindow_edl_shared) 
//							vwindow_edl->Garbage::remove_user();
//						vwindow_edl_shared = 0;
//						vwindow_edl = new_edl;

//						append_vwindow_edl(new_edl, 0);

				}
				else
// Discard if not replacing EDL
				{
					new_edl->Garbage::remove_user();
					new_edl = 0;
				}
			}
		}while(!result);

// delete leftovers
        while(atracks.last)
        {
            delete atracks.last;
        }
        while(vtracks.last)
        {
            delete vtracks.last;
        }
	}
	boundaries();
//dump();

	return error;
}

// Output path is the path of the output file if name truncation is desired.
// It is a "" if complete names should be used.
// Called recursively by copy for clips, thus the string can't be terminated.
// The string is not terminated in this call.
int EDL::save_xml(FileXML *file, 
	const char *output_path,
	int is_clip,
	int is_vwindow)
{
	copy(0, 
		MAX(tracks->total_length(), labels->total_length()), 
		1, 
		is_clip,
		is_vwindow,
		file, 
		output_path,
		0);
	return 0;
}

void EDL::start_auto_copy(FileXML *file,
    double selectionstart, 
	double selectionend)
{
	file->tag.set_title("AUTO_CLIPBOARD");
// single keyframe has length 0
	file->tag.set_property("LENGTH", selectionend - selectionstart);
	file->tag.set_property("FRAMERATE", session->frame_rate);
	file->tag.set_property("SAMPLERATE", session->sample_rate);
	file->append_tag();
	file->append_newline();
	file->append_newline();
}

void EDL::end_auto_copy(FileXML *file)
{
	file->tag.set_title("/AUTO_CLIPBOARD");
	file->append_tag();
	file->append_newline();
	file->terminate_string();
}



int EDL::copy_all(EDL *edl)
{
	if(this == edl) return 0;

    nested_depth = edl->nested_depth;
	index_state->copy_from(edl->index_state);
	nested_edls->clear();
// nested EDLs are copied in the tracks
	copy_session(edl);
	copy_assets(edl);
	copy_clips(edl);
	tracks->copy_from(edl->tracks);
	labels->copy_from(edl->labels);
	return 0;
}

void EDL::copy_clips(EDL *edl)
{
	if(this == edl) return;

//	remove_vwindow_edls();

//	if(vwindow_edl && !vwindow_edl_shared) 
//		vwindow_edl->Garbage::remove_user();
//	vwindow_edl = 0;
//	vwindow_edl_shared = 0;

// 	for(int i = 0; i < edl->total_vwindow_edls(); i++)
// 	{
// 		EDL *new_edl = new EDL(this);
// 		new_edl->create_objects();
// 		new_edl->copy_all(edl->get_vwindow_edl(i));
// 		append_vwindow_edl(new_edl, 0);
// 	}

	for(int i = 0; i < clips.size(); i++)
		clips.get(i)->Garbage::remove_user();
	clips.remove_all();
	for(int i = 0; i < edl->clips.total; i++)
	{
		add_clip(edl->clips.values[i]);
	}
}

void EDL::copy_assets(EDL *edl)
{
	if(this == edl) return;

	if(!parent_edl)
	{
		assets->copy_from(edl->assets);
	}
}

void EDL::copy_session(EDL *edl, int session_only)
{
	if(this == edl) return;

	if(!session_only)
	{
		strcpy(this->path, edl->path);
//printf("EDL::copy_session %p %s\n", this, this->path);

		folders.remove_all_objects();
		for(int i = 0; i < edl->folders.total; i++)
		{
			char *new_folder;
			folders.append(new_folder = new char[strlen(edl->folders.values[i]) + 1]);
			strcpy(new_folder, edl->folders.values[i]);
		}
	}

	if(!parent_edl)
	{
		session->copy(edl->session);
	}

	if(!session_only)
	{
		local_session->copy_from(edl->local_session);
	}
}

int EDL::copy_assets(double start, 
	double end, 
	FileXML *file, 
	int all, 
	const char *output_path)
{
	ArrayList<Asset*> asset_list;
	ArrayList<EDL*> nested_list;
	Track* current;
    int error = 0;

	file->tag.set_title("ASSETS");
	file->append_tag();
	file->append_newline();

// Copy everything for a save
	if(all)
	{
		for(Asset *asset = assets->first;
			asset;
			asset = asset->next)
		{
			asset_list.append(asset);
		}

        for(int i = 0; i < nested_edls->size(); i++)
        {
            nested_list.append(nested_edls->get(i));
        }
	}
	else
// Copy just the ones in the selection for a clipboard
	{
		for(current = tracks->first; 
			current; 
			current = NEXT)
		{
			if(current->record)
			{
				current->copy_assets(start, 
					end, 
					&asset_list,
                    &nested_list);
			}
		}
	}

// Paths relativised here
	for(int i = 0; i < asset_list.total; i++)
	{
		asset_list.values[i]->write(file, 
			0, 
			output_path);
	}

	file->tag.set_title("/ASSETS");
	file->append_tag();
	file->append_newline();


    if(nested_list.size())
    {
	    file->tag.set_title("NESTED_EDLS");
	    file->append_tag();
	    file->append_newline();

	    for(int i = 0; i < nested_list.total; i++)
	    {
		    nested_list.values[i]->copy_nested(file, 
			    output_path);
	    }

	    file->tag.set_title("/NESTED_EDLS");
	    file->append_tag();
	    file->append_newline();
    }
	file->append_newline();
	return error;
}

void EDL::copy_nested(FileXML *file, 
		const char *output_path)
{
// 	char new_path[BCTEXTLEN];
// 	char asset_directory[BCTEXTLEN];
// 	char output_directory[BCTEXTLEN];
// 	FileSystem fs;
//     
// // Make path relative
// 	fs.extract_dir(asset_directory, path);
// 	if(output_path && output_path[0]) 
// 	{
//     	fs.extract_dir(output_directory, output_path);
// 	}
//     else
// 	{
//     	output_directory[0] = 0;
//     }
// 
// // Asset and EDL are in same directory.  Extract just the name.
// 	if(!strcmp(asset_directory, output_directory))
// 	{
// 		fs.extract_name(new_path, path);
// 	}
// 	else
// 	{
// 		strcpy(new_path, path);
// 	}

	file->tag.set_title("NESTED_EDL");
//	file->tag.set_property("SRC", new_path);
	file->tag.set_property("SRC", path);
	file->tag.set_property("SAMPLE_RATE", session->nested_sample_rate);
	file->tag.set_property("FRAME_RATE", session->nested_frame_rate);
	file->append_tag();
	file->append_newline();
}

int EDL::copy(double start, 
	double end, 
	int all, 
	int is_clip,
	int is_vwindow,
	FileXML *file, 
	const char *output_path,
	int rewind_it)
{
//printf("EDL::copy 1\n");
// begin file
	if(is_clip)
		file->tag.set_title("CLIP_EDL");
	else
	if(is_vwindow)
		file->tag.set_title("VWINDOW_EDL");
	else
	{
		file->tag.set_title("EDL");
		file->tag.set_property("VERSION", CINELERRA_VERSION);
// Save path for restoration of the project title from a backup.
		if(this->path[0])
		{
			file->tag.set_property("PATH", path);
		}
	}

	file->append_tag();
	file->append_newline();

// Set clipboard samples only if copying to clipboard
	if(!all)
	{
		file->tag.set_title("CLIPBOARD");
		file->tag.set_property("LENGTH", end - start);
		file->append_tag();
		file->append_newline();
		file->append_newline();
	}
//printf("EDL::copy 1\n");

// Sessions
	local_session->save_xml(file, start);

//printf("EDL::copy 1\n");

// Top level stuff.
//	if(!parent_edl)
	{
// Need to copy all this from child EDL if pasting is desired.
// Session
		session->save_xml(file);
		session->save_video_config(file);
		session->save_audio_config(file);

// Folders
		for(int i = 0; i < folders.total; i++)
		{
			file->tag.set_title("FOLDER");
			file->append_tag();
			file->append_text(folders.values[i]);
			file->tag.set_title("/FOLDER");
			file->append_tag();
			file->append_newline();
		}

// Media
// Don't replicate all assets for every clip.
// The assets for the clips are probably in the mane EDL.
		if(!is_clip)
        {
			copy_assets(start, 
				end, 
				file, 
				all, 
				output_path);
        }

// Clips
// Don't want this if using clipboard
		if(all)
		{
// 			for(int i = 0; i < total_vwindow_edls(); i++)
// 			{
// 				get_vwindow_edl(i)->save_xml(file, 
// 					output_path,
// 					0,
// 					1);
// 			}

			for(int i = 0; i < clips.total; i++)
				clips.values[i]->save_xml(file, 
					output_path,
					1,
					0);
		}

		file->append_newline();
		file->append_newline();
	}


//printf("EDL::copy 1\n");

	labels->copy(start, end, file);
//printf("EDL::copy 1\n");
	tracks->copy(start, end, all, file, output_path);
//printf("EDL::copy 2\n");

// terminate file
	if(is_clip)
		file->tag.set_title("/CLIP_EDL");
	else
	if(is_vwindow)
		file->tag.set_title("/VWINDOW_EDL");
	else
		file->tag.set_title("/EDL");
	file->append_tag();
	file->append_newline();


// For editing operations we want to rewind it for immediate pasting.
// For clips and saving to disk leave it alone.
	if(rewind_it)
	{
		file->terminate_string();
		file->rewind();
	}
	return 0;
}

void EDL::rechannel()
{
	for(Track *current = tracks->first; current; current = NEXT)
	{
		if(current->data_type == TRACK_AUDIO)
		{
			PanAutos *autos = (PanAutos*)current->automation->autos[AUTOMATION_PAN];
			((PanAuto*)autos->default_auto)->rechannel();
			for(PanAuto *keyframe = (PanAuto*)autos->first;
				keyframe;
				keyframe = (PanAuto*)keyframe->next)
			{
				keyframe->rechannel();
			}
		}
	}
}

void EDL::resample(double old_rate, double new_rate, int data_type)
{
	for(Track *current = tracks->first; current; current = NEXT)
	{
		if(current->data_type == data_type)
		{
			current->resample(old_rate, new_rate);
		}
	}
}


void EDL::synchronize_params(EDL *edl)
{
	local_session->synchronize_params(edl->local_session);
	for(Track *this_track = tracks->first, *that_track = edl->tracks->first; 
		this_track && that_track; 
		this_track = this_track->next,
		that_track = that_track->next)
	{
		this_track->synchronize_params(that_track);
	}
}

int EDL::trim_selection(double start, 
	double end,
	int edit_labels,
	int edit_plugins,
	int edit_autos)
{
	if(start != end)
	{
// clear the data
		clear(0, 
			start,
			edit_labels,
			edit_plugins,
			edit_autos);
		clear(end - start, 
			tracks->total_length(),
			edit_labels,
			edit_plugins,
			edit_autos);
	}
	return 0;
}


int EDL::equivalent(double position1, double position2)
{
	double threshold = (double).5 / session->frame_rate;
	if(session->cursor_on_frames) 
		threshold = (double).5 / session->frame_rate;
	else
		threshold = (double)1 / session->sample_rate;

	if(fabs(position2 - position1) < threshold)
    	return 1;
    else
        return 0;
}

double EDL::equivalent_output(EDL *edl)
{
	double result = -1;
	session->equivalent_output(edl->session, &result);
	tracks->equivalent_output(edl->tracks, &result);
	return result;
}


void EDL::set_path(const char *path)
{
	strcpy(this->path, path);
}

void EDL::set_inpoint(double position)
{
	if(equivalent(local_session->get_inpoint(), position) && 
		local_session->get_inpoint() >= 0)
	{
		local_session->unset_inpoint();
	}
	else
	{
		local_session->set_inpoint(align_to_frame(position, 0));
		if(local_session->get_outpoint() <= local_session->get_inpoint()) 
			local_session->unset_outpoint();
	}
}

void EDL::set_outpoint(double position)
{
	if(equivalent(local_session->get_outpoint(), position) && 
		local_session->get_outpoint() >= 0)
	{
		local_session->unset_outpoint();
	}
	else
	{
		local_session->set_outpoint(align_to_frame(position, 0));
		if(local_session->get_inpoint() >= local_session->get_outpoint()) 
			local_session->unset_inpoint();
	}
}

int EDL::deglitch(double position)
{

	if(session->cursor_on_frames)
	{
		Track *current_track;



		for(current_track = tracks->first; 
			current_track; 
			current_track = current_track->next)
		{
			if(current_track->record &&
				current_track->data_type == TRACK_AUDIO) 
			{
				ATrack *atrack = (ATrack*)current_track;
				atrack->deglitch(position, 
					session->labels_follow_edits, 
					session->plugins_follow_edits, 
					session->autos_follow_edits);
			}
		}
	}
    return 0;
}


int EDL::clear(double start, 
	double end, 
	int clear_labels,
	int clear_plugins,
	int edit_autos)
{
	if(start == end)
	{
		double distance = 0;
		tracks->clear_handle(start, 
			end,
			distance, 
			clear_labels,
			clear_plugins,
			edit_autos);
		if(clear_labels && distance > 0)
			labels->paste_silence(start, 
				start + distance);
	}
	else
	{
		tracks->clear(start, 
			end,
			clear_plugins,
			edit_autos);
		if(clear_labels) 
			labels->clear(start, 
				end, 
				1);
	}

// Need to put at beginning so a subsequent paste operation starts at the
// right position.
	double position = local_session->get_selectionstart();
	local_session->set_selectionend(position);
	local_session->set_selectionstart(position);
	return 0;
}

void EDL::label_edits(double start, double end, int color)
{
	for(Track *current_track = tracks->first; 
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
					current_edit->startproject < end_units)
				{
                    double position = current_track->from_units(current_edit->startproject);
                    labels->toggle_label(position, 
                        position, 
                        MWindow::session->label_color);
                }
            }

// top armed track only
            break;
        }
    }
}

void EDL::modify_transitionhandles(
    Edit *edit,
    Transition *transition,
    double oldposition, 
	double newposition, 
	int currentend)
{
    Track *track = edit->track;
    int64_t new_position = track->to_units(newposition, 0);
    
    if(currentend == LEFT_HANDLE)
    {
        if(new_position >= edit->startproject + transition->length)
// delete the transition
            edit->detach_transition();
        else
// change the length
        if(new_position < edit->startproject)
        {
            transition->length = transition->length + edit->startproject - new_position;
        }
        else
            transition->length = edit->startproject + transition->length - new_position;
    }
    else
    {
        if(new_position <= edit->startproject)
// delete the transition
            edit->detach_transition();
        else
            transition->length = new_position - edit->startproject;
    }
}


void EDL::modify_edithandles(double oldposition, 
	double newposition, 
	int currentend,
	int handle_mode,
	int edit_labels,
	int edit_plugins,
	int edit_autos)
{
	tracks->modify_edithandles(oldposition, 
		newposition, 
		currentend,
		handle_mode,
		edit_labels, 
		edit_plugins,
		edit_autos);
	labels->modify_handles(oldposition, 
		newposition, 
		currentend,
		handle_mode,
		edit_labels);
}

void EDL::modify_pluginhandles(double oldposition, 
	double newposition, 
	int currentend, 
	int handle_mode,
	int edit_labels,
	int edit_autos,
	Edits *trim_edits)
{
	tracks->modify_pluginhandles(oldposition, 
		newposition, 
		currentend, 
		handle_mode,
		edit_labels,
		edit_autos,
		trim_edits);
	optimize();
}

void EDL::paste_silence(double start, 
	double end, 
	int edit_labels, 
	int edit_plugins,
	int edit_autos)
{
	if(edit_labels) 
		labels->paste_silence(start, end);
	tracks->paste_silence(start, 
		end, 
		edit_plugins,
		edit_autos);
}


void EDL::remove_from_project(ArrayList<EDL*> *clips)
{
	for(int i = 0; i < clips->size(); i++)
	{
		for(int j = 0; j < this->clips.size(); j++)
		{
			if(this->clips.get(j) == clips->values[i])
			{
				this->clips.get(j)->Garbage::remove_user();
				this->clips.remove(this->clips.get(j));
			}
		}
	}
}

void EDL::remove_from_project(ArrayList<Indexable*> *assets)
{
// Remove from clips
	if(!parent_edl)
    {
		for(int j = 0; j < clips.total; j++)
		{
			clips.values[j]->remove_from_project(assets);
		}
    }

// Remove from VWindow EDLs
//	for(int i = 0; i < total_vwindow_edls(); i++)
//		get_vwindow_edl(i)->remove_from_project(assets);

	for(int i = 0; i < assets->size(); i++)
	{
// Remove from tracks
		for(Track *track = tracks->first; track; track = track->next)
		{
			track->remove_asset(assets->get(i));
		}

// Remove from assets
		if(!parent_edl && assets->get(i)->is_asset)
		{
			this->assets->remove_asset((Asset*)assets->get(i));
		}
		else
		if(!parent_edl && !assets->get(i)->is_asset)
		{
			this->nested_edls->remove_edl((EDL*)assets->get(i));
		}
	}
}

void EDL::update_assets(EDL *src)
{
	for(Asset *current = src->assets->first;
		current;
		current = NEXT)
	{
		assets->update(current);
	}
}

void EDL::update_nested(EDL *src)
{
    int error = 0;
	for(int i = 0; i < src->nested_edls->size(); i++)
	{
        EDL *nested_src = src->nested_edls->get(i);
        EDL *nested_dst = nested_edls->get(nested_src->path, &error);
		if(nested_dst)
        {
            nested_dst->session->nested_sample_rate = nested_src->session->nested_sample_rate;
            nested_dst->session->nested_frame_rate = nested_src->session->nested_frame_rate;
        }
	}
}

int EDL::get_tracks_height(Theme *theme)
{
	int total_pixels = 0;
	for(Track *current = tracks->first;
		current;
		current = NEXT)
	{
		total_pixels += current->vertical_span(theme);
	}
	return total_pixels;
}

int64_t EDL::get_tracks_width()
{
	int64_t total_pixels = 0;
	for(Track *current = tracks->first;
		current;
		current = NEXT)
	{
		int64_t pixels = current->horizontal_span();
		if(pixels > total_pixels) total_pixels = pixels;
	}
//printf("EDL::get_tracks_width %d\n", total_pixels);
	return total_pixels;
}

// int EDL::calculate_output_w(int single_channel)
// {
// 	if(single_channel) return session->output_w;
// 
// 	int widest = 0;
// 	for(int i = 0; i < session->video_channels; i++)
// 	{
// 		if(session->vchannel_x[i] + session->output_w > widest) widest = session->vchannel_x[i] + session->output_w;
// 	}
// 	return widest;
// }
// 
// int EDL::calculate_output_h(int single_channel)
// {
// 	if(single_channel) return session->output_h;
// 
// 	int tallest = 0;
// 	for(int i = 0; i < session->video_channels; i++)
// 	{
// 		if(session->vchannel_y[i] + session->output_h > tallest) tallest = session->vchannel_y[i] + session->output_h;
// 	}
// 	return tallest;
// }

// Get the total output size scaled to aspect ratio
void EDL::calculate_conformed_dimensions(int single_channel, float &w, float &h)
{
	w = session->output_w;
	h = session->output_h;

	if((float)session->output_w / session->output_h > get_aspect_ratio())
	{
		h = (float)h * 
			(session->output_w / get_aspect_ratio() / session->output_h);
	}
	else
	{
		w = (float)w * 
			(h * get_aspect_ratio() / session->output_w);
	}
}

float EDL::get_aspect_ratio()
{
	return session->aspect_w / session->aspect_h;
}

int EDL::dump()
{
	if(parent_edl)
	{
    	printf("CLIP\n");
	}
    else
	{
    	printf("EDL\n");
	}
    printf("  clip_title: %s\n"
		"  parent_edl: %p\n", local_session->clip_title, parent_edl);
	printf("  selectionstart %f\n  selectionend %f\n  loop_start %f\n  loop_end %f\n", 
		local_session->get_selectionstart(1), 
		local_session->get_selectionend(1),
		local_session->loop_start,
		local_session->loop_end);
	for(int i = 0; i < TOTAL_PANES; i++)
	{
		printf("  pane %d view_start=%ld track_start=%ld\n", 
			i,
			local_session->view_start[i],
			local_session->track_start[i]);
	}

	if(!parent_edl)
	{
		printf("  audio_channels: %d\n"
			"  audio_tracks: %d\n"
			"  sample_rate: %d\n",
			(int)session->audio_channels,
			(int)session->audio_tracks,
			(int)session->sample_rate);
		printf("  video_channels: %d\n"
			"  video_tracks: %d\n"
			"  frame_rate: %.2f\n"
			"  frames_per_foot: %.2f\n"
    		"  output_w: %d\n"
    		"  output_h: %d\n"
    		"  aspect_w: %f\n"
    		"  aspect_h: %f\n"
			"  color_model: %d\n"
			"  proxy_scale: %d\n", 
				session->video_channels,
				session->video_tracks,
				session->frame_rate,
				session->frames_per_foot,
    			session->output_w,
    			session->output_h,
    			session->aspect_w,
    			session->aspect_h,
				session->color_model,
				session->proxy_scale);

		printf(" CLIPS\n");
		printf("  total: %d\n", clips.total);
	
		for(int i = 0; i < clips.total; i++)
		{
			printf("\n\n");
			clips.values[i]->dump();
			printf("\n\n");
		}

// 		printf(" VWINDOW EDLS\n");
// 		printf("  total: %d\n", total_vwindow_edls());
// 		
// 		for(int i = 0; i < total_vwindow_edls(); i++)
// 		{
// 			printf("   %s\n", get_vwindow_edl(i)->local_session->clip_title);
// 		}
	
		printf(" ASSETS\n");
		assets->dump();
		printf(" NESTED EDLS\n");
		nested_edls->dump();
	}
	printf(" LABELS\n");
	labels->dump();
	printf(" TRACKS\n");
	tracks->dump();
//printf("EDL::dump 2\n");
	return 0;
}

EDL* EDL::add_clip(EDL *edl)
{
// Copy argument.  New edls are deleted from MWindow::load_filenames.
	EDL *new_edl = new EDL(this);
	new_edl->create_objects();
	new_edl->copy_all(edl);
	clips.append(new_edl);
printf("EDL::add_clip %d %d\n", __LINE__, clips.size());
	return new_edl;
}

void EDL::insert_asset(Asset *asset, 
	EDL *nested_edl,
	double position, 
	Track *first_track, 
	RecordLabels *labels)
{
// Insert asset into asset table
	Asset *new_asset = 0;
	EDL *new_nested_edl = 0;

	if(asset) new_asset = assets->update(asset);
	if(nested_edl) new_nested_edl = nested_edls->get_copy(nested_edl);

// Paste video
	int vtrack = 0;
	Track *current = first_track ? first_track : tracks->first;


// Fix length of single frame
	double length;
	int layers = 0;
	int channels = 0;

	if(new_nested_edl)
	{
		length = new_nested_edl->tracks->total_playable_length() *
            new_nested_edl->session->frame_rate /
            new_nested_edl->session->get_nested_frame_rate();
		layers = new_nested_edl->tracks->total_playable_tracks(TRACK_VIDEO);
		channels = new_nested_edl->tracks->total_playable_tracks(TRACK_AUDIO);
	}

#define NOSEEK_SECONDS 3600
	if(new_asset)
	{
// insert arbitrary time for unknown length
        if(new_asset->video_length == NOSEEK_LENGTH)
        {
            length = NOSEEK_SECONDS;
        }
        else
// Insert 1 frame for still photo
		if(new_asset->video_length == STILL_PHOTO_LENGTH) 
		{
        	length = 1.0 / session->frame_rate; 
		}
        else
		if(new_asset->frame_rate > 0)
		{
        	length = ((double)new_asset->video_length / new_asset->frame_rate);
		}
        else
		{
        	length = 1.0 / session->frame_rate;
		}
        layers = new_asset->layers;
		channels = new_asset->channels;
	}

	for( ;
		current && vtrack < layers;
		current = NEXT)
	{
		if(!current->record || 
			current->data_type != TRACK_VIDEO)
        {
			continue;
        }

		current->insert_asset(new_asset, 
			new_nested_edl,
			length, 
			position, 
			vtrack);

		vtrack++;
	}

	int atrack = 0;
	if(new_asset)
	{
		if(new_asset->audio_length == NOSEEK_LENGTH)
		{
// insert arbitrary time for unknown length
			length = NOSEEK_SECONDS;
		}
		else
        {
			length = (double)new_asset->audio_length / 
					new_asset->sample_rate;
        }
	}

	for(current = tracks->first;
		current && atrack < channels;
		current = NEXT)
	{
		if(!current->record ||
			current->data_type != TRACK_AUDIO)
		{
        	continue;
        }

		current->insert_asset(new_asset, 
			new_nested_edl,
			length, 
			position, 
			atrack);


		atrack++;
	}

// Insert labels from a recording window.
	if(labels)
	{
		for(RecordLabel *label = labels->first; label; label = label->next)
		{
			this->labels->toggle_label(label->position, 
                label->position, 
                MWindow::session->label_color);
		}
	}
}



void EDL::set_index_file(Indexable *indexable)
{
	if(indexable->is_asset) 
	{
    	assets->update_index((Asset*)indexable);
    }
	else
    {
		nested_edls->update_index((EDL*)indexable);
    }
}

void EDL::optimize()
{
//printf("EDL::optimize 1\n");
	double length = tracks->total_length();
//	if(local_session->preview_end > length) local_session->preview_end = length;
//	if(local_session->preview_start > length ||
//		local_session->preview_start < 0) local_session->preview_start = 0;
	for(Track *current = tracks->first; current; current = NEXT)
		current->optimize();
}

int EDL::next_id()
{
	id_lock->lock("EDL::next_id");
	int result = EDLSession::current_id++;
	id_lock->unlock();
	return result;
}

void EDL::get_shared_plugins(Track *source, 
	ArrayList<SharedLocation*> *plugin_locations,
	int omit_recordable,
	int data_type)
{
	for(Track *track = tracks->first; track; track = track->next)
	{
		if(!track->record || !omit_recordable)
		{
			if(track != source && 
				track->data_type == data_type)
			{
				for(int i = 0; i < track->plugin_set.total; i++)
				{
					Plugin *plugin = track->get_current_plugin(
						local_session->get_selectionstart(1), 
						i, 
						PLAY_FORWARD, 
						1,
						0);
					if(plugin && plugin->plugin_type == PLUGIN_STANDALONE)
					{
						plugin_locations->append(new SharedLocation(tracks->number_of(track), i));
					}
				}
			}
		}
	}
}

void EDL::get_shared_tracks(Track *track, 
	ArrayList<SharedLocation*> *module_locations,
	int omit_recordable,
	int data_type)
{
	for(Track *current = tracks->first; current; current = NEXT)
	{
		if(!omit_recordable || !current->record)
		{
			if(current != track && 
				current->data_type == data_type)
			{
				module_locations->append(new SharedLocation(tracks->number_of(current), 0));
			}
		}
	}
}

// Convert position to frames if cursor alignment is enabled
double EDL::align_to_frame(double position, int round)
{
//printf("EDL::align_to_frame 1 %f\n", position);
	if(session->cursor_on_frames)
	{
// Seconds -> Frames
		double temp = (double)position * session->frame_rate;
//printf("EDL::align_to_frame 2 %f\n", temp);

// Assert some things
		if(session->sample_rate == 0)
			printf("EDL::align_to_frame: sample_rate == 0\n");

		if(session->frame_rate == 0)
			printf("EDL::align_to_frame: frame_rate == 0\n");

// Round frames
// Always round down negative numbers
// but round up only if requested
		if(round) 
		{
			temp = Units::round(temp);
		}
		else
		{
// 			if(temp < 0)
// 			{
// 				temp -= 0.5;
// 			}
// 			else
				temp = Units::to_int64(temp);
		}
//printf("EDL::align_to_frame 3 %f\n", temp);

// Frames -> Seconds
		temp /= session->frame_rate;

//printf("EDL::align_to_frame 5 %f\n", temp);

		return temp;
	}
//printf("EDL::align_to_frame 3 %d\n", position);


	return position;
}


void EDL::new_folder(const char *folder)
{
	for(int i = 0; i < folders.total; i++)
	{
		if(!strcasecmp(folders.values[i], folder)) return;
	}

	char *new_folder;
	folders.append(new_folder = new char[strlen(folder) + 1]);
	strcpy(new_folder, folder);
}

void EDL::delete_folder(char *folder)
{
	int i;
	for(i = 0; i < folders.total; i++)
	{
		if(!strcasecmp(folders.values[i], folder))
		{
			break;
		}
	}

	if(i < folders.total) delete folders.values[i];

	for( ; i < folders.total - 1; i++)
	{
		folders.values[i] = folders.values[i + 1];
	}
}

int EDL::get_use_vconsole(VEdit* *playable_edit,
	int64_t position, 
	int direction,
	PlayableTracks *playable_tracks)
{
	int share_playable_tracks = 1;
	int result = 0;
	VTrack *playable_track = 0;
	const int debug = 0;
	*playable_edit = 0;

// Calculate playable tracks when being called as a nested EDL
	if(!playable_tracks)
	{
		share_playable_tracks = 0;
		playable_tracks = new PlayableTracks(this,
			position,
			direction,
			TRACK_VIDEO /*,
			1 */);
	}


// Total number of playable tracks is 1
	if(playable_tracks->size() != 1) 
	{
		result = 1;
	}
	else
	{
		playable_track = (VTrack*)playable_tracks->get(0);
	}
// printf("EDL::get_use_vconsole %d position=%d playable_tracks=%d\n",
// __LINE__,
// (int)position,
// playable_tracks->size());

// Don't need playable tracks anymore
	if(!share_playable_tracks)
	{
		delete playable_tracks;
	}

if(debug) printf("EDL::get_use_vconsole %d playable_tracks->size()=%d\n", 
__LINE__,
playable_tracks->size());
	if(result) return 1;


// Test mutual conditions between direct copy rendering and this.
	if(!playable_track->direct_copy_possible(position, 
		direction,
		1))
		return 1;
if(debug) printf("EDL::get_use_vconsole %d\n", __LINE__);

	*playable_edit = (VEdit*)playable_track->edits->editof(position, 
		direction,
		1);
// No edit at current location
	if(!*playable_edit) return 1;
if(debug) printf("EDL::get_use_vconsole %d\n", __LINE__);


// Edit is nested EDL
	if((*playable_edit)->nested_edl)
	{
// Test nested EDL
		EDL *nested_edl = (*playable_edit)->nested_edl;
		int64_t nested_position = (int64_t)((position - 
				(*playable_edit)->startproject +
				(*playable_edit)->startsource) * 
			nested_edl->session->frame_rate /
			session->frame_rate);


		VEdit *playable_edit_temp = 0;
		if(session->output_w != nested_edl->session->output_w ||
			session->output_h != nested_edl->session->output_h ||
			nested_edl->get_use_vconsole(&playable_edit_temp,
				nested_position, 
				direction,
				0)) 
			return 1;
		
		return 0;
	}

if(debug) printf("EDL::get_use_vconsole %d\n", __LINE__);
// Edit is not a nested EDL
// Edit is silence
	if(!(*playable_edit)->asset) return 1;
if(debug) printf("EDL::get_use_vconsole %d\n", __LINE__);


// Asset and output device must have the same dimensions
	if((*playable_edit)->asset->width != session->output_w ||
		(*playable_edit)->asset->height != session->output_h)
		return 1;


if(debug) printf("EDL::get_use_vconsole %d\n", __LINE__);



// If we get here the frame is going to be directly copied.  Whether it is
// decompressed in hardware depends on the colormodel.
	return 0;
}


// For Indexable
int EDL::get_audio_channels()
{
	return session->audio_channels;
}

int EDL::get_sample_rate()
{
	return session->sample_rate;
}

int64_t EDL::get_audio_samples()
{
	return (int64_t)(tracks->total_playable_length() *
		session->sample_rate);
}

int EDL::have_audio()
{
	return 1;
}

int EDL::have_video()
{
	return 1;
}


int EDL::get_w()
{
	return session->output_w;
}

int EDL::get_h()
{
	return session->output_h;
}

double EDL::get_frame_rate()
{
	return session->frame_rate;
}

int EDL::get_video_layers()
{
	return 1;
}

int64_t EDL::get_video_frames()
{
	return (int64_t)(tracks->total_playable_length() *
		session->frame_rate);
}


// void EDL::remove_vwindow_edls()
// {
// 	for(int i = 0; i < total_vwindow_edls(); i++)
// 	{
// 		get_vwindow_edl(i)->Garbage::remove_user();
// 	}
// 	vwindow_edls.remove_all();
// }
// 
// void EDL::remove_vwindow_edl(EDL *edl)
// {
// 	if(vwindow_edls.number_of(edl) >= 0)
// 	{
// 		edl->Garbage::remove_user();
// 
// 		vwindow_edls.remove(edl);
// 	}
// }
// 
// 
// EDL* EDL::get_vwindow_edl(int number)
// {
//     if(vwindow_edls.size())
//     {
//     	return vwindow_edls.get(number);
//     }
//     else
//     {
//         return 0;
//     }
// }
// 
// int EDL::total_vwindow_edls()
// {
// 	return vwindow_edls.size();
// }
// 
// void EDL::append_vwindow_edl(EDL *edl, int increase_counter)
// {
// 	if(vwindow_edls.number_of(edl) >= 0) return;
// 
// 	if(increase_counter) edl->Garbage::add_user();
// 	vwindow_edls.append(edl);
// }


