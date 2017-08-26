
/*
 * CINELERRA
 * Copyright (C) 2017 Adam Williams <broadcast at earthling dot net>
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
#include "bchash.h"
#include "bcsignals.h"
#include "clip.h"
#include "filexml.h"
#include "language.h"
#include "360fuser.h"


#include <string.h>




REGISTER_PLUGIN(Fuse360Main)




Fuse360Config::Fuse360Config()
{
	fov = 1.0;
	aspect = 1.0;
	radius = 0.5;
	distance_x = 50;
	distance_y = 0;
	feather = 0;
	mode = Fuse360Config::DO_NOTHING;
	center_x = 50.0;
	center_y = 50.0;
	draw_guides = 1;
}

int Fuse360Config::equivalent(Fuse360Config &that)
{
	if(EQUIV(fov, that.fov) &&
		EQUIV(aspect, that.aspect) &&
		EQUIV(feather, that.feather) && 
		EQUIV(distance_x, that.distance_x) &&
		EQUIV(distance_y, that.distance_y) &&
		EQUIV(radius, that.radius) &&
		EQUIV(center_x, that.center_x) &&
		EQUIV(center_y, that.center_y) &&
		mode == that.mode &&
		draw_guides == that.draw_guides)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

void Fuse360Config::copy_from(Fuse360Config &that)
{
	*this = that;
}

void Fuse360Config::interpolate(Fuse360Config &prev, 
	Fuse360Config &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	fov = prev.fov * prev_scale + next.fov * next_scale;
	aspect = prev.aspect * prev_scale + next.aspect * next_scale;
	distance_x = prev.distance_x * prev_scale + next.distance_x * next_scale;
	distance_y = prev.distance_y * prev_scale + next.distance_y * next_scale;
	feather = prev.feather * prev_scale + next.feather * next_scale;
	radius = prev.radius * prev_scale + next.radius * next_scale;
	center_x = prev.center_x * prev_scale + next.center_x * next_scale;
	center_y = prev.center_y * prev_scale + next.center_y * next_scale;
	mode = prev.mode;
	draw_guides = prev.draw_guides;

	boundaries();
}

void Fuse360Config::boundaries()
{
	CLAMP(center_x, 0.0, 99.0);
	CLAMP(center_y, 0.0, 99.0);
	CLAMP(fov, 0.0, 1.0);
	CLAMP(aspect, 0.3, 3.0);
	CLAMP(radius, 0.3, 3.0);
	CLAMP(distance_x, 0.0, 100.0);
	CLAMP(distance_y, 0.0, 100.0);
}




Fuse360Slider::Fuse360Slider(Fuse360Main *client, 
	Fuse360GUI *gui,
	Fuse360Text *text,
	float *output, 
	int x, 
	int y, 
	float min,
	float max)
 : BC_FSlider(x, y, 0, 200, 200, min, max, *output)
{
	this->gui = gui;
	this->client = client;
	this->output = output;
	this->text = text;
	set_precision(0.01);
}

int Fuse360Slider::handle_event()
{
	float prev_output = *output;
	*output = get_value();
	text->update(*output);


	client->send_configure_change();
	return 1;
}



Fuse360Text::Fuse360Text(Fuse360Main *client, 
	Fuse360GUI *gui,
	Fuse360Slider *slider,
	float *output, 
	int x, 
	int y)
 : BC_TextBox(x, y, 100, 1, *output)
{
	this->gui = gui;
	this->client = client;
	this->output = output;
	this->slider = slider;
}

int Fuse360Text::handle_event()
{
	float prev_output = *output;
	*output = atof(get_text());
	slider->update(*output);


	client->send_configure_change();
	return 1;
}



Fuse360Toggle::Fuse360Toggle(Fuse360Main *client, 
	int *output, 
	int x, 
	int y,
	const char *text)
 : BC_CheckBox(x, y, *output, text)
{
	this->output = output;
	this->client = client;
}

int Fuse360Toggle::handle_event()
{
	*output = get_value();
	client->send_configure_change();
	return 1;
}










Fuse360Mode::Fuse360Mode(Fuse360Main *plugin,  
	Fuse360GUI *gui,
	int x,
	int y)
 : BC_PopupMenu(x,
 	y,
	calculate_w(gui),
	"",
	1)
{
	this->plugin = plugin;
	this->gui = gui;
}

int Fuse360Mode::handle_event()
{
	plugin->config.mode = from_text(get_text());
	plugin->send_configure_change();
	return 1;

}

void Fuse360Mode::create_objects()
{
	add_item(new BC_MenuItem(to_text(Fuse360Config::DO_NOTHING)));
	add_item(new BC_MenuItem(to_text(Fuse360Config::STRETCHXY)));
	add_item(new BC_MenuItem(to_text(Fuse360Config::STRETCHY)));
	add_item(new BC_MenuItem(to_text(Fuse360Config::BLEND)));
	update(plugin->config.mode);
}

void Fuse360Mode::update(int mode)
{
	char string[BCTEXTLEN];
	sprintf(string, "%s", to_text(mode));
	set_text(string);
}

int Fuse360Mode::calculate_w(Fuse360GUI *gui)
{
	int result = 0;
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(Fuse360Config::STRETCHXY)));
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(Fuse360Config::STRETCHY)));
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(Fuse360Config::BLEND)));
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(Fuse360Config::DO_NOTHING)));
	return result + 50;
}

int Fuse360Mode::from_text(char *text)
{
	for(int i = 0; i < 4; i++)
	{
		if(!strcmp(text, to_text(i)))
		{
			return i;
		}
	}

	return Fuse360Config::STRETCHXY;
}

const char* Fuse360Mode::to_text(int mode)
{
	switch(mode)
	{
		case Fuse360Config::DO_NOTHING:
			return "Do nothing";
			break;
		case Fuse360Config::STRETCHXY:
			return "Stretch XY";
			break;
		case Fuse360Config::STRETCHY:
			return "Stretch Y only";
			break;
	}
	return "Blend";
}
















Fuse360GUI::Fuse360GUI(Fuse360Main *client)
 : PluginClientWindow(client,
	350, 
	510, 
	350, 
	510, 
	0)
{
	this->client = client;
}

Fuse360GUI::~Fuse360GUI()
{
}


void Fuse360GUI::create_objects()
{
	int x = 10;
	int y = 10;
	int x1;
	int margin = 10;
	BC_Title *title;
	Fuse360Toggle *toggle;

	add_tool(title = new BC_Title(x, y, _("Field of View:")));
	y += title->get_h() + 5;
	add_tool(fov_slider = new Fuse360Slider(client, 
		this,
		0,
		&client->config.fov, 
		x, 
		y, 
		0.0001,
		1.0));


	x1 = x + fov_slider->get_w() + margin;
	add_tool(fov_text = new Fuse360Text(client, 
		this,
		fov_slider,
		&client->config.fov, 
		x1, 
		y));
	fov_slider->text = fov_text;
	y += fov_text->get_h() + margin;




	add_tool(title = new BC_Title(x, y, _("Aspect Ratio:")));
	y += title->get_h() + 5;
	add_tool(aspect_slider = new Fuse360Slider(client, 
		this,
		0,
		&client->config.aspect, 
		x, 
		y, 
		0.333,
		3.0));
	x1 = x + aspect_slider->get_w() + margin;
	add_tool(aspect_text = new Fuse360Text(client, 
		this,
		aspect_slider,
		&client->config.aspect, 
		x1, 
		y));
	aspect_slider->text = aspect_text;
	y += aspect_text->get_h() + 5;


	add_tool(title = new BC_Title(x, y, _("Eye Radius:")));
	y += title->get_h() + margin;
	add_tool(radius_slider = new Fuse360Slider(client, 
		this,
		0,
		&client->config.radius, 
		x, 
		y, 
		0.333,
		3.0));
	x1 = x + radius_slider->get_w() + margin;
	add_tool(radius_text = new Fuse360Text(client, 
		this,
		radius_slider,
		&client->config.radius, 
		x1, 
		y));
	radius_slider->text = radius_text;
	y += radius_text->get_h() + margin;


	add_tool(title = new BC_Title(x, y, _("Center X:")));
	y += title->get_h() + 5;
	add_tool(centerx_slider = new Fuse360Slider(client, 
		this,
		0,
		&client->config.center_x, 
		x, 
		y, 
		0.0,
		99.0));
	x1 = x + centerx_slider->get_w() + margin;
	add_tool(centerx_text = new Fuse360Text(client, 
		this,
		centerx_slider,
		&client->config.center_x, 
		x1, 
		y));
	centerx_slider->text = centerx_text;
	centerx_slider->set_precision(1.0);
	y += centerx_text->get_h() + margin;


	add_tool(title = new BC_Title(x, y, _("Center Y:")));
	y += title->get_h() + 5;
	add_tool(centery_slider = new Fuse360Slider(client, 
		this,
		0,
		&client->config.center_y, 
		x, 
		y, 
		0.0,
		99.0));
	x1 = x + centery_slider->get_w() + margin;
	add_tool(centery_text = new Fuse360Text(client, 
		this,
		centery_slider,
		&client->config.center_y, 
		x1, 
		y));
	centery_slider->text = centery_text;
	centery_slider->set_precision(1.0);
	y += centery_text->get_h() + margin;




	add_tool(title = new BC_Title(x, y, _("Eye Spacing:")));
	y += title->get_h() + 5;
	add_tool(distance_slider = new Fuse360Slider(client, 
		this,
		0,
		&client->config.distance_x, 
		x, 
		y, 
		0.0,
		100.0));
	x1 = x + distance_slider->get_w() + margin;
	add_tool(distance_text = new Fuse360Text(client, 
		this,
		distance_slider,
		&client->config.distance_x, 
		x1, 
		y));
	distance_slider->text = distance_text;
	distance_slider->set_precision(1.0);
	y += distance_text->get_h() + margin;

//printf("Fuse360GUI::create_objects %d %f\n", __LINE__, client->config.distance);


// 	BC_Bar *bar;
// 	add_tool(bar = new BC_Bar(x, y, get_w() - x * 2));
// 	y += bar->get_h() + margin;


// 	add_tool(reverse = new Fuse360Toggle(client, 
// 		&client->config.reverse, 
// 		x, 
// 		y,
// 		_("Reverse")));
// 	y += reverse->get_h() + 5;

	add_tool(draw_guides = new Fuse360Toggle(client, 
		&client->config.draw_guides, 
		x, 
		y,
		_("Draw guides")));
	y += draw_guides->get_h() + margin;

	
	add_tool(title = new BC_Title(x, y, _("Mode:")));
	add_tool(mode = new Fuse360Mode(client, 
		this, 
		x + title->get_w() + margin, 
		y));
	mode->create_objects();
	y += mode->get_h() + margin;



	show_window();
	flush();
}







Fuse360Main::Fuse360Main(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
}

Fuse360Main::~Fuse360Main()
{
	delete engine;
}

NEW_WINDOW_MACRO(Fuse360Main, Fuse360GUI)
LOAD_CONFIGURATION_MACRO(Fuse360Main, Fuse360Config)
int Fuse360Main::is_realtime() { return 1; }
const char* Fuse360Main::plugin_title() { return N_("360 Fuser"); }

void Fuse360Main::update_gui()
{
	if(thread)
	{
		if(load_configuration())
		{
			((Fuse360GUI*)thread->window)->lock_window("Fuse360Main::update_gui");
			((Fuse360GUI*)thread->window)->fov_slider->update(config.fov);
			((Fuse360GUI*)thread->window)->fov_text->update(config.fov);
			((Fuse360GUI*)thread->window)->aspect_slider->update(config.aspect);
			((Fuse360GUI*)thread->window)->aspect_text->update(config.aspect);
			((Fuse360GUI*)thread->window)->radius_slider->update(config.radius);
			((Fuse360GUI*)thread->window)->radius_text->update(config.radius);
			((Fuse360GUI*)thread->window)->centerx_slider->update(config.center_x);
			((Fuse360GUI*)thread->window)->centerx_text->update(config.center_x);
			((Fuse360GUI*)thread->window)->centery_slider->update(config.center_y);
			((Fuse360GUI*)thread->window)->centery_text->update(config.center_y);
			((Fuse360GUI*)thread->window)->distance_slider->update(config.distance_x);
			((Fuse360GUI*)thread->window)->distance_text->update(config.distance_x);
			((Fuse360GUI*)thread->window)->mode->update(config.mode);
			((Fuse360GUI*)thread->window)->draw_guides->update(config.draw_guides);
			((Fuse360GUI*)thread->window)->unlock_window();
		}
	}
}


void Fuse360Main::save_data(KeyFrame *keyframe)
{
	FileXML output;
	char string[BCTEXTLEN];



// cause data to be stored directly in text
	output.set_shared_string(keyframe->get_data(), MESSAGESIZE);
	output.tag.set_title("360FUSER");
	output.tag.set_property("FOCAL_LENGTH", config.fov);
	output.tag.set_property("ASPECT", config.aspect);
	output.tag.set_property("RADIUS", config.radius);
	output.tag.set_property("MODE", config.mode);
	output.tag.set_property("CENTER_X", config.center_x);
	output.tag.set_property("CENTER_Y", config.center_y);
	output.tag.set_property("DRAW_GUIDES", config.draw_guides);
	output.tag.set_property("DISTANCE_X", config.distance_x);
	output.tag.set_property("DISTANCE_Y", config.distance_y);
	output.append_tag();
	output.terminate_string();

}


void Fuse360Main::read_data(KeyFrame *keyframe)
{
	FileXML input;
	char string[BCTEXTLEN];


	input.set_shared_string(keyframe->get_data(), strlen(keyframe->get_data()));

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("360FUSER"))
			{
				config.fov = input.tag.get_property("FOCAL_LENGTH", config.fov);
				config.aspect = input.tag.get_property("ASPECT", config.aspect);
				config.radius = input.tag.get_property("RADIUS", config.radius);
				config.mode = input.tag.get_property("MODE", config.mode);
				config.center_x = input.tag.get_property("CENTER_X", config.center_x);
				config.center_y = input.tag.get_property("CENTER_Y", config.center_y);
				config.draw_guides = input.tag.get_property("DRAW_GUIDES", config.draw_guides);
				config.distance_x = input.tag.get_property("DISTANCE_X", config.distance_x);
				config.distance_y = input.tag.get_property("DISTANCE_Y", config.distance_y);
			}
		}
	}
}



int Fuse360Main::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	VFrame *input;
	load_configuration();
	
	input = new_temp(frame->get_w(), frame->get_h(), frame->get_color_model());
	
	read_frame(input, 
		0, 
		start_position, 
		frame_rate,
		0); // use opengl

	if(config.mode == Fuse360Config::DO_NOTHING)
	{
		get_output()->copy_from(input);
	}
	else
	{
	

		if(!engine) engine = new Fuse360Engine(this);
		engine->process_packages();
	}



	if(config.draw_guides)
	{
// Draw center
#define CENTER_H 20
#define CENTER_W 20


		int w = get_output()->get_w();
		int h = get_output()->get_h();
		int center_x = (int)(config.center_x * w / 100);
		int center_y = (int)(config.center_y * h / 100);
		int center_x1 = (int)((config.center_x - config.distance_x / 2) * w / 100);
		int center_y1 = (int)((config.center_y - config.distance_y) * h / 100);
		int center_x2 = (int)((config.center_x + config.distance_x / 2) * w / 100);
		int center_y2 = (int)((config.center_y + config.distance_y) * h / 100);
		int radius = (int)(config.radius * h);

		get_output()->draw_line(center_x, 0, center_x, h);

// draw lenses
		get_output()->draw_oval(center_x1 - radius * config.aspect, 
			center_y1 - radius / config.aspect, 
			center_x1 + radius * config.aspect, 
			center_y1 + radius / config.aspect);
// 		get_output()->draw_oval(center_x2 - radius, 
// 			center_y2 - radius, 
// 			center_x2 + radius, 
// 			center_y2 + radius);

		

	}

	return 0;
}







Fuse360Package::Fuse360Package()
 : LoadPackage() {}





Fuse360Unit::Fuse360Unit(Fuse360Engine *engine, Fuse360Main *plugin)
 : LoadClient(engine)
{
	this->plugin = plugin;
}

Fuse360Unit::~Fuse360Unit()
{
}



void Fuse360Unit::process_stretch_xy(Fuse360Package *pkg)
{
	float fov = plugin->config.fov;
	float aspect = plugin->config.aspect;
	int row1 = pkg->row1;
	int row2 = pkg->row2;
	double x_factor = aspect;
	double y_factor = 1.0 / aspect;
	if(x_factor < 1) x_factor = 1;
	if(y_factor < 1) y_factor = 1;
	int width = plugin->get_input()->get_w();
	int height = plugin->get_input()->get_h();
//	double dim = MAX(width, height) * plugin->config.radius;
//	double max_z = dim * sqrt(2.0) / 2;
	double max_z = sqrt(SQR(width) + SQR(height)) / 2;
	double center_x = width * plugin->config.center_x / 100.0;
	double center_y = height * plugin->config.center_y / 100.0;
	double r = max_z / M_PI / (fov / 2.0);



#define PROCESS_STRETCH_XY(type, components, chroma) \
{ \
	type **in_rows = (type**)plugin->get_temp()->get_rows(); \
	type **out_rows = (type**)plugin->get_input()->get_rows(); \
	type black[4] = { 0, chroma, chroma, 0 }; \
 \
	for(int y = row1; y < row2; y++) \
	{ \
		type *out_row = out_rows[y]; \
		type *in_row = in_rows[y]; \
		double y_diff = y - center_y; \
 \
		for(int x = 0; x < width; x++) \
		{ \
			double x_diff = (x - center_x); \
/* Compute magnitude */ \
			double z = sqrt(x_diff * x_diff + \
				y_diff * y_diff); \
