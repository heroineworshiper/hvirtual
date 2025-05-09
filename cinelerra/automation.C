
/*
 * CINELERRA
 * Copyright (C) 2008-2013 Adam Williams <broadcast at earthling dot net>
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

#include "autoconf.h"
#include "automation.h"
#include "autos.h"
#include "atrack.inc"
#include "bcsignals.h"
#include "colors.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "floatautos.h"
#include "intautos.h"
#include "track.h"
#include "transportque.inc"


Automation::Automation(EDL *edl, Track *track)
{
	this->edl = edl;
	this->track = track;
	bzero(autos, sizeof(Autos*) * AUTOMATION_TOTAL);
}

Automation::~Automation()
{
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		delete autos[i];
	}
}



const char* Automation::get_save_title(int type)
{
// These must match the enumerations
    static const char *save_titles[] = 
    {
	    "MUTEAUTOS",
	    "CAMERA_X",
	    "CAMERA_Y",
	    "CAMERA_Z",
	    "PROJECTOR_X",
	    "PROJECTOR_Y",
	    "PROJECTOR_Z",
	    "FADEAUTOS",
	    "PANAUTOS",
	    "MODEAUTOS",
	    "MASKAUTOS",
	    "SPEEDAUTOS",
        "TRANSITION", 
        "PLUGINSET"
    };
    
    return save_titles[type];
}


void Automation::create_objects()
{
	autos[AUTOMATION_MUTE] = new IntAutos(edl, track, 0, AUTOMATION_MUTE);
	autos[AUTOMATION_MUTE]->create_objects();

	autos[AUTOMATION_SPEED] = new FloatAutos(edl, track, 1.0, AUTOMATION_SPEED);
	autos[AUTOMATION_SPEED]->create_objects();
}

Automation& Automation::operator=(Automation& automation)
{
printf("Automation::operator= 1\n");
	copy_from(&automation);
	return *this;
}

void Automation::equivalent_output(Automation *automation, int64_t *result)
{
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		if(autos[i] && automation->autos[i])
			autos[i]->equivalent_output(automation->autos[i], 0, result);
	}
}

void Automation::copy_from(Automation *automation)
{
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		if(autos[i] && automation->autos[i])
			autos[i]->copy_from(automation->autos[i]);
	}
}

int Automation::load(FileXML *file)
{
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		if(file->tag.title_is(get_save_title(i)) && autos[i])
		{
			autos[i]->load(file);
			return 1;
		}
	}
	return 0;
}

int Automation::paste(int64_t start, 
	int64_t length, 
	double scale,
	FileXML *file, 
	int default_only,
	int active_only,
	AutoConf *autoconf)
{
	if(!autoconf) autoconf = edl->session->auto_conf;

	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		if(file->tag.title_is(get_save_title(i)) && autos[i] && autoconf->autos[i])
		{
			autos[i]->paste(start, 
				length, 
				scale, 
				file, 
				default_only,
				active_only);
			return 1;
		}
	}
	return 0;
}

int Automation::copy(int64_t start, 
	int64_t end, 
	FileXML *file, 
	int default_only,
	int active_only)
{
// Copy regardless of what's visible.
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		if(autos[i])
		{
			file->tag.set_title(get_save_title(i));
			file->append_tag();
			file->append_newline();
			autos[i]->copy(start, 
							end, 
							file, 
							default_only,
							active_only);
			char string[BCTEXTLEN];
			sprintf(string, "/%s", get_save_title(i));
			file->tag.set_title(string);
			file->append_tag();
			file->append_newline();
		}
	}

	return 0;
}


void Automation::clear(int64_t start, 
	int64_t end, 
	AutoConf *autoconf, 
	int shift_autos)
{
	AutoConf *temp_autoconf = 0;

	if(!autoconf)
	{
		temp_autoconf = new AutoConf;
		temp_autoconf->set_all(1);
		autoconf = temp_autoconf;
	}

	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		if(autos[i] && autoconf->autos[i])
		{
			autos[i]->clear(start, end, shift_autos);
		}
	}

	if(temp_autoconf) delete temp_autoconf;
}

void Automation::set_automation_mode(int64_t start, 
	int64_t end, 
	int mode,
	AutoConf *autoconf)
{
	AutoConf *temp_autoconf = 0;

	if(!autoconf)
	{
		temp_autoconf = new AutoConf;
		temp_autoconf->set_all(1);
		autoconf = temp_autoconf;
	}

	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		if(autos[i] && autoconf->autos[i])
		{
			autos[i]->set_automation_mode(start, end, mode);
		}
	}

	if(temp_autoconf) delete temp_autoconf;
}


void Automation::paste_silence(int64_t start, int64_t end)
{
// Unit conversion done in calling routine
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		if(autos[i])
			autos[i]->paste_silence(start, end);
	}
}

// We don't replace it in pasting but
// when inserting the first EDL of a load operation we need to replace
// the default keyframe.
void Automation::insert_track(Automation *automation, 
	int64_t start_unit, 
	int64_t length_units,
	int replace_default)
{
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		if(autos[i] && automation->autos[i])
		{
			autos[i]->insert_track(automation->autos[i], 
				start_unit, 
				length_units, 
				replace_default);
		}
	}


}

void Automation::resample(double old_rate, double new_rate)
{
// Run resample for all the autos structures and all the keyframes
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		if(autos[i]) autos[i]->resample(old_rate, new_rate);
	}
}



int Automation::direct_copy_possible(int64_t start, int direction)
{
	return 1;
}




void Automation::get_projector(float *x, 
	float *y, 
	float *z, 
	int64_t position,
	int direction)
{
}

void Automation::get_camera(float *x, 
	float *y, 
	float *z, 
	int64_t position,
	int direction)
{
}




int64_t Automation::get_length()
{
	int64_t length = 0;
	int64_t total_length = 0;

	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		if(autos[i])
		{
			length = autos[i]->get_length();
			if(length > total_length) total_length = length;
		}
	}


	return total_length;
}

void Automation::get_extents(float *min, 
	float *max,
	int *coords_undefined,
	int64_t unit_start,
	int64_t unit_end)
{
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		if(autos[i] && edl->session->auto_conf->autos[i])
		{
			autos[i]->get_extents(min, max, coords_undefined, unit_start, unit_end);
		}
	}
}


void Automation::dump()
{
	printf("   Automation: %p\n", this);


	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		if(autos[i])
		{
			printf("    %s %p\n", get_save_title(i), autos[i]);
			autos[i]->dump();
		}
	}

}
