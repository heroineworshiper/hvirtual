/*
 * CINELERRA
 * Copyright (C) 2010-2024 Adam Williams <broadcast at earthling dot net>
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
#include "blur.h"
#include "blurwindow.h"
#include "language.h"
#include "theme.h"





BlurWindow::BlurWindow(BlurMain *client)
 : PluginClientWindow(client, 
	DP(300), 
	DP(330), 
	DP(300), 
	DP(330), 
	0)
{ 
	this->client = client; 
	char string[BCTEXTLEN];
// set the default directory
	sprintf(string, "%sblur.rc", BCASTDIR);
    defaults = new BC_Hash(string);
	defaults->load();
    client->lock = defaults->get("LOCK", client->lock);
}

BlurWindow::~BlurWindow()
{
	defaults->update("LOCK", client->lock);
	defaults->save();
    delete defaults;
//printf("BlurWindow::~BlurWindow 1\n");
}

void BlurWindow::create_objects()
{
    int window_border = client->get_theme()->window_border;
    int widget_border = client->get_theme()->widget_border;
	int x = window_border, y = window_border;
	BC_Title *title;

//	add_subwindow(new BC_Title(x, y, _("Blur")));
//	y += DP(20);
	add_subwindow(title = new BC_Title(x, y, _("Horizontal:")));
	y += title->get_h() + widget_border;
	add_subwindow(h = new BlurValue(client, 
        this, 
        x, 
        y, 
        &client->config.horizontal));
	add_subwindow(h_text = new BlurValueText(client, 
        this, 
        x + h->get_w() + widget_border, 
        y, 
        100, 
        &client->config.horizontal));
    h->text = h_text;
    h_text->pot = h;
	y += h->get_h() + widget_border;

	add_subwindow(title = new BC_Title(x, y, _("Vertical:")));
	y += title->get_h() + widget_border;
	add_subwindow(v = new BlurValue(client, 
        this, 
        x, 
        y, 
        &client->config.vertical));
	add_subwindow(v_text = new BlurValueText(client, 
        this, 
        x + h->get_w() + widget_border, 
        y, 
        100, 
        &client->config.vertical));
    v->text = v_text;
    v_text->pot = v;
	y += v->get_h() + widget_border;


    BlurToggle *toggle;
	add_tool(toggle = new BlurToggle(client, 
		&client->lock, 
		x, 
		y,
		"Lock"));
	y += toggle->get_h() + widget_border;




	add_subwindow(a_key = new BlurAKey(client, x, y));
	y += a_key->get_h() + widget_border;
	add_subwindow(a = new BlurA(client, x, y));
	y += a->get_h() + widget_border;
	add_subwindow(r = new BlurR(client, x, y));
	y += r->get_h() + widget_border;
	add_subwindow(g = new BlurG(client, x, y));
	y += g->get_h() + widget_border;
	add_subwindow(b = new BlurB(client, x, y));
	
	show_window();
}

void BlurWindow::sync_values(BlurValue *pot_src)
{
    if(!client->lock) return;

    if(pot_src != h)
    {
        h->update(*pot_src->output);
        h_text->update(*pot_src->output);
        *h->output = *pot_src->output;
    }

    if(pot_src != v)
    {
        v->update(*pot_src->output);
        v_text->update(*pot_src->output);
        *v->output = *pot_src->output;
    }
}



BlurToggle::BlurToggle(BlurMain *client, 
	int *output, 
	int x, 
	int y,
	const char *text)
 : BC_CheckBox(x, y, *output, text)
{
	this->output = output;
	this->client = client;
}

int BlurToggle::handle_event()
{
	*output = get_value();
	client->send_configure_change();
	return 1;
}



BlurValue::BlurValue(BlurMain *client, 
    BlurWindow *gui, 
    int x, 
    int y,
    float *output)
 : BC_FPot(x, 
 	y, 
	*output, 
	0.0, // allow the user to disable it by setting it below MIN_RADIUS
	MAX_RADIUS)
{
	this->client = client;
	this->gui = gui;
    this->output = output;
    set_precision(.1);
}
int BlurValue::handle_event()
{
	*output = get_value();
	text->update(*output);
    gui->sync_values(this);
	client->send_configure_change();
	return 1;
}




BlurValueText::BlurValueText(BlurMain *client, 
    BlurWindow *gui, 
    int x, 
    int y, 
    int w,
    float *output)
 : BC_TextBox(x, 
	y, 
	w, 
	1, // rows
	*output,
    1, // has_border
    MEDIUMFONT,
    1) // precision
{
	this->client = client;
	this->gui = gui;
    this->output = output;
    set_precision(1);
}

int BlurValueText::handle_event()
{
	*output = atof(get_text());
	pot->update(*output);
    gui->sync_values(pot);
	client->send_configure_change();
	return 1;
}




// BlurVertical::BlurVertical(BlurMain *client, BlurWindow *window, int x, int y)
//  : BC_CheckBox(x, 
//  	y, 
// 	client->config.vertical, 
// 	_("Vertical"))
// {
// 	this->client = client;
// 	this->window = window;
// }
// BlurVertical::~BlurVertical()
// {
// }
// int BlurVertical::handle_event()
// {
// 	client->config.vertical = get_value();
// 	client->send_configure_change();
//     return 0;
// }
// 
// BlurHorizontal::BlurHorizontal(BlurMain *client, BlurWindow *window, int x, int y)
//  : BC_CheckBox(x, 
//  	y, 
// 	client->config.horizontal, 
// 	_("Horizontal"))
// {
// 	this->client = client;
// 	this->window = window;
// }
// BlurHorizontal::~BlurHorizontal()
// {
// }
// int BlurHorizontal::handle_event()
// {
// 	client->config.horizontal = get_value();
// 	client->send_configure_change();
//     return 0;
// }




BlurA::BlurA(BlurMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.a, _("Blur alpha"))
{
	this->client = client;
}
int BlurA::handle_event()
{
	client->config.a = get_value();
	client->send_configure_change();
	return 1;
}




BlurAKey::BlurAKey(BlurMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.a_key, _("Alpha determines radius"))
{
	this->client = client;
}
int BlurAKey::handle_event()
{
	client->config.a_key = get_value();
	client->send_configure_change();
	return 1;
}

BlurR::BlurR(BlurMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.r, _("Blur red"))
{
	this->client = client;
}
int BlurR::handle_event()
{
	client->config.r = get_value();
	client->send_configure_change();
	return 1;
}

BlurG::BlurG(BlurMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.g, _("Blur green"))
{
	this->client = client;
}
int BlurG::handle_event()
{
	client->config.g = get_value();
	client->send_configure_change();
	return 1;
}

BlurB::BlurB(BlurMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.b, _("Blur blue"))
{
	this->client = client;
}
int BlurB::handle_event()
{
	client->config.b = get_value();
	client->send_configure_change();
	return 1;
}


