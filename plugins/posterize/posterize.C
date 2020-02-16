
/*
 * CINELERRA
 * Copyright (C) 2008-2020 Adam Williams <broadcast at earthling dot net>
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
#include "filexml.h"
#include "language.h"
#include "posterize.h"
#include "theme.h"

#include <stdio.h>
#include <string.h>


REGISTER_PLUGIN(PosterizeMain)


#define MIN_STEPS 1
#define MAX_STEPS 255

PosterizeConfig::PosterizeConfig()
{
	steps = 255;
}

int PosterizeConfig::equivalent(PosterizeConfig &that)
{
    return steps == that.steps;
}

void PosterizeConfig::copy_from(PosterizeConfig &that)
{
    steps = that.steps;
}

void PosterizeConfig::boundaries()
{
    CLAMP(steps, MIN_STEPS, MAX_STEPS);
}


void PosterizeConfig::interpolate(PosterizeConfig &prev, 
	PosterizeConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);
	this->steps = (int)(prev.steps * prev_scale + next.steps * next_scale);
	
}

PosterizeMain::PosterizeMain(PluginServer *server)
 : PluginVClient(server)
{
	
}

PosterizeMain::~PosterizeMain()
{
	
}

const char* PosterizeMain::plugin_title() { return N_("Posterize"); }
int PosterizeMain::is_realtime() { return 1; }
VFrame* PosterizeMain::new_picon() { return 0; }


NEW_WINDOW_MACRO(PosterizeMain, PosterizeWindow)
LOAD_CONFIGURATION_MACRO(PosterizeMain, PosterizeConfig)


void PosterizeMain::update_gui()
{
	if(thread)
	{
		if(load_configuration())
        {
	        thread->window->lock_window();
            ((PosterizeWindow*)thread->window)->update(1, 1);
	        thread->window->unlock_window();
        }
	}
}



void PosterizeMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->get_data(), MESSAGESIZE);
	output.tag.set_title("POSTERIZE");
	output.tag.set_property("STEPS", config.steps);
	output.append_tag();
	output.terminate_string();
}

void PosterizeMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), strlen(keyframe->get_data()));

	int result = 0;
	float new_threshold;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("POSTERIZE"))
			{
				config.steps = input.tag.get_property("STEPS", config.steps);
			}
		}
	}
    
	config.boundaries();
}


#define PROCESS(type, components, yuv) \
{ \
	for(int i = 0; i < h; i++) \
	{ \
		type *in_row = (type*)frame->get_rows()[i]; \
		type *out_row = (type*)frame->get_rows()[i]; \
 \
        for(int j = 0; j < w; j++) \
        { \
            if(sizeof(type) == 4) \
            { \
                out_row[j * components + 0] = (int)(in_row[j * components + 0] / division) * division; \
                out_row[j * components + 1] = (int)(in_row[j * components + 1] / division) * division; \
                out_row[j * components + 2] = (int)(in_row[j * components + 2] / division) * division; \
            } \
            else \
            { \
                out_row[j * components + 0] = table_r[(int)in_row[j * components + 0]]; \
                out_row[j * components + 1] = table_g[(int)in_row[j * components + 1]]; \
                out_row[j * components + 2] = table_b[(int)in_row[j * components + 2]]; \
            } \
        } \
	} \
}


int PosterizeMain::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	load_configuration();

	read_frame(frame, 
		0, 
		start_position, 
		frame_rate,
		0);

	int w = frame->get_w();
	int h = frame->get_h();

    int table_r[256];
    int table_g[256];
    int table_b[256];
    float division = (float)255 / config.steps;
    for(int i = 0; i < 256; i++)
    {
// luma/red
        table_r[i] = (int)(i / division) * division;
//printf("PosterizeMain::process_buffer %d i=%d %d\n", __LINE__, i, table_r[i]);
//         if(BC_CModels::is_yuv(frame->get_color_model()))
//         {
//             table_g[i] = (int)(i / division) * division;
//             table_b[i] = (int)(i / division) * division;
//         }
//         else
//         {
            table_g[i] = table_r[i];
            table_b[i] = table_r[i];
//        }
    }



	switch(frame->get_color_model())
	{
		case BC_YUV888:
			PROCESS(unsigned char, 3, 1);
			break;
		case BC_YUVA8888:
			PROCESS(unsigned char, 4, 1);
			break;
		case BC_RGB888:
			PROCESS(unsigned char, 3, 0);
			break;
		case BC_RGBA8888:
			PROCESS(unsigned char, 4, 0);
			break;
		case BC_RGB_FLOAT:
            division = (float)1 / config.steps;
			PROCESS(float, 3, 0);
			break;
		case BC_RGBA_FLOAT:
            division = (float)1 / config.steps;
			PROCESS(float, 4, 0);
			break;
	}

	return 0;
}



PosterizeText::PosterizeText(PosterizeMain *plugin, 
    PosterizeWindow *gui,
    int x, 
    int y, 
    int w, 
    int *output)
 : BC_TextBox(x, y, w, 1, (int64_t)*output, 1, MEDIUMFONT)
{
    this->plugin = plugin;
    this->gui = gui;
	this->output = output;
}

int PosterizeText::handle_event()
{
    *output = atoi(get_text());
    gui->update(1, 0);
    plugin->send_configure_change();
    return 1;
}




PosterizeSlider::PosterizeSlider(PosterizeMain *plugin, 
    PosterizeWindow *gui,
	int x, 
	int y, 
    int w,
	int *output,
	int min,
	int max)
 : BC_ISlider(x, y, 0, w, w, min, max, *output)
{
	this->plugin = plugin;
    this->gui = gui;
	this->output = output;
}
int PosterizeSlider::handle_event()
{
	*output = get_value();
    gui->update(0, 1);
	plugin->send_configure_change();
	return 1;
}





PosterizeWindow::PosterizeWindow(PosterizeMain *plugin)
 : PluginClientWindow(plugin,
	DP(300), 
	DP(100), 
	DP(300), 
	DP(100), 
	0)
{ 
	this->plugin = plugin; 
}

PosterizeWindow::~PosterizeWindow()
{
}

void PosterizeWindow::create_objects()
{
    int text_w = DP(100);
	int margin = client->get_theme()->widget_border;
	int x = margin, y = margin;
    BC_Title *title;
	add_tool(title = new BC_Title(x, y, _("Steps per channel:")));
    y += title->get_h() + margin;
    add_tool(slider = new PosterizeSlider(plugin, 
        this,
	    x, 
	    y, 
        get_w() - text_w - margin - margin - margin,
	    &plugin->config.steps,
	    MIN_STEPS,
	    MAX_STEPS));
    x += slider->get_w() + margin;
    add_tool(text = new PosterizeText(plugin, 
        this,
        x, 
        y, 
        text_w, 
        &plugin->config.steps));

	show_window();
	flush();
}

void PosterizeWindow::update(int do_slider, int do_text)
{
	if(do_text) text->update((int64_t)plugin->config.steps);
	if(do_slider) slider->update(plugin->config.steps);
}





