
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

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "bcdisplayinfo.h"
#include "clip.h"
#include "bchash.h"
#include "downsampleengine.h"
#include "filexml.h"
#include "keyframe.h"
#include "language.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "theme.h"
#include "vframe.h"


class DownSampleMain;
class DownSampleServer;
class DownSampleWindow;

#define MIN_SIZE 1
#define MAX_SIZE 100
#define MIN_OFFSET 0
#define MAX_OFFSET 100
#define MIN_SPEED -100
#define MAX_SPEED 100
#define SPEED_FACTOR 4

class DownSampleConfig
{
public:
	DownSampleConfig();

	int equivalent(DownSampleConfig &that);
	void copy_from(DownSampleConfig &that);
	void interpolate(DownSampleConfig &prev, 
		DownSampleConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);
    void boundaries();

	int x_offset;
	int y_offset;
	int horizontal;
	int vertical;
    int x_speed;
    int y_speed;
	int r;
	int g;
	int b;
	int a;
};


class DownSampleToggle : public BC_CheckBox
{
public:
	DownSampleToggle(DownSampleMain *plugin, 
		int x, 
		int y, 
		int *output, 
		char *string);
	int handle_event();
	DownSampleMain *plugin;
	int *output;
};

class DownSampleSize : public BC_ISlider
{
public:
	DownSampleSize(DownSampleMain *plugin, 
        DownSampleWindow *gui,
		int x, 
		int y, 
        int w,
		int *output,
		int min,
		int max);
	int handle_event();
	DownSampleMain *plugin;
    DownSampleWindow *gui;
	int *output;
};

class DownSampleText : public BC_TextBox
{
public:
    DownSampleText(DownSampleMain *plugin, 
        DownSampleWindow *gui, 
        int x, 
        int y, 
        int w,
        int *output);
    int handle_event();
	DownSampleMain *plugin;
    DownSampleWindow *gui;
	int *output;
};

class DownSampleWindow : public PluginClientWindow
{
public:
	DownSampleWindow(DownSampleMain *plugin);
	~DownSampleWindow();
	
	void create_objects();
    void update(int do_sliders, int do_texts, int do_toggles);

	DownSampleToggle *r, *g, *b, *a;
	DownSampleSize *h, *v, *h_x, *v_y, *x_speed, *y_speed;
    DownSampleText *h_text, *v_text, *h_x_text, *v_y_text, *x_speed_text, *y_speed_text;
	DownSampleMain *plugin;
};





class DownSampleMain : public PluginVClient
{
public:
	DownSampleMain(PluginServer *server);
	~DownSampleMain();

	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	int handle_opengl();

	PLUGIN_CLASS_MEMBERS(DownSampleConfig)

	VFrame *input, *output;
	DownSampleServer *engine;
};















REGISTER_PLUGIN(DownSampleMain)



DownSampleConfig::DownSampleConfig()
{
	horizontal = 2;
	vertical = 2;
	x_offset = 0;
	y_offset = 0;
    x_speed = 0;
    y_speed = 0;
	r = 1;
	g = 1;
	b = 1;
	a = 1;
}

int DownSampleConfig::equivalent(DownSampleConfig &that)
{
	return 
		horizontal == that.horizontal &&
		vertical == that.vertical &&
		x_offset == that.x_offset &&
		y_offset == that.y_offset &&
        x_speed == that.x_speed &&
        y_speed == that.y_speed && 
		r == that.r &&
		g == that.g &&
		b == that.b &&
		a == that.a;
}

void DownSampleConfig::copy_from(DownSampleConfig &that)
{
	horizontal = that.horizontal;
	vertical = that.vertical;
	x_offset = that.x_offset;
	y_offset = that.y_offset;
    x_speed = that.x_speed;
    y_speed = that.y_speed;
	r = that.r;
	g = that.g;
	b = that.b;
	a = that.a;
}

