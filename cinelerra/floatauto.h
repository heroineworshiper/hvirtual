
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
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

#ifndef FLOATAUTO_H
#define FLOATAUTO_H

// Automation point that takes floating point values

class FloatAuto;

#include "auto.h"
#include "edl.inc"
#include "floatautos.inc"

class FloatAuto : public Auto
{
public:
	FloatAuto() {};
	FloatAuto(EDL *edl, FloatAutos *autos);
	~FloatAuto();

	int operator==(Auto &that);
	int operator==(FloatAuto &that);
	int identical(FloatAuto *src);
	void copy_from(Auto *that);
	void copy_from(FloatAuto *that);
	void copy(int64_t start, int64_t end, FileXML *file, int default_only);
	void load(FileXML *xml);

 	float value_to_percentage();
 	float invalue_to_percentage();
 	float outvalue_to_percentage();

// Control values are relative to value
	float value, control_in_value, control_out_value;

private:
	int value_to_str(char *string, float value);
};



#endif
