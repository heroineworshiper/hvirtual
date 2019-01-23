
/*
 * CINELERRA
 * Copyright (C) 2008-2017 Adam Williams <broadcast at earthling dot net>
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

#include "cdripwindow.h"
#include "language.h"
#include "mwindow.inc"

#include <string.h>

CDRipWindow::CDRipWindow(CDRipMain *cdripper, int x, int y)
 : BC_Window(PROGRAM_NAME ": CD Ripper", 
 	x,
	y,
 	DP(450), 
	DP(230), 
	DP(450), 
	DP(230),
	0,
	0,
	1)
{ 
	this->cdripper = cdripper; 
}

CDRipWindow::~CDRipWindow()
{
}

void CDRipWindow::create_objects()
{
	int y = DP(10), x = DP(10);
	add_tool(new BC_Title(x, y, _("Select the range to transfer:"))); 
	y += DP(25);
	add_tool(new BC_Title(x, y, _("Track"))); 
	x += DP(70);
	add_tool(new BC_Title(x, y, _("Min"))); 
	x += DP(70);
	add_tool(new BC_Title(x, y, _("Sec"))); 
	x += DP(100);

	add_tool(new BC_Title(x, y, _("Track"))); 
	x += DP(70);
	add_tool(new BC_Title(x, y, _("Min"))); 
	x += DP(70);
	add_tool(new BC_Title(x, y, _("Sec"))); 
	x += DP(100);
	
	x = DP(10);  
	y += DP(25);
	add_tool(track1 = new CDRipTextValue(this, &(cdripper->track1), x, y, DP(50)));
	x += DP(70);
	add_tool(min1 = new CDRipTextValue(this, &(cdripper->min1), x, y, DP(50)));
	x += DP(70);
	add_tool(sec1 = new CDRipTextValue(this, &(cdripper->sec1), x, y, DP(50)));
	x += DP(100);
	
	add_tool(track2 = new CDRipTextValue(this, &(cdripper->track2), x, y, DP(50)));
	x += DP(70);
	add_tool(min2 = new CDRipTextValue(this, &(cdripper->min2), x, y, DP(50)));
	x += DP(70);
	add_tool(sec2 = new CDRipTextValue(this, &(cdripper->sec2), x, y, DP(50)));

	x = DP(10);   
	y += DP(30);
	add_tool(new BC_Title(x, y, _("From"), LARGEFONT, RED));
	x += DP(240);
	add_tool(new BC_Title(x, y, _("To"), LARGEFONT, RED));

	x = DP(10);   
	y += DP(35);
	add_tool(new BC_Title(x, y, _("CD Device:")));
	x += DP(100);
	add_tool(device = new CDRipWindowDevice(this, cdripper->device, x, y, DP(200)));

	x = DP(10);   
	y += DP(35);
	add_tool(new BC_OKButton(this));
	x += DP(300);
	add_tool(new BC_CancelButton(this));
	show_window();
}








CDRipTextValue::CDRipTextValue(CDRipWindow *window, int *output, int x, int y, int w)
 : BC_TextBox(x, y, w, 1, *output)
{
	this->output = output;
	this->window = window;
}

CDRipTextValue::~CDRipTextValue()
{
}
	
int CDRipTextValue::handle_event()
{
	*output = atol(get_text());
	return 1;
}

CDRipWindowDevice::CDRipWindowDevice(CDRipWindow *window, char *device, int x, int y, int w)
 : BC_TextBox(x, y, w, 1, device)
{
	this->window = window;
	this->device = device;
}

CDRipWindowDevice::~CDRipWindowDevice()
{
}

int CDRipWindowDevice::handle_event()
{
	strcpy(device, get_text());
	return 1;
}
