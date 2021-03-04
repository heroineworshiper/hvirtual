
/*
 * CINELERRA
 * Copyright (C) 2017 Adam Williams <broadcast at earthling dot net>
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

#ifndef TRANSLATEWIN_H
#define TRANSLATEWIN_H

#include "guicast.h"

class SphereTranslateWin;
class SphereTranslateText;


#include "filexml.h"
#include "mutex.h"
#include "pluginclient.h"
#include "spheretranslate.h"




class SphereTranslateSlider : public BC_FSlider
{
public:
	SphereTranslateSlider(SphereTranslateMain *client, 
		SphereTranslateWin *gui,
		SphereTranslateText *text,
		float *output, 
		int x, 
		int y, 
		float min,
		float max);
	int handle_event();

	SphereTranslateWin *gui;
	SphereTranslateMain *client;
	SphereTranslateText *text;
	float *output;
};

class SphereTranslateText : public BC_TextBox
{
public:
	SphereTranslateText(SphereTranslateMain *client, 
		SphereTranslateWin *gui,
		SphereTranslateSlider *slider,
		float *output, 
		int x, 
		int y);
	int handle_event();

	SphereTranslateWin *gui;
	SphereTranslateMain *client;
	SphereTranslateSlider *slider;
	float *output;
};


class SphereTranslateToggle : public BC_CheckBox
{
public:
	SphereTranslateToggle(SphereTranslateMain *client, 
		int *output, 
		int x, 
		int y,
		const char *text);
	int handle_event();

	SphereTranslateMain *client;
	int *output;
};



class SphereTranslateWin : public PluginClientWindow
{
public:
	SphereTranslateWin(SphereTranslateMain *client);
	~SphereTranslateWin();

	void create_objects();
	int new_control(SphereTranslateSlider **slider, 
		SphereTranslateText **text,
		float *value,
		int x,
		int y,
		char *title_text,
		float min,
		float max);

	SphereTranslateSlider *translate_x, *translate_y, *translate_z;
	SphereTranslateSlider *rotate_x, *rotate_y, *rotate_z;
	SphereTranslateText *translate_x_text, *translate_y_text, *translate_z_text;
	SphereTranslateText *rotate_x_text, *rotate_y_text, *rotate_z_text;

	SphereTranslateSlider *pivot_x, *pivot_y;
	SphereTranslateText *pivot_x_text, *pivot_y_text;
	
	SphereTranslateToggle *draw_pivot;

	SphereTranslateMain *client;
};

class SphereTranslateCoord : public BC_TumbleTextBox
{
public:
	SphereTranslateCoord(SphereTranslateWin *win, 
		SphereTranslateMain *client, 
		int x, 
		int y, 
		float *value);
	~SphereTranslateCoord();
	int handle_event();

	SphereTranslateMain *client;
	SphereTranslateWin *win;
	float *value;
};


#endif
