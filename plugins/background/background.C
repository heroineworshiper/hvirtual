
/*
 * CINELERRA
 * Copyright (C) 2008-2019 Adam Williams <broadcast at earthling dot net>
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
#include "filexml.h"
#include "gradient.h"
#include "keyframe.h"
#include "language.h"
#include "overlayframe.h"
#include "picon_png.h"
#include "theme.h"
#include "vframe.h"




REGISTER_PLUGIN(BackgroundMain)






BackgroundConfig::BackgroundConfig()
{
	angle = 0;
	in_radius = 0;
	out_radius = 100;
	in_r = 0xff;
	in_g = 0xff;
	in_b = 0xff;
	in_a = 0xff;
	out_r = 0x0;
	out_g = 0x0;
	out_b = 0x0;
	out_a = 0x0;
	shape = BackgroundConfig::LINEAR;
	rate = BackgroundConfig::LINEAR;
	center_x = 50;
	center_y = 50;
}

int BackgroundConfig::equivalent(BackgroundConfig &that)
{
	return (EQUIV(angle, that.angle) &&
		EQUIV(in_radius, that.in_radius) &&
		EQUIV(out_radius, that.out_radius) &&
		in_r == that.in_r &&
		in_g == that.in_g &&
		in_b == that.in_b &&
		in_a == that.in_a &&
		out_r == that.out_r &&
		out_g == that.out_g &&
		out_b == that.out_b &&
		out_a == that.out_a &&
		shape == that.shape &&
		rate == that.rate &&
		EQUIV(center_x, that.center_x) &&
		EQUIV(center_y, that.center_y));
}

void BackgroundConfig::copy_from(BackgroundConfig &that)
{
	angle = that.angle;
	in_radius = that.in_radius;
	out_radius = that.out_radius;
	in_r = that.in_r;
	in_g = that.in_g;
	in_b = that.in_b;
	in_a = that.in_a;
	out_r = that.out_r;
	out_g = that.out_g;
	out_b = that.out_b;
	out_a = that.out_a;
	shape = that.shape;
	rate = that.rate;
	center_x = that.center_x;
	center_y = that.center_y;
}

void BackgroundConfig::interpolate(BackgroundConfig &prev, 
	BackgroundConfig &next, 
	long prev_frame, 
	long next_frame, 
	long current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);


	this->angle = (int)(prev.angle * prev_scale + next.angle * next_scale);
	this->in_radius = (int)(prev.in_radius * prev_scale + next.in_radius * next_scale);
	this->out_radius = (int)(prev.out_radius * prev_scale + next.out_radius * next_scale);
	in_r = (int)(prev.in_r * prev_scale + next.in_r * next_scale);
	in_g = (int)(prev.in_g * prev_scale + next.in_g * next_scale);
	in_b = (int)(prev.in_b * prev_scale + next.in_b * next_scale);
	in_a = (int)(prev.in_a * prev_scale + next.in_a * next_scale);
	out_r = (int)(prev.out_r * prev_scale + next.out_r * next_scale);
	out_g = (int)(prev.out_g * prev_scale + next.out_g * next_scale);
	out_b = (int)(prev.out_b * prev_scale + next.out_b * next_scale);
	out_a = (int)(prev.out_a * prev_scale + next.out_a * next_scale);
	shape = prev.shape;
	rate = prev.rate;
	center_x = prev.center_x * prev_scale + next.center_x * next_scale;
	center_y = prev.center_y * prev_scale + next.center_y * next_scale;
}

int BackgroundConfig::get_in_color()
{
	int result = (in_r << 16) | (in_g << 8) | (in_b);
	return result;
}

int BackgroundConfig::get_out_color()
{
	int result = (out_r << 16) | (out_g << 8) | (out_b);
	return result;
}











#define COLOR_W DP(100)
#define COLOR_H DP(30)

BackgroundWindow::BackgroundWindow(BackgroundMain *plugin)
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
}

BackgroundWindow::~BackgroundWindow()
{
	delete in_color_thread;
	delete out_color_thread;
}

void BackgroundWindow::create_objects()
{
	int margin = plugin->get_theme()->widget_border;
	int x = DP(10), y = DP(10);
	BC_Title *title;

	add_subwindow(title = new BC_Title(x, y, _("Shape:")));
	add_subwindow(shape = new BackgroundShape(plugin, 
		this, 
		x + title->get_w() + margin, 
		y));
	shape->create_objects();
	y += shape->get_h() + margin;
	shape_x = x;
	shape_y = y;
	y += BC_Pot::calculate_h() + margin;

	add_subwindow(title = new BC_Title(x, y, _("Rate:")));
	add_subwindow(rate = new BackgroundRate(plugin,
		x + title->get_w() + margin,
		y));
	rate->create_objects();
	y += rate->get_h() + margin;

	int x1 = x;
	int y1 = y;

	BC_Title *title1;
	add_subwindow(title1 = new BC_Title(x, y, _("Inner radius:")));

	y += BC_Slider::get_span(0) + margin;

	BC_Title *title2;
	add_subwindow(title2 = new BC_Title(x, y, _("Outer radius:")));

	y = y1;
	x += MAX(title1->get_w(), title2->get_w()) + margin;
	
	add_subwindow(in_radius = new BackgroundInRadius(plugin, x, y));
	y += in_radius->get_h() + margin;

	add_subwindow(out_radius = new BackgroundOutRadius(plugin, x, y));
	y += out_radius->get_h() + margin;

	x = x1;
	y1 = y;
	add_subwindow(in_color = new BackgroundInColorButton(plugin, this, x, y));
	y += COLOR_H + margin;


	add_subwindow(out_color = new BackgroundOutColorButton(plugin, this, x, y));
	x += MAX(in_color->get_w(), out_color->get_w()) + margin;
	y = y1;

	in_color_x = x;
	in_color_y = y;
	y += COLOR_H + margin;
	out_color_x = x;
	out_color_y = y;
	in_color_thread = new BackgroundInColorThread(plugin, this);
	out_color_thread = new BackgroundOutColorThread(plugin, this);
	update_in_color();
	update_out_color();
	update_shape();

	draw_3d_border(in_color_x - 2, 
		in_color_y - 2, 
		COLOR_W + 4, 
		COLOR_H + 4, 
		1);

	draw_3d_border(out_color_x - 2, 
		out_color_y - 2, 
		COLOR_W + 4, 
		COLOR_H + 4, 
		1);

	show_window();
}

void BackgroundWindow::update_shape()
{
	int x = shape_x, y = shape_y;

	if(plugin->config.shape == BackgroundConfig::LINEAR)
	{
		delete center_x_title;
		delete center_y_title;
		delete center_x;
		delete center_y;
		center_x_title = 0;
		center_y_title = 0;
		center_x = 0;
		center_y = 0;

		if(!angle)
		{
			add_subwindow(angle_title = new BC_Title(x, y, _("Angle:")));
			add_subwindow(angle = new BackgroundAngle(plugin, x + angle_title->get_w() + 10, y));
		}
	}
	else
	{
		delete angle_title;
		delete angle;
		angle_title = 0;
		angle = 0;
		if(!center_x)
		{
			add_subwindow(center_x_title = new BC_Title(x, y, _("Center X:")));
			add_subwindow(center_x = new BackgroundCenterX(plugin,
				x + center_x_title->get_w() + 10,
				y));
			x += center_x_title->get_w() + 10 + center_x->get_w() + 10;
			add_subwindow(center_y_title = new BC_Title(x, y, _("Center Y:")));
			add_subwindow(center_y = new BackgroundCenterY(plugin,
				x + center_y_title->get_w() + 10,
				y));
		}
	}
}




void BackgroundWindow::update_in_color()
{
//printf("BackgroundWindow::update_in_color 1 %08x\n", plugin->config.get_in_color());
	set_color(plugin->config.get_in_color());
	draw_box(in_color_x, in_color_y, COLOR_W, COLOR_H);
	flash(in_color_x, in_color_y, COLOR_W, COLOR_H);
}

void BackgroundWindow::update_out_color()
{
//printf("BackgroundWindow::update_out_color 1 %08x\n", plugin->config.get_in_color());
	set_color(plugin->config.get_out_color());
	draw_box(out_color_x, out_color_y, COLOR_W, COLOR_H);
	flash(out_color_x, out_color_y, COLOR_W, COLOR_H);
}









BackgroundShape::BackgroundShape(BackgroundMain *plugin, 
	BackgroundWindow *gui, 
	int x, 
	int y)
 : BC_PopupMenu(x, y, DP(100), to_text(plugin->config.shape), 1)
{
	this->plugin = plugin;
	this->gui = gui;
}
void BackgroundShape::create_objects()
{
	add_item(new BC_MenuItem(to_text(BackgroundConfig::LINEAR)));
	add_item(new BC_MenuItem(to_text(BackgroundConfig::RADIAL)));
}
char* BackgroundShape::to_text(int shape)
{
	switch(shape)
	{
		case BackgroundConfig::LINEAR:
			return _("Linear");
		default:
			return _("Radial");
	}
}
int BackgroundShape::from_text(char *text)
{
	if(!strcmp(text, to_text(BackgroundConfig::LINEAR))) 
		return BackgroundConfig::LINEAR;
	return BackgroundConfig::RADIAL;
}
int BackgroundShape::handle_event()
{
	plugin->config.shape = from_text(get_text());
	gui->update_shape();
	plugin->send_configure_change();
}




BackgroundCenterX::BackgroundCenterX(BackgroundMain *plugin, int x, int y)
 : BC_FPot(x, y, plugin->config.center_x, 0, 100)
{
	this->plugin = plugin;
}
int BackgroundCenterX::handle_event()
{
	plugin->config.center_x = get_value();
	plugin->send_configure_change();
	return 1;
}



BackgroundCenterY::BackgroundCenterY(BackgroundMain *plugin, int x, int y)
 : BC_FPot(x, y, plugin->config.center_y, 0, 100)
{
	this->plugin = plugin;
}

int BackgroundCenterY::handle_event()
{
	plugin->config.center_y = get_value();
	plugin->send_configure_change();
	return 1;
}




BackgroundAngle::BackgroundAngle(BackgroundMain *plugin, int x, int y)
 : BC_FPot(x,
 	y,
	plugin->config.angle,
	-180,
	180)
{
	this->plugin = plugin;
}

int BackgroundAngle::handle_event()
{
	plugin->config.angle = get_value();
	plugin->send_configure_change();
	return 1;
}


BackgroundRate::BackgroundRate(BackgroundMain *plugin, int x, int y)
 : BC_PopupMenu(x,
 	y,
	DP(100),
	to_text(plugin->config.rate),
	1)
{
	this->plugin = plugin;
}
void BackgroundRate::create_objects()
{
	add_item(new BC_MenuItem(to_text(BackgroundConfig::LINEAR)));
	add_item(new BC_MenuItem(to_text(BackgroundConfig::LOG)));
	add_item(new BC_MenuItem(to_text(BackgroundConfig::SQUARE)));
}
char* BackgroundRate::to_text(int shape)
{
	switch(shape)
	{
		case BackgroundConfig::LINEAR:
			return _("Linear");
		case BackgroundConfig::LOG:
			return _("Log");
		default:
			return _("Square");
	}
}
int BackgroundRate::from_text(char *text)
{
	if(!strcmp(text, to_text(BackgroundConfig::LINEAR))) 
		return BackgroundConfig::LINEAR;
	if(!strcmp(text, to_text(BackgroundConfig::LOG)))
		return BackgroundConfig::LOG;
	return BackgroundConfig::SQUARE;
}
int BackgroundRate::handle_event()
{
	plugin->config.rate = from_text(get_text());
	plugin->send_configure_change();
	return 1;
}



BackgroundInRadius::BackgroundInRadius(BackgroundMain *plugin, int x, int y)
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

int BackgroundInRadius::handle_event()
{
	plugin->config.in_radius = get_value();
	plugin->send_configure_change();
	return 1;
}


BackgroundOutRadius::BackgroundOutRadius(BackgroundMain *plugin, int x, int y)
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

int BackgroundOutRadius::handle_event()
{
	plugin->config.out_radius = get_value();
	plugin->send_configure_change();
	return 1;
}

BackgroundInColorButton::BackgroundInColorButton(BackgroundMain *plugin, BackgroundWindow *window, int x, int y)
 : BC_GenericButton(x, y, _("Inner color:"))
{
	this->plugin = plugin;
	this->window = window;
}

int BackgroundInColorButton::handle_event()
{
	window->in_color_thread->start_window(
		plugin->config.get_in_color(),
		plugin->config.in_a);
	return 1;
}


BackgroundOutColorButton::BackgroundOutColorButton(BackgroundMain *plugin, BackgroundWindow *window, int x, int y)
 : BC_GenericButton(x, y, _("Outer color:"))
{
	this->plugin = plugin;
	this->window = window;
}

int BackgroundOutColorButton::handle_event()
{
	window->out_color_thread->start_window(
		plugin->config.get_out_color(),
		plugin->config.out_a);
	return 1;
}



BackgroundInColorThread::BackgroundInColorThread(BackgroundMain *plugin, 
	BackgroundWindow *window)
 : ColorThread(1, _("Inner color"))
{
	this->plugin = plugin;
	this->window = window;
}

int BackgroundInColorThread::handle_new_color(int output, int alpha)
{
	plugin->config.in_r = (output & 0xff0000) >> 16;
	plugin->config.in_g = (output & 0xff00) >> 8;
	plugin->config.in_b = (output & 0xff);
	plugin->config.in_a = alpha;
	
	window->lock_window("BackgroundInColorThread::handle_new_color");
	window->update_in_color();
	window->flush();
	window->unlock_window();
	plugin->send_configure_change();
// printf("BackgroundInColorThread::handle_event 1 %d %d %d %d %d %d %d %d\n",
// plugin->config.in_r,
// plugin->config.in_g,
// plugin->config.in_b,
// plugin->config.in_a,
// plugin->config.out_r,
// plugin->config.out_g,
// plugin->config.out_b,
// plugin->config.out_a);

	return 1;
}



BackgroundOutColorThread::BackgroundOutColorThread(BackgroundMain *plugin, 
	BackgroundWindow *window)
 : ColorThread(1, _("Outer color"))
{
	this->plugin = plugin;
	this->window = window;
}

int BackgroundOutColorThread::handle_new_color(int output, int alpha)
{
	plugin->config.out_r = (output & 0xff0000) >> 16;
	plugin->config.out_g = (output & 0xff00) >> 8;
	plugin->config.out_b = (output & 0xff);
	plugin->config.out_a = alpha;
	window->lock_window("BackgroundOutColorThread::handle_new_color");
	window->update_out_color();
	window->flush();
	window->unlock_window();
	plugin->send_configure_change();
// printf("BackgroundOutColorThread::handle_event 1 %d %d %d %d %d %d %d %d\n",
// plugin->config.in_r,
// plugin->config.in_g,
// plugin->config.in_b,
// plugin->config.in_a,
// plugin->config.out_r,
// plugin->config.out_g,
// plugin->config.out_b,
// plugin->config.out_a);
	return 1;
}












BackgroundMain::BackgroundMain(PluginServer *server)
 : PluginVClient(server)
{
	
	need_reconfigure = 1;
	gradient = 0;
	engine = 0;
	overlayer = 0;
}

BackgroundMain::~BackgroundMain()
{
	

	if(gradient) delete gradient;
	if(engine) delete engine;
	if(overlayer) delete overlayer;
}

const char* BackgroundMain::plugin_title() { return N_("Background"); }
int BackgroundMain::is_realtime() { return 1; }


NEW_PICON_MACRO(BackgroundMain)
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
	this->input = frame;
	this->output = frame;
	need_reconfigure |= load_configuration();

	int need_alpha = config.in_a != 0xff || config.out_a != 0xff;
	if(need_alpha)
		read_frame(frame, 
			0, 
			start_position, 
			frame_rate,
			get_use_opengl());
	if(get_use_opengl()) return run_opengl();

	int gradient_cmodel = input->get_color_model();
	if(need_alpha && BC_CModels::components(gradient_cmodel) == 3)
	{
		switch(gradient_cmodel)
		{
			case BC_RGB888:
				gradient_cmodel = BC_RGBA8888;
				break;
			case BC_RGB_FLOAT:
				gradient_cmodel = BC_RGBA_FLOAT;
				break;
			case BC_YUV888:
				gradient_cmodel = BC_YUVA8888;
				break;
		}
	}

	if(gradient && gradient->get_color_model() != gradient_cmodel)
	{
		delete gradient;
		gradient = 0;
	}

	if(!gradient) gradient = new VFrame(0, 
		-1,
		input->get_w(),
		input->get_h(),
		gradient_cmodel,
		-1);

	if(!engine) engine = new BackgroundServer(this,
		get_project_smp() + 1,
		get_project_smp() + 1);
	engine->process_packages();

// Use overlay routine in BackgroundServer if mismatched colormodels
	if(gradient->get_color_model() == output->get_color_model())
	{
		if(!overlayer) overlayer = new OverlayFrame(get_project_smp() + 1);
		overlayer->overlay(output, 
			gradient,
			0, 
			0, 
			input->get_w(), 
			input->get_h(),
			0, 
			0, 
			input->get_w(), 
			input->get_h(), 
			1.0, 
			TRANSFER_NORMAL,
			NEAREST_NEIGHBOR);
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
			((BackgroundWindow*)thread->window)->rate->set_text(BackgroundRate::to_text(config.rate));
			((BackgroundWindow*)thread->window)->in_radius->update(config.in_radius);
			((BackgroundWindow*)thread->window)->out_radius->update(config.out_radius);
			((BackgroundWindow*)thread->window)->shape->set_text(BackgroundShape::to_text(config.shape));
			if(((BackgroundWindow*)thread->window)->angle)
				((BackgroundWindow*)thread->window)->angle->update(config.angle);
			if(((BackgroundWindow*)thread->window)->center_x)
				((BackgroundWindow*)thread->window)->center_x->update(config.center_x);
			if(((BackgroundWindow*)thread->window)->center_y)
				((BackgroundWindow*)thread->window)->center_y->update(config.center_y);
			((BackgroundWindow*)thread->window)->update_in_color();
			((BackgroundWindow*)thread->window)->update_out_color();
			((BackgroundWindow*)thread->window)->update_shape();
			((BackgroundWindow*)thread->window)->unlock_window();
			((BackgroundWindow*)thread->window)->in_color_thread->update_gui(config.get_in_color(), config.in_a);
			((BackgroundWindow*)thread->window)->out_color_thread->update_gui(config.get_out_color(), config.out_a);
		}
	}
}





void BackgroundMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->get_data(), MESSAGESIZE);
	output.tag.set_title("GRADIENT");

	output.tag.set_property("ANGLE", config.angle);
	output.tag.set_property("IN_RADIUS", config.in_radius);
	output.tag.set_property("OUT_RADIUS", config.out_radius);
	output.tag.set_property("IN_R", config.in_r);
	output.tag.set_property("IN_G", config.in_g);
	output.tag.set_property("IN_B", config.in_b);
	output.tag.set_property("IN_A", config.in_a);
	output.tag.set_property("OUT_R", config.out_r);
	output.tag.set_property("OUT_G", config.out_g);
	output.tag.set_property("OUT_B", config.out_b);
	output.tag.set_property("OUT_A", config.out_a);
	output.tag.set_property("SHAPE", config.shape);
	output.tag.set_property("RATE", config.rate);
	output.tag.set_property("CENTER_X", config.center_x);
	output.tag.set_property("CENTER_Y", config.center_y);
	output.append_tag();
	output.terminate_string();
}

void BackgroundMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), strlen(keyframe->get_data()));

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("GRADIENT"))
			{
				config.angle = input.tag.get_property("ANGLE", config.angle);
				config.rate = input.tag.get_property("RATE", config.rate);
				config.in_radius = input.tag.get_property("IN_RADIUS", config.in_radius);
				config.out_radius = input.tag.get_property("OUT_RADIUS", config.out_radius);
				config.in_r = input.tag.get_property("IN_R", config.in_r);
				config.in_g = input.tag.get_property("IN_G", config.in_g);
				config.in_b = input.tag.get_property("IN_B", config.in_b);
				config.in_a = input.tag.get_property("IN_A", config.in_a);
				config.out_r = input.tag.get_property("OUT_R", config.out_r);
				config.out_g = input.tag.get_property("OUT_G", config.out_g);
				config.out_b = input.tag.get_property("OUT_B", config.out_b);
				config.out_a = input.tag.get_property("OUT_A", config.out_a);
				config.shape = input.tag.get_property("SHAPE", config.shape);
				config.center_x = input.tag.get_property("CENTER_X", config.center_x);
				config.center_y = input.tag.get_property("CENTER_Y", config.center_y);
			}
		}
	}
}

int BackgroundMain::handle_opengl()
{
#ifdef HAVE_GL
	const char *head_frag =
		"uniform sampler2D tex;\n"
		"uniform float half_w;\n"
		"uniform float half_h;\n"
		"uniform float center_x;\n"
		"uniform float center_y;\n"
		"uniform float half_gradient_size;\n"
		"uniform float sin_angle;\n"
		"uniform float cos_angle;\n"
		"uniform vec4 out_color;\n"
		"uniform vec4 in_color;\n"
		"uniform float in_radius;\n"
		"uniform float out_radius;\n"
		"uniform float radius_diff;\n"
		"\n"
		"void main()\n"
		"{\n"
		"	vec2 out_coord = gl_TexCoord[0].st;\n";

	const char *linear_shape = 
		"	vec2 in_coord = vec2(out_coord.x - half_w, half_h - out_coord.y);\n"
		"	float mag = half_gradient_size - \n"
		"		(in_coord.x * sin_angle + in_coord.y * cos_angle);\n";

	const char *radial_shape =
		"	vec2 in_coord = vec2(out_coord.x - center_x, out_coord.y - center_y);\n"
		"	float mag = length(vec2(in_coord.x, in_coord.y));\n";

// No clamp function in NVidia
	const char *linear_rate = 
		"	mag = min(max(mag, in_radius), out_radius);\n"
		"	float opacity = (mag - in_radius) / radius_diff;\n";

// NVidia warns about exp, but exp is in the GLSL spec.
	const char *log_rate = 
		"	mag = max(mag, in_radius);\n"
		"	float opacity = 1.0 - \n"
		"		exp(1.0 * -(mag - in_radius) / radius_diff);\n";

	const char *square_rate = 
		"	mag = min(max(mag, in_radius), out_radius);\n"
		"	float opacity = pow((mag - in_radius) / radius_diff, 2.0);\n"
		"	opacity = min(opacity, 1.0);\n";

	const char *tail_frag = 
		"	vec4 color = mix(in_color, out_color, opacity);\n"
		"	vec4 bg_color = texture2D(tex, out_coord);\n"
		"	gl_FragColor.rgb = mix(bg_color.rgb, color.rgb, color.a);\n"
		"	gl_FragColor.a = max(bg_color.a, color.a);\n"
		"}\n";


	const char *shader_stack[5] = { 0, 0, 0, 0, 0 };
	shader_stack[0] = head_frag;

	switch(config.shape)
	{
		case BackgroundConfig::LINEAR:
			shader_stack[1] = linear_shape;
			break;

		default:
			shader_stack[1] = radial_shape;
			break;
	}

	switch(config.rate)
	{
		case BackgroundConfig::LINEAR:
			shader_stack[2] = linear_rate;
			break;
		case BackgroundConfig::LOG:
			shader_stack[2] = log_rate;
			break;
		case BackgroundConfig::SQUARE:
			shader_stack[2] = square_rate;
			break;
	}

	shader_stack[3] = tail_frag;
// Force frame to create texture without copying to it if full alpha.
	if(config.in_a >= 0xff &&
		config.out_a >= 0xff)
		get_output()->set_opengl_state(VFrame::TEXTURE);
	get_output()->to_texture();
	get_output()->enable_opengl();
	get_output()->init_screen();
	get_output()->bind_texture(0);

	unsigned int frag = VFrame::make_shader(0, 
		shader_stack[0], 
		shader_stack[1], 
		shader_stack[2], 
		shader_stack[3], 
		0);

	if(frag)
	{
		glUseProgram(frag);
		float w = get_output()->get_w();
		float h = get_output()->get_h();
		float texture_w = get_output()->get_texture_w();
		float texture_h = get_output()->get_texture_h();
		glUniform1i(glGetUniformLocation(frag, "tex"), 0);
		glUniform1f(glGetUniformLocation(frag, "half_w"), w / 2 / texture_w);
		glUniform1f(glGetUniformLocation(frag, "half_h"), h / 2 / texture_h);
		if(config.shape == BackgroundConfig::LINEAR)
		{
			glUniform1f(glGetUniformLocation(frag, "center_x"), 
				w / 2 / texture_w);
			glUniform1f(glGetUniformLocation(frag, "center_y"), 
				h / 2 / texture_h);
		}
		else
		{
			glUniform1f(glGetUniformLocation(frag, "center_x"), 
				(float)config.center_x * w / 100 / texture_w);
			glUniform1f(glGetUniformLocation(frag, "center_y"), 
				(float)config.center_y * h / 100 / texture_h);
		}
		float gradient_size = hypotf(w / texture_w, h / texture_h);
		glUniform1f(glGetUniformLocation(frag, "half_gradient_size"), 
			gradient_size / 2);
		glUniform1f(glGetUniformLocation(frag, "sin_angle"), 
			sin(config.angle * (M_PI / 180)));
		glUniform1f(glGetUniformLocation(frag, "cos_angle"), 
			cos(config.angle * (M_PI / 180)));
		float in_radius = (float)config.in_radius / 100 * gradient_size;
		glUniform1f(glGetUniformLocation(frag, "in_radius"), in_radius);
		float out_radius = (float)config.out_radius / 100 * gradient_size;
		glUniform1f(glGetUniformLocation(frag, "out_radius"), out_radius);
		glUniform1f(glGetUniformLocation(frag, "radius_diff"), 
			out_radius - in_radius);

		switch(get_output()->get_color_model())
		{
			case BC_YUV888:
			case BC_YUVA8888:
			{
				float in1, in2, in3, in4;
				float out1, out2, out3, out4;
				YUV::rgb_to_yuv_f((float)config.in_r / 0xff,
					(float)config.in_g / 0xff,
					(float)config.in_b / 0xff,
					in1,
					in2,
					in3);
				in4 = (float)config.in_a / 0xff;
				YUV::rgb_to_yuv_f((float)config.out_r / 0xff,
					(float)config.out_g / 0xff,
					(float)config.out_b / 0xff,
					out1,
					out2,
					out3);
				in2 += 0.5;
				in3 += 0.5;
				out2 += 0.5;
				out3 += 0.5;
				out4 = (float)config.out_a / 0xff;
				glUniform4f(glGetUniformLocation(frag, "out_color"), 
					out1, out2, out3, out4);
				glUniform4f(glGetUniformLocation(frag, "in_color"), 
					in1, in2, in3, in4);
				break;
			}

			default:
				glUniform4f(glGetUniformLocation(frag, "out_color"), 
					(float)config.out_r / 0xff,
					(float)config.out_g / 0xff,
					(float)config.out_b / 0xff,
					(float)config.out_a / 0xff);
				glUniform4f(glGetUniformLocation(frag, "in_color"), 
					(float)config.in_r / 0xff,
					(float)config.in_g / 0xff,
					(float)config.in_b / 0xff,
					(float)config.in_a / 0xff);
				break;
		}
	}

	get_output()->draw_texture();
	glUseProgram(0);
	get_output()->set_opengl_state(VFrame::SCREEN);
	
#endif
}











BackgroundPackage::BackgroundPackage()
 : LoadPackage()
{
}




BackgroundUnit::BackgroundUnit(BackgroundServer *server, BackgroundMain *plugin)
 : LoadClient(server)
{
	this->plugin = plugin;
	this->server = server;
}




static float calculate_opacity(float mag, 
	float in_radius, 
	float out_radius,
	int rate)
{
	float opacity;
	switch(rate)
	{
		case BackgroundConfig::LINEAR:
			if(mag < in_radius)
				opacity = 0.0;
			else
			if(mag >= out_radius)
				opacity = 1.0;
			else
				opacity = (float)(mag - in_radius) / (out_radius - in_radius);
			break;

		case BackgroundConfig::LOG:
			if(mag < in_radius)
				opacity = 0;
			else
// Let this one decay beyond out_radius
				opacity = 1 - exp(1.0 * -(float)(mag - in_radius) /
					(out_radius - in_radius));
			break;

		case BackgroundConfig::SQUARE:
			if(mag < in_radius)
				opacity = 0.0; 
			else
			if(mag >= out_radius) 
				opacity = 1.0;
			else
				opacity = powf((float)(mag - in_radius) /
					(out_radius - in_radius), 2.0);
			break;
	}
 	CLAMP(opacity, 0.0, 1.0);
	return opacity;
}

#define CREATE_GRADIENT(type, temp, components, max) \
{ \
/* Synthesize linear gradient for lookups */ \
 \
 	r_table = malloc(sizeof(type) * gradient_size); \
 	g_table = malloc(sizeof(type) * gradient_size); \
 	b_table = malloc(sizeof(type) * gradient_size); \
 	a_table = malloc(sizeof(type) * gradient_size); \
 \
	for(int i = 0; i < gradient_size; i++) \
	{ \
		float opacity = calculate_opacity(i, in_radius, out_radius, plugin->config.rate); \
		float transparency; \
 \
		transparency = 1.0 - opacity; \
		((type*)r_table)[i] = (type)(out1 * opacity + in1 * transparency); \
		((type*)g_table)[i] = (type)(out2 * opacity + in2 * transparency); \
		((type*)b_table)[i] = (type)(out3 * opacity + in3 * transparency); \
		((type*)a_table)[i] = (type)(out4 * opacity + in4 * transparency); \
	} \
 \
	for(int i = pkg->y1; i < pkg->y2; i++) \
	{ \
		type *gradient_row = (type*)plugin->gradient->get_rows()[i]; \
		type *out_row = (type*)plugin->get_output()->get_rows()[i]; \
 \
 		switch(plugin->config.shape) \
		{ \
			case BackgroundConfig::LINEAR: \
				for(int j = 0; j < w; j++) \
				{ \
					int x = j - half_w; \
					int y = -(i - half_h); \
		 \
/* Rotate by effect angle */ \
					int mag = (int)(gradient_size / 2 - \
						(x * sin_angle + y * cos_angle) + \
						0.5); \
		 \
/* Get gradient value from these coords */ \
		 \
					if(sizeof(type) == 4) \
					{ \
						float opacity = calculate_opacity(mag,  \
							in_radius,  \
							out_radius, \
							plugin->config.rate); \
						float transparency = 1.0 - opacity; \
						gradient_row[0] = (type)(out1 * opacity + in1 * transparency); \
						gradient_row[1] = (type)(out2 * opacity + in2 * transparency); \
						gradient_row[2] = (type)(out3 * opacity + in3 * transparency); \
						if(components == 4) gradient_row[3] = (type)(out4 * opacity + in4 * transparency); \
					} \
					else \
 					if(mag < 0) \
					{ \
						gradient_row[0] = out1; \
						gradient_row[1] = out2; \
						gradient_row[2] = out3; \
						if(components == 4) gradient_row[3] = out4; \
					} \
					else \
					if(mag >= gradient_size) \
					{ \
						gradient_row[0] = in1; \
						gradient_row[1] = in2; \
						gradient_row[2] = in3; \
						if(components == 4) gradient_row[3] = in4; \
					} \
					else \
					{ \
						gradient_row[0] = ((type*)r_table)[mag]; \
						gradient_row[1] = ((type*)g_table)[mag]; \
						gradient_row[2] = ((type*)b_table)[mag]; \
						if(components == 4) gradient_row[3] = ((type*)a_table)[mag]; \
					} \
 \
/* Overlay mixed colormodels onto output */ \
 					if(gradient_cmodel != output_cmodel) \
					{ \
						temp opacity = gradient_row[3]; \
						temp transparency = max - opacity; \
						out_row[0] = (transparency * out_row[0] + opacity * gradient_row[0]) / max; \
						out_row[1] = (transparency * out_row[1] + opacity * gradient_row[1]) / max; \
						out_row[2] = (transparency * out_row[2] + opacity * gradient_row[2]) / max; \
						out_row += 3; \
					} \
 \
 					gradient_row += components; \
				} \
				break; \
 \
			case BackgroundConfig::RADIAL: \
				for(int j = 0; j < w; j++) \
				{ \
					double x = j - center_x; \
					double y = i - center_y; \
					double magnitude = hypot(x, y); \
					int mag = (int)magnitude; \
					if(sizeof(type) == 4) \
					{ \
						float opacity = calculate_opacity(mag,  \
							in_radius,  \
							out_radius, \
							plugin->config.rate); \
						float transparency = 1.0 - opacity; \
						gradient_row[0] = (type)(out1 * opacity + in1 * transparency); \
						gradient_row[1] = (type)(out2 * opacity + in2 * transparency); \
						gradient_row[2] = (type)(out3 * opacity + in3 * transparency); \
						if(components == 4) gradient_row[3] = (type)(out4 * opacity + in4 * transparency); \
					} \
					else \
					{ \
						gradient_row[0] = ((type*)r_table)[mag]; \
						gradient_row[1] = ((type*)g_table)[mag]; \
						gradient_row[2] = ((type*)b_table)[mag]; \
						if(components == 4) gradient_row[3] = ((type*)a_table)[mag]; \
					} \
 \
/* Overlay mixed colormodels onto output */ \
 					if(gradient_cmodel != output_cmodel) \
					{ \
						temp opacity = gradient_row[3]; \
						temp transparency = max - opacity; \
						out_row[0] = (transparency * out_row[0] + opacity * gradient_row[0]) / max; \
						out_row[1] = (transparency * out_row[1] + opacity * gradient_row[1]) / max; \
						out_row[2] = (transparency * out_row[2] + opacity * gradient_row[2]) / max; \
						out_row += 3; \
					} \
 \
					gradient_row += components; \
				} \
				break; \
		} \
	} \
}

