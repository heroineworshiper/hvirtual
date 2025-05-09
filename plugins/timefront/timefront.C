/*
 * CINELERRA
 * Copyright (C) 2008-2024 Adam Williams <broadcast at earthling dot net>
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

// Timefront contributed by Andraz Tori & later fixed


#include <math.h>
#include <stdint.h>
#include <string.h>

#include "bcdisplayinfo.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "timefront.h"
#include "keyframe.h"
#include "language.h"
#include "overlayframe.h"
#include "picon_png.h"
#include "vframe.h"
#include "theme.h"




REGISTER_PLUGIN(TimeFrontMain)





TimeFrontConfig::TimeFrontConfig()
{
	angle = 0;
	in_radius = 0;
	out_radius = 100;
	frame_range = 16;
	track_usage = TimeFrontConfig::OTHERTRACK_INTENSITY;
	shape = TimeFrontConfig::LINEAR;
	rate = TimeFrontConfig::LINEAR;
	center_x = 50;
	center_y = 50;
	invert = 0;
	show_grayscale = 0;
}

int TimeFrontConfig::equivalent(TimeFrontConfig &that)
{
	return (EQUIV(angle, that.angle) &&
		EQUIV(in_radius, that.in_radius) &&
		EQUIV(out_radius, that.out_radius) &&
		frame_range == that.frame_range &&
		track_usage == that.track_usage &&
		shape == that.shape &&
		rate == that.rate &&
		EQUIV(center_x, that.center_x) &&
		EQUIV(center_y, that.center_y) && 
		invert == that.invert &&
		show_grayscale == that.show_grayscale);
}

void TimeFrontConfig::copy_from(TimeFrontConfig &that)
{
	angle = that.angle;
	in_radius = that.in_radius;
	out_radius = that.out_radius;
	frame_range = that.frame_range;
	track_usage = that.track_usage;
	shape = that.shape;
	rate = that.rate;
	center_x = that.center_x;
	center_y = that.center_y;
	invert = that.invert;
	show_grayscale = that.show_grayscale;
}

void TimeFrontConfig::interpolate(TimeFrontConfig &prev, 
	TimeFrontConfig &next, 
	long prev_frame, 
	long next_frame, 
	long current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);


	this->angle = (prev.angle * prev_scale + next.angle * next_scale);
	this->in_radius = (prev.in_radius * prev_scale + next.in_radius * next_scale);
	this->out_radius = (prev.out_radius * prev_scale + next.out_radius * next_scale);
	frame_range = (int)(prev.frame_range * prev_scale + next.frame_range * next_scale);
	track_usage = prev.track_usage;
	shape = prev.shape;
	rate = prev.rate;
	center_x = prev.center_x * prev_scale + next.center_x * next_scale;
	center_y = prev.center_y * prev_scale + next.center_y * next_scale;
	invert = prev.invert;
	show_grayscale = prev.show_grayscale;
}








TimeFrontWindow::TimeFrontWindow(TimeFrontMain *plugin)
 : PluginClientWindow(plugin,
	DP(350), 
	DP(290), 
	DP(350), 
	DP(290), 
	0)
{
	this->plugin = plugin;
	angle = 0;
	angle_title = 0;
	center_x = 0;
	center_y = 0;
	center_x_title = 0;
	center_y_title = 0;
	rate_title = 0;
	rate = 0;
	in_radius_title = 0;
	in_radius = 0;
	out_radius_title = 0;
	out_radius = 0;
	track_usage_title = 0;
	track_usage = 0;
	prev_shape = -1;
}

TimeFrontWindow::~TimeFrontWindow()
{
}

void TimeFrontWindow::create_objects()
{
	int x = DP(10), y = DP(10);
	BC_Title *title;

	add_subwindow(title = new BC_Title(x, y, _("Type:")));
	add_subwindow(shape = new TimeFrontShape(plugin, 
		this, 
		x + title->get_w() + DP(10), 
		y));
	shape->create_objects();
	y += DP(40);
	shape_x = x;
	shape_y = y;
	y += DP(140);
	add_subwindow(title = new BC_Title(x, y, _("Time range:")));
	add_subwindow(frame_range = new TimeFrontFrameRange(plugin, x + title->get_w() + DP(10), y));
	frame_range_x = x + frame_range->get_w() + DP(10);
	frame_range_y = y;
	y += DP(35);
	update_shape();
	
	add_subwindow(invert = new TimeFrontInvert(plugin, x, y));
	add_subwindow(show_grayscale = new TimeFrontShowGrayscale(plugin, x + DP(100), y));


	show_window();
	flush();
}

void TimeFrontWindow::update_shape()
{
	int x = shape_x, y = shape_y;

    if(prev_shape == plugin->config.shape) return;

	if(plugin->config.shape == TimeFrontConfig::LINEAR)
	{
		delete center_x_title;
		delete center_y_title;
		delete center_x;
		delete center_y;
		delete track_usage_title;
		delete track_usage;
		center_x_title = 0;
		center_y_title = 0;
		center_x = 0;
		center_y = 0;
		track_usage_title = 0;
		track_usage = 0;
		if(!angle)
		{
			add_subwindow(angle_title = new BC_Title(x, y, _("Angle:")));
			add_subwindow(angle = new TimeFrontAngle(plugin, x + angle_title->get_w() + DP(10), y));
		}
		if(!rate){
			y = shape_y + DP(40);
	
			add_subwindow(rate_title = new BC_Title(x, y, _("Rate:")));
			add_subwindow(rate = new TimeFrontRate(plugin,
				x + rate_title->get_w() + DP(10),
				y));
			rate->create_objects();
			y += DP(40);
			add_subwindow(in_radius_title = new BC_Title(x, y, _("Inner radius:")));
			add_subwindow(in_radius = new TimeFrontInRadius(plugin, x + in_radius_title->get_w() + DP(10), y));
			y += DP(30);
			add_subwindow(out_radius_title = new BC_Title(x, y, _("Outer radius:")));
			add_subwindow(out_radius = new TimeFrontOutRadius(plugin, x + out_radius_title->get_w() + DP(10), y));
			y += DP(35);

		}
	} else
	if(plugin->config.shape == TimeFrontConfig::RADIAL)
	{
		delete angle_title;
		delete angle;
		delete track_usage_title;
		delete track_usage;
		angle_title = 0;
		angle = 0;
		track_usage_title = 0;
		track_usage = 0;
		if(!center_x)
		{
			add_subwindow(center_x_title = new BC_Title(x, y, _("Center X:")));
			add_subwindow(center_x = new TimeFrontCenterX(plugin,
				x + center_x_title->get_w() + DP(10),
				y));
			x += center_x_title->get_w() + DP(10) + center_x->get_w() + DP(10);
			add_subwindow(center_y_title = new BC_Title(x, y, _("Center Y:")));
			add_subwindow(center_y = new TimeFrontCenterY(plugin,
				x + center_y_title->get_w() + DP(10),
				y));
		}
		
		
		if(!rate)
		{
			y = shape_y + DP(40);
			x = shape_x;
			add_subwindow(rate_title = new BC_Title(x, y, _("Rate:")));
			add_subwindow(rate = new TimeFrontRate(plugin,
				x + rate_title->get_w() + DP(10),
				y));
			rate->create_objects();
			y += DP(40);
			add_subwindow(in_radius_title = new BC_Title(x, y, _("Inner radius:")));
			add_subwindow(in_radius = new TimeFrontInRadius(plugin, x + in_radius_title->get_w() + DP(10), y));
			y += DP(30);
			add_subwindow(out_radius_title = new BC_Title(x, y, _("Outer radius:")));
			add_subwindow(out_radius = new TimeFrontOutRadius(plugin, x + out_radius_title->get_w() + DP(10), y));
			y += DP(35);
		}
	} else
	if(plugin->config.shape == TimeFrontConfig::OTHERTRACK)
	{
		delete center_x_title;
		delete center_y_title;
		delete center_x;
		delete center_y;
		delete angle_title;
		delete angle;
		delete rate_title;
		delete rate;
		delete in_radius_title;
		delete in_radius;
		delete out_radius_title;
		delete out_radius;
		center_x_title = 0;
		center_y_title = 0;
		center_x = 0;
		center_y = 0;
		angle_title = 0;
		angle = 0;
		rate_title = 0;
		rate = 0;
		in_radius_title = 0;
		in_radius = 0;
		out_radius_title = 0;
		out_radius = 0;
		if(!track_usage)
		{
			add_subwindow(track_usage_title = new BC_Title(x, y, _("As timefront use:")));
			add_subwindow(track_usage = new TimeFrontTrackUsage(plugin,
				this,
				x + track_usage_title->get_w() + DP(10),
				y));
			track_usage->create_objects();

		}
	} else
	if(plugin->config.shape == TimeFrontConfig::ALPHA)
	{
		delete center_x_title;
		delete center_y_title;
		delete center_x;
		delete center_y;
		delete angle_title;
		delete angle;
		delete rate_title;
		delete rate;
		delete in_radius_title;
		delete in_radius;
		delete out_radius_title;
		delete out_radius;
		delete track_usage_title;
		delete track_usage;
		center_x_title = 0;
		center_y_title = 0;
		center_x = 0;
		center_y = 0;
		angle_title = 0;
		angle = 0;
		rate_title = 0;
		rate = 0;
		in_radius_title = 0;
		in_radius = 0;
		out_radius_title = 0;
		out_radius = 0;
		track_usage_title = 0;
		track_usage = 0;

	}
	show_window();
    prev_shape = plugin->config.shape;
}













TimeFrontShape::TimeFrontShape(TimeFrontMain *plugin, 
	TimeFrontWindow *gui, 
	int x, 
	int y)
 : BC_PopupMenu(x, 
    y, 
    gui->get_w() - x - plugin->get_theme()->widget_border, 
    to_text(plugin->config.shape), 
    1)
{
	this->plugin = plugin;
	this->gui = gui;
}
void TimeFrontShape::create_objects()
{
	add_item(new BC_MenuItem(to_text(TimeFrontConfig::LINEAR)));
	add_item(new BC_MenuItem(to_text(TimeFrontConfig::RADIAL)));
	add_item(new BC_MenuItem(to_text(TimeFrontConfig::ALPHA)));
	add_item(new BC_MenuItem(to_text(TimeFrontConfig::OTHERTRACK)));
}
char* TimeFrontShape::to_text(int shape)
{
	switch(shape)
	{
		case TimeFrontConfig::LINEAR:
			return _("Linear");
		case TimeFrontConfig::OTHERTRACK:
			return _("Other track as timefront");
		case TimeFrontConfig::ALPHA:
			return _("Alpha as timefront");
		default:
			return _("Radial");
	}
}
int TimeFrontShape::from_text(char *text)
{
	if(!strcmp(text, to_text(TimeFrontConfig::LINEAR))) 
		return TimeFrontConfig::LINEAR;
	if(!strcmp(text, to_text(TimeFrontConfig::OTHERTRACK))) 
		return TimeFrontConfig::OTHERTRACK;
	if(!strcmp(text, to_text(TimeFrontConfig::ALPHA))) 
		return TimeFrontConfig::ALPHA;
	return TimeFrontConfig::RADIAL;
}
int TimeFrontShape::handle_event()
{
	plugin->config.shape = from_text(get_text());
	gui->update_shape();
	plugin->send_configure_change();
    return 0;
}


TimeFrontTrackUsage::TimeFrontTrackUsage(TimeFrontMain *plugin, 
	TimeFrontWindow *gui, 
	int x, 
	int y)
 : BC_PopupMenu(x, 
    y, 
    gui->get_w() - x - plugin->get_theme()->widget_border, 
    to_text(plugin->config.track_usage), 1)
{
	this->plugin = plugin;
	this->gui = gui;
}
void TimeFrontTrackUsage::create_objects()
{
	add_item(new BC_MenuItem(to_text(TimeFrontConfig::OTHERTRACK_INTENSITY)));
	add_item(new BC_MenuItem(to_text(TimeFrontConfig::OTHERTRACK_ALPHA)));
}
char* TimeFrontTrackUsage::to_text(int track_usage)
{
	switch(track_usage)
	{
		case TimeFrontConfig::OTHERTRACK_INTENSITY:
			return _("Intensity");
		case TimeFrontConfig::OTHERTRACK_ALPHA:
			return _("Alpha mask");
		default:
			return _("Unknown");
	}
}
int TimeFrontTrackUsage::from_text(char *text)
{
	if(!strcmp(text, to_text(TimeFrontConfig::OTHERTRACK_INTENSITY))) 
		return TimeFrontConfig::OTHERTRACK_INTENSITY;
	if(!strcmp(text, to_text(TimeFrontConfig::OTHERTRACK_ALPHA))) 
		return TimeFrontConfig::OTHERTRACK_ALPHA;

	return TimeFrontConfig::OTHERTRACK_INTENSITY;
}
int TimeFrontTrackUsage::handle_event()
{
	plugin->config.track_usage = from_text(get_text());
	gui->update_shape();
	plugin->send_configure_change();
    return 0;
}




TimeFrontCenterX::TimeFrontCenterX(TimeFrontMain *plugin, int x, int y)
 : BC_FPot(x, y, plugin->config.center_x, 0, DP(100))
{
	this->plugin = plugin;
}
int TimeFrontCenterX::handle_event()
{
	plugin->config.center_x = get_value();
	plugin->send_configure_change();
	return 1;
}



TimeFrontCenterY::TimeFrontCenterY(TimeFrontMain *plugin, int x, int y)
 : BC_FPot(x, y, plugin->config.center_y, 0, DP(100))
{
	this->plugin = plugin;
}

int TimeFrontCenterY::handle_event()
{
	plugin->config.center_y = get_value();
	plugin->send_configure_change();
	return 1;
}




TimeFrontAngle::TimeFrontAngle(TimeFrontMain *plugin, int x, int y)
 : BC_FPot(x,
 	y,
	plugin->config.angle,
	-180,
	180)
{
	this->plugin = plugin;
    set_precision(.1);
}

int TimeFrontAngle::handle_event()
{
	plugin->config.angle = get_value();
	plugin->send_configure_change();
	return 1;
}


TimeFrontRate::TimeFrontRate(TimeFrontMain *plugin, int x, int y)
 : BC_PopupMenu(x,
 	y,
	DP(200),
	to_text(plugin->config.rate),
	1)
{
	this->plugin = plugin;
}
void TimeFrontRate::create_objects()
{
	add_item(new BC_MenuItem(to_text(TimeFrontConfig::LINEAR)));
	add_item(new BC_MenuItem(to_text(TimeFrontConfig::LOG)));
	add_item(new BC_MenuItem(to_text(TimeFrontConfig::SQUARE)));
}
char* TimeFrontRate::to_text(int shape)
{
	switch(shape)
	{
		case TimeFrontConfig::LINEAR:
			return _("Linear");
		case TimeFrontConfig::LOG:
			return _("Log");
		default:
			return _("Square");
	}
}
int TimeFrontRate::from_text(char *text)
{
	if(!strcmp(text, to_text(TimeFrontConfig::LINEAR))) 
		return TimeFrontConfig::LINEAR;
	if(!strcmp(text, to_text(TimeFrontConfig::LOG)))
		return TimeFrontConfig::LOG;
	return TimeFrontConfig::SQUARE;
}
int TimeFrontRate::handle_event()
{
	plugin->config.rate = from_text(get_text());
	plugin->send_configure_change();
	return 1;
}



TimeFrontInRadius::TimeFrontInRadius(TimeFrontMain *plugin, int x, int y)
 : BC_FSlider(x,
 	y,
	0,
	DP(200),
	DP(200),
	(float)0,
	(float)100,
	(float)plugin->config.in_radius)
{
	this->plugin = plugin;
}

int TimeFrontInRadius::handle_event()
{
	plugin->config.in_radius = get_value();
	plugin->send_configure_change();
	return 1;
}


TimeFrontOutRadius::TimeFrontOutRadius(TimeFrontMain *plugin, int x, int y)
 : BC_FSlider(x,
 	y,
	0,
	DP(200),
	DP(200),
	(float)0,
	(float)100,
	(float)plugin->config.out_radius)
{
	this->plugin = plugin;
}

int TimeFrontOutRadius::handle_event()
{
	plugin->config.out_radius = get_value();
	plugin->send_configure_change();
	return 1;
}

TimeFrontFrameRange::TimeFrontFrameRange(TimeFrontMain *plugin, int x, int y)
 : BC_ISlider(x,
 	y,
	0,
	DP(200),
	DP(200),
	(int)MIN_FRAMES,
	(int)MAX_FRAMES,
	(int)plugin->config.frame_range)
{
	this->plugin = plugin;
}

int TimeFrontFrameRange::handle_event()
{
	plugin->config.frame_range = get_value();
	plugin->send_configure_change();
	return 1;
}


TimeFrontInvert::TimeFrontInvert(TimeFrontMain *client, int x, int y)
 : BC_CheckBox(x, 
 	y, 
	client->config.invert, 
	_("Inversion"))
{
	this->plugin = client;
}

int TimeFrontInvert::handle_event()
{
	plugin->config.invert = get_value();
	plugin->send_configure_change();
	return 1;
}

TimeFrontShowGrayscale::TimeFrontShowGrayscale(TimeFrontMain *client, int x, int y)
 : BC_CheckBox(x, 
 	y, 
	client->config.show_grayscale, 
	_("Show grayscale for tuning"))
{
	this->plugin = client;
}

int TimeFrontShowGrayscale::handle_event()
{
	plugin->config.show_grayscale = get_value();
	plugin->send_configure_change();
	return 1;
}



TimeFrontMain::TimeFrontMain(PluginServer *server)
 : PluginVClient(server)
{
	
	need_reconfigure = 1;
	gradient = 0;
	engine = 0;
	overlayer = 0;
}

TimeFrontMain::~TimeFrontMain()
{
	framelist.remove_all_objects();

	if(gradient) delete gradient;
	if(engine) delete engine;
	if(overlayer) delete overlayer;
}

const char* TimeFrontMain::plugin_title() { return N_("TimeFront"); }
int TimeFrontMain::is_realtime() { return 1; }
int TimeFrontMain::is_multichannel() { return 1; }


NEW_PICON_MACRO(TimeFrontMain)

NEW_WINDOW_MACRO(TimeFrontMain, TimeFrontWindow)

LOAD_CONFIGURATION_MACRO(TimeFrontMain, TimeFrontConfig)

int TimeFrontMain::is_synthesis()
{
	return 1;
}

#define GRADIENTFROMAVG(type, inttype, components, maxval) \
	for(int i = 0; i < tfframe->get_h(); i++) \
	{ \
		type *in_row = (type *)tfframe->get_rows()[i]; \
		unsigned char *grad_row = gradient->get_rows()[i]; \
		for(int j = 0; j < tfframe->get_w(); j++) \
		{ \
			inttype tmp =	(inttype) in_row[j * components] + \
						  in_row[j * components + 1] + \
						  in_row[j * components + 2]; \
			if (components == 3) \
				grad_row[j] = (unsigned char) (CLIP((float)config.frame_range * tmp / maxval / 3, 0.0F, config.frame_range)); \
			else if(components == 4) \
				grad_row[j] = (unsigned char) (CLIP((float)config.frame_range * tmp * in_row[j * components + 3] / maxval / maxval / 3, 0.0F, config.frame_range)); \
		} \
	}
	
#define GRADIENTFROMCHANNEL(type, components, max, channel) \
	for(int i = 0; i < tfframe->get_h(); i++) \
	{ \
		type *in_row = (type *)tfframe->get_rows()[i]; \
		unsigned char *grad_row = gradient->get_rows()[i]; \
		for(int j = 0; j < tfframe->get_w(); j++) \
		{ \
			if (components == 3) \
				grad_row[j] = (unsigned char) (CLIP((float)config.frame_range * in_row[j * components + channel] / max, 0.0F, config.frame_range)); \
			else if(components == 4) \
				grad_row[j] = (unsigned char) (CLIP((float)config.frame_range * in_row[j * components + channel] * in_row[j * components + 3]/ max /max, 0.0F, config.frame_range)); \
		} \
	}

#define SETALPHA(type, max) \
	for(int i = 0; i < outframes[0]->get_h(); i++) \
	{ \
		type *out_row = (type *)outframes[0]->get_rows()[i]; \
		for(int j = 0; j < outframes[0]->get_w(); j++) \
		{ \
			out_row[j * 4 + 3] = max; \
		} \
	}

#define GRADIENTTOPICTURE(type, inttype, components, max, invertion) \
	for(int i = 0; i < height; i++) \
	{ \
		type *out_row = (type *)outframes[0]->get_rows()[i]; \
		unsigned char *grad_row = gradient->get_rows()[i]; \
		for (int j = 0; j < width; j++) \
		{ \
			out_row[0] = (inttype)max * (invertion grad_row[0]) / config.frame_range; \
			out_row[1] = (inttype)max * (invertion grad_row[0]) / config.frame_range; \
			out_row[2] = (inttype)max * (invertion grad_row[0]) / config.frame_range; \
			if (components == 4) \
				out_row[3] = max; \
			out_row += components; \
			grad_row ++; \
		} \
	}
	
#define GRADIENTTOYUVPICTURE(type, inttype, components, max, invertion) \
	for(int i = 0; i < height; i++) \
	{ \
		type *out_row = (type *)outframes[0]->get_rows()[i]; \
		unsigned char *grad_row = gradient->get_rows()[i]; \
		for (int j = 0; j < width; j++) \
		{ \
			out_row[0] = (inttype)max * (invertion grad_row[0]) / config.frame_range; \
			out_row[1] = max/2; \
			out_row[2] = max/2; \
			if (components == 4) \
				out_row[3] = max; \
			out_row += components; \
			grad_row ++; \
		} \
	}

#define COMPOSITEIMAGE(type, components, invertion) \
 	for (int i = 0; i < height; i++) \
	{ \
		type *out_row = (type *)outframes[0]->get_rows()[i]; \
		unsigned char *gradient_row = gradient->get_rows()[i]; \
		for (int j = 0; j < width; j++) \
		{ \
			unsigned int choice = invertion gradient_row[j]; \
            if(choice >= framelist.size()) choice = framelist.size() - 1; \
 \
			{ \
				out_row[0] = framelist.get(choice)->get_rows()[i][j * components + 0]; \
				out_row[1] = framelist.get(choice)->get_rows()[i][j * components + 1]; \
				out_row[2] = framelist.get(choice)->get_rows()[i][j * components + 2]; \
				if (components == 4) \
					out_row[3] = framelist.get(choice)->get_rows()[i][j * components + 3]; \
			} \
 \
			out_row += components; \
		} \
	}



int TimeFrontMain::process_buffer(VFrame **frame,
		int64_t start_position,
		double frame_rate)
//int TimeFrontMain::process_realtime(VFrame *input_ptr, VFrame *output_ptr)
{
	VFrame **outframes = frame;
	int width = outframes[0]->get_w();
	int height = outframes[0]->get_h();

	need_reconfigure |= load_configuration();

// It always wants frame_range + 1 so frame_range 2 draws 3 frames including
// the current frame.
    int want_range = config.frame_range + 1;

// resize the frame cache
// the original loaded frame_range + 1 & dropped frame 0 in inverse mode
    if(want_range != framelist.size())
    {
        if(framelist.size() < want_range)
        {
            while(framelist.size() < want_range)
            {
                VFrame *temp;
                framelist.append(temp = new VFrame(outframes[0]->get_w(), 
                    outframes[0]->get_h(), 
                    outframes[0]->get_color_model()));
                temp->clear_frame();
                frame_numbers.append(-1);
            }
        }

        if(framelist.size() > want_range)
        {
            while(framelist.size() > want_range)
            {
                framelist.remove_object();
                frame_numbers.remove();
            }
        }
    }

// update the frame history
    VFrame* new_framelist[want_range];
    int64_t new_frame_numbers[want_range];

// transfer existing frames
    for(int i = 0; i < want_range; i++)
    {
        int64_t want = start_position - i;
        new_frame_numbers[i] = want;
        int got_it = 0;
        for(int j = 0; j < frame_numbers.size(); j++)
        {
            if(frame_numbers.get(j) == want)
            {
                new_framelist[i] = framelist.get(j);
                framelist.remove_number(j);
                frame_numbers.remove_number(j);
                got_it = 1;
                break;
            }
        }
        if(!got_it) new_framelist[i] = 0;
    }

// load missing frames backwards so we read forwards in the timeline
    for(int i = want_range - 1; i >= 0; i--)
    {
        if(!new_framelist[i])
        {
            new_framelist[i] = framelist.get(0);
            framelist.remove_number(0);
            if(new_frame_numbers[i] >= 0)
            {
                read_frame(new_framelist[i],
		            0,
		            new_frame_numbers[i],
		            frame_rate,
		            0);
            }
            else
            {
                new_framelist[i]->clear_frame();
            }
        }
    }

// transfer new frames to the history
    frame_numbers.remove_all();
    for(int i = 0; i < want_range; i++)
    {
        framelist.append(new_framelist[i]);
        frame_numbers.append(new_frame_numbers[i]);
    }
    



// read the current frame
//	framelist[0] = new VFrame (0, -1, outframes[0]->get_w(), outframes[0]->get_h(), outframes[0]->get_color_model(), -1);
// 	read_frame(framelist.get(0),
// 		0,
// 		start_position,
// 		frame_rate,
// 		0);
	this->input = framelist.get(0);
	this->output = outframes[0];
	if (config.shape == TimeFrontConfig::OTHERTRACK)
	{
//		this->output = frame[1];
		if (get_total_buffers() != 2) 
		{
			// FIXME, maybe this should go to some other notification area?
			printf("ERROR: TimeFront plugin - If you are using another track for timefront, you have to have it under shared effects\n");
			return 0;
		}
		if (outframes[0]->get_w() != outframes[1]->get_w() || outframes[0]->get_h() != outframes[1]->get_h())
		{
			printf("Sizes of master track and timefront track do not match\n");
			return 0;
		}
	}

// Generate new gradient
	if(!gradient) gradient = new VFrame(0, 
		-1,
		outframes[0]->get_w(),
		outframes[0]->get_h(),
		BC_A8,
		-1);

	if(need_reconfigure)
	{
		need_reconfigure = 0;
		if (config.shape != TimeFrontConfig::OTHERTRACK &&
		    config.shape != TimeFrontConfig::ALPHA)
		{
			if(!engine) engine = new TimeFrontServer(this,
				get_project_smp() + 1,
				get_project_smp() + 1);
			engine->process_packages();
		}
		
	}

	if (config.shape == TimeFrontConfig::ALPHA)
	{
		VFrame *tfframe = framelist.get(0);
		switch (tfframe->get_color_model())
		{
			case BC_YUVA8888:
			case BC_RGBA8888:
				GRADIENTFROMCHANNEL(unsigned char, 4, 255, 3);


				break;
			case BC_RGBA_FLOAT:
				GRADIENTFROMCHANNEL(float, 4, 1.0f, 3);
				break;
			
			default:
				{
					printf("TimeFront plugin error: ALPHA used, but project color model does not have alpha\n");
					return 1;
					break;
				}
		}

	} else
	if (config.shape == TimeFrontConfig::OTHERTRACK)
	{
		VFrame *tfframe = outframes[1];
		read_frame(tfframe,
			1,
			start_position,
			frame_rate,
			0);
		if (config.track_usage == TimeFrontConfig::OTHERTRACK_INTENSITY)
		{
			switch (tfframe->get_color_model())
			{
				case BC_RGBA8888:
					GRADIENTFROMAVG(unsigned char, unsigned short, 4, 255);       // Has to be 2 ranges bigger, sice we need precision for alpha
					break;
				case BC_RGB888:
					GRADIENTFROMAVG(unsigned char, unsigned short, 3, 255);
					break;
				case BC_RGB_FLOAT:
					GRADIENTFROMAVG(float, float, 3, 1.0f);
					break;
				case BC_RGBA_FLOAT:
					GRADIENTFROMAVG(float, float, 4, 1.0f);
					break;
				case BC_YUV888:							// We cheat and take Y component as intensity
					GRADIENTFROMCHANNEL(unsigned char, 3, 255, 0);
					break;
				case BC_YUVA8888:
					GRADIENTFROMCHANNEL(unsigned char, 4, 255, 0);
					break;
				default:
					break;
			}
		} else
		if (config.track_usage == TimeFrontConfig::OTHERTRACK_ALPHA)
		{
			switch (tfframe->get_color_model())
			{
				case BC_YUVA8888:
				case BC_RGBA8888:
					GRADIENTFROMCHANNEL(unsigned char, 4, 255, 3);


					break;
				case BC_RGBA_FLOAT:
					GRADIENTFROMCHANNEL(float, 4, 1.0f, 3);
					break;
				
				default:
					{
						printf("TimeFront plugin error: ALPHA track used, but project color model does not have alpha\n");
						return 1;
						break;
					}
			}
		} else
		{
			printf("TimeFront plugin error: unsupported track_usage parameter\n");
			return 1;
		}
	}	

// 	if (!config.show_grayscale)
// 	{
// 		for (int i = 1; i <= config.frame_range; i++) 
// 		{
// 			framelist[i] = new VFrame (0, 
// 				-1, 
// 				outframes[0]->get_w(), 
// 				outframes[0]->get_h(), 
// 				outframes[0]->get_color_model(),
// 				-1);
// 
// 			read_frame(framelist[i],
// 				0,
// 				start_position - i,
// 				frame_rate,
// 				0);
// 		}
// 	}
	

	if (config.show_grayscale)
	{
		if (!config.invert)
		{
			switch (outframes[0]->get_color_model())
			{
				case BC_RGB888:
					GRADIENTTOPICTURE(unsigned char, unsigned short, 3, 255, );
					break;
				case BC_RGBA8888:
					GRADIENTTOPICTURE(unsigned char, unsigned short, 4, 255, );
					break;
				case BC_YUV888:
					GRADIENTTOYUVPICTURE(unsigned char, unsigned short, 3, 255, );
					break;
				case BC_YUVA8888:
					GRADIENTTOYUVPICTURE(unsigned char, unsigned short, 4, 255, );
					break;
				case BC_RGB_FLOAT:
					GRADIENTTOPICTURE(float, float, 3, 1.0f, );
					break;
				case BC_RGBA_FLOAT:
					GRADIENTTOPICTURE(float, float, 4, 1.0f, );
					break;
				default:
					break;
			}
		} else
		{
			switch (outframes[0]->get_color_model())
			{
				case BC_RGB888:
					GRADIENTTOPICTURE(unsigned char, unsigned short, 3, 255, config.frame_range -);
					break;
				case BC_RGBA8888:
					GRADIENTTOPICTURE(unsigned char, unsigned short, 4, 255, config.frame_range -);
					break;
				case BC_YUV888:
					GRADIENTTOYUVPICTURE(unsigned char, unsigned short, 3, 255, config.frame_range -);
					break;
				case BC_YUVA8888:
					GRADIENTTOYUVPICTURE(unsigned char, unsigned short, 4, 255, config.frame_range -);
					break;
				case BC_RGB_FLOAT:
					GRADIENTTOPICTURE(float, float, 3, 1.0f, config.frame_range -);
					break;
				case BC_RGBA_FLOAT:
					GRADIENTTOPICTURE(float, float, 4, 1.0f, config.frame_range -);
					break;
				default:
					break;
			}
		}
	} else 
	if (!config.invert)
	{
		switch (outframes[0]->get_color_model())
		{
			case BC_RGB888:
				COMPOSITEIMAGE(unsigned char, 3, );
				break;
			case BC_RGBA8888:
				COMPOSITEIMAGE(unsigned char, 4, );
				break;
			case BC_YUV888:
				COMPOSITEIMAGE(unsigned char, 3, );
				break;
			case BC_YUVA8888:
				COMPOSITEIMAGE(unsigned char, 4, );
				break;
			case BC_RGB_FLOAT:
				COMPOSITEIMAGE(float, 3, );
				break;
			case BC_RGBA_FLOAT:
				COMPOSITEIMAGE(float, 4, );
				break;

			default:
				break;
		}
	} else
	{
		switch (outframes[0]->get_color_model())
		{
			case BC_RGB888:
				COMPOSITEIMAGE(unsigned char, 3, config.frame_range -);
				break;
			case BC_RGBA8888:
				COMPOSITEIMAGE(unsigned char, 4, config.frame_range -);
				break;
			case BC_YUV888:
				COMPOSITEIMAGE(unsigned char, 3, config.frame_range -);
				break;
			case BC_YUVA8888:
				COMPOSITEIMAGE(unsigned char, 4, config.frame_range -);
				break;
			case BC_RGB_FLOAT:
				COMPOSITEIMAGE(float, 3, config.frame_range -);
				break;
			case BC_RGBA_FLOAT:
				COMPOSITEIMAGE(float, 4, config.frame_range -);
				break;

			default:
				break;
		}
	}


	if (config.shape == TimeFrontConfig::ALPHA)
	{
		// Set alpha to max
		switch (outframes[0]->get_color_model())
		{
			case BC_YUVA8888:
			case BC_RGBA8888:
				SETALPHA(unsigned char, 255);
				break;
			case BC_RGBA_FLOAT:
				SETALPHA(float, 1.0f);
				break;
				
			default:
				break;
		}
	}

// 	delete framelist[0];
// 	if (!config.show_grayscale)
// 	{
// 		for (int i = 1; i <= config.frame_range; i++) 
// 			delete framelist[i];
// 	}
	return 0;
}


void TimeFrontMain::update_gui()
{
	if(thread)
	{
		if(load_configuration())
		{
			thread->window->lock_window("TimeFrontMain::update_gui");
            TimeFrontWindow *win = (TimeFrontWindow*)thread->window;
			win->frame_range->update(config.frame_range);
			win->shape->set_text(TimeFrontShape::to_text(config.shape));
			win->show_grayscale->update(config.show_grayscale);
			win->invert->update(config.invert);
			if (win->rate)
				win->rate->set_text(TimeFrontRate::to_text(config.rate));
			if (win->in_radius)
				win->in_radius->update(config.in_radius);
			if (win->out_radius)
				win->out_radius->update(config.out_radius);
			if (win->track_usage)
				win->track_usage->set_text(TimeFrontTrackUsage::to_text(config.track_usage));
			if(win->angle)
				win->angle->update(config.angle);
			if(win->center_x)
				win->center_x->update(config.center_x);
			if(win->center_y)
				win->center_y->update(config.center_y);

			win->update_shape();
			thread->window->unlock_window();
		}
	}
}





void TimeFrontMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->get_data(), MESSAGESIZE);
	output.tag.set_title("TIMEFRONT");

	output.tag.set_property("ANGLE", config.angle);
	output.tag.set_property("IN_RADIUS", config.in_radius);
	output.tag.set_property("OUT_RADIUS", config.out_radius);
	output.tag.set_property("FRAME_RANGE", config.frame_range);
	output.tag.set_property("SHAPE", config.shape);
	output.tag.set_property("TRACK_USAGE", config.track_usage);
	output.tag.set_property("RATE", config.rate);
	output.tag.set_property("CENTER_X", config.center_x);
	output.tag.set_property("CENTER_Y", config.center_y);
	output.tag.set_property("INVERT", config.invert);
	output.tag.set_property("SHOW_GRAYSCALE", config.show_grayscale);
	output.append_tag();
	output.terminate_string();
}

void TimeFrontMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), strlen(keyframe->get_data()));

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("TIMEFRONT"))
			{
				config.angle = input.tag.get_property("ANGLE", config.angle);
				config.rate = input.tag.get_property("RATE", config.rate);
				config.in_radius = input.tag.get_property("IN_RADIUS", config.in_radius);
				config.out_radius = input.tag.get_property("OUT_RADIUS", config.out_radius);
				config.frame_range = input.tag.get_property("FRAME_RANGE", config.frame_range);
				config.shape = input.tag.get_property("SHAPE", config.shape);
				config.track_usage = input.tag.get_property("TRACK_USAGE", config.track_usage);
				config.center_x = input.tag.get_property("CENTER_X", config.center_x);
				config.center_y = input.tag.get_property("CENTER_Y", config.center_y);
				config.invert = input.tag.get_property("INVERT", config.invert);
				config.show_grayscale = input.tag.get_property("SHOW_GRAYSCALE", config.show_grayscale);
			}
		}
	}
    

    CLAMP(config.frame_range, MIN_FRAMES, MAX_FRAMES);
}






TimeFrontPackage::TimeFrontPackage()
 : LoadPackage()
{
}




TimeFrontUnit::TimeFrontUnit(TimeFrontServer *server, TimeFrontMain *plugin)
 : LoadClient(server)
{
	this->plugin = plugin;
	this->server = server;
}


#define LOG_RANGE 1

#define CREATE_GRADIENT \
{ \
/* Synthesize linear gradient for lookups */ \
 \
 	a_table = (unsigned char *)malloc(sizeof(unsigned char) * gradient_size); \
 \
	for(int i = 0; i < gradient_size; i++) \
	{ \
		float opacity; \
		float transparency; \
		switch(plugin->config.rate) \
		{ \
			case TimeFrontConfig::LINEAR: \
				if(i < in_radius) \
					opacity = 0.0; \
				else \
				if(i >= out_radius) \
					opacity = 1.0; \
				else \
					opacity = (float)(i - in_radius) / (out_radius - in_radius); \
				break; \
			case TimeFrontConfig::LOG: \
				opacity = 1 - exp(LOG_RANGE * -(float)(i - in_radius) / (out_radius - in_radius)); \
				break; \
			case TimeFrontConfig::SQUARE: \
				opacity = SQR((float)(i - in_radius) / (out_radius - in_radius)); \
				break; \
		} \
 \
 		CLAMP(opacity, 0, 1); \
		transparency = 1.0 - opacity; \
		a_table[i] = (unsigned char)(out4 * opacity + in4 * transparency); \
	} \
 \
	for(int i = pkg->y1; i < pkg->y2; i++) \
	{ \
		unsigned char *out_row = plugin->gradient->get_rows()[i]; \
 \
 		switch(plugin->config.shape) \
		{ \
			case TimeFrontConfig::LINEAR: \
				for(int j = 0; j < w; j++) \
				{ \
					int x = j - half_w; \
					int y = -(i - half_h); \
		 \
/* Rotate by effect angle */ \
					int input_y = (int)(gradient_size / 2 - \
						(x * sin_angle + y * cos_angle) + \
						0.5); \
		 \
/* Get gradient value from these coords */ \
		 \
 					if(input_y < 0) \
					{ \
						out_row[0] = out4; \
					} \
					else \
					if(input_y >= gradient_size) \
					{ \
						out_row[0] = in4; \
					} \
					else \
					{ \
						out_row[0] = a_table[input_y]; \
					} \
		 \
 					out_row ++; \
				} \
				break; \
 \
			case TimeFrontConfig::RADIAL: \
				for(int j = 0; j < w; j++) \
				{ \
					double x = j - center_x; \
					double y = i - center_y; \
					double magnitude = hypot(x, y); \
					int input_y = (int)magnitude; \
					out_row[0] = a_table[input_y]; \
					out_row ++; \
				} \
				break; \
		} \
	} \
}

