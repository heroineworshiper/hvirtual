
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

#include "clip.h"
#include "filexml.h"
#include "maskauto.h"
#include "maskautos.h"

#include <stdlib.h>
#include <string.h>




MaskPoint::MaskPoint()
{
	x = 0;
	y = 0;
	control_x1 = 0;
	control_y1 = 0;
	control_x2 = 0;
	control_y2 = 0;
}

void MaskPoint::copy_from(MaskPoint &ptr)
{
	this->x = ptr.x;
	this->y = ptr.y;
	this->control_x1 = ptr.control_x1;
	this->control_y1 = ptr.control_y1;
	this->control_x2 = ptr.control_x2;
	this->control_y2 = ptr.control_y2;
}

MaskPoint& MaskPoint::operator=(MaskPoint& ptr)
{
	copy_from(ptr);
}

int MaskPoint::operator==(MaskPoint& ptr)
{
	return EQUIV(x, ptr.x) &&
		EQUIV(y, ptr.y) &&
		EQUIV(control_x1, ptr.control_x1) &&
		EQUIV(control_y1, ptr.control_y1) &&
		EQUIV(control_x2, ptr.control_x2) &&
		EQUIV(control_y2, ptr.control_y2);
}

SubMask::SubMask(MaskAuto *keyframe)
{
	this->keyframe = keyframe;
}

SubMask::~SubMask()
{
}

int SubMask::equivalent(SubMask& ptr)
{
	if(points.size() != ptr.points.size()) return 0;

	for(int i = 0; i < points.size(); i++)
	{
		if(!(*points.get(i) == *ptr.points.get(i)))
			return 0;
	}
	
	return 1;
}


int SubMask::operator==(SubMask& ptr)
{
	return equivalent(ptr);
}

void SubMask::copy_from(SubMask& ptr)
{
	points.remove_all_objects();
//printf("SubMask::copy_from 1 %p %d\n", this, ptr.points.total);
	for(int i = 0; i < ptr.points.total; i++)
	{
		MaskPoint *point = new MaskPoint;
		*point = *ptr.points.values[i];
		points.append(point);
	}
}

void SubMask::load(FileXML *file)
{
	points.remove_all_objects();

	int result = 0;
	while(!result)
	{
		result = file->read_tag();
		
		if(!result)
		{
			if(file->tag.title_is("/MASK"))
			{
				result = 1;
			}
			else
			if(file->tag.title_is("POINT"))
			{
				char string[BCTEXTLEN];
				string[0] = 0;
				file->read_text_until("/POINT", string, BCTEXTLEN);

				MaskPoint *point = new MaskPoint;
				char *ptr = string;
//printf("MaskAuto::load 1 %s\n", ptr);

				point->x = atof(ptr);
				ptr = strchr(ptr, ',');
//printf("MaskAuto::load 2 %s\n", ptr + 1);
				if(ptr) 
				{
					point->y = atof(ptr + 1);
					ptr = strchr(ptr + 1, ',');
				
					if(ptr)
					{
//printf("MaskAuto::load 3 %s\n", ptr + 1);
						point->control_x1 = atof(ptr + 1);
						ptr = strchr(ptr + 1, ',');
						if(ptr)
						{
//printf("MaskAuto::load 4 %s\n", ptr + 1);
							point->control_y1 = atof(ptr + 1);
							ptr = strchr(ptr + 1, ',');
							if(ptr)
							{
//printf("MaskAuto::load 5 %s\n", ptr + 1);
								point->control_x2 = atof(ptr + 1);
								ptr = strchr(ptr + 1, ',');
								if(ptr) point->control_y2 = atof(ptr + 1);
							}
						}
					}
					
				}
				points.append(point);
			}
		}
	}
}

