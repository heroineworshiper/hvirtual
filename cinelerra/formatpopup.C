/*
 * CINELERRA
 * Copyright (C) 2008-2022 Adam Williams <broadcast at earthling dot net>
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

#include "bcsignals.h"
#include "file.inc"
#include "formatpopup.h"
#include "language.h"
#include "pluginserver.h"



FormatPopup::FormatPopup(ArrayList<PluginServer*> *plugindb, 
	int x, 
	int y,
	int use_brender)
 : BC_ListBox(x, 
 	y, 
	DP(200), 
	DP(200),
	LISTBOX_TEXT,
	0,
	0,
	0,
	1,
	0,
	1)
{
	this->plugindb = plugindb;
	this->use_brender = use_brender;
	set_tooltip(_("Change file format"));
}


// Can't use language translations since these are the names which go into the EDL.
void FormatPopup::create_objects()
{
	if(!use_brender)
	{
        format_items.append(new BC_ListBoxItem(COMMAND_NAME));
		format_items.append(new BC_ListBoxItem(AC3_NAME));
		format_items.append(new BC_ListBoxItem(AIFF_NAME));
		format_items.append(new BC_ListBoxItem(AU_NAME));
		format_items.append(new BC_ListBoxItem(FLAC_NAME));
		format_items.append(new BC_ListBoxItem(JPEG_NAME));
	}

	format_items.append(new BC_ListBoxItem(JPEG_LIST_NAME));

	if(!use_brender)
	{
		format_items.append(new BC_ListBoxItem(AVI_NAME));
		format_items.append(new BC_ListBoxItem(EXR_NAME));
		format_items.append(new BC_ListBoxItem(EXR_LIST_NAME));
		format_items.append(new BC_ListBoxItem(WAV_NAME));
		format_items.append(new BC_ListBoxItem(MOV_NAME));
		format_items.append(new BC_ListBoxItem(AMPEG_NAME));
		format_items.append(new BC_ListBoxItem(VMPEG_NAME));
		format_items.append(new BC_ListBoxItem(OGG_NAME));
		format_items.append(new BC_ListBoxItem(PCM_NAME));
		format_items.append(new BC_ListBoxItem(PNG_NAME));
	}

	format_items.append(new BC_ListBoxItem(PNG_LIST_NAME));

	if(!use_brender)
	{
		format_items.append(new BC_ListBoxItem(TGA_NAME));
	}

	format_items.append(new BC_ListBoxItem(TGA_LIST_NAME));

	if(!use_brender)
	{
		format_items.append(new BC_ListBoxItem(TIFF_NAME));
	}

	format_items.append(new BC_ListBoxItem(TIFF_LIST_NAME));
	update(&format_items,
		0,
		0,
		1);
}

FormatPopup::~FormatPopup()
{
	for(int i = 0; i < format_items.total; i++) delete format_items.values[i];
}

int FormatPopup::handle_event()
{
    return 0;
}
