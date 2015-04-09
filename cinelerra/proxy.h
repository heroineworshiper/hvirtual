
/*
 * CINELERRA
 * Copyright (C) 2015 Adam Williams <broadcast at earthling dot net>
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



#ifndef PROXY_H
#define PROXY_H

// functions for handling proxies


#include "asset.h"
#include "bcdialog.h"
#include "formattools.inc"
#include "mwindow.inc"

#include <string>

class ProxyThread;
class ProxyWindow;

class ProxyMenuItem : public BC_MenuItem
{
public:
	ProxyMenuItem(MWindow *mwindow);

	int handle_event();
	void create_objects();

	MWindow *mwindow;
	ProxyThread *thread;
};

class FromProxyMenuItem : public BC_MenuItem
{
public:
	FromProxyMenuItem(MWindow *mwindow);

	int handle_event();
	MWindow *mwindow;
};

class ProxyThread : public BC_DialogThread
{
public:
	ProxyThread(MWindow *mwindow);
	BC_Window* new_gui();
	void handle_close_event(int result);
	static void create_path(string *new_path, Asset *asset, int scale);


	MWindow *mwindow;
	ProxyWindow *gui;
	Asset *asset;
	int new_scale;
	int orig_scale;
};

class ProxyReset : public BC_GenericButton
{
public:
	ProxyReset(int x, int y, MWindow *mwindow, ProxyWindow *pwindow);
	int handle_event();
	MWindow *mwindow;
	ProxyWindow *pwindow;
};

class ProxyMenu : public BC_PopupMenu
{
public:
	ProxyMenu(int x, int y, int w, char *text, MWindow *mwindow, ProxyWindow *pwindow);
	int handle_event();
	MWindow *mwindow;
	ProxyWindow *pwindow;
};


class ProxyTumbler : public BC_Tumbler
{
public:
	ProxyTumbler(MWindow *mwindow, ProxyWindow *pwindow, int x, int y);

	int handle_up_event();
	int handle_down_event();
	
	ProxyWindow *pwindow;
	MWindow *mwindow;
};


class ProxyWindow : public BC_Window
{
public:
	ProxyWindow(MWindow *mwindow, ProxyThread *thread, int x, int y);
	~ProxyWindow();

	void create_objects();
	void update();

	MWindow *mwindow;
	ProxyThread *thread;
	FormatTools *format_tools;
	BC_Title *new_dimensions;
	BC_PopupMenu *scale_factor;
	ProxyReset *reset;
};



#endif