void SubMask::copy(FileXML *file)
{
	if(points.total)
	{
		file->tag.set_title("MASK");
		file->tag.set_property("NUMBER", keyframe->masks.number_of(this));
		file->append_tag();
		file->append_newline();

		for(int i = 0; i < points.total; i++)
		{
			file->append_newline();
			file->tag.set_title("POINT");
			file->append_tag();
			char string[BCTEXTLEN];
//printf("SubMask::copy 1 %p %d %p\n", this, i, points.values[i]);
			sprintf(string, "%.6e, %.6e, %.6e, %.6e, %.6e, %.6e", 
				points.values[i]->x, 
				points.values[i]->y, 
				points.values[i]->control_x1, 
				points.values[i]->control_y1, 
				points.values[i]->control_x2, 
				points.values[i]->control_y2);
//printf("SubMask::copy 2\n");
			file->append_text(string);
			file->tag.set_title("/POINT");
			file->append_tag();
		}
		file->append_newline();

		file->tag.set_title("/MASK");
		file->append_tag();
		file->append_newline();
	}
}

void SubMask::dump()
{
	for(int i = 0; i < points.total; i++)
	{
		printf("	  point=%d x=%.2f y=%.2f in_x=%.2f in_y=%.2f out_x=%.2f out_y=%.2f\n",
			i,
			points.values[i]->x, 
			points.values[i]->y, 
			points.values[i]->control_x1, 
			points.values[i]->control_y1, 
			points.values[i]->control_x2, 
			points.values[i]->control_y2);
	}
}


MaskAuto::MaskAuto(EDL *edl, MaskAutos *autos)
 : Auto(edl, autos)
{
	mode = MASK_SUBTRACT_ALPHA;
	feather = 0;
	value = 100;
    radius = 1;

// We define a fixed number of submasks so that interpolation for each
// submask matches.

	for(int i = 0; i < SUBMASKS; i++)
		masks.append(new SubMask(this));
}

MaskAuto::~MaskAuto()
{
	masks.remove_all_objects();
}

int MaskAuto::operator==(Auto &that)
{
	return identical((MaskAuto*)&that);
}



int MaskAuto::operator==(MaskAuto &that)
{
	return identical((MaskAuto*)&that);
}


int MaskAuto::identical(MaskAuto *src)
{
	if(value != src->value ||
		mode != src->mode ||
		!EQUIV(feather, src->feather) ||
		!EQUIV(radius, src->radius) ||
		masks.size() != src->masks.size()) return 0;

	for(int i = 0; i < masks.size(); i++)
		if(!(*masks.values[i] == *src->masks.values[i])) return 0;

	return 1;
}

void MaskAuto::update_parameter(MaskAuto *ref, MaskAuto *src)
{
	if(src->value != ref->value)
	{
		this->value = src->value;
	}

	if(src->mode != ref->mode)
	{
		this->mode = src->mode;
	}

	if(!EQUIV(src->feather, ref->feather))
	{
		this->feather = src->feather;
	}

	if(!EQUIV(src->radius, ref->radius))
	{
		this->radius = src->radius;
	}

	for(int i = 0; i < masks.size(); i++)
	{
		if(!src->get_submask(i)->equivalent(*ref->get_submask(i)))
			this->get_submask(i)->copy_from(*src->get_submask(i));
	}
}

void MaskAuto::copy_from(Auto *src)
{
	copy_from((MaskAuto*)src);
}

void MaskAuto::copy_from(MaskAuto *src)
{
	Auto::copy_from(src);
	copy_data(src);
}

void MaskAuto::copy_data(MaskAuto *src)
{
	mode = src->mode;
	feather = src->feather;
	value = src->value;
    radius = src->radius;

	masks.remove_all_objects();
	for(int i = 0; i < src->masks.size(); i++)
	{
		masks.append(new SubMask(this));
		masks.values[i]->copy_from(*src->masks.values[i]);
	}
}


SubMask* MaskAuto::get_submask(int number)
{
	CLAMP(number, 0, masks.size() - 1);
	return masks.values[number];
}

