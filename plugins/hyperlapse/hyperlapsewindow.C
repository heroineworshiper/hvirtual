/*
 * CINELERRA
 * Copyright (C) 2012 Adam Williams <broadcast at earthling dot net>
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
#include "clip.h"
#include "language.h"
#include "hyperlapse.h"
#include "hyperlapsewindow.h"

HyperlapseWindow::HyperlapseWindow(Hyperlapse *plugin)
 : PluginClientWindow(plugin,
 	320, 
	240, 
	320,
	240,
	0)
{
	this->plugin = plugin; 
}

HyperlapseWindow::~HyperlapseWindow()
{
}

void HyperlapseWindow::create_objects()
{
	int x1 = 10, x = 10, y = 10;
	int x2 = 310;
	BC_Title *title;



	add_subwindow(vectors = new HyperlapseVectors(this,
		x,
		y));

	show_window(1);
}


HyperlapseVectors::HyperlapseVectors(HyperlapseWindow *gui,
	int x, 
	int y)
 : BC_CheckBox(x, 
 	y, 
	gui->plugin->config.draw_vectors,
	_("Draw vectors"))
{
	this->gui = gui;
}

int HyperlapseVectors::handle_event()
{
	gui->plugin->config.draw_vectors = get_value();
	gui->plugin->send_configure_change();
	return 1;
}






