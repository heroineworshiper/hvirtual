
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
#include "bchash.h"
#include "filesystem.h"
#include "gainwindow.h"
#include "language.h"

#include <string.h>







GainWindow::GainWindow(Gain *gain)
 : PluginClientWindow(gain, 
	DP(230), 
	DP(60), 
	DP(230), 
	DP(60), 
	0)
{
	this->gain = gain;
}

GainWindow::~GainWindow()
{
}

void GainWindow::create_objects()
{
	int x = DP(10), y = DP(10);
	add_tool(new BC_Title(DP(5), y, _("Level:")));
	y += DP(20);
	add_tool(level = new GainLevel(gain, x, y));
	show_window();
}








GainLevel::GainLevel(Gain *gain, int x, int y)
 : BC_FSlider(x, 
 	y, 
	0,
	DP(200),
	DP(200),
	INFINITYGAIN, 
	40,
	gain->config.level)
{
	this->gain = gain;
}
int GainLevel::handle_event()
{
	gain->config.level = get_value();
	gain->send_configure_change();
	return 1;
}
