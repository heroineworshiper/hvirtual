
/*
 * CINELERRA
 * Copyright (C) 2008-2021 Adam Williams <broadcast at earthling dot net>
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
#include "bitspopup.h"
#include "file.h"
#include "filesystem.h"
#include "fileformat.h"
#include "language.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "new.h"
#include "theme.h"




FileFormat::FileFormat(MWindow *mwindow)
 : BC_Window(PROGRAM_NAME ": File Format", 
		mwindow->gui->get_abs_cursor_x(0),
		mwindow->gui->get_abs_cursor_y(0),
 		DP(500), 
		DP(300), 
		DP(500), 
		DP(300))
{
	this->mwindow = mwindow;
}

FileFormat::~FileFormat()
{
	lock_window("FileFormat::~FileFormat");
	delete lohi;
	delete hilo;
	delete signed_button;
	delete header_button;
	delete rate_button;
	delete channels_button;
	delete bitspopup;
	unlock_window();
}

void FileFormat::create_objects(Asset *asset, char *string2)
{
// ================================= copy values
	this->asset = asset;
	create_objects_(string2);
}

void FileFormat::create_objects_(char *string2)
{
	BC_Resources *resources = BC_WindowBase::get_resources();
	int margin = mwindow->theme->widget_border;
	FileSystem dir;
	File file;
	char string[BCTEXTLEN];
	int x = margin;
    int y = margin;
    int y1;
	int x1 = x;
    int x2 = 0;
    int text_h = BC_TextBox::calculate_h(this, 
        MEDIUMFONT, 
        1, 
        1);

	lock_window("FileFormat::create_objects_");
    BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, string2));
	y += title->get_h() + margin;
	add_subwindow(title = new BC_Title(x, y, _("Assuming raw PCM:")));
	y += title->get_h() + margin;
    
    y1 = y;
	add_subwindow(title = new BC_Title(x, y, _("Channels:")));
    if(title->get_w() > x2)
    {
        x2 = title->get_w();
    }
    y += text_h + margin;

	add_subwindow(title = new BC_Title(x, y, _("Sample rate:")));
    if(title->get_w() > x2)
    {
        x2 = title->get_w();
    }
    y += text_h + margin;

	add_subwindow(title = new BC_Title(x, y, _("Bits:")));
    if(title->get_w() > x2)
    {
        x2 = title->get_w();
    }
    y += text_h + margin;

	add_subwindow(title = new BC_Title(x, y, _("Header length:")));
    if(title->get_w() > x2)
    {
        x2 = title->get_w();
    }
    y += text_h + margin;

	add_subwindow(title = new BC_Title(x, y, _("Byte order:")));
    if(title->get_w() > x2)
    {
        x2 = title->get_w();
    }
    y = y1;
    x = x2 + margin;
    
    

    

	sprintf(string, "%d", asset->channels);
	channels_button = new FileFormatChannels(x, y, this, string);
	channels_button->create_objects();
	y += text_h + margin;

	sprintf(string, "%d", asset->sample_rate);
	add_subwindow(rate_button = new FileFormatRate(x, y, this, string));
	add_subwindow(new SampleRatePulldown(mwindow, 
        rate_button, 
        x + rate_button->get_w(), 
        y));
	
	y += text_h + margin;
	bitspopup = new BitsPopup(this, 
		x, 
		y, 
		&asset->bits, 
		0, 
		1, 
		1, 
		0, 
		1);
	bitspopup->create_objects();
	
	y += text_h + margin;
	sprintf(string, "%d", asset->header);
	add_subwindow(header_button = new FileFormatHeader(x, y, this, string));
	
	y += text_h + margin;

//printf("FileFormat::create_objects_ 1 %d\n", asset->byte_order);
	add_subwindow(lohi = new FileFormatByteOrderLOHI(x, y, this, asset->byte_order));
	add_subwindow(hilo = new FileFormatByteOrderHILO(x + lohi->get_w() + margin, 
        y, 
        this, 
        !asset->byte_order));
	
	y += text_h + margin;
    x = margin;
	add_subwindow(signed_button = new FileFormatSigned(x, 
        y, 
        this, 
        asset->signed_));
	
	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	
	show_window(1);
	unlock_window();
}

FileFormatChannels::FileFormatChannels(int x, int y, FileFormat *fwindow, char *text)
 : BC_TumbleTextBox(fwindow, 
 	(int)atol(text), 
	(int)1, 
	(int)MAXCHANNELS, 
	x, 
	y, 
	50)
{
	this->fwindow = fwindow;
}

int FileFormatChannels::handle_event()
{
	fwindow->asset->channels = atol(get_text());
	return 0;
}

FileFormatRate::FileFormatRate(int x, int y, FileFormat *fwindow, char *text)
 : BC_TextBox(x, y, 100, 1, text)
{
	this->fwindow = fwindow;
}

int FileFormatRate::handle_event()
{
	fwindow->asset->sample_rate = atol(get_text());
	return 0;
}

FileFormatHeader::FileFormatHeader(int x, int y, FileFormat *fwindow, char *text)
 : BC_TextBox(x, y, 100, 1, text)
{
	this->fwindow = fwindow;
}

int FileFormatHeader::handle_event()
{
	fwindow->asset->header = atol(get_text());
	return 0;
}

FileFormatByteOrderLOHI::FileFormatByteOrderLOHI(int x, int y, FileFormat *fwindow, int value)
 : BC_Radial(x, y, value, _("Lo Hi"))
{
	this->fwindow = fwindow;
}

int FileFormatByteOrderLOHI::handle_event()
{
	update(1);
	fwindow->asset->byte_order = 1;
	fwindow->hilo->update(0);
	return 1;
}

FileFormatByteOrderHILO::FileFormatByteOrderHILO(int x, int y, FileFormat *fwindow, int value)
 : BC_Radial(x, y, value, _("Hi Lo"))
{
	this->fwindow = fwindow;
}

int FileFormatByteOrderHILO::handle_event()
{
	update(1);
	fwindow->asset->byte_order = 0;
	fwindow->lohi->update(0);
	return 1;
}

FileFormatSigned::FileFormatSigned(int x, int y, FileFormat *fwindow, int value)
 : BC_CheckBox(x, y, value, _("Values are signed"))
{
	this->fwindow = fwindow;
}

int FileFormatSigned::handle_event()
{
	fwindow->asset->signed_ = get_value();
	return 1;
}