void DownSampleConfig::interpolate(DownSampleConfig &prev, 
	DownSampleConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);
	this->horizontal = (int)(prev.horizontal * prev_scale + next.horizontal * next_scale);
	this->vertical = (int)(prev.vertical * prev_scale + next.vertical * next_scale);
	this->x_offset = (int)(prev.x_offset * prev_scale + next.x_offset * next_scale);
	this->y_offset = (int)(prev.y_offset * prev_scale + next.y_offset * next_scale);
	this->x_speed = prev.x_speed;
    this->y_speed = prev.y_speed;
    r = prev.r;
	g = prev.g;
	b = prev.b;
	a = prev.a;
    boundaries();
}

void DownSampleConfig::boundaries()
{
    CLAMP(horizontal, MIN_SIZE, MAX_SIZE);
    CLAMP(vertical, MIN_SIZE, MAX_SIZE);
    CLAMP(x_offset, MIN_OFFSET, MAX_OFFSET);
    CLAMP(y_offset, MIN_OFFSET, MAX_OFFSET);
    CLAMP(x_speed, MIN_SPEED, MAX_SPEED);
    CLAMP(y_speed, MIN_SPEED, MAX_SPEED);
}









DownSampleWindow::DownSampleWindow(DownSampleMain *plugin)
 : PluginClientWindow(plugin,
	DP(230), 
	DP(500), 
	DP(230), 
	DP(500), 
	0)
{
	this->plugin = plugin; 
}

DownSampleWindow::~DownSampleWindow()
{
}

void DownSampleWindow::create_objects()
{
    int text_w = DP(50);
	int margin = client->get_theme()->widget_border;
	int x = margin, y = margin;
    int slider_w = get_w() - text_w - margin - margin - margin;
    BC_Title *title;

	add_subwindow(title = new BC_Title(x, y, _("Horizontal")));
	y += title->get_h() + margin;
	add_subwindow(h = new DownSampleSize(plugin, 
        this,
		x, 
		y, 
        slider_w,
		&plugin->config.horizontal,
		MIN_SIZE,
		MAX_SIZE));
    int text_x = x + h->get_w() + margin;
    add_tool(h_text = new DownSampleText(plugin, 
        this,
        text_x, 
        y, 
        text_w, 
        &plugin->config.horizontal));



	y += h_text->get_h() + margin;
	add_subwindow(new BC_Title(x, y, _("Horizontal offset")));
	y += title->get_h() + margin;
	add_subwindow(h_x = new DownSampleSize(plugin, 
        this,
		x, 
		y, 
        slider_w,
		&plugin->config.x_offset,
		MIN_OFFSET,
		MAX_OFFSET));
    add_tool(h_x_text = new DownSampleText(plugin, 
        this,
        text_x, 
        y, 
        text_w, 
        &plugin->config.x_offset));
	y += h_text->get_h() + margin;



	add_subwindow(new BC_Title(x, y, _("Horizontal speed")));
	y += title->get_h() + margin;
	add_subwindow(x_speed = new DownSampleSize(plugin, 
        this,
		x, 
		y, 
        slider_w,
		&plugin->config.x_speed,
		MIN_SPEED,
		MAX_SPEED));
    add_tool(x_speed_text = new DownSampleText(plugin, 
        this,
        text_x, 
        y, 
        text_w, 
        &plugin->config.x_speed));
	y += h_text->get_h() + margin;



	add_subwindow(new BC_Title(x, y, _("Vertical")));
	y += title->get_h() + margin;
	add_subwindow(v = new DownSampleSize(plugin, 
        this,
		x, 
		y, 
        slider_w,
		&plugin->config.vertical,
		MIN_SIZE,
		MAX_SIZE));
    add_tool(v_text = new DownSampleText(plugin, 
        this,
        text_x, 
        y, 
        text_w, 
        &plugin->config.vertical));
	y += h_text->get_h() + margin;


	add_subwindow(new BC_Title(x, y, _("Vertical offset")));
	y += title->get_h() + margin;
	add_subwindow(v_y = new DownSampleSize(plugin, 
        this,
		x, 
		y, 
        slider_w,
		&plugin->config.y_offset,
		MIN_OFFSET,
		MAX_OFFSET));
    add_tool(v_y_text = new DownSampleText(plugin, 
        this,
        text_x, 
        y, 
        text_w, 
        &plugin->config.y_offset));


	y += h_text->get_h() + margin;
	add_subwindow(new BC_Title(x, y, _("Vertical speed")));
	y += title->get_h() + margin;
	add_subwindow(y_speed = new DownSampleSize(plugin, 
        this,
		x, 
		y, 
        slider_w,
		&plugin->config.y_speed,
		MIN_SPEED,
		MAX_SPEED));
    add_tool(y_speed_text = new DownSampleText(plugin, 
        this,
        text_x, 
        y, 
        text_w, 
        &plugin->config.y_speed));
        
        
        
        
        
	y += h_text->get_h() + margin;
	add_subwindow(r = new DownSampleToggle(plugin, 
		x, 
		y, 
		&plugin->config.r, 
		_("Red")));
	y += r->get_h() + margin;
	add_subwindow(g = new DownSampleToggle(plugin, 
		x, 
		y, 
		&plugin->config.g, 
		_("Green")));
	y += r->get_h() + margin;
	add_subwindow(b = new DownSampleToggle(plugin, 
		x, 
		y, 
		&plugin->config.b, 
		_("Blue")));
	y += r->get_h() + margin;
	add_subwindow(a = new DownSampleToggle(plugin, 
		x, 
		y, 
		&plugin->config.a, 
		_("Alpha")));
	y += r->get_h() + margin;

	show_window();
}