/* Compute angle */ \
			double angle; \
			if(x == center_x) \
			{ \
				if(y < center_y) \
					angle = 3 * M_PI / 2; \
				else \
					angle = M_PI / 2; \
			} \
			else \
			{ \
				angle = atan(y_diff / x_diff); \
			} \
			if(x_diff < 0.0) angle += M_PI; \
 \
			for(int i = 0; i < components; i++) \
			{ \
/* Compute new radius */ \
				double radius1 = (z / r) * 2 * plugin->config.radius; \
				double z_in = r * atan(radius1) / (M_PI / 2); \
 \
				double x_in = z_in * cos(angle) * x_factor + center_x; \
				double y_in = z_in * sin(angle) * y_factor + center_y; \
 \
 				if(x_in < 0.0 || x_in >= width - 1 || \
					y_in < 0.0 || y_in >= height - 1) \
				{ \
					*out_row++ = black[i]; \
				} \
				else \
				{ \
					float y1_fraction = y_in - floor(y_in); \
					float y2_fraction = 1.0 - y1_fraction; \
					float x1_fraction = x_in - floor(x_in); \
					float x2_fraction = 1.0 - x1_fraction; \
					type *in_pixel1 = in_rows[(int)y_in] + (int)x_in * components; \
					type *in_pixel2 = in_rows[(int)y_in + 1] + (int)x_in * components; \
					*out_row++ = (type)(in_pixel1[i] * x2_fraction * y2_fraction + \
								in_pixel2[i] * x2_fraction * y1_fraction + \
								in_pixel1[i + components] * x1_fraction * y2_fraction + \
								in_pixel2[i + components] * x1_fraction * y1_fraction); \
				} \
			} \
		} \
	} \
 \
 	type *out_pixel = out_rows[(int)center_y] + (int)center_x * components; \
 	type *in_pixel = in_rows[(int)center_y] + (int)center_x * components; \
	for(int c = 0; c < components; c++) \
	{ \
		*out_pixel++ = *in_pixel++; \
	} \
}


	switch(plugin->get_input()->get_color_model())
	{
		case BC_RGB888:
			PROCESS_STRETCH_XY(unsigned char, 3, 0x0);
			break;
		case BC_RGBA8888:
			PROCESS_STRETCH_XY(unsigned char, 4, 0x0);
			break;
		case BC_RGB_FLOAT:
			PROCESS_STRETCH_XY(float, 3, 0.0);
			break;
		case BC_RGBA_FLOAT:
			PROCESS_STRETCH_XY(float, 4, 0.0);
			break;
		case BC_YUV888:
			PROCESS_STRETCH_XY(unsigned char, 3, 0x80);
			break;
		case BC_YUVA8888:
			PROCESS_STRETCH_XY(unsigned char, 4, 0x80);
			break;
	}
}

