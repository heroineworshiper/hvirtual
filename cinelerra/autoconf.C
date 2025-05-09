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

#include "autoconf.h"
#include "bchash.h"
#include "filexml.h"



static int auto_defaults[OVERLAY_TOTAL] = 
{
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
    1, // TRANSITION_OVERLAYS
    1  // PLUGIN_KEYFRAMES
};

AutoConf::AutoConf()
{
    set_all(0);
}

const char* AutoConf::get_show_title(int type)
{
    static const char *show_titles[] = 
    {
	    "SHOW_MUTE",
	    "SHOW_CAMERA_X",
	    "SHOW_CAMERA_Y",
	    "SHOW_CAMERA_Z",
	    "SHOW_PROJECTOR_X",
	    "SHOW_PROJECTOR_Y",
	    "SHOW_PROJECTOR_Z",
	    "SHOW_FADE",
	    "SHOW_PAN",
	    "SHOW_MODE",
	    "SHOW_MASK",
	    "SHOW_SPEED",
        "SHOW_TRANSITIONS",
        "SHOW_PLUGINS"
    };
    
    return show_titles[type];
}

int AutoConf::load_defaults(BC_Hash* defaults)
{
	for(int i = 0; i < OVERLAY_TOTAL; i++)
	{
		autos[i] = defaults->get(get_show_title(i), auto_defaults[i]);
	}
//	transitions = defaults->get("SHOW_TRANSITIONS", 1);
//	plugins = defaults->get("SHOW_PLUGINS", 1);
	return 0;
}

void AutoConf::load_xml(FileXML *file)
{
	for(int i = 0; i < OVERLAY_TOTAL; i++)
	{
		autos[i] = file->tag.get_property(get_show_title(i), auto_defaults[i]);
	}
//	transitions = file->tag.get_property("SHOW_TRANSITIONS", 1);
//	plugins = file->tag.get_property("SHOW_PLUGINS", 1);
}

int AutoConf::save_defaults(BC_Hash* defaults)
{
	for(int i = 0; i < OVERLAY_TOTAL; i++)
	{
		defaults->update(get_show_title(i), autos[i]);
	}
//	defaults->update("SHOW_TRANSITIONS", transitions);
//	defaults->update("SHOW_PLUGINS", plugins);
	return 0;
}

void AutoConf::save_xml(FileXML *file)
{
	for(int i = 0; i < OVERLAY_TOTAL; i++)
	{
		file->tag.set_property(get_show_title(i), autos[i]);
	}
//	file->tag.set_property("SHOW_TRANSITIONS", transitions);
//	file->tag.set_property("SHOW_PLUGINS", plugins);
}

int AutoConf::set_all(int value)
{
	for(int i = 0; i < OVERLAY_TOTAL; i++)
	{
		autos[i] = value;
	}
//	transitions = value;
//	plugins = value;
	return 0;
}


AutoConf& AutoConf::operator=(AutoConf &that)
{
	copy_from(&that);
	return *this;
}

void AutoConf::copy_from(AutoConf *src)
{
	for(int i = 0; i < OVERLAY_TOTAL; i++)
	{
		autos[i] = src->autos[i];
	}
//	transitions = src->transitions;
//	plugins = src->plugins;
}