void DownSampleWindow::update(int do_sliders, int do_texts, int do_toggles)
{
    if(do_texts)
    {
        h_text->update((int64_t)plugin->config.horizontal);
        v_text->update((int64_t)plugin->config.vertical);
        h_x_text->update((int64_t)plugin->config.x_offset);
        v_y_text->update((int64_t)plugin->config.y_offset);
        x_speed_text->update((int64_t)plugin->config.x_speed);
        y_speed_text->update((int64_t)plugin->config.y_speed);
    }
    
    if(do_sliders)
    {
        h->update(plugin->config.horizontal);
        v->update(plugin->config.vertical);
        h_x->update(plugin->config.x_offset);
        v_y->update(plugin->config.y_offset);
        x_speed->update(plugin->config.x_speed);
        y_speed->update(plugin->config.y_speed);
    }
    
    if(do_toggles)
    {
		r->update(plugin->config.r);
		g->update(plugin->config.g);
		b->update(plugin->config.b);
		a->update(plugin->config.a);
    }
}











DownSampleToggle::DownSampleToggle(DownSampleMain *plugin, 
	int x, 
	int y, 
	int *output, 
	char *string)
 : BC_CheckBox(x, y, *output, string)
{
	this->plugin = plugin;
	this->output = output;
}

int DownSampleToggle::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}







