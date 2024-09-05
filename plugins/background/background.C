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

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "bcdisplayinfo.h"
#include "clip.h"
#include "background.h"
#include "bchash.h"
#include "filexml.h"
#include "keyframe.h"
#include "language.h"
#include "overlayframe.h"
#include "theme.h"
#include "vframe.h"




REGISTER_PLUGIN(BackgroundMain)






BackgroundConfig::BackgroundConfig()
{
	r = 0xff;
	g = 0xff;
	b = 0xff;
	a = 0xff;
    type = COLOR;
}

int BackgroundConfig::equivalent(BackgroundConfig &that)
{
	return (r == that.r &&
		g == that.g &&
		b == that.b &&
		a == that.a &&
        type == that.type);
}

void BackgroundConfig::copy_from(BackgroundConfig &that)
{
	r = that.r;
	g = that.g;
	b = that.b;
	a = that.a;
    type = that.type;
}

void BackgroundConfig::interpolate(BackgroundConfig &prev, 
	BackgroundConfig &next, 
	long prev_frame, 
	long next_frame, 
	long current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);


	r = (int)(prev.r * prev_scale + next.r * next_scale);
	g = (int)(prev.g * prev_scale + next.g * next_scale);
	b = (int)(prev.b * prev_scale + next.b * next_scale);
	a = (int)(prev.a * prev_scale + next.a * next_scale);
    type = prev.type;
}

int BackgroundConfig::get_color()
{
	int result = (r << 16) | (g << 8) | (b);
	return result;
}




BackgroundColorObjects::BackgroundColorObjects(BackgroundWindow *window, 
        BackgroundMain *plugin,
        int x,
        int y)
 : ColorObjects(window, 1, x, y)
{
    this->window = window;
    this->plugin = plugin;
}

void BackgroundColorObjects::handle_event()
{
	float r, g, b;
	HSV::hsv_to_rgb(r, g, b, h, s, v);
	plugin->config.r = r * 255;
	plugin->config.g = g * 255;
	plugin->config.b = b * 255;
	plugin->config.a = a * 255;
	plugin->send_configure_change();
}



BackgroundType::BackgroundType(BackgroundWindow *window, 
    BackgroundMain *plugin,
    int x,
    int y,
    int w)
 : BC_PopupMenu(x,
    y,
    w,
    BackgroundType::format_to_text(plugin->config.type),
    1)
{
    this->window = window;
    this->plugin = plugin;
}
int BackgroundType::handle_event()
{
    plugin->config.type = text_to_format(get_text());
	plugin->send_configure_change();
    return 1;
}
char* BackgroundType::format_to_text(int format)
{
    switch(format)
    {
        case COLOR: return _("Color");
        case LUT:   return _("Color Cube");
    }
    return _("Color");
}
int BackgroundType::text_to_format(char *text)
{
    if(!strcmp(format_to_text(COLOR), text)) return COLOR;
    if(!strcmp(format_to_text(LUT), text)) return LUT;
    return COLOR;
}






BackgroundWindow::BackgroundWindow(BackgroundMain *plugin)
 : PluginClientWindow(plugin,
	ColorObjects::calculate_w(), 
	ColorObjects::calculate_h() + DP(100), 
	ColorObjects::calculate_w(), 
	ColorObjects::calculate_h() + DP(100), 
	0)
{
	this->plugin = plugin;
}

BackgroundWindow::~BackgroundWindow()
{
}

void BackgroundWindow::create_objects()
{
	int margin = plugin->get_theme()->widget_border;
	int left_margin = client->get_theme()->window_border;
	int top_margin = client->get_theme()->window_border;
	int x = left_margin, y = top_margin;

    color_objs = new BackgroundColorObjects(this, plugin, x, y);
    color_objs->create_objects();
    y += color_objs->calculate_h();

    BC_Title *title;
    add_subwindow(title = new BC_Title(x, y, _("Background type:")));
    y += title->get_h() + margin;
    add_subwindow(type = new BackgroundType(this, 
        plugin,
        x,
        y,
        get_w() - x - left_margin));
    type->add_item(new BC_MenuItem(BackgroundType::format_to_text(COLOR)));
    type->add_item(new BC_MenuItem(BackgroundType::format_to_text(LUT)));
	update();

	show_window();
}

