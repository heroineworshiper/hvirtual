/*
 * CINELERRA
 * Copyright (C) 2014-2017 Adam Williams <broadcast at earthling dot net>
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
#include "bcsignals.h"
#include "clip.h"
#include "language.h"
#include "hyperlapse.h"
#include "hyperlapsewindow.h"
#include "theme.h"

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
PRINT_TRACE
	int x1 = 10, x = 10, y = 10;
	int x2 = 310;
	int margin = plugin->get_theme()->widget_border;
	BC_Title *title;



	add_subwindow(vectors = new HyperlapseVectors(this,
		x,
		y));
	y += vectors->get_h() + margin;
	add_subwindow(do_stabilization = new HyperlapseDoStabilization(this,
		x,
		y));
	y += do_stabilization->get_h() + margin;
	add_subwindow(title = new BC_Title(x, y, _("Block size:")));
	add_subwindow(block_size = new HyperlapseBlockSize(this,
		x + title->get_w() + margin,
		y));
	y += block_size->get_h() + margin;
	add_subwindow(title = new BC_Title(x, y, _("Search radius:")));
	add_subwindow(search_radius = new HyperlapseSearchRadius(this,
		x + title->get_w() + margin,
		y));
	y += search_radius->get_h() + margin;
	add_subwindow(title = new BC_Title(x, y, _("Settling speed:")));
	add_subwindow(settling_speed = new HyperlapseSettling(this,
		x + title->get_w() + margin,
		y));

	show_window(1);
PRINT_TRACE
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








HyperlapseDoStabilization::HyperlapseDoStabilization(HyperlapseWindow *gui,
	int x, 
	int y)
 : BC_CheckBox(x, 
 	y, 
	gui->plugin->config.do_stabilization,
	_("Do stabilization"))
{
	this->gui = gui;
}

int HyperlapseDoStabilization::handle_event()
{
	gui->plugin->config.do_stabilization = get_value();
	gui->plugin->send_configure_change();
	return 1;
}





HyperlapseBlockSize::HyperlapseBlockSize(HyperlapseWindow *gui,
	int x, 
	int y)
 : BC_IPot(x, 
	y, 
	(int64_t)gui->plugin->config.block_size,
	(int64_t)5,
	(int64_t)100)
{
	this->gui = gui;
}

int HyperlapseBlockSize::handle_event()
{
	gui->plugin->config.block_size = (int)get_value();
	gui->plugin->send_configure_change();
	return 1;
}


HyperlapseSearchRadius::HyperlapseSearchRadius(HyperlapseWindow *gui,
	int x, 
	int y)
 : BC_IPot(x, 
	y, 
	(int64_t)gui->plugin->config.search_radius,
	(int64_t)1,
	(int64_t)100)
{
	this->gui = gui;
}

int HyperlapseSearchRadius::handle_event()
{
	gui->plugin->config.search_radius = (int)get_value();
	gui->plugin->send_configure_change();
	return 1;
}


HyperlapseMaxMovement::HyperlapseMaxMovement(HyperlapseWindow *gui,
	int x, 
	int y)
 : BC_IPot(x, 
	y, 
	(int64_t)gui->plugin->config.max_movement,
	(int64_t)1,
	(int64_t)100)
{
	this->gui = gui;
}

int HyperlapseMaxMovement::handle_event()
{
	gui->plugin->config.max_movement = (int)get_value();
	gui->plugin->send_configure_change();
	return 1;
}



HyperlapseSettling::HyperlapseSettling(HyperlapseWindow *gui,
	int x, 
	int y)
 : BC_IPot(x, 
	y, 
	(int64_t)gui->plugin->config.settling_speed,
	(int64_t)0,
	(int64_t)100)
{
	this->gui = gui;
}

int HyperlapseSettling::handle_event()
{
	gui->plugin->config.settling_speed = (int)get_value();
	gui->plugin->send_configure_change();
	return 1;
}
