DownSampleSize::DownSampleSize(DownSampleMain *plugin, 
    DownSampleWindow *gui,
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
int DownSampleSize::handle_event()
{
	*output = get_value();
    gui->update(0, 1, 0);
	plugin->send_configure_change();
	return 1;
}

DownSampleText::DownSampleText(DownSampleMain *plugin, 
    DownSampleWindow *gui, 
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


int DownSampleText::handle_event()
{
    *output = atoi(get_text());
    gui->update(1, 0, 0);
    plugin->send_configure_change();
    return 1;
}









DownSampleMain::DownSampleMain(PluginServer *server)
 : PluginVClient(server)
{
	
	engine = 0;
}

DownSampleMain::~DownSampleMain()
{
	

	if(engine) delete engine;
}

const char* DownSampleMain::plugin_title() { return N_("Downsample"); }
int DownSampleMain::is_realtime() { return 1; }


NEW_PICON_MACRO(DownSampleMain)
NEW_WINDOW_MACRO(DownSampleMain, DownSampleWindow)

LOAD_CONFIGURATION_MACRO(DownSampleMain, DownSampleConfig)


int DownSampleMain::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	this->input = frame;
	this->output = frame;
	load_configuration();

// This can't be done efficiently in a shader because every output pixel
// requires summing a large, arbitrary block of the input pixels.
// Scaling down a texture wouldn't average every pixel.
	read_frame(frame, 
		0, 
		start_position, 
		frame_rate,
		0);
//		get_use_opengl());

// Use hardware
// 	if(get_use_opengl())
// 	{
// 		run_opengl();
// 		return 0;
// 	}


// apply speed
    int x_offset = config.x_offset;
    int y_offset = config.y_offset;
	int64_t prev_position = edl_to_local(
		get_prev_keyframe(
			get_source_position())->position);

	if(prev_position == 0)
	{
		prev_position = get_source_start();
	}
    double magnitude = (double)(start_position - prev_position) * 
        SPEED_FACTOR / 
		frame_rate;
    x_offset += (int)((double)config.x_speed * magnitude);
    y_offset += (int)((double)config.y_speed * magnitude);
    x_offset %= config.horizontal;
    y_offset %= config.vertical;

// Process in destination
	if(!engine) engine = new DownSampleServer(get_project_smp() + 1,
		get_project_smp() + 1);
	engine->process_frame(output, 
		output, 
		config.r, 
		config.g, 
		config.b, 
		config.a,
		config.vertical,
		config.horizontal,
		y_offset,
		x_offset);

	return 0;
}


void DownSampleMain::update_gui()
{
	if(thread)
	{
		load_configuration();
		((DownSampleWindow*)thread->window)->lock_window();
        ((DownSampleWindow*)thread->window)->update(1, 1, 1);
		((DownSampleWindow*)thread->window)->unlock_window();
	}
}





void DownSampleMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->get_data(), MESSAGESIZE);
	output.tag.set_title("DOWNSAMPLE");

	output.tag.set_property("HORIZONTAL", config.horizontal);
	output.tag.set_property("VERTICAL", config.vertical);
	output.tag.set_property("X_OFFSET", config.x_offset);
	output.tag.set_property("Y_OFFSET", config.y_offset);
	output.tag.set_property("X_SPEED", config.x_speed);
	output.tag.set_property("Y_SPEED", config.y_speed);
	output.tag.set_property("R", config.r);
	output.tag.set_property("G", config.g);
	output.tag.set_property("B", config.b);
	output.tag.set_property("A", config.a);
	output.append_tag();
	output.terminate_string();
}

void DownSampleMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), strlen(keyframe->get_data()));

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("DOWNSAMPLE"))
			{
				config.horizontal = input.tag.get_property("HORIZONTAL", config.horizontal);
				config.vertical = input.tag.get_property("VERTICAL", config.vertical);
				config.x_offset = input.tag.get_property("X_OFFSET", config.x_offset);
				config.y_offset = input.tag.get_property("Y_OFFSET", config.y_offset);
				config.x_speed = input.tag.get_property("X_SPEED", config.x_speed);
				config.y_speed = input.tag.get_property("Y_SPEED", config.y_speed);
				config.r = input.tag.get_property("R", config.r);
				config.g = input.tag.get_property("G", config.g);
				config.b = input.tag.get_property("B", config.b);
				config.a = input.tag.get_property("A", config.a);
			}
		}
	}
	config.boundaries();
}


int DownSampleMain::handle_opengl()
{
#ifdef HAVE_GL
	static char *downsample_frag = 
		"uniform sampler2D tex;\n"
		"uniform float w;\n"
		"uniform float h;\n"
		"uniform float x_offset;\n"
		"uniform float y_offset;\n";
#endif
}