void BackgroundWindow::update()
{
 	color_objs->r = (float)plugin->config.r / 255;
 	color_objs->g = (float)plugin->config.g / 255;
 	color_objs->b = (float)plugin->config.b / 255;
 	HSV::rgb_to_hsv(color_objs->r, 
        color_objs->g, 
        color_objs->b, 
        color_objs->h, 
        color_objs->s, 
        color_objs->v);
 	color_objs->a = (float)plugin->config.a / 255;
    type->set_text(BackgroundType::format_to_text(plugin->config.type));
}




BackgroundMain::BackgroundMain(PluginServer *server)
 : PluginVClient(server)
{
}

BackgroundMain::~BackgroundMain()
{
}

const char* BackgroundMain::plugin_title() { return N_("Background"); }
int BackgroundMain::is_realtime() { return 1; }

VFrame* BackgroundMain::new_picon() { return 0; }

NEW_WINDOW_MACRO(BackgroundMain, BackgroundWindow)

LOAD_CONFIGURATION_MACRO(BackgroundMain, BackgroundConfig)

int BackgroundMain::is_synthesis()
{
	return 1;
}


int BackgroundMain::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	load_configuration();

	int need_alpha = config.a != 0xff;
	if(need_alpha)
    {
		read_frame(frame, 
			0, 
			start_position, 
			frame_rate,
			get_use_opengl());
	}
    
    int w = frame->get_w();
    int h = frame->get_h();
    
    
#define DRAW_COLOR(type, temp, components, is_yuv) \
{ \
    temp c1, c2, c3, a; \
    temp transparency; \
    temp max; \
    if(is_yuv) \
    { \
        yuv.rgb_to_yuv_8(config.r, \
			config.g, \
			config.b, \
			c1, \
			c2, \
			c3); \
        a = config.a; \
        transparency = 0xff - a; \
        max = 0xff; \
 \
/* multiply alpha */ \
        if(components == 3) \
        { \
            c1 = a * c1 / 0xff; \
            c2 = (a * c2 + 0x80 * transparency) / 0xff; \
            c3 = (a * c3 + 0x80 * transparency) / 0xff; \
        } \
    } \
    else \
    { \
        c1 = config.r; \
        c2 = config.g; \
        c3 = config.b; \
        a = config.a; \
        transparency = 0xff - a; \
        max = 0xff; \
        if(sizeof(type) == 4) \
        { \
            c1 /= 0xff; \
            c2 /= 0xff; \
            c3 /= 0xff; \
            a /= 0xff; \
            transparency /= 0xff; \
            max = 1.0; \
        } \
 \
/* multiply alpha */ \
        if(components == 3) \
        { \
            c1 = a * c1 / max; \
            c2 = a * c2 / max; \
            c3 = a * c3 / max; \
        } \
    } \
 \
    for(int i = 0; i < h; i++) \
    { \
        type *row = (type*)frame->get_rows()[i]; \
        for(int j = 0; j < w; j++) \
        { \
            if(components == 3) \
            { \
                *row++ = c1; \
                *row++ = c2; \
                *row++ = c3; \
            } \
            else \
            { \
                row[0] = (transparency * row[0] + a * c1) / max; \
                row[1] = (transparency * row[1] + a * c2) / max; \
                row[2] = (transparency * row[2] + a * c3) / max; \
                row[3] = MAX(row[3], a); \
                row += components; \
            } \
        } \
    } \
}

// shades of R, G, & B in the LUT image
#define STEPS 64
#define SQRT_STEPS 8
// size of the LUT image 
#define W (STEPS * SQRT_STEPS)
#define H (STEPS * SQRT_STEPS)

