/*
 * CINELERRA
 * Copyright (C) 2020-2024 Adam Williams <broadcast at earthling dot net>
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


// output 1 frame for each block of frames
// take the most similar frame in the current block of frames to the previous frame
// this is for 3D printers where you want the bed in the same position


#include "bcdisplayinfo.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "guicast.h"
#include "keyframe.h"
#include "language.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "transportque.inc"
#include "pluginvclient.h"
#include "theme.h"
#include "vframe.h"

#include <string.h>
#include <stdint.h>


class TimelapseHelper;
class TimelapseHelperWindow;


class TimelapseHelperConfig
{
public:
	TimelapseHelperConfig();
	void copy_from(TimelapseHelperConfig &config);
	int equivalent(TimelapseHelperConfig &config);
    void interpolate(TimelapseHelperConfig &prev, 
	    TimelapseHelperConfig &next, 
	    int64_t prev_frame, 
	    int64_t next_frame, 
	    int64_t current_frame);

	int block_size;
};




class TimelapseHelperSize : public BC_TumbleTextBox
{
public:
	TimelapseHelperSize(TimelapseHelper *plugin, 
		TimelapseHelperWindow *gui, 
		int x, 
		int y,
        int w);
	int handle_event();
	TimelapseHelper *plugin;
	TimelapseHelperWindow *gui;
};


class TimelapseHelperWindow : public PluginClientWindow
{
public:
	TimelapseHelperWindow(TimelapseHelper *plugin);
	~TimelapseHelperWindow();

	void create_objects();

	TimelapseHelper *plugin;
	TimelapseHelperSize *size;
};





class TimelapseHelper : public PluginVClient
{
public:
	TimelapseHelper(PluginServer *server);
	~TimelapseHelper();

	PLUGIN_CLASS_MEMBERS(TimelapseHelperConfig)

	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();

	int64_t calculate_difference(VFrame *frame1, VFrame *frame2);


// frame used from the last block
	VFrame *ref;
};












TimelapseHelperConfig::TimelapseHelperConfig()
{
	block_size = 10;
}

void TimelapseHelperConfig::copy_from(TimelapseHelperConfig &config)
{
	this->block_size = config.block_size;
}

int TimelapseHelperConfig::equivalent(TimelapseHelperConfig &config)
{
	return this->block_size == config.block_size;
}

void TimelapseHelperConfig::interpolate(TimelapseHelperConfig &prev, 
	TimelapseHelperConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	this->block_size = next.block_size;
}






#define TEXT _("Number of frames per block:")
#define W BC_Title::calculate_w(MWindow::instance->gui, TEXT) + MWindow::theme->window_border * 2

TimelapseHelperWindow::TimelapseHelperWindow(TimelapseHelper *plugin)
 : PluginClientWindow(plugin, 
	W, 
	DP(160), 
	W, 
	DP(160), 
	0)
{
	this->plugin = plugin;
}

TimelapseHelperWindow::~TimelapseHelperWindow()
{
}

void TimelapseHelperWindow::create_objects()
{
    int margin = client->get_theme()->window_border;
    int margin2 = client->get_theme()->widget_border;
	int x = margin, y = margin;

	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, _("Number of frames per block:")));
	y += title->get_h() + margin2;
	size = new TimelapseHelperSize(plugin, 
		this, 
		x, 
		y,
        get_w() - x - margin - BC_Tumbler::calculate_w());
    size->create_objects();
	show_window();
}













TimelapseHelperSize::TimelapseHelperSize(TimelapseHelper *plugin, 
	TimelapseHelperWindow *gui, 
	int x, 
	int y,
    int w)
 : BC_TumbleTextBox(gui,
    plugin->config.block_size, 
    1,
    1000,
    x,
	y, 
	w)
{
	this->plugin = plugin;
	this->gui = gui;
}

int TimelapseHelperSize::handle_event()
{
	plugin->config.block_size = atoi(get_text());
	plugin->send_configure_change();
	return 1;
}










REGISTER_PLUGIN(TimelapseHelper)






TimelapseHelper::TimelapseHelper(PluginServer *server)
 : PluginVClient(server)
{
	ref = 0;
}


TimelapseHelper::~TimelapseHelper()
{
	if(ref)
    {
        delete ref;
    }
}

#define DIFFERENCE_MACRO(type, temp_type, components) \
{ \
	temp_type result2 = 0; \
	for(int i = 0; i < h; i++) \
	{ \
		type *row1 = (type*)frame1->get_rows()[i]; \
		type *row2 = (type*)frame2->get_rows()[i]; \
		for(int j = 0; j < w * components; j++) \
		{ \
			temp_type temp = *row1 - *row2; \
			result2 += (temp > 0 ? temp : -temp); \
			row1++; \
			row2++; \
		} \
	} \
	result = (int64_t)result2; \
}

int64_t TimelapseHelper::calculate_difference(VFrame *frame1, VFrame *frame2)
{
	int w = frame1->get_w();
	int h = frame1->get_h();
	int64_t result = 0;
	switch(frame1->get_color_model())
	{
		case BC_RGB888:
		case BC_YUV888:
			DIFFERENCE_MACRO(unsigned char, int64_t, 3);
			break;
		case BC_RGB_FLOAT:
			DIFFERENCE_MACRO(float, double, 3);
			break;
		case BC_RGBA8888:
		case BC_YUVA8888:
			DIFFERENCE_MACRO(unsigned char, int64_t, 4);
			break;
		case BC_RGBA_FLOAT:
			DIFFERENCE_MACRO(float, double, 4);
			break;
		case BC_RGB161616:
		case BC_YUV161616:
			DIFFERENCE_MACRO(uint16_t, int64_t, 3);
			break;
		case BC_RGBA16161616:
		case BC_YUVA16161616:
			DIFFERENCE_MACRO(uint16_t, int64_t, 4);
			break;
	}
	return result;
}



int TimelapseHelper::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	load_configuration();

// calculate frame positions
    int64_t ref_position = get_source_start();
    int64_t block_start;
    int64_t block_end;
    block_start = ref_position + (start_position - ref_position) * config.block_size;
    block_end = block_start + config.block_size;

printf("TimelapseHelper::process_buffer %d current_position=%ld ref_position=%ld block_start=%ld block_end=%ld\n", 
__LINE__, 
start_position,
ref_position,
block_start,
block_end);

// load initial reference frame from plugin start
	if(!ref)
	{
		ref = new VFrame(0,
			-1,
			frame->get_w(),
			frame->get_h(),
			frame->get_color_model(),
			-1);
        read_frame(ref, 
			0, 
			ref_position, 
			frame_rate,
			0);
        frame->copy_from(ref);
	}
    else
// compare next block of frames to reference frame
    {
        VFrame *temp = new_temp(frame->get_w(),
			frame->get_h(),
			frame->get_color_model());
        int64_t best = 0x7fffffffffffffffLL;
        for(int64_t i = block_start; i < block_end; i++)
        {

            if(get_direction() == PLAY_FORWARD)
            {
                read_frame(temp, 
			        0, 
			        i, 
			        frame_rate,
			        0);
            }
            else
            {
                read_frame(temp, 
			        0, 
			        i + 1, 
			        frame_rate,
			        0);
            }
            
            int64_t diff = calculate_difference(temp, 
					ref);
            if(diff < best)
            {
                best = diff;
                frame->copy_from(temp);
            }
        }

// replace reference frame with best match
        ref->copy_from(frame);
    }

	return 0;
}



const char* TimelapseHelper::plugin_title() { return N_("Timelapse Helper"); }
int TimelapseHelper::is_realtime() { return 1; }

NEW_WINDOW_MACRO(TimelapseHelper, TimelapseHelperWindow)

LOAD_CONFIGURATION_MACRO(TimelapseHelper, TimelapseHelperConfig)
VFrame* TimelapseHelper::new_picon() { return 0; }


void TimelapseHelper::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->get_data());
	output.tag.set_title("TIMELAPSEHELPER");
	output.tag.set_property("BLOCK_SIZE", config.block_size);
	output.append_tag();
	output.terminate_string();
}

void TimelapseHelper::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data());

	int result = 0;

	while(!input.read_tag())
	{
		if(input.tag.title_is("TIMELAPSEHELPER"))
		{
			config.block_size = input.tag.get_property("BLOCK_SIZE", config.block_size);
        }
	}
}

void TimelapseHelper::update_gui()
{
	if(thread)
	{
		if(load_configuration())
		{
			((TimelapseHelperWindow*)thread->window)->lock_window("TimelapseHelper::update_gui");
			((TimelapseHelperWindow*)thread->window)->size->update((int64_t)config.block_size);
			((TimelapseHelperWindow*)thread->window)->unlock_window();
		}
	}
}




