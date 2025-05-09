
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
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
#include "colormodels.h"
#include "filexml.h"
#include "language.h"
#include "picon_png.h"
#include "rgb601.h"
#include "rgb601window.h"

#include <stdio.h>
#include <string.h>


REGISTER_PLUGIN(RGB601Main)




RGB601Config::RGB601Config()
{
	direction = 0;
}

RGB601Main::RGB601Main(PluginServer *server)
 : PluginVClient(server)
{
	
}

RGB601Main::~RGB601Main()
{
	
}

const char* RGB601Main::plugin_title() { return N_("RGB - 601"); }
int RGB601Main::is_realtime() { return 1; }


NEW_WINDOW_MACRO(RGB601Main, RGB601Window)

NEW_PICON_MACRO(RGB601Main)

void RGB601Main::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		((RGB601Window*)thread->window)->forward->update(config.direction == 1);
		((RGB601Window*)thread->window)->reverse->update(config.direction == 2);
		thread->window->unlock_window();
	}
}


int RGB601Main::load_configuration()
{
	KeyFrame *prev_keyframe;

	prev_keyframe = get_prev_keyframe(get_source_position());
// Must also switch between interpolation between keyframes and using first keyframe
	read_data(prev_keyframe);
	return 1;
}


void RGB601Main::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->get_data(), MESSAGESIZE);
	output.tag.set_title("RGB601");
	output.tag.set_property("DIRECTION", config.direction);
	output.append_tag();
	output.terminate_string();
}

void RGB601Main::read_data(KeyFrame *keyframe)
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
			if(input.tag.title_is("RGB601"))
			{
				config.direction = input.tag.get_property("DIRECTION", config.direction);
			}
		}
	}

	if(thread) 
	{
		((RGB601Window*)thread->window)->update();
	}
}


#define CREATE_TABLE(max) \
{ \
	for(int i = 0; i < max; i++) \
	{ \
		int forward_output = (int)((double)0.8588 * i + max * 0.0627 + 0.5); \
		int reverse_output = (int)((double)1.1644 * i - max * 0.0627 + 0.5); \
		forward_table[i] = CLIP(forward_output, 0, max - 1); \
		reverse_table[i] = CLIP(reverse_output, 0, max - 1); \
	} \
}

void RGB601Main::create_table(VFrame *input_ptr)
{
	switch(input_ptr->get_color_model())
	{
		case BC_RGB888:
		case BC_YUV888:
		case BC_RGBA8888:
		case BC_YUVA8888:
			CREATE_TABLE(0x100);
			break;

		case BC_RGB161616:
		case BC_YUV161616:
		case BC_RGBA16161616:
		case BC_YUVA16161616:
			CREATE_TABLE(0x10000);
			break;
	}
}

#define PROCESS(table, type, components, yuv) \
{ \
	int bytes = w * components; \
	for(int i = 0; i < h; i++) \
	{ \
		type *in_row = (type*)input_ptr->get_rows()[i]; \
		type *out_row = (type*)output_ptr->get_rows()[i]; \
 \
		if(yuv) \
		{ \
/* Just do Y */ \
			for(int j = 0; j < w; j++) \
			{ \
				out_row[j * components] = table[(int)in_row[j * components]]; \
				out_row[j * components + 1] = in_row[j * components + 1]; \
				out_row[j * components + 2] = in_row[j * components + 2]; \
			} \
		} \
		else \
		if(sizeof(type) == 4) \
		{ \
			for(int j = 0; j < w; j++) \
			{ \
				if(table == forward_table) \
				{ \
					out_row[j * components] = (type)(in_row[j * components] * 0.8588 + 0.0627); \
					out_row[j * components + 1] = (type)(in_row[j * components + 1] * 0.8588 + 0.0627); \
					out_row[j * components + 2] = (type)(in_row[j * components + 2] * 0.8588 + 0.0627); \
				} \
				else \
				{ \
					out_row[j * components] = (type)(in_row[j * components] * 1.1644 - 0.0627); \
					out_row[j * components + 1] = (type)(in_row[j * components + 1] * 1.1644 - 0.0627); \
					out_row[j * components + 2] = (type)(in_row[j * components + 2] * 1.1644 - 0.0627); \
				} \
			} \
		} \
		else \
		{ \
			for(int j = 0; j < w; j++) \
			{ \
				out_row[j * components] = table[(int)in_row[j * components]]; \
				out_row[j * components + 1] = table[(int)in_row[j * components + 1]]; \
				out_row[j * components + 2] = table[(int)in_row[j * components + 2]]; \
			} \
		} \
	} \
}

