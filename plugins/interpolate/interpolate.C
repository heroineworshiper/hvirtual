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

#include "bcdisplayinfo.h"
#include "clip.h"
#include "colormodels.h"
#include "filexml.h"
#include "aggregated.h"
#include "language.h"
#include "picon_png.h"
#include "interpolate.h"

#include <stdio.h>
#include <string.h>


REGISTER_PLUGIN(InterpolatePixelsMain)






InterpolatePixelsOffset::InterpolatePixelsOffset(InterpolatePixelsWindow *window, 
	int x, 
	int y, 
	int *output)
 : BC_ISlider(x,
 	y,
	0,
	DP(50),
	DP(50),
	0,
	1,
	*output,
	0)
{
	this->window = window;
	this->output = output;
}

InterpolatePixelsOffset::~InterpolatePixelsOffset()
{
}

int InterpolatePixelsOffset::handle_event()
{
	*output = get_value();
	window->client->send_configure_change();
	return 1;
}






InterpolatePixelsWindow::InterpolatePixelsWindow(InterpolatePixelsMain *client)
 : PluginClientWindow(client,
	DP(200), 
	DP(100), 
	DP(200), 
	DP(100), 
	0)
{ 
	this->client = client; 
}

InterpolatePixelsWindow::~InterpolatePixelsWindow()
{
}

void InterpolatePixelsWindow::create_objects()
{
	int x = DP(10), y = DP(10);
	
	BC_Title *title;
	add_tool(title = new BC_Title(x, y, _("X Offset:")));
	add_tool(x_offset = new InterpolatePixelsOffset(this, 
		x + title->get_w() + DP(5),
		y, 
		&client->config.x));
	y += MAX(x_offset->get_h(), title->get_h()) + DP(5);
	add_tool(title = new BC_Title(x, y, _("Y Offset:")));
	add_tool(y_offset = new InterpolatePixelsOffset(this, 
		x + title->get_w() + DP(5),
		y, 
		&client->config.y));
	y += MAX(y_offset->get_h(), title->get_h()) + DP(5);

	show_window();
}















InterpolatePixelsConfig::InterpolatePixelsConfig()
{
	x = 0;
	y = 0;
}

int InterpolatePixelsConfig::equivalent(InterpolatePixelsConfig &that)
{
	return x == that.x && y == that.y;
}

void InterpolatePixelsConfig::copy_from(InterpolatePixelsConfig &that)
{
	x = that.x;
	y = that.y;
}

void InterpolatePixelsConfig::interpolate(InterpolatePixelsConfig &prev,
    InterpolatePixelsConfig &next,
    int64_t prev_position,
    int64_t next_position,
    int64_t current_position)
{
    this->x = prev.x;
    this->y = prev.y;
}






InterpolatePixelsMain::InterpolatePixelsMain(PluginServer *server)
 : PluginVClient(server)
{
	
	engine = 0;
}

InterpolatePixelsMain::~InterpolatePixelsMain()
{
	
	delete engine;
}

const char* InterpolatePixelsMain::plugin_title() { return N_("Interpolate Bayer"); }
int InterpolatePixelsMain::is_realtime() { return 1; }


NEW_WINDOW_MACRO(InterpolatePixelsMain, InterpolatePixelsWindow)

NEW_PICON_MACRO(InterpolatePixelsMain)

void InterpolatePixelsMain::update_gui()
{
	if(thread)
	{
		int changed = load_configuration();
		if(changed)
		{
			thread->window->lock_window("InterpolatePixelsMain::update_gui");
			((InterpolatePixelsWindow*)thread->window)->x_offset->update(config.x);
			((InterpolatePixelsWindow*)thread->window)->y_offset->update(config.y);
			thread->window->unlock_window();
		}
	}
}


LOAD_CONFIGURATION_MACRO(InterpolatePixelsMain, InterpolatePixelsConfig)


void InterpolatePixelsMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->get_data(), MESSAGESIZE);
	output.tag.set_title("INTERPOLATEPIXELS");
	output.tag.set_property("X", config.x);
	output.tag.set_property("Y", config.y);
	output.append_tag();
	output.terminate_string();
}

void InterpolatePixelsMain::read_data(KeyFrame *keyframe)
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
			if(input.tag.title_is("INTERPOLATEPIXELS"))
			{
				config.x = input.tag.get_property("X", config.x);
				config.y = input.tag.get_property("Y", config.y);
			}
		}
	}
}



