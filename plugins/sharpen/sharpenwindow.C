/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
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
#include "language.h"
#include "sharpenwindow.h"
#include "theme.h"





#define WINDOW_W DP(220)
#define WINDOW_H DP(150)





SharpenWindow::SharpenWindow(SharpenMain *client)
 : PluginClientWindow(client,
	WINDOW_W, 
	WINDOW_H, 
	WINDOW_W, 
	WINDOW_H, 
	0)
{ 
	this->client = client; 
}

SharpenWindow::~SharpenWindow()
{
}

void SharpenWindow::create_objects()
{
	int widget_border = client->get_theme()->widget_border;
	int window_border = client->get_theme()->window_border;
	int x = window_border, y = window_border;
	BC_Title *title;
    add_tool(title = new BC_Title(x, y, _("Sharpness")));
	y += title->get_h() + widget_border;
	add_tool(sharpen_slider = new SharpenSlider(client, &(client->config.sharpness), x, y));
	y += sharpen_slider->get_h() + widget_border;
	add_tool(sharpen_interlace = new SharpenInterlace(client, x, y));
	y += sharpen_interlace->get_h() + widget_border;
	add_tool(sharpen_horizontal = new SharpenHorizontal(client, x, y));
//	y += sharpen_horizontal->get_h() + widget_border;
//	add_tool(sharpen_luminance = new SharpenLuminance(client, x, y));
	show_window();
	flush();
}






SharpenSlider::SharpenSlider(SharpenMain *client, float *output, int x, int y)
 : BC_ISlider(x, 
 	y, 
	0,
	DP(200), 
	DP(200),
	0, 
	MAXSHARPNESS, 
	(int)*output, 
	0, 
	0, 
	0)
{
	this->client = client;
	this->output = output;
}
SharpenSlider::~SharpenSlider()
{
}
int SharpenSlider::handle_event()
{
	*output = get_value();
	client->send_configure_change();
	return 1;
}




SharpenInterlace::SharpenInterlace(SharpenMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.interlace, _("Interlace"))
{
	this->client = client;
}
SharpenInterlace::~SharpenInterlace()
{
}
int SharpenInterlace::handle_event()
{
	client->config.interlace = get_value();
	client->send_configure_change();
	return 1;
}




SharpenHorizontal::SharpenHorizontal(SharpenMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.horizontal, _("Horizontal only"))
{
	this->client = client;
}
SharpenHorizontal::~SharpenHorizontal()
{
}
int SharpenHorizontal::handle_event()
{
	client->config.horizontal = get_value();
	client->send_configure_change();
	return 1;
}



SharpenLuminance::SharpenLuminance(SharpenMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.luminance, _("Luminance only"))
{
	this->client = client;
}
SharpenLuminance::~SharpenLuminance()
{
}
int SharpenLuminance::handle_event()
{
	client->config.luminance = get_value();
	client->send_configure_change();
	return 1;
}

