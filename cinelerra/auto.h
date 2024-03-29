
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

#ifndef AUTO_H
#define AUTO_H

#include "auto.inc"
#include "edl.inc"
#include "guicast.h"
#include "filexml.inc"
#include "autos.inc"






// The default constructor is used for menu effects.

class Auto : public ListItem<Auto>
{
public:
	Auto();
	Auto(EDL *edl, Autos *autos);
	virtual ~Auto() {};

	virtual Auto& operator=(Auto &that);
	virtual int operator==(Auto &that);
	virtual void copy_from(Auto *that);
	virtual void copy(int64_t start, 
		int64_t end, 
		FileXML *file, 
		int default_only);

	virtual void load(FileXML *file);

	virtual void get_caption(char *string) {};

 	virtual float value_to_percentage();
 	virtual float invalue_to_percentage();
 	virtual float outvalue_to_percentage();


	int skip;       // if added by selection event for moves
	EDL *edl;
	Autos *autos;
	int WIDTH, HEIGHT;
	int is_default;
// Units native to the track
	int64_t position;
// Calculation to use for floats
	int mode;
	enum
	{
		BEZIER_UNLOCKED,
		LINEAR,
        BEZIER_LOCKED
	}; 

private:
	virtual int value_to_str(char *string, float value) { return 0; };
};



#endif