#define DRAW_LUT(type, temp, components, is_yuv, max) \
{ \
    type r, g, b; \
    type c1, c2, c3; \
    for(int i = 0; i < h; i++) \
    { \
        type *row = (type*)frame->get_rows()[i]; \
        for(int j = 0; j < w; j++) \
        { \
            r = (temp)(j % STEPS) * max / 64; \
            g = (temp)(i % STEPS) * max / 64; \
            b = (temp)((i / STEPS) * SQRT_STEPS + j / STEPS) * max / 64; \
            if(is_yuv) \
            { \
                yuv.rgb_to_yuv_8(r, \
			        g, \
			        b, \
			        c1, \
			        c2, \
			        c3); \
            } \
            else \
            { \
                c1 = r; \
                c2 = g; \
                c3 = b; \
            } \
            if(components == 3) \
            { \
                *row++ = c1; \
                *row++ = c2; \
                *row++ = c3; \
            } \
            else \
            { \
                row[0] = c1; \
                row[1] = c2; \
                row[2] = c3; \
                row[3] = max; \
                row += components; \
            } \
        } \
    } \
}

    if(config.type == COLOR)
    {
        switch(frame->get_color_model())
        {
            case BC_RGB888:
                DRAW_COLOR(uint8_t, int, 3, 0)
                break;
            case BC_RGB_FLOAT:
                DRAW_COLOR(float, float, 3, 0)
                break;
            case BC_YUV888:
                DRAW_COLOR(uint8_t, int, 3, 1)
                break;
            case BC_RGBA8888:
                DRAW_COLOR(uint8_t, int, 4, 0)
                break;
            case BC_RGBA_FLOAT:
                DRAW_COLOR(float, float, 4, 0)
                break;
            case BC_YUVA8888:
                DRAW_COLOR(uint8_t, int, 4, 1)
                break;
        }
    }
    else
    {
        w = MIN(w, W);
        h = MIN(h, H);
        frame->clear_frame();
        switch(frame->get_color_model())
        {
            case BC_RGB888:
                DRAW_LUT(uint8_t, int, 3, 0, 0xff);
                break;
            case BC_RGBA8888:
                DRAW_LUT(uint8_t, int, 4, 0, 0xff);
                break;
            case BC_YUV888:
                DRAW_LUT(uint8_t, int, 3, 1, 0xff);
                break;
            case BC_YUVA8888:
                DRAW_LUT(uint8_t, int, 4, 1, 0xff);
                break;
            case BC_RGB_FLOAT:
                DRAW_LUT(float, float, 3, 0, 1.0);
                break;
            case BC_RGBA_FLOAT:
                DRAW_LUT(float, float, 4, 0, 1.0);
                break;
        }
    }


	return 0;
}


void BackgroundMain::update_gui()
{
	if(thread)
	{
		if(load_configuration())
		{
			((BackgroundWindow*)thread->window)->lock_window("BackgroundMain::update_gui");
			((BackgroundWindow*)thread->window)->update();
			((BackgroundWindow*)thread->window)->unlock_window();
		}
	}
}





void BackgroundMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->get_data(), MESSAGESIZE);
	output.tag.set_title("BACKGROUND");

	output.tag.set_property("R", config.r);
	output.tag.set_property("G", config.g);
	output.tag.set_property("B", config.b);
	output.tag.set_property("A", config.a);
	output.tag.set_property("TYPE", config.type);
	output.append_tag();
	output.terminate_string();
}

void BackgroundMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), strlen(keyframe->get_data()));

	while(!input.read_tag())
	{
		if(input.tag.title_is("BACKGROUND"))
		{
			config.r = input.tag.get_property("R", config.r);
			config.g = input.tag.get_property("G", config.g);
			config.b = input.tag.get_property("B", config.b);
			config.a = input.tag.get_property("A", config.a);
			config.type = input.tag.get_property("TYPE", config.type);
		}
	}
}