void MaskAuto::get_points(ArrayList<MaskPoint*> *points, 
	int submask)
{
	points->remove_all_objects();
	SubMask *submask_ptr = get_submask(submask);
	for(int i = 0; i < submask_ptr->points.size(); i++)
	{
		MaskPoint *point = new MaskPoint;
		point->copy_from(*submask_ptr->points.get(i));
		points->append(point);
	}
}

void MaskAuto::set_points(ArrayList<MaskPoint*> *points, 
	int submask)
{
	SubMask *submask_ptr = get_submask(submask);
	submask_ptr->points.remove_all_objects();
	for(int i = 0; i < points->size(); i++)
	{
		MaskPoint *point = new MaskPoint;
		point->copy_from(*points->get(i));
		submask_ptr->points.append(point);
	}
}


void MaskAuto::load(FileXML *file)
{
	mode = file->tag.get_property("MODE", mode);
	feather = file->tag.get_property("FEATHER", feather);
	radius = file->tag.get_property("RADIUS", radius);
	value = file->tag.get_property("VALUE", value);
	for(int i = 0; i < masks.size(); i++)
	{
		delete masks.values[i];
		masks.values[i] = new SubMask(this);
	}

	int result = 0;
	while(!result)
	{
		result = file->read_tag();

		if(!result)
		{
			if(file->tag.title_is("/AUTO")) 
				result = 1;
			else
			if(file->tag.title_is("MASK"))
			{
				SubMask *mask = masks.values[file->tag.get_property("NUMBER", 0)];
				mask->load(file);
			}
		}
	}
//	dump();
}

void MaskAuto::copy(int64_t start, int64_t end, FileXML *file, int default_auto)
{
	file->tag.set_title("AUTO");
	file->tag.set_property("MODE", mode);
	file->tag.set_property("VALUE", value);
	file->tag.set_property("FEATHER", feather);
	file->tag.set_property("RADIUS", radius);
	if(default_auto)
		file->tag.set_property("POSITION", 0);
	else
		file->tag.set_property("POSITION", position - start);
	file->append_tag();
	file->append_newline();

	for(int i = 0; i < masks.size(); i++)
	{
//printf("MaskAuto::copy 1 %p %d %p\n", this, i, masks.values[i]);
		masks.values[i]->copy(file);
//printf("MaskAuto::copy 10\n");
	}

	file->append_newline();
	file->tag.set_title("/AUTO");
	file->append_tag();
	file->append_newline();
}

void MaskAuto::dump()
{
	printf("	 mode=%d value=%d\n", mode, value);
	for(int i = 0; i < masks.size(); i++)
	{
		printf("	 submask %d\n", i);
		masks.values[i]->dump();
	}
}

void MaskAuto::translate_submasks(float translate_x, float translate_y)
{
	for(int i = 0; i < masks.size(); i++)
	{
		SubMask *mask = get_submask(i);
		for (int j = 0; j < mask->points.total; j++) 
		{
			mask->points.values[j]->x += translate_x;
			mask->points.values[j]->y += translate_y;
		}
	}
}

void MaskAuto::scale_submasks(int orig_scale, int new_scale)
{
	for(int i = 0; i < masks.size(); i++)
	{
		SubMask *mask = get_submask(i);
		for (int j = 0; j < mask->points.total; j++) 
		{
			float orig_x = mask->points.values[j]->x * orig_scale;
			float orig_y = mask->points.values[j]->y * orig_scale;
			mask->points.values[j]->x = orig_x / new_scale;
			mask->points.values[j]->y = orig_y / new_scale;
			
			orig_x = mask->points.values[j]->control_x1 * orig_scale;
			orig_y = mask->points.values[j]->control_y1 * orig_scale;
			mask->points.values[j]->control_x1 = orig_x / new_scale;
			mask->points.values[j]->control_y1 = orig_y / new_scale;
			
			orig_x = mask->points.values[j]->control_x2 * orig_scale;
			orig_y = mask->points.values[j]->control_y2 * orig_scale;
			mask->points.values[j]->control_x2 = orig_x / new_scale;
			mask->points.values[j]->control_y2 = orig_y / new_scale;
		}
	}
}