int InterpolatePixelsMain::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	load_configuration();

// Set aggregation parameters
	frame->get_params()->update("INTERPOLATEPIXELS_X", config.x);
	frame->get_params()->update("INTERPOLATEPIXELS_Y", config.y);

// opengl has been dropped because of the continual manetenance & because
// the real solution requires a complete port of dcraw's FIR filter.
	read_frame(frame, 
		0, 
		start_position, 
		frame_rate,
        0);
//		get_use_opengl());
//frame->dump_params();

// 	if(get_use_opengl())
// 	{
// // Aggregate with gamma
// 		if(next_effect_is("Gamma") ||
// 			next_effect_is("Histogram") ||
// 			next_effect_is("Color Balance"))
// 			return 0;
// 
// 
// 		return run_opengl();
// 	}


	if(get_output()->get_color_model() != BC_RGB_FLOAT &&
		get_output()->get_color_model() != BC_RGBA_FLOAT)
	{
		printf("InterpolatePixelsMain::process_buffer: only supports float colormodels\n");
		return 1;
	}

	new_temp(frame->get_w(), frame->get_h(), frame->get_color_model());
	get_temp()->copy_from(frame);
	if(!engine)
		engine = new InterpolatePixelsEngine(this);
	engine->process_packages();

	return 0;
}

// The pattern is
//   G B
//   R G
// And is offset to recreate the 4 possibilities.
// The color values are interpolated in the most basic way.
// No adaptive algorithm is used.

int InterpolatePixelsMain::handle_opengl()
{
printf("InterpolatePixelsMain::handle_opengl\n");
#ifdef HAVE_GL


	get_output()->to_texture();
	get_output()->enable_opengl();

	const char *shader_stack[] = { 0, 0, 0 };
	int current_shader = 0;
	INTERPOLATE_COMPILE(shader_stack, current_shader)
	unsigned int frag = VFrame::make_shader(0,
					shader_stack[0],
					0);
	if(frag > 0)
	{
		glUseProgram(frag);
		glUniform1i(glGetUniformLocation(frag, "tex"), 0);
		INTERPOLATE_UNIFORMS(frag)
	}


	get_output()->init_screen();
	get_output()->bind_texture(0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);


	get_output()->draw_texture();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glUseProgram(0);
	get_output()->set_opengl_state(VFrame::SCREEN);

#endif
	return 0;
}













InterpolatePixelsPackage::InterpolatePixelsPackage()
 : LoadPackage()
{
	
}






InterpolatePixelsUnit::InterpolatePixelsUnit(InterpolatePixelsEngine *server, InterpolatePixelsMain *plugin)
 : LoadClient(server)
{
	this->plugin = plugin;
	this->server = server;
}

