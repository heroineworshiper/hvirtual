
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

#include "bcdisplayinfo.h"
#include "flipwindow.h"
#include "language.h"







FlipWindow::FlipWindow(FlipMain *client)
 : PluginClientWindow(client,
	DP(140),
	DP(100),
	DP(140),
	DP(100),
	0)
{ 
	this->client = client; 
}

FlipWindow::~FlipWindow()
{
}

void FlipWindow::create_objects()
{
	int x = DP(10), y = DP(10);
	add_tool(flip_vertical = new FlipToggle(client, 
		&(client->config.flip_vertical), 
		_("Vertical"),
		x, 
		y));
	y += DP(30);
	add_tool(flip_horizontal = new FlipToggle(client, 
		&(client->config.flip_horizontal), 
		_("Horizontal"),
		x, 
		y));
	show_window();
}


FlipToggle::FlipToggle(FlipMain *client, int *output, char *string, int x, int y)
 : BC_CheckBox(x, y, *output, string)
{
	this->client = client;
	this->output = output;
}
FlipToggle::~FlipToggle()
{
}
int FlipToggle::handle_event()
{
	*output = get_value();
	client->send_configure_change();
	return 1;
}
