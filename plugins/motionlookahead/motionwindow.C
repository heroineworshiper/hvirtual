/*
 * CINELERRA
 * Copyright (C) 2024 Adam Williams <broadcast at earthling dot net>
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

#include "clip.h"
#include "language.h"
#include "motion.h"
#include "motionscan.h"
#include "motionwindow.h"
#include "theme.h"


MotionCheck::MotionCheck(MotionLookahead *plugin, 
	int x, 
	int y,
    int *output,
    const char *text)
 : BC_CheckBox(x, 
 	y, 
	*output,
	text)
{
    this->output = output;
    this->plugin = plugin;
}

int MotionCheck::handle_event()
{
    *output = get_value();
    plugin->send_configure_change();
	return 1;
}


MotionText::MotionText(MotionLookahead *plugin, 
    int x,
    int y,
    float *output)
 : BC_TextBox(x,
 	y,
	DP(100),
	1,
	*output)
{
    this->plugin = plugin;
    output_f = output;
    output_i = 0;
    fpot = 0;
    ipot = 0;
}

MotionText::MotionText(MotionLookahead *plugin, 
    int x,
    int y,
    int *output)
 : BC_TextBox(x,
 	y,
	DP(100),
	1,
	*output)
{
    this->plugin = plugin;
    output_f = 0;
    output_i = output;
}

int MotionText::handle_event()
{
    if(output_f)
    {
        *output_f = atof(get_text());
        fpot->update(*output_f);
    }

    if(output_i)
    {
        *output_i = atoi(get_text());
        ipot->update(*output_i);
    }

    plugin->send_configure_change();
    return 1;
}




MotionFPot::MotionFPot(MotionLookahead *plugin, 
		int x, 
		int y,
        float *output,
        float min,
        float max)
 : BC_FPot(x,
 	y,
	*output,
	min, 
	max)
{
    this->plugin = plugin;
    this->output = output;
    textbox = 0;
}

int MotionFPot::handle_event()
{
    *output = get_value();
    if(textbox) textbox->update(*output);
    plugin->send_configure_change();
    return 1;
}


MotionIPot::MotionIPot(MotionLookahead *plugin, 
		int x, 
		int y,
        int *output,
        int min,
        int max)
 : BC_IPot(x,
 	y,
	(int64_t)*output,
	(int64_t)min, 
	(int64_t)max)
{
    this->plugin = plugin;
    this->output = output;
    textbox = 0;
}

int MotionIPot::handle_event()
{
    *output = get_value();
    if(textbox) textbox->update((int64_t)*output);
    plugin->send_configure_change();
    return 1;
}



MotionSlider::MotionSlider(MotionLookahead *plugin, 
	int x, 
	int y,
    int w,
    int *output,
    int min,
    int max)
 : BC_ISlider(x, 
	y,
	0,
	w, 
	w, 
	(int64_t)min, 
	(int64_t)max, 
	(int64_t)*output)
{
    this->plugin = plugin;
    this->output = output;
}

int MotionSlider::handle_event()
{
    *output = get_value();
    plugin->send_configure_change();
    return 1;
}



MotionLookaheadWindow::MotionLookaheadWindow(MotionLookahead *plugin)
 : PluginClientWindow(plugin,
 	DP(340), 
	DP(400), 
	DP(340),
	DP(400),
	0)
{
	this->plugin = plugin; 
}

MotionLookaheadWindow::~MotionLookaheadWindow()
{
}

void MotionLookaheadWindow::create_objects()
{
    int window_border = plugin->get_theme()->window_border;
    int widget_border = plugin->get_theme()->widget_border;

	int x1 = window_border;
    int x = window_border;
    int y = window_border;
	BC_Title *title1, *title2, *title3, *title4;
    
    
	add_subwindow(enable = new MotionCheck(plugin,
		x,
		y,
        &plugin->config.enable,
        _("Enable")));
    y += enable->get_h() + widget_border;

	add_subwindow(title1 = new BC_Title(x, 
		y, 
		_("Number of frames to look ahead:")));
    y += title1->get_h() + widget_border;

    add_subwindow(frames = new MotionSlider(plugin, 
		x, 
		y,
        get_w() - window_border * 2,
        &plugin->config.frames,
        MIN_LOOKAHEAD,
        MAX_LOOKAHEAD));
    y += frames->get_h() + widget_border;


    BC_Bar *bar;
    add_subwindow(bar = new BC_Bar(x, y, get_w() - window_border * 2));
    y += bar->get_h() + widget_border;

    int y1 = y;

	add_subwindow(title1 = new BC_Title(x, 
		y, 
		_("Translation search radius:\n(W/H Percent of image)")));
    int x2 = x + title1->get_w() + widget_border;
    y += MAX(title1->get_h(), BC_Pot::calculate_h()) + widget_border;

	add_subwindow(title2 = new BC_Title(x1, 
		y, 
		_("Translation block size:\n(W/H Percent of image)")));
    x2 = MAX(x2, x + title2->get_w() + widget_border);
    y += MAX(title2->get_h(), BC_Pot::calculate_h()) + widget_border;

	add_subwindow(title3 = new BC_Title(x, 
        y, 
        _("Block position:\n(X/Y Percent of image)")));
    x2 = MAX(x2, x + title3->get_w() + widget_border);
    y += MAX(title3->get_h(), BC_Pot::calculate_h()) + widget_border;

    add_subwindow(bar = new BC_Bar(x, y, get_w() - window_border * 2));
    y += bar->get_h() + widget_border;

	add_subwindow(do_rotate = new MotionCheck(plugin,
		x,
		y,
        &plugin->config.do_rotate,
        _("Track rotation")));
	y += do_rotate->get_h() + widget_border;
    x2 = MAX(x2, x + do_rotate->get_w() + widget_border);

	add_subwindow(title4 = new BC_Title(x, 
		y, 
		_("Maximum angle offset:")));
    x2 = MAX(x2, x + title4->get_w() + widget_border);
    y += MAX(title4->get_h(), BC_Pot::calculate_h()) + widget_border;

    int y2 = y;
    y = y1;
    x = x2;
	add_subwindow(range_w = new MotionIPot(plugin, 
		x, 
		title1->get_y(),
		&plugin->config.range_w,
        MIN_RADIUS,
        MAX_RADIUS));
    x += range_w->get_w() + widget_border;
    add_subwindow(range_h = new MotionIPot(plugin, 
		x, 
		title1->get_y(),
		&plugin->config.range_h,
        MIN_RADIUS,
        MAX_RADIUS));

    x = x2;
	add_subwindow(block_w = new MotionIPot(plugin, 
		x, 
		title2->get_y(),
		&plugin->config.block_w,
        MIN_RADIUS,
        MAX_RADIUS));
    x += block_w->get_w() + widget_border;
	add_subwindow(block_h = new MotionIPot(plugin, 
		x, 
		title2->get_y(),
		&plugin->config.block_h,
        MIN_RADIUS,
        MAX_RADIUS));

    x = x2;
	add_subwindow(block_x = new MotionFPot(plugin, 
		x, 
		title3->get_y(),
        &plugin->config.block_x,
        (float)0,
        (float)100));
    x += block_x->get_w() + widget_border;
	add_subwindow(block_y = new MotionFPot(plugin, 
		x, 
		title3->get_y(),
        &plugin->config.block_y,
        (float)0,
        (float)100));


    add_subwindow(rotation_range = new MotionIPot(plugin, 
		x2, 
		title4->get_y(),
		&plugin->config.rotation_range,
        MIN_ROTATION,
        MAX_ROTATION));

    x = x1;
    y = y2;

    add_subwindow(bar = new BC_Bar(x, y, get_w() - window_border * 2));
    y += bar->get_h() + widget_border;

	add_subwindow(draw_vectors = new MotionCheck(plugin,
		x,
		y,
        &plugin->config.draw_vectors,
        _("Draw vectors")));

    y += draw_vectors->get_h() + widget_border;
    BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, _("Calculation:")));
    y += title->get_h() + widget_border;
	add_subwindow(tracking_type = new TrackingType(&plugin->config.tracking_type,
        plugin, 
		this, 
		x, 
		y));
	tracking_type->create_objects();

	show_window(1);
}

















