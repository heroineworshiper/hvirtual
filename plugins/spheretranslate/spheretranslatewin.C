
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

#include "bcdisplayinfo.h"
#include "clip.h"
#include "language.h"
#include "spheretranslate.h"
#include "spheretranslatewin.h"
#include "theme.h"

#define TEXT_W DP(90)

SphereTranslateSlider::SphereTranslateSlider(SphereTranslateMain *client, 
	SphereTranslateWin *gui,
	SphereTranslateText *text,
	float *output, 
	int x, 
	int y, 
	float min,
	float max)
 : BC_FSlider(x, 
 	y, 
	0, 
	gui->get_w() - client->get_theme()->widget_border * 3 - TEXT_W, 
	gui->get_w() - client->get_theme()->widget_border * 3 - TEXT_W, 
	min, 
	max, 
	*output)
{
	this->gui = gui;
	this->client = client;
	this->output = output;
	this->text = text;
	set_precision(0.01);
}

int SphereTranslateSlider::handle_event()
{
	float prev_output = *output;
	*output = get_value();
	text->update(*output);


	client->send_configure_change();
	return 1;
}



SphereTranslateText::SphereTranslateText(SphereTranslateMain *client, 
	SphereTranslateWin *gui,
	SphereTranslateSlider *slider,
	float *output, 
	int x, 
	int y)
 : BC_TextBox(x, y, TEXT_W, 1, *output)
{
	this->gui = gui;
	this->client = client;
	this->output = output;
	this->slider = slider;
}

int SphereTranslateText::handle_event()
{
	float prev_output = *output;
	*output = atof(get_text());
	slider->update(*output);


	client->send_configure_change();
	return 1;
}








SphereTranslateToggle::SphereTranslateToggle(SphereTranslateMain *client, 
	int *output, 
	int x, 
	int y,
	const char *text)
 : BC_CheckBox(x, y, *output, text)
{
	this->output = output;
	this->client = client;
}

int SphereTranslateToggle::handle_event()
{
	*output = get_value();
	client->send_configure_change();
	return 1;
}










SphereTranslateWin::SphereTranslateWin(SphereTranslateMain *client)
 : PluginClientWindow(client,
	DP(300), 
	DP(300), 
	DP(300), 
	DP(300), 
	0)
{ 
	this->client = client; 
}

SphereTranslateWin::~SphereTranslateWin()
{
}

int SphereTranslateWin::new_control(SphereTranslateSlider **slider, 
	SphereTranslateText **text,
	float *value,
	int x,
	int y,
	char *title_text,
	float min,
	float max)
{
	int margin = client->get_theme()->widget_border;
	BC_Title *title = 0;
	add_tool(title = new BC_Title(x, y, title_text));
	y += title->get_h() + margin;
	add_tool((*slider) = new SphereTranslateSlider(client, 
		this,
		0,
		value, 
		x, 
		y, 
		min,
		max));
	(*slider)->set_precision(0.1);
	x += (*slider)->get_w() + margin;
	add_tool((*text) = new SphereTranslateText(client, 
		this,
		(*slider),
		value, 
		x, 
		y));
	(*slider)->text = (*text);
	y += (*text)->get_h() + margin;
	return y;
}

void SphereTranslateWin::create_objects()
{
	int margin = client->get_theme()->widget_border;
	int x = margin;
	int y = margin;

// 	y = new_control(&translate_x, 
// 		&translate_x_text,
// 		&client->config.translate_x,
// 		x,
// 		y,
// 		_("Translate X:"),
// 		-1,
// 		1);
// 
// 	y = new_control(&translate_y, 
// 		&translate_y_text,
// 		&client->config.translate_y,
// 		x,
// 		y,
// 		_("Translate Y:"),
// 		-1,
// 		1);
// 
// 	y = new_control(&translate_z, 
// 		&translate_z_text,
// 		&client->config.translate_z,
// 		x,
// 		y,
// 		_("Translate Z:"),
// 		-1,
// 		1);

	y = new_control(&rotate_x, 
		&rotate_x_text,
		&client->config.rotate_x,
		x,
		y,
		_("Rotate X:"),
		-180,
		180);

	y = new_control(&rotate_y, 
		&rotate_y_text,
		&client->config.rotate_y,
		x,
		y,
		_("Rotate Y:"),
		-180,
		180);

	y = new_control(&rotate_z, 
		&rotate_z_text,
		&client->config.rotate_z,
		x,
		y,
		_("Rotate Z:"),
		-180,
		180);

	y = new_control(&pivot_x, 
		&pivot_x_text,
		&client->config.pivot_x,
		x,
		y,
		_("Pivot X:"),
		0,
		100);

	y = new_control(&pivot_y, 
		&pivot_y_text,
		&client->config.pivot_y,
		x,
		y,
		_("Pivot Y:"),
		0,
		100);

	add_tool(draw_pivot = new SphereTranslateToggle(client, 
		&client->config.draw_pivot, 
		x, 
		y,
		_("Draw pivot")));

	show_window(1);
}