void BackgroundUnit::process_package(LoadPackage *package)
{
	BackgroundPackage *pkg = (BackgroundPackage*)package;
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
	void *r_table = 0;
	void *g_table = 0;
	void *b_table = 0;
	void *a_table = 0;
	int gradient_cmodel = plugin->gradient->get_color_model();
	int output_cmodel = plugin->get_output()->get_color_model();

	if(in_radius > out_radius)
	{
	    in_radius ^= out_radius;
	    out_radius ^= in_radius;
	    in_radius ^= out_radius;
	}


	switch(gradient_cmodel)
	{
		case BC_RGB888:
		{
			int in1 = plugin->config.in_r;
			int in2 = plugin->config.in_g;
			int in3 = plugin->config.in_b;
			int in4 = plugin->config.in_a;
			int out1 = plugin->config.out_r;
			int out2 = plugin->config.out_g;
			int out3 = plugin->config.out_b;
			int out4 = plugin->config.out_a;
			CREATE_GRADIENT(unsigned char, int, 3, 0xff)
			break;
		}

		case BC_RGBA8888:
		{
			int in1 = plugin->config.in_r;
			int in2 = plugin->config.in_g;
			int in3 = plugin->config.in_b;
			int in4 = plugin->config.in_a;
			int out1 = plugin->config.out_r;
			int out2 = plugin->config.out_g;
			int out3 = plugin->config.out_b;
			int out4 = plugin->config.out_a;
			CREATE_GRADIENT(unsigned char, int, 4, 0xff)
			break;
		}

		case BC_RGB_FLOAT:
		{
			float in1 = (float)plugin->config.in_r / 0xff;
			float in2 = (float)plugin->config.in_g / 0xff;
			float in3 = (float)plugin->config.in_b / 0xff;
			float in4 = (float)plugin->config.in_a / 0xff;
			float out1 = (float)plugin->config.out_r / 0xff;
			float out2 = (float)plugin->config.out_g / 0xff;
			float out3 = (float)plugin->config.out_b / 0xff;
			float out4 = (float)plugin->config.out_a / 0xff;
			CREATE_GRADIENT(float, float, 3, 1.0)
			break;
		}

		case BC_RGBA_FLOAT:
		{
			float in1 = (float)plugin->config.in_r / 0xff;
			float in2 = (float)plugin->config.in_g / 0xff;
			float in3 = (float)plugin->config.in_b / 0xff;
			float in4 = (float)plugin->config.in_a / 0xff;
			float out1 = (float)plugin->config.out_r / 0xff;
			float out2 = (float)plugin->config.out_g / 0xff;
			float out3 = (float)plugin->config.out_b / 0xff;
			float out4 = (float)plugin->config.out_a / 0xff;
			CREATE_GRADIENT(float, float, 4, 1.0)
			break;
		}

		case BC_YUV888:
		{
			int in1, in2, in3, in4;
			int out1, out2, out3, out4;
			yuv.rgb_to_yuv_8(plugin->config.in_r,
				plugin->config.in_g,
				plugin->config.in_b,
				in1,
				in2,
				in3);
			in4 = plugin->config.in_a;
			yuv.rgb_to_yuv_8(plugin->config.out_r,
				plugin->config.out_g,
				plugin->config.out_b,
				out1,
				out2,
				out3);
			out4 = plugin->config.out_a;
			CREATE_GRADIENT(unsigned char, int, 3, 0xff)
			break;
		}

		case BC_YUVA8888:
		{
			int in1, in2, in3, in4;
			int out1, out2, out3, out4;
			yuv.rgb_to_yuv_8(plugin->config.in_r,
				plugin->config.in_g,
				plugin->config.in_b,
				in1,
				in2,
				in3);
			in4 = plugin->config.in_a;
			yuv.rgb_to_yuv_8(plugin->config.out_r,
				plugin->config.out_g,
				plugin->config.out_b,
				out1,
				out2,
				out3);
			out4 = plugin->config.out_a;
			CREATE_GRADIENT(unsigned char, int, 4, 0xff)
			break;
		}
	}

	if(r_table) free(r_table);
	if(g_table) free(g_table);
	if(b_table) free(b_table);
	if(a_table) free(a_table);
}



