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

#include "autos.h"
#include "clip.h"
#include "edl.h"
#include "filexml.h"
#include "floatauto.h"
#include "localsession.h"

FloatAuto::FloatAuto(EDL *edl, FloatAutos *autos)
 : Auto(edl, (Autos*)autos)
{
	value = 0;
	control_in_value = 0;
	control_out_value = 0;
	mode = BEZIER_UNLOCKED;
//	control_in_position = 0;
//	control_out_position = 0;
}

FloatAuto::~FloatAuto()
{
//printf("FloatAuto::~FloatAuto %d %p\n", __LINE__, this);
}

int FloatAuto::operator==(Auto &that)
{
	return identical((FloatAuto*)&that);
}


int FloatAuto::operator==(FloatAuto &that)
{
	return identical((FloatAuto*)&that);
}


int FloatAuto::identical(FloatAuto *src)
{
	return EQUIV(value, src->value) &&
		EQUIV(control_in_value, src->control_in_value) &&
		EQUIV(control_out_value, src->control_out_value) &&
		mode == src->mode;
}

float FloatAuto::value_to_percentage()
{
	if(!edl) return 0;
	float automation_min = edl->local_session->automation_min;
	float automation_max = edl->local_session->automation_max;
	float automation_range = automation_max - automation_min;
	return (value - automation_min) / automation_range;
}

float FloatAuto::invalue_to_percentage()
{
	if(!edl) return 0;
	float automation_min = edl->local_session->automation_min;
	float automation_max = edl->local_session->automation_max;
	float automation_range = automation_max - automation_min;
	return (value + control_in_value - automation_min) / 
		automation_range;
}

float FloatAuto::outvalue_to_percentage()
{
	if(!edl) return 0;
	float automation_min = edl->local_session->automation_min;
	float automation_max = edl->local_session->automation_max;
	float automation_range = automation_max - automation_min;
	return (value + control_out_value - automation_min) / 
		automation_range;
}

void FloatAuto::to_locked()
{
// preserve the bigger of the 2 control points & mirror it
    if(fabs(control_in_value) > fabs(control_out_value))
    {
        control_out_value = -control_in_value;
    }
    else
    {
        control_in_value = -control_out_value;
    }
}

void FloatAuto::copy_from(Auto *that, int use_default)
{
	copy_from((FloatAuto*)that, use_default);
}

void FloatAuto::copy_from(FloatAuto *that, int use_default)
{
	Auto::copy_from(that, use_default);
	this->value = that->value;
	this->control_in_value = that->control_in_value;
	this->control_out_value = that->control_out_value;
	this->mode = that->mode;
}

int FloatAuto::value_to_str(char *string, float value)
{
	int j = 0, i = 0;
	if(value > 0) 
		sprintf(string, "+%.2f", value);
	else
		sprintf(string, "%.2f", value);

// fix number
	if(value == 0)
	{
		j = 0;
		string[1] = 0;
	}
	else
	if(value < 1 && value > -1) 
	{
		j = 1;
		string[j] = string[0];
	}
	else 
	{
		j = 0;
		string[3] = 0;
	}
	
	while(string[j] != 0) string[i++] = string[j++];
	string[i] = 0;

	return 0;
}

void FloatAuto::copy(int64_t start, int64_t end, FileXML *file, int default_auto)
{
	file->tag.set_title("AUTO");
	if(default_auto)
		file->tag.set_property("POSITION", 0);
	else
		file->tag.set_property("POSITION", position - start);
	file->tag.set_property("VALUE", value);
	file->tag.set_property("CONTROL_IN_VALUE", control_in_value);
	file->tag.set_property("CONTROL_OUT_VALUE", control_out_value);
	file->tag.set_property("MODE", mode);
	file->append_tag();
	file->append_newline();
}

void FloatAuto::load(FileXML *file)
{
	value = file->tag.get_property("VALUE", value);
	control_in_value = file->tag.get_property("CONTROL_IN_VALUE", control_in_value);
	control_out_value = file->tag.get_property("CONTROL_OUT_VALUE", control_out_value);
	mode = file->tag.get_property("MODE", mode);
}