void RGB601Main::process(int *table, VFrame *input_ptr, VFrame *output_ptr)
{
	int w = input_ptr->get_w();
	int h = input_ptr->get_h();
	
	if(config.direction == 1)
		switch(input_ptr->get_color_model())
		{
			case BC_YUV888:
				PROCESS(forward_table, unsigned char, 3, 1);
				break;
			case BC_YUVA8888:
				PROCESS(forward_table, unsigned char, 4, 1);
				break;
			case BC_YUV161616:
				PROCESS(forward_table, u_int16_t, 3, 1);
				break;
			case BC_YUVA16161616:
				PROCESS(forward_table, u_int16_t, 4, 1);
				break;
			case BC_RGB888:
				PROCESS(forward_table, unsigned char, 3, 0);
				break;
			case BC_RGBA8888:
				PROCESS(forward_table, unsigned char, 4, 0);
				break;
			case BC_RGB_FLOAT:
				PROCESS(forward_table, float, 3, 0);
				break;
			case BC_RGBA_FLOAT:
				PROCESS(forward_table, float, 4, 0);
				break;
			case BC_RGB161616:
				PROCESS(forward_table, u_int16_t, 3, 0);
				break;
			case BC_RGBA16161616:
				PROCESS(forward_table, u_int16_t, 4, 0);
				break;
		}
	else
	if(config.direction == 2)
		switch(input_ptr->get_color_model())
		{
			case BC_YUV888:
				PROCESS(reverse_table, unsigned char, 3, 1);
				break;
			case BC_YUVA8888:
				PROCESS(reverse_table, unsigned char, 4, 1);
				break;
			case BC_YUV161616:
				PROCESS(reverse_table, u_int16_t, 3, 1);
				break;
			case BC_YUVA16161616:
				PROCESS(reverse_table, u_int16_t, 4, 1);
				break;
			case BC_RGB888:
				PROCESS(reverse_table, unsigned char, 3, 0);
				break;
			case BC_RGBA8888:
				PROCESS(reverse_table, unsigned char, 4, 0);
				break;
			case BC_RGB_FLOAT:
				PROCESS(reverse_table, float, 3, 0);
				break;
			case BC_RGBA_FLOAT:
				PROCESS(reverse_table, float, 4, 0);
				break;
			case BC_RGB161616:
				PROCESS(reverse_table, u_int16_t, 3, 0);
				break;
			case BC_RGBA16161616:
				PROCESS(reverse_table, u_int16_t, 4, 0);
				break;
		}
}

int RGB601Main::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	load_configuration();

// Set parameters for aggregation with previous or next effect.
	frame->get_params()->update("RGB601_DIRECTION", config.direction);

	read_frame(frame, 
		0, 
		start_position, 
		frame_rate,
		get_use_opengl());

// Deinterlace effects may aggregate this one,
	if(get_use_opengl() &&
		(prev_effect_is("Frames to fields") ||
		next_effect_is("Frames to fields")))
	{
		return 0;
	}

	if(get_use_opengl() && config.direction)
	{
		run_opengl();
		return 0;
	}
	

	create_table(frame);

	if(config.direction == 1)
		process(forward_table, frame, frame);
	else
	if(config.direction == 2)
		process(reverse_table, frame, frame);

	return 0;
}

int RGB601Main::handle_opengl()
{
#ifdef HAVE_GL
	static const char *yuv_fwd_frag = 
		"uniform sampler2D tex;\n"
		"void main()\n"
		"{\n"
		"	gl_FragColor = texture2D(tex, gl_TexCoord[0].st);\n"
		"	gl_FragColor.r = gl_FragColor.r * 0.8588 + 0.0627;\n"
		"}\n";
	static const char *yuv_rev_frag = 
		"uniform sampler2D tex;\n"
		"void main()\n"
		"{\n"
		"	gl_FragColor = texture2D(tex, gl_TexCoord[0].st);\n"
		"	gl_FragColor.r = gl_FragColor.r * 1.1644 - 0.0627;\n"
		"}\n";
	static const char *rgb_fwd_frag = 
		"uniform sampler2D tex;\n"
		"void main()\n"
		"{\n"
		"	gl_FragColor = texture2D(tex, gl_TexCoord[0].st);\n"
		"	gl_FragColor.rgb = gl_FragColor.rgb * vec3(0.8588, 0.8588, 0.8588) + vec3(0.0627, 0.0627, 0.0627);\n"
		"}\n";
	static const char *rgb_rev_frag = 
		"uniform sampler2D tex;\n"
		"void main()\n"
		"{\n"
		"	gl_FragColor = texture2D(tex, gl_TexCoord[0].st);\n"
		"	gl_FragColor.rgb = gl_FragColor.rgb * vec3(1.1644, 1.1644, 1.1644) - vec3(0.0627, 0.0627, 0.0627);\n"
		"}\n";


	get_output()->to_texture();
	get_output()->enable_opengl();
	get_output()->bind_texture(0);

	unsigned int frag_shader = 0;
	switch(get_output()->get_color_model())
	{
		case BC_YUV888:
		case BC_YUVA8888:
			frag_shader = VFrame::make_shader(0,
				config.direction == 1 ? yuv_fwd_frag : yuv_rev_frag,
				0);
		break;

		default:
			frag_shader = VFrame::make_shader(0,
				config.direction == 1 ? rgb_fwd_frag : rgb_rev_frag,
				0);
		break;
	}

	if(frag_shader)
	{
		glUseProgram(frag_shader);
		glUniform1i(glGetUniformLocation(frag_shader, "tex"), 0);
	}
	VFrame::init_screen(get_output()->get_w(), get_output()->get_h());
	get_output()->draw_texture();
	glUseProgram(0);
	get_output()->set_opengl_state(VFrame::SCREEN);
#endif
    return 0;
}



