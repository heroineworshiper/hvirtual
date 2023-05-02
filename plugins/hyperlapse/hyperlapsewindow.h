/*
 * CINELERRA
 * Copyright (C) 2008-2014 Adam Williams <broadcast at earthling dot net>
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

#ifndef HYPERLAPSEWINDOW_H
#define HYPERLAPSEWINDOW_H


#include "guicast.h"

class Hyperlapse;
class HyperlapseWindow;


class HyperlapseVectors : public BC_CheckBox
{
public:
	HyperlapseVectors(HyperlapseWindow *gui,
		int x, 
		int y);
	int handle_event();
	HyperlapseWindow *gui;
};

class HyperlapseDoStabilization : public BC_CheckBox
{
public:
	HyperlapseDoStabilization(HyperlapseWindow *gui,
		int x, 
		int y);
	int handle_event();
	HyperlapseWindow *gui;
};

class HyperlapseBlockSize : public BC_IPot
{
public:
	HyperlapseBlockSize(HyperlapseWindow *gui,
		int x, 
		int y);
	int handle_event();
	HyperlapseWindow *gui;
};

class HyperlapseSearchRadius : public BC_IPot
{
public:
	HyperlapseSearchRadius(HyperlapseWindow *gui,
		int x, 
		int y);
	int handle_event();
	HyperlapseWindow *gui;
};

class HyperlapseMaxMovement : public BC_IPot
{
public:
	HyperlapseMaxMovement(HyperlapseWindow *gui,
		int x, 
		int y);
	int handle_event();
	HyperlapseWindow *gui;
};

class HyperlapseSettling : public BC_IPot
{
public:
	HyperlapseSettling(HyperlapseWindow *gui,
		int x, 
		int y);
	int handle_event();
	HyperlapseWindow *gui;
};

class HyperlapseWindow : public PluginClientWindow
{
public:
	HyperlapseWindow(Hyperlapse *plugin);
	~HyperlapseWindow();

	void create_objects();

	Hyperlapse *plugin;
	HyperlapseVectors *vectors;
	HyperlapseDoStabilization *do_stabilization;
	HyperlapseBlockSize *block_size;
	HyperlapseSearchRadius *search_radius;
// not implemented
//	HyperlapseMaxMovement *max_movement;
	HyperlapseSettling *settling_speed;
};



#endif

