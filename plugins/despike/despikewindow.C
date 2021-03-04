
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
#include "despikewindow.h"
#include "language.h"
#include <string.h>








DespikeWindow::DespikeWindow(Despike *despike)
 : PluginClientWindow(despike, 
	DP(230), 
	DP(110), 
	DP(230), 
	DP(110), 
	0)
{ 
	this->despike = despike; 
}

DespikeWindow::~DespikeWindow()
{
}

void DespikeWindow::create_objects()
{
	int x = DP(10), y = DP(10);
	add_tool(new BC_Title(DP(5), y, _("Maximum level:")));
	y += DP(20);
	add_tool(level = new DespikeLevel(despike, x, y));
	y += DP(30);
	add_tool(new BC_Title(DP(5), y, _("Maximum rate of change:")));
	y += DP(20);
	add_tool(slope = new DespikeSlope(despike, x, y));
	show_window();
}





DespikeLevel::DespikeLevel(Despike *despike, int x, int y)
 : BC_FSlider(x, 
 	y, 
	0,
	DP(200),
	DP(200),
	INFINITYGAIN, 
	0,
	despike->config.level)
{
	this->despike = despike;
}
int DespikeLevel::handle_event()
{
	despike->config.level = get_value();
	despike->send_configure_change();
	return 1;
}

DespikeSlope::DespikeSlope(Despike *despike, int x, int y)
 : BC_FSlider(x, 
 	y, 
	0,
	DP(200),
	DP(200),
	INFINITYGAIN, 
	0,
	despike->config.slope)
{
	this->despike = despike;
}
int DespikeSlope::handle_event()
{
	despike->config.slope = get_value();
	despike->send_configure_change();
	return 1;
}