void InterpolatePixelsUnit::process_package(LoadPackage *package)
{
	InterpolatePixelsPackage *pkg = (InterpolatePixelsPackage*)package;
	int h = plugin->get_temp()->get_h();
	int w = plugin->get_temp()->get_w();
	int pattern_offset_x = plugin->config.x;
	int pattern_offset_y = plugin->config.y;
	int y1 = pkg->y1;
	int y2 = pkg->y2;
	int components = cmodel_components(plugin->get_output()->get_color_model());
	float color_matrix[9];
	memcpy(color_matrix, server->color_matrix, sizeof(color_matrix));

// printf("InterpolatePixelsUnit::process_package %d color_matrix=", __LINE__);
// for(int i = 0; i < 9; i++)
// {
// 	printf("%f ", color_matrix[i]);
// }
// printf("\n");

	y1 = MAX(y1, 1);
	y2 = MIN(y2, h - 1);

// Only supports float because it's useless in any other colormodel.
	for(int i = y1; i < y2; i++)
	{
		int pattern_coord_y = (i - pattern_offset_y) % 2;
		float *prev_row = (float*)plugin->get_temp()->get_rows()[i - 1];
		float *current_row = (float*)plugin->get_temp()->get_rows()[i];
		float *next_row = (float*)plugin->get_temp()->get_rows()[i + 1];
		float *out_row = (float*)plugin->get_output()->get_rows()[i];

		prev_row += components;
		current_row += components;
		next_row += components;
		out_row += components;
		float r;
		float g;
		float b;
#undef RED
#define RED 0
#undef GREEN
#define GREEN 1
#undef BLUE
#define BLUE 2
		if(pattern_coord_y == 0)
		{
			for(int j = 1; j < w - 1; j++)
			{
				int pattern_coord_x = (j - pattern_offset_x) % 2;
// Top left pixel
				if(pattern_coord_x == 0)
				{
					r = (prev_row[RED] + next_row[RED]) / 2;
					g = current_row[GREEN];
					b = (current_row[-components + BLUE] + current_row[components + BLUE]) / 2;
				}
				else
// Top right pixel
				{
					r = (prev_row[-components + RED] + 
						prev_row[components + RED] +
						next_row[-components + RED] +
						next_row[components + RED]) / 4;
					g = (current_row[-components + GREEN] +
						prev_row[GREEN] +
						current_row[components + GREEN] +
						next_row[GREEN]) / 4;
					b = current_row[BLUE];
				}

// 				out_row[0] = r * color_matrix[0] + g * color_matrix[1] + b * color_matrix[2];
// 				out_row[1] = r * color_matrix[3] + g * color_matrix[4] + b * color_matrix[5];
// 				out_row[2] = r * color_matrix[6] + g * color_matrix[7] + b * color_matrix[8];

				out_row[0] = r;
				out_row[1] = g;
				out_row[2] = b;

				prev_row += components;
				current_row += components;
				next_row += components;
				out_row += components;
			}
		}
		else
		{
			for(int j = 1; j < w - 1; j++)
			{
				int pattern_coord_x = (j - pattern_offset_x) % 2;
// Bottom left pixel
				if(pattern_coord_x == 0)
				{
					r = current_row[RED];
					g = (current_row[-components + GREEN] +
						prev_row[GREEN] +
						current_row[components + GREEN] +
						next_row[GREEN]) / 4;
					b = (prev_row[-components + BLUE] + 
						prev_row[components + BLUE] +
						next_row[-components + BLUE] +
						next_row[components + BLUE]) / 4;
				}
				else
// Bottom right pixel
				{
					float r = (current_row[-components + RED] + current_row[components + RED]) / 2;
					float g = current_row[GREEN];
					float b = (prev_row[BLUE] + next_row[BLUE]) / 2;
				}

//				out_row[0] = r * color_matrix[0] + g * color_matrix[1] + b * color_matrix[2];
//				out_row[1] = r * color_matrix[3] + g * color_matrix[4] + b * color_matrix[5];
//				out_row[2] = r * color_matrix[6] + g * color_matrix[7] + b * color_matrix[8];

				out_row[0] = r;
				out_row[1] = g;
				out_row[2] = b;

				prev_row += components;
				current_row += components;
				next_row += components;
				out_row += components;
			}
		}
	}
}




InterpolatePixelsEngine::InterpolatePixelsEngine(InterpolatePixelsMain *plugin)
 : LoadServer(plugin->get_project_smp() + 1, plugin->get_project_smp() + 1)
{
	this->plugin = plugin;
}

void InterpolatePixelsEngine::init_packages()
{
	char string[BCTEXTLEN];
	string[0] = 0;
	plugin->get_output()->get_params()->get("DCRAW_MATRIX", string);

// printf("InterpolatePixelsEngine::init_packages %d\n", __LINE__);
// plugin->get_output()->dump_params();

	sscanf(string, 
		"%f %f %f %f %f %f %f %f %f", 
		&color_matrix[0],
		&color_matrix[1],
		&color_matrix[2],
		&color_matrix[3],
		&color_matrix[4],
		&color_matrix[5],
		&color_matrix[6],
		&color_matrix[7],
		&color_matrix[8]);
	for(int i = 0; i < get_total_packages(); i++)
	{
		InterpolatePixelsPackage *package = (InterpolatePixelsPackage*)get_package(i);
		package->y1 = plugin->get_temp()->get_h() * i / get_total_packages();
		package->y2 = plugin->get_temp()->get_h() * (i + 1) / get_total_packages();
	}
}


LoadClient* InterpolatePixelsEngine::new_client()
{
	return new InterpolatePixelsUnit(this, plugin);
}


LoadPackage* InterpolatePixelsEngine::new_package()
{
	return new InterpolatePixelsPackage;
}


