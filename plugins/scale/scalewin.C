/*
 * CINELERRA
 * Copyright (C) 2008-2025 Adam Williams <broadcast at earthling dot net>
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
#include "scale.h"
#include "theme.h"



ScaleWin::ScaleWin(ScaleMain *client)
 : PluginClientWindow(client,
	DP(180), 
	DP(200), 
	DP(180), 
	DP(200), 
	0)
{ 
	this->client = client; 
}

ScaleWin::~ScaleWin()
{
}

void ScaleWin::create_objects()
{
    int margin = client->get_theme()->widget_border;
	int x =margin, y = margin;

    BC_Title *title;
	add_tool(title = new BC_Title(x, y, _("X Scale:")));
	y += title->get_h() + margin;
	width = new ScaleWidth(this, client, x, y);
	width->create_objects();
	y += width->get_h() + margin;
	add_tool(title = new BC_Title(x, y, _("Y Scale:")));
	y += title->get_h() + margin;
	height = new ScaleHeight(this, client, x, y);
	height->create_objects();
	y += height->get_h() + margin;
	add_tool(constrain = new ScaleToggle(client, 
        x, 
        y, 
        _("Constrain ratio"), 
        &client->config.constrain));
    y += constrain->get_h() + margin;
	add_tool(nearest = new ScaleToggle(client, 
        x, 
        y, 
        _("Nearest neighbor"), 
        &client->config.nearest));
	show_window();
	flush();
}

ScaleWidth::ScaleWidth(ScaleWin *win, 
	ScaleMain *client, 
	int x, 
	int y)
 : BC_TumbleTextBox(win,
 	(float)client->config.w,
	(float)0,
	(float)100,
	x, 
	y, 
	DP(100))
{
//printf("ScaleWidth::ScaleWidth %f\n", client->config.w);
	this->client = client;
	this->win = win;
	set_increment(0.01);
}

ScaleWidth::~ScaleWidth()
{
}

int ScaleWidth::handle_event()
{
	client->config.w = atof(get_text());
	CLAMP(client->config.w, 0, 100);

	if(client->config.constrain)
	{
		client->config.h = client->config.w;
		win->height->update(client->config.h);
	}

//printf("ScaleWidth::handle_event 1 %f\n", client->config.w);
	client->send_configure_change();
	return 1;
}




ScaleHeight::ScaleHeight(ScaleWin *win, ScaleMain *client, int x, int y)
 : BC_TumbleTextBox(win,
 	(float)client->config.h,
	(float)0,
	(float)100,
	x, 
	y, 
	DP(100))
{
	this->client = client;
	this->win = win;
	set_increment(0.01);
}
ScaleHeight::~ScaleHeight()
{
}
int ScaleHeight::handle_event()
{
	client->config.h = atof(get_text());
	CLAMP(client->config.h, 0, 100);

	if(client->config.constrain)
	{
		client->config.w = client->config.h;
		win->width->update(client->config.w);
	}

	client->send_configure_change();
	return 1;
}






ScaleToggle::ScaleToggle(ScaleMain *client, 
    int x, 
    int y, 
    const char *text, 
    int *value)
 : BC_CheckBox(x, y, *value, text)
{
	this->client = client;
    this->value = value;
}
ScaleToggle::~ScaleToggle()
{
}
int ScaleToggle::handle_event()
{
	*value = get_value();
	client->send_configure_change();
    return 0;
}


