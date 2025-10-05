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

#include "aedit.h"
#include "aedits.h"
#include "amodule.h"
//#include "apluginset.h"
#include "atrack.h"
#include "autoconf.h"
#include "aautomation.h"
#include "edit.h"
#include "edl.h"
#include "edlsession.h"
#include "cache.h"
#include "clip.h"
#include "datatype.h"
#include "file.h"
#include "filexml.h"
#include "floatautos.h"
#include "language.h"
#include "localsession.h"
#include "mainsession.h"
#include "panautos.h"
#include "pluginset.h"
#include "theme.h"
#include "trackcanvas.h"
#include "tracks.h"

#include <string.h>



ATrack::ATrack(EDL *edl, Tracks *tracks)
 : Track(edl, tracks)
{
	data_type = TRACK_AUDIO;
}

ATrack::~ATrack()
{
}

Edits* ATrack::new_edits()
{
    return new AEdits(edl, this);
}

void ATrack::create_objects()
{
	Track::create_objects();
	automation = new AAutomation(edl, this);
	automation->create_objects();
}

// Used by PlaybackEngine
void ATrack::synchronize_params(Track *track)
{
	Track::synchronize_params(track);

	ATrack *atrack = (ATrack*)track;
}

int ATrack::copy_settings(Track *track)
{
	Track::copy_settings(track);

	ATrack *atrack = (ATrack*)track;
	return 0;
}



int ATrack::vertical_span(Theme *theme)
{
	int track_h = Track::vertical_span(theme);
	int patch_h = 0;
	if(expand_view)
	{
		patch_h += theme->title_h + theme->play_h + theme->fade_h + theme->meter_h + theme->pan_h;
	}
	return MAX(track_h, patch_h);
}

// PluginSet* ATrack::new_plugins()
// {
// 	return new APluginSet(edl, this);
// }

int ATrack::load_defaults(BC_Hash *defaults)
{
	Track::load_defaults(defaults);
	return 0;
}

void ATrack::set_default_title()
{
	Track *current = ListItem<Track>::owner->first;
	int i;
	for(i = 0; current; current = NEXT)
	{
		if(current->data_type == TRACK_AUDIO) i++;
	}
    
    char temp[BCTEXTLEN];
	sprintf(temp, _("Audio %d"), i);
    title.assign(temp);
}

int64_t ATrack::to_units(double position, int round)
{
	if(round)
		return Units::round(position * edl->session->sample_rate);
	else
		return Units::to_int64(position * edl->session->sample_rate);
}

double ATrack::to_doubleunits(double position)
{
	return position * edl->session->sample_rate;
}

double ATrack::from_units(int64_t position)
{
	return (double)position / edl->session->sample_rate;
}


int ATrack::identical(int64_t sample1, int64_t sample2)
{
// Units of samples
	if(labs(sample1 - sample2) <= 1) return 1; else return 0;
}
























int64_t ATrack::length()
{
	return edits->length();
}

// int ATrack::get_dimensions(double &view_start, 
// 	double &view_units, 
// 	double &zoom_units)
// {
// 	view_start = (double)edl->local_session->view_start * 
// 		edl->session->sample_rate;
// 	view_units = (double)0;
// 	zoom_units = (double)edl->local_session->zoom_sample;
// }







int ATrack::paste_derived(int64_t start, int64_t end, int64_t total_length, FileXML *xml, int &current_channel)
{
	if(!strcmp(xml->tag.get_title(), "PANAUTOS"))
	{
		current_channel = xml->tag.get_property("CHANNEL", current_channel);
//		pan_autos->paste(start, end, total_length, xml, "/PANAUTOS", mwindow->session->autos_follow_edits);
		return 1;
	}
	return 0;
}



void ATrack::deglitch(double position, 
	int edit_labels,
	int edit_plugins,
	int edit_autos)
{
	int64_t samples = to_units(position, 0);
	
	
	edits->deglitch(samples);
				
				
	if(edit_plugins)
	{
		for(int i = 0; i < plugin_set.size(); i++)
		{
			plugin_set.get(i)->deglitch(samples);
		}
	}


}










 