void Fuse360Unit::process_package(LoadPackage *package)
{
	Fuse360Package *pkg = (Fuse360Package*)package;

	switch(plugin->config.mode)
	{
		case Fuse360Config::STRETCHXY:
//			process_stretch_xy(pkg);
			break;
		case Fuse360Config::STRETCHY:
			break;
		case Fuse360Config::BLEND:
			break;
	}
}





Fuse360Engine::Fuse360Engine(Fuse360Main *plugin)
 : LoadServer(plugin->PluginClient::smp + 1, plugin->PluginClient::smp + 1)
// : LoadServer(1, 1)
{
	this->plugin = plugin;
}

Fuse360Engine::~Fuse360Engine()
{
}

void Fuse360Engine::init_packages()
{
	for(int i = 0; i < LoadServer::get_total_packages(); i++)
	{
		Fuse360Package *package = (Fuse360Package*)LoadServer::get_package(i);
		package->row1 = plugin->get_input()->get_h() * i / LoadServer::get_total_packages();
		package->row2 = plugin->get_input()->get_h() * (i + 1) / LoadServer::get_total_packages();
	}
}

LoadClient* Fuse360Engine::new_client()
{
	return new Fuse360Unit(this, plugin);
}

LoadPackage* Fuse360Engine::new_package()
{
	return new Fuse360Package;
}


