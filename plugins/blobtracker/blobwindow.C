/*
 * CINELERRA
 * Copyright (C) 2026 Adam Williams <broadcast at earthling dot net>
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

#include "blobtracker.h"
#include "blobwindow.h"
#include "clip.h"
#include "language.h"
#include "theme.h"
#include <string.h>

#define WINDOW_W DP(400)
#define SLIDER_W WINDOW_W - x - plugin->get_theme()->widget_border


BlobWindow::BlobWindow(BlobTracker *plugin)
 : PluginClientWindow(plugin,
 	WINDOW_W, 
	DP(200), 
	WINDOW_W,
	DP(200),
	0)
{
    this->plugin = plugin;
}

BlobWindow::~BlobWindow()
{
}


void BlobWindow::create_objects()
{
    int window_border = plugin->get_theme()->window_border;
    int widget_border = plugin->get_theme()->widget_border;
    int x = window_border;
    int y = window_border;
    int x2 = 0;
    int y1 = y;
    BC_Title *title;
    add_subwindow(title = new BC_Title(x, y, _("Algorithm:")));
    x2 = MAX(x + title->get_w(), x2);
    y += BC_PopupMenu::calculate_h() + widget_border;
    add_subwindow(title = new BC_Title(x, y, _("Min blob size:")));
    x2 = MAX(x + title->get_w(), x2);
    y += BC_Slider::get_span(0) + widget_border;
    add_subwindow(title = new BC_Title(x, y, _("Detected size:")));
    x2 = MAX(x + title->get_w(), x2);
    y += title->get_h() + widget_border;
    add_subwindow(title = new BC_Title(x, y, _("Blob layer:")));
    x2 = MAX(x + title->get_w(), x2);
    y += BC_PopupMenu::calculate_h() + widget_border;
    add_subwindow(title = new BC_Title(x, y, _("Target layer:")));
    x2 = MAX(x + title->get_w(), x2);
    y = y1;
   
	add_subwindow(tracking_type = new BlobTrackMode(&plugin->config.mode,
        plugin, 
		this, 
		x2, 
		y));
	tracking_type->create_objects();
    y += tracking_type->get_h() + widget_border;

    add_subwindow(min_size = new BlobSlider(plugin, 
        x2, 
        y, 
        1,
        MAX_BLOB_SIZE,
        &plugin->config.min_size));
    y += min_size->get_h() + widget_border;

    add_subwindow(detected_size = new BC_Title(x2, y, _("Unknown")));
    y += detected_size->get_h() + widget_border;

	add_subwindow(ref_layer = new BlobLayer(&plugin->config.ref_layer,
        plugin, 
		this, 
		x2, 
		y));
	ref_layer->create_objects();
    y += ref_layer->get_h() + widget_border;
	add_subwindow(targ_layer = new BlobLayer(&plugin->config.targ_layer,
        plugin, 
		this, 
		x2, 
		y));
	targ_layer->create_objects();
    y += targ_layer->get_h() + widget_border;
    add_subwindow(draw_guides = new BlobToggle(plugin,
        &plugin->config.draw_guides,
		x, 
		y,
        _("Draw guides")));
    y += draw_guides->get_h() + widget_border;
    add_subwindow(stabilize = new BlobToggle(plugin,
        &plugin->config.stabilize,
		x, 
		y,
        _("Stabilize")));

	show_window(1);
}

void BlobWindow::update_mode()
{
    tracking_type->set_text(BlobTrackMode::to_text(plugin->config.mode));
    ref_layer->set_text(BlobLayer::to_text(plugin->config.ref_layer));
    targ_layer->set_text(BlobLayer::to_text(plugin->config.targ_layer));
}



BlobTrackMode::BlobTrackMode(int *output, 
    PluginClient *plugin, 
    BC_WindowBase *gui, 
    int x, 
    int y)
 : BC_PopupMenu(x, 
 	y, 
	calculate_w(gui),
	to_text(*output))
{
	this->output = output;
    this->plugin = plugin;
}

int BlobTrackMode::handle_event()
{
	*output = from_text(get_text());
	plugin->send_configure_change();
	return 1;
}

void BlobTrackMode::create_objects()
{
//	add_item(new BC_MenuItem(to_text(LEFTMOST_BLOB)));
	add_item(new BC_MenuItem(to_text(LARGEST_BLOB)));
//	add_item(new BC_MenuItem(to_text(BRIGHTEST_WINDOW)));
	add_item(new BC_MenuItem(to_text(BRIGHTEST_BLOB)));
}

int BlobTrackMode::from_text(const char *text)
{
	if(!strcmp(text, _("Leftmost blob"))) return LEFTMOST_BLOB;
	if(!strcmp(text, _("Largest blob"))) return LARGEST_BLOB;
	if(!strcmp(text, _("Brightest window"))) return BRIGHTEST_WINDOW;
	if(!strcmp(text, _("Brightest blob"))) return BRIGHTEST_BLOB;
    return 0;
}

const char* BlobTrackMode::to_text(int mode)
{
	switch(mode)
	{
		case LEFTMOST_BLOB:
			return _("Leftmost blob");
			break;
		case LARGEST_BLOB:
			return _("Largest blob");
			break;
		case BRIGHTEST_WINDOW:
			return _("Brightest window");
			break;
		case BRIGHTEST_BLOB:
			return _("Brightest blob");
			break;
	}
    return 0;
}

int BlobTrackMode::calculate_w(BC_WindowBase *gui)
{
	int result = 0;
//	result = MAX(result, BC_PopupMenu::calculate_w(gui, to_text(LEFTMOST_BLOB)));
	result = MAX(result, BC_PopupMenu::calculate_w(gui, to_text(LARGEST_BLOB)));
//	result = MAX(result, BC_PopupMenu::calculate_w(gui, to_text(BRIGHTEST_WINDOW)));
	result = MAX(result, BC_PopupMenu::calculate_w(gui, to_text(BRIGHTEST_BLOB)));
	return result;
}


BlobLayer::BlobLayer(int *output, PluginClient *plugin, BC_WindowBase *gui, int x, int y)
 : BC_PopupMenu(x, 
 	y, 
	calculate_w(gui),
	to_text(*output))
{
	this->output = output;
    this->plugin = plugin;
}

int BlobLayer::handle_event()
{
	*output = from_text(get_text());
	plugin->send_configure_change();
	return 1;
}


void BlobLayer::create_objects()
{
	add_item(new BC_MenuItem(to_text(0)));
	add_item(new BC_MenuItem(to_text(1)));
}

int BlobLayer::from_text(char *text)
{
	if(!strcmp(text, _("Top"))) return TOP;
	if(!strcmp(text, _("Bottom"))) return BOTTOM;
    return 0;
}

int BlobLayer::calculate_w(BC_WindowBase *gui)
{
	int result = 0;
	result = MAX(result, BC_PopupMenu::calculate_w(gui, to_text(TOP)));
	result = MAX(result, BC_PopupMenu::calculate_w(gui, to_text(BOTTOM)));
	return result;
}

char* BlobLayer::to_text(int mode)
{
    switch(mode)
    {
        case TOP: return _("Top"); break;
        case BOTTOM: return _("Bottom"); break;
    }
    return 0;
};



BlobToggle::BlobToggle(PluginClient *plugin, 
	int *output, 
	int x, 
	int y,
	const char *text)
 : BC_CheckBox(x, y, *output, text)
{
	this->output = output;
	this->plugin = plugin;
}

int BlobToggle::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}






BlobSlider::BlobSlider(BlobTracker *plugin, 
    int x, 
    int y, 
    int min,
    int max,
    int *output)
 : BC_ISlider(x,
	y,
	0,
	SLIDER_W, 
	SLIDER_W, 
	min, 
    max, 
    *output)
{
    this->plugin = plugin;
    this->output = output;
}

int BlobSlider::handle_event()
{
	*output = get_value();
	plugin->send_configure_change ();
	return 1;
}