void TimeFrontUnit::process_package(LoadPackage *package)
{
	TimeFrontPackage *pkg = (TimeFrontPackage*)package;
	int h = plugin->input->get_h();
	int w = plugin->input->get_w();
	int half_w = w / 2;
	int half_h = h / 2;
	int gradient_size = (int)(ceil(hypot(w, h)));
	int in_radius = (int)(plugin->config.in_radius / 100 * gradient_size);
	int out_radius = (int)(plugin->config.out_radius / 100 * gradient_size);
	double sin_angle = sin(plugin->config.angle * (M_PI / 180));
	double cos_angle = cos(plugin->config.angle * (M_PI / 180));
	double center_x = plugin->config.center_x * w / 100;
	double center_y = plugin->config.center_y * h / 100;
	unsigned char *a_table = 0;

	if(in_radius > out_radius)
	{
	    in_radius ^= out_radius;
	    out_radius ^= in_radius;
	    in_radius ^= out_radius;
	}


	int in4 = plugin->config.frame_range;
	int out4 = 0;
	CREATE_GRADIENT

	if(a_table) free(a_table);
}






TimeFrontServer::TimeFrontServer(TimeFrontMain *plugin, 
	int total_clients, 
	int total_packages)
 : LoadServer(total_clients, total_packages)
{
	this->plugin = plugin;
}

void TimeFrontServer::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		TimeFrontPackage *package = (TimeFrontPackage*)get_package(i);
		package->y1 = plugin->input->get_h() * 
			i / 
			get_total_packages();
		package->y2 = plugin->input->get_h() * 
			(i + 1) /
			get_total_packages();
	}
}

LoadClient* TimeFrontServer::new_client()
{
	return new TimeFrontUnit(this, plugin);
}

LoadPackage* TimeFrontServer::new_package()
{
	return new TimeFrontPackage;
}





