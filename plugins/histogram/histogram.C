/*
 * CINELERRA
 * Copyright (C) 2008-2021 Adam Williams <broadcast at earthling dot net>
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
#include <unistd.h>

#include "bcdisplayinfo.h"
#include "bcsignals.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "histogram.h"
#include "histogramconfig.h"
#include "histogramwindow.h"
#include "keyframe.h"
#include "language.h"
#include "loadbalance.h"
#include "playback3d.h"
#include "cicolors.h"
#include "vframe.h"
#include "workarounds.h"

#include "aggregated.h"
#include "../colorbalance/aggregated.h"
#include "../interpolate/aggregated.h"
#include "../gamma/aggregated.h"

class HistogramMain;
class HistogramEngine;
class HistogramWindow;





REGISTER_PLUGIN(HistogramMain)









HistogramMain::HistogramMain(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	for(int i = 0; i < HISTOGRAM_MODES; i++)
	{
		lookup[i] = 0;
		lookup16[i] = 0;
		accum[i] = 0;
		preview_lookup[i] = 0;
	}
	current_point = -1;
	mode = HISTOGRAM_VALUE;
	dragging_point = 0;
	input = 0;
	output = 0;
	w = DP(440);
	h = DP(500);
	parade = 0;
}

HistogramMain::~HistogramMain()
{
	
	for(int i = 0; i < HISTOGRAM_MODES;i++)
	{
		delete [] lookup[i];
		delete [] lookup16[i];
		delete [] accum[i];
		delete [] preview_lookup[i];
	}
	delete engine;
}

const char* HistogramMain::plugin_title() { return N_("Histogram"); }
int HistogramMain::is_realtime() { return 1; }


#include "picon_png.h"
NEW_PICON_MACRO(HistogramMain)

NEW_WINDOW_MACRO(HistogramMain, HistogramWindow)

LOAD_CONFIGURATION_MACRO(HistogramMain, HistogramConfig)

void HistogramMain::render_gui(void *data, int size)
{
	if(thread)
	{
// Process just the RGB values to determine the automatic points or
// all the points if manual
		if(!config.automatic)
		{
// Generate curves for RGB channels from the control points
// Lock out changes to curves
			((HistogramWindow*)thread->window)->lock_window("HistogramMain::render_gui 1");
			tabulate_curve(HISTOGRAM_RED, 0);
			tabulate_curve(HISTOGRAM_GREEN, 0);
			tabulate_curve(HISTOGRAM_BLUE, 0);
			((HistogramWindow*)thread->window)->unlock_window();
		}

// calculate histograms for the RGB channels & 
// the VALUE channel only if not automatic RGB
		calculate_histogram((VFrame*)data, !config.automatic);


// calculate automatic curves for the RGB channels
		if(config.automatic)
		{
			calculate_automatic((VFrame*)data, 0);

// Generate curves for value histogram
// Lock out changes to curves
			((HistogramWindow*)thread->window)->lock_window("HistogramMain::render_gui 1");
			tabulate_curve(HISTOGRAM_RED, 0);
			tabulate_curve(HISTOGRAM_GREEN, 0);
			tabulate_curve(HISTOGRAM_BLUE, 0);
			((HistogramWindow*)thread->window)->unlock_window();


// calculate histogram for the LUMA channel
			calculate_histogram((VFrame*)data, 1);
		}

// calculate automatic curve for the LUMA channel
        if(config.automatic_v)
        {
            calculate_automatic((VFrame*)data, 1);
        }

		((HistogramWindow*)thread->window)->lock_window("HistogramMain::render_gui 2");
// Always draw the histogram but don't update widgets if automatic
		int skip_widgets = (config.automatic && mode != HISTOGRAM_VALUE) ||
            (config.automatic_v && mode == HISTOGRAM_VALUE);
        ((HistogramWindow*)thread->window)->update(1,
			!skip_widgets,
			!skip_widgets,
			0);

		((HistogramWindow*)thread->window)->unlock_window();
	}
}

void HistogramMain::update_gui()
{
	if(thread)
	{
		((HistogramWindow*)thread->window)->lock_window("HistogramMain::update_gui");
		int reconfigure = load_configuration();
		if(reconfigure) 
		{
			((HistogramWindow*)thread->window)->update(1,
				1,
				1,
				1);
		}
		((HistogramWindow*)thread->window)->unlock_window();
	}
}




void HistogramMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->get_data(), MESSAGESIZE);
	output.tag.set_title("HISTOGRAM");

	char string[BCTEXTLEN];

	output.tag.set_property("AUTOMATIC", config.automatic);
	output.tag.set_property("AUTOMATIC_V", config.automatic_v);
	output.tag.set_property("THRESHOLD", config.threshold);
	output.tag.set_property("PLOT", config.plot);
	output.tag.set_property("SPLIT", config.split);
	output.tag.set_property("W", w);
	output.tag.set_property("H", h);
	output.tag.set_property("PARADE", parade);
	output.tag.set_property("MODE", mode);

	for(int i = 0; i < HISTOGRAM_MODES; i++)
	{
		sprintf(string, "LOW_OUTPUT_%d", i);
		output.tag.set_property(string, config.low_output[i]);
		sprintf(string, "HIGH_OUTPUT_%d", i);
	   	output.tag.set_property(string, config.high_output[i]);
		sprintf(string, "LOW_INPUT_%d", i);
	   	output.tag.set_property(string, config.low_input[i]);
		sprintf(string, "HIGH_INPUT_%d", i);
	   	output.tag.set_property(string, config.high_input[i]);
		sprintf(string, "GAMMA_%d", i);
	   	output.tag.set_property(string, config.gamma[i]);
//printf("HistogramMain::save_data %d %f %d\n", config.input_min[i], config.input_mid[i], config.input_max[i]);
	}

	output.append_tag();
	output.append_newline();






	output.terminate_string();
}

void HistogramMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), strlen(keyframe->get_data()));

	int result = 0;
	int current_input_mode = 0;


	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("HISTOGRAM"))
			{
				config.automatic = input.tag.get_property("AUTOMATIC", config.automatic);
				config.automatic_v = input.tag.get_property("AUTOMATIC_V", config.automatic_v);
				config.threshold = input.tag.get_property("THRESHOLD", config.threshold);
				config.plot = input.tag.get_property("PLOT", config.plot);
				config.split = input.tag.get_property("SPLIT", config.split);

				if(is_defaults())
				{
					w = input.tag.get_property("W", w);
					h = input.tag.get_property("H", h);
					parade = input.tag.get_property("PARADE", parade);
					mode = input.tag.get_property("MODE", mode);
				}

				char string[BCTEXTLEN];
				for(int i = 0; i < HISTOGRAM_MODES; i++)
				{
					sprintf(string, "LOW_OUTPUT_%d", i);
					config.low_output[i] = input.tag.get_property(string, config.low_output[i]);
					sprintf(string, "HIGH_OUTPUT_%d", i);
					config.high_output[i] = input.tag.get_property(string, config.high_output[i]);
					sprintf(string, "GAMMA_%d", i);
					config.gamma[i] = input.tag.get_property(string, config.gamma[i]);

					if(!(config.automatic && i != HISTOGRAM_VALUE) &&
                        !(config.automatic_v && i == HISTOGRAM_VALUE))
					{
						sprintf(string, "LOW_INPUT_%d", i);
						config.low_input[i] = input.tag.get_property(string, config.low_input[i]);
						sprintf(string, "HIGH_INPUT_%d", i);
						config.high_input[i] = input.tag.get_property(string, config.high_input[i]);
					}
//printf("HistogramMain::read_data %d %f %d\n", config.input_min[i], config.input_mid[i], config.input_max[i]);
				}
			}
		}
	}

	config.boundaries();

}

float HistogramMain::calculate_level(float input, 
	int mode,
	int use_value)
{
	int done = 0;
	float output = 0.0;

// Scale to input range
	if(!EQUIV(config.high_input[mode], config.low_input[mode]))
	{
		output = (input - config.low_input[mode]) /
			(config.high_input[mode] - config.low_input[mode]);
	}
	else
	{
		output = input;
	}
	


	if(!EQUIV(config.gamma[mode], 0))
	{
        if(output > 0)
        {
    		output = pow(output, 1.0 / config.gamma[mode]);
        }

		CLAMP(output, 0, 1.0);
	}

// Apply value curve to rendered output for the channel
	if(use_value && mode != HISTOGRAM_VALUE)
	{
		output = calculate_level(output, HISTOGRAM_VALUE, 0);
	}




// scale to output range
	if(!EQUIV(config.low_output[mode], config.high_output[mode]))
	{
		output = output * (config.high_output[mode] - config.low_output[mode]) +
			config.low_output[mode];
	}

	CLAMP(output, 0, 1.0);

	return output;
}



void HistogramMain::calculate_histogram(VFrame *data, int do_value)
{

	if(!engine) engine = new HistogramEngine(this,
		get_project_smp() + 1,
		get_project_smp() + 1);

	if(!accum[0])
	{
		for(int i = 0; i < HISTOGRAM_MODES; i++)
			accum[i] = new int[HISTOGRAM_SLOTS];
	}

	engine->process_packages(HistogramEngine::HISTOGRAM, data, do_value);

	for(int i = 0; i < engine->get_total_clients(); i++)
	{
		HistogramUnit *unit = (HistogramUnit*)engine->get_client(i);

		if(i == 0)
		{
			for(int j = 0; j < HISTOGRAM_MODES; j++)
			{
				memcpy(accum[j], unit->accum[j], sizeof(int) * HISTOGRAM_SLOTS);
			}
		}
		else
		{
			for(int j = 0; j < HISTOGRAM_MODES; j++)
			{
				int *out = accum[j];
				int *in = unit->accum[j];
				for(int k = 0; k < HISTOGRAM_SLOTS; k++)
					out[k] += in[k];
			}
		}
	}

// Remove top and bottom from calculations.  Doesn't work in high
// precision colormodels.
	for(int i = 0; i < HISTOGRAM_MODES; i++)
	{
		accum[i][0] = 0;
		accum[i][HISTOGRAM_SLOTS - 1] = 0;
	}
}


void HistogramMain::calculate_automatic(VFrame *data, int do_value)
{
// must calculate RGB curves before generating the automatic histogram
    if(do_value)
    {
        for(int i = 0; i < 3; i++)
        {
            tabulate_curve(i, 0);
        }
    }

	calculate_histogram(data, do_value);
	config.reset_points(!do_value, do_value);

// Do each channel
	for(int i = 0; i < HISTOGRAM_MODES; i++)
	{
        if((!do_value && i < 3) || 
            (do_value && i == 3))
        {
		    int *accum = this->accum[i];
		    int pixels = data->get_w() * data->get_h();
		    float white_fraction = 1.0 - (1.0 - config.threshold) / 2;
		    int threshold = (int)(white_fraction * pixels);
		    int total = 0;
		    float max_level = 1.0;
		    float min_level = 0.0;

    // Get histogram slot above threshold % of pixels
		    for(int j = 0; j < HISTOGRAM_SLOTS; j++)
		    {
			    total += accum[j];
			    if(total >= threshold)
			    {
				    max_level = (float)j / 
                        HISTOGRAM_SLOTS * 
                        FLOAT_RANGE + 
                        HISTOGRAM_MIN_INPUT;
				    break;
			    }
		    }

    // Get slot below threshold % of pixels
		    total = 0;
		    for(int j = HISTOGRAM_SLOTS - 1; j >= 0; j--)
		    {
			    total += accum[j];
			    if(total >= threshold)
			    {
				    min_level = (float)j / 
                        HISTOGRAM_SLOTS * 
                        FLOAT_RANGE + 
                        HISTOGRAM_MIN_INPUT;
				    break;
			    }
		    }


		    config.low_input[i] = min_level;
		    config.high_input[i] = max_level;
        }
	}
}





int HistogramMain::calculate_use_opengl()
{
// glHistogram doesn't work.
	int result = get_use_opengl() &&
		!config.automatic && 
        !config.automatic_v && 
		(!config.plot || !gui_open());
	return result;
}


int HistogramMain::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	int need_reconfigure = load_configuration();
	int use_opengl = calculate_use_opengl();
	this->input = frame;
	this->output = frame;

//printf("HistogramMain::process_buffer %d: use_opengl=%d\n", __LINE__, use_opengl);
	read_frame(frame, 
		0, 
		start_position, 
		frame_rate,
		use_opengl);
// Generate tables here.  The same table is used by many packages to render
// each horizontal stripe.  Need to cover the entire output range in  each
// table to avoid green borders
//printf("HistogramMain::process_buffer %d: use_opengl=%d\n", __LINE__, use_opengl);
//frame->dump(4);

	if(need_reconfigure || 
		!lookup[0] || 
		!lookup16[0] || 
		config.automatic ||
        config.automatic_v)
	{
// Calculate new curves
		if(config.automatic || config.automatic_v)
		{
			calculate_automatic(input, config.automatic_v);
		}


// Generate RGB transfer tables with baked value curve
		for(int i = 0; i < 3; i++)
		{
        	tabulate_curve(i, 1);
        }
	}

// Apply histogram in GPU
	if(use_opengl) return run_opengl();

	if(!engine) engine = new HistogramEngine(this,
		get_project_smp() + 1,
		get_project_smp() + 1);
// Always plot to set the curves if automatic
	if(config.plot || config.automatic) send_render_gui(frame, 1);


// printf("HistogramMain::process_buffer %d %f %f %f  %f %f %f  %f %f %f\n", 
// __LINE__, 
// config.low_input[HISTOGRAM_RED],
// config.gamma[HISTOGRAM_RED],
// config.high_input[HISTOGRAM_RED],
// config.low_input[HISTOGRAM_GREEN],
// config.gamma[HISTOGRAM_GREEN],
// config.high_input[HISTOGRAM_GREEN],
// config.low_input[HISTOGRAM_BLUE],
// config.gamma[HISTOGRAM_BLUE],
// config.high_input[HISTOGRAM_BLUE]);



// Apply histogram
	engine->process_packages(HistogramEngine::APPLY, input, 0);


	return 0;
}

void HistogramMain::tabulate_curve(int subscript, int use_value)
{
	int i;
	if(!lookup[subscript])
		lookup[subscript] = new int[HISTOGRAM_SLOTS];
	if(!lookup16[subscript])
		lookup16[subscript] = new int[HISTOGRAM_SLOTS];
	if(!preview_lookup[subscript])
		preview_lookup[subscript] = new int[HISTOGRAM_SLOTS];

//printf("HistogramMain::tabulate_curve %d input=%p\n", __LINE__, input);


// Generate lookup tables for integer colormodels
	for(i = 0; i < 256; i++)
	{
		lookup[subscript][i] = 
			(int)(calculate_level((float)i / 0xff, subscript, use_value) * 
			0xff);
		CLAMP(lookup[subscript][i], 0, 0xff);
	}

	for(i = 0; i < 65536; i++)
	{
		lookup16[subscript][i] = 
			(int)(calculate_level((float)i / 0xffff, subscript, use_value) * 
			0xffff);
		CLAMP(lookup16[subscript][i], 0, 0xffff);
	}
// for(i = 0; i < 0x100; i++)
// {
// if(subscript == HISTOGRAM_BLUE) printf("%d ", lookup[subscript][i * 0x100]);
// }
// if(subscript == HISTOGRAM_BLUE) printf("\n");

// Lookup table for preview only used for GUI
	if(!use_value)
	{
		for(i = 0; i < 0x10000; i++)
		{
			preview_lookup[subscript][i] = 
				(int)(calculate_level((float)i / 0xffff, subscript, use_value) * 
				0xffff);
			CLAMP(preview_lookup[subscript][i], 0, 0xffff);
		}
	}
}

int HistogramMain::handle_opengl()
{
#ifdef HAVE_GL
// Functions to get pixel from either previous effect or texture
	static const char *histogram_get_pixel1 =
		"vec4 histogram_get_pixel()\n"
		"{\n"
		"	return gl_FragColor;\n"
		"}\n";

	static const char *histogram_get_pixel2 =
		"uniform sampler2D tex;\n"
		"vec4 histogram_get_pixel()\n"
		"{\n"
		"	return texture2D(tex, gl_TexCoord[0].st);\n"
		"}\n";

	static const char *head_frag = 
		"uniform vec4 low_input;\n"
		"uniform vec4 high_input;\n"
		"uniform vec4 gamma;\n"
		"uniform vec4 low_output;\n"
		"uniform vec4 output_scale;\n"
		"uniform int lookup_r[256];\n"
		"uniform int lookup_g[256];\n"
		"uniform int lookup_b[256];\n"
		"void main()\n"
		"{\n"
		"	float temp = 0.0;\n";

	static const char *get_rgb_frag =
		"	vec4 pixel = histogram_get_pixel();\n";

	static const char *get_yuv_frag =
		"	vec4 pixel = histogram_get_pixel();\n"
			YUV_TO_RGB_FRAG("pixel");

#define APPLY_INPUT_CURVE(PIXEL, LOW_INPUT, HIGH_INPUT, GAMMA) \
		"// apply input curve\n" \
		"	temp = (" PIXEL " - " LOW_INPUT ") / \n" \
		"		(" HIGH_INPUT " - " LOW_INPUT ");\n" \
		"	temp = max(temp, 0.0);\n" \
		"	" PIXEL " = pow(temp, 1.0 / " GAMMA ");\n"



	static const char *apply_lut_frag = 
        "   int r = int(clamp(pixel.r * 255.0, 0, 255));\n"
        "   int g = int(clamp(pixel.g * 255.0, 0, 255));\n"
        "   int b = int(clamp(pixel.b * 255.0, 0, 255));\n"
        "   pixel.r = float(lookup_r[r]) / 255.0;\n"
        "   pixel.g = float(lookup_g[g]) / 255.0;\n"
        "   pixel.b = float(lookup_b[b]) / 255.0;\n";

	static const char *apply_float_frag = 
		APPLY_INPUT_CURVE("pixel.r", "low_input.r", "high_input.r", "gamma.r")
		APPLY_INPUT_CURVE("pixel.g", "low_input.g", "high_input.g", "gamma.g")
		APPLY_INPUT_CURVE("pixel.b", "low_input.b", "high_input.b", "gamma.b")
		"// apply output curve\n"
		"	pixel.rgb *= output_scale.rgb;\n"
		"	pixel.rgb += low_output.rgb;\n"
		APPLY_INPUT_CURVE("pixel.r", "low_input.a", "high_input.a", "gamma.a")
		APPLY_INPUT_CURVE("pixel.g", "low_input.a", "high_input.a", "gamma.a")
		APPLY_INPUT_CURVE("pixel.b", "low_input.a", "high_input.a", "gamma.a")
		"// apply output curve\n"
		"	pixel.rgb *= vec3(output_scale.a, output_scale.a, output_scale.a);\n"
		"	pixel.rgb += vec3(low_output.a, low_output.a, low_output.a);\n";

	static const char *put_rgb_frag =
		"	gl_FragColor = pixel;\n"
		"}\n";

	static const char *put_yuv_frag =
			RGB_TO_YUV_FRAG("pixel")
		"	gl_FragColor = pixel;\n"
		"}\n";



	get_output()->to_texture();
	get_output()->enable_opengl();

	const char *shader_stack[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	int current_shader = 0;
	int aggregate_interpolation = 0;
	int aggregate_gamma = 0;
	int aggregate_colorbalance = 0;
// All aggregation possibilities must be accounted for because unsupported
// effects can get in between the aggregation members.
// 	if(!strcmp(get_output()->get_prev_effect(2), "Interpolate Pixels") &&
// 		!strcmp(get_output()->get_prev_effect(1), "Gamma") &&
// 		!strcmp(get_output()->get_prev_effect(0), "Color Balance"))
// 	{
// 		aggregate_interpolation = 1;
// 		aggregate_gamma = 1;
// 		aggregate_colorbalance = 1;
// 	}
// 	else
	if(!strcmp(get_output()->get_prev_effect(1), "Gamma") &&
		!strcmp(get_output()->get_prev_effect(0), "Color Balance"))
	{
		aggregate_gamma = 1;
		aggregate_colorbalance = 1;
	}
	else
// 	if(!strcmp(get_output()->get_prev_effect(1), "Interpolate Pixels") &&
// 		!strcmp(get_output()->get_prev_effect(0), "Gamma"))
// 	{
// 		aggregate_interpolation = 1;
// 		aggregate_gamma = 1;
// 	}
// 	else
// 	if(!strcmp(get_output()->get_prev_effect(1), "Interpolate Pixels") &&
// 		!strcmp(get_output()->get_prev_effect(0), "Color Balance"))
// 	{
// 		aggregate_interpolation = 1;
// 		aggregate_colorbalance = 1;
// 	}
// 	else
// 	if(!strcmp(get_output()->get_prev_effect(0), "Interpolate Pixels"))
// 		aggregate_interpolation = 1;
// 	else
	if(!strcmp(get_output()->get_prev_effect(0), "Gamma"))
		aggregate_gamma = 1;
	else
	if(!strcmp(get_output()->get_prev_effect(0), "Color Balance"))
		aggregate_colorbalance = 1;


// The order of processing is fixed by this sequence
	if(aggregate_interpolation)
		INTERPOLATE_COMPILE(shader_stack, 
			current_shader)

	if(aggregate_gamma)
		GAMMA_COMPILE(shader_stack, 
			current_shader, 
			aggregate_interpolation)

	if(aggregate_colorbalance)
		COLORBALANCE_COMPILE(shader_stack, 
			current_shader, 
			aggregate_interpolation || aggregate_gamma)


	if(aggregate_interpolation || aggregate_gamma || aggregate_colorbalance)
		shader_stack[current_shader++] = histogram_get_pixel1;
	else
		shader_stack[current_shader++] = histogram_get_pixel2;

	unsigned int shader = 0;
	switch(get_output()->get_color_model())
	{
        case BC_RGB888:
        case BC_RGBA8888:
			shader_stack[current_shader++] = head_frag;
			shader_stack[current_shader++] = get_rgb_frag;
			shader_stack[current_shader++] = apply_lut_frag;
			shader_stack[current_shader++] = put_rgb_frag;
            break;
    
    
		case BC_YUV888:
		case BC_YUVA8888:
			shader_stack[current_shader++] = head_frag;
			shader_stack[current_shader++] = get_yuv_frag;
			shader_stack[current_shader++] = apply_lut_frag;
			shader_stack[current_shader++] = put_yuv_frag;
			break;

		case BC_RGB_FLOAT:
        case BC_RGBA_FLOAT:
			shader_stack[current_shader++] = head_frag;
			shader_stack[current_shader++] = get_rgb_frag;
			shader_stack[current_shader++] = apply_float_frag;
			shader_stack[current_shader++] = put_rgb_frag;
			break;

        case BC_YUV_FLOAT:
            shader_stack[current_shader++] = head_frag;
            shader_stack[current_shader++] = get_yuv_frag;
			shader_stack[current_shader++] = apply_float_frag;
			shader_stack[current_shader++] = put_yuv_frag;
            break;


        default:
            printf("HistogramMain::handle_opengl %d unsupported color model\n", 
                __LINE__);
            break;
	}

	shader = VFrame::make_shader(0,
		shader_stack[0],
		shader_stack[1],
		shader_stack[2],
		shader_stack[3],
		shader_stack[4],
		shader_stack[5],
		shader_stack[6],
		shader_stack[7],
		shader_stack[8],
		shader_stack[9],
		shader_stack[10],
		shader_stack[11],
		shader_stack[12],
		shader_stack[13],
		shader_stack[14],
		shader_stack[15],
		0);

// printf("HistogramMain::handle_opengl %d %d %d %d shader=%d\n", 
// aggregate_interpolation, 
// aggregate_gamma,
// aggregate_colorbalance,
// current_shader,
// shader);

	float low_input[4];
	float high_input[4];
	float gamma[4];
	float low_output[4];
	float output_scale[4];


// printf("min x    min y    max x    max y\n");
// printf("%f %f %f %f\n", input_min_r[0], input_min_r[1], input_max_r[0], input_max_r[1]);
// printf("%f %f %f %f\n", input_min_g[0], input_min_g[1], input_max_g[0], input_max_g[1]);
// printf("%f %f %f %f\n", input_min_b[0], input_min_b[1], input_max_b[0], input_max_b[1]);
// printf("%f %f %f %f\n", input_min_v[0], input_min_v[1], input_max_v[0], input_max_v[1]);

	for(int i = 0; i < HISTOGRAM_MODES; i++)
	{
		low_input[i] = config.low_input[i];
		high_input[i] = config.high_input[i];
		gamma[i] = config.gamma[i];
		low_output[i] = config.low_output[i];
		output_scale[i] = config.high_output[i] - config.low_output[i];
	}

	if(shader > 0)
	{
		glUseProgram(shader);
		glUniform1i(glGetUniformLocation(shader, "tex"), 0);
		if(aggregate_gamma) GAMMA_UNIFORMS(shader)
		if(aggregate_interpolation) INTERPOLATE_UNIFORMS(shader)
		if(aggregate_colorbalance) COLORBALANCE_UNIFORMS(shader)
		glUniform4fv(glGetUniformLocation(shader, "low_input"), 1, low_input);
		glUniform4fv(glGetUniformLocation(shader, "high_input"), 1, high_input);
		glUniform4fv(glGetUniformLocation(shader, "gamma"), 1, gamma);
		glUniform4fv(glGetUniformLocation(shader, "low_output"), 1, low_output);
		glUniform4fv(glGetUniformLocation(shader, "output_scale"), 1, output_scale);
        glUniform1iv(glGetUniformLocation(shader, "lookup_r"), 256, lookup[0]);
        glUniform1iv(glGetUniformLocation(shader, "lookup_g"), 256, lookup[1]);
        glUniform1iv(glGetUniformLocation(shader, "lookup_b"), 256, lookup[2]);
	}

	get_output()->init_screen();
	get_output()->bind_texture(0);

	glDisable(GL_BLEND);

// Draw the affected half
	if(config.split)
	{
		glBegin(GL_TRIANGLES);
		glNormal3f(0, 0, 1.0);

		glTexCoord2f(0.0 / get_output()->get_texture_w(), 
			0.0 / get_output()->get_texture_h());
		glVertex3f(0.0, -(float)get_output()->get_h(), 0);


		glTexCoord2f((float)get_output()->get_w() / get_output()->get_texture_w(), 
			(float)get_output()->get_h() / get_output()->get_texture_h());
		glVertex3f((float)get_output()->get_w(), -0.0, 0);

		glTexCoord2f(0.0 / get_output()->get_texture_w(), 
			(float)get_output()->get_h() / get_output()->get_texture_h());
		glVertex3f(0.0, -0.0, 0);


		glEnd();
	}
	else
	{
		get_output()->draw_texture();
	}

	glUseProgram(0);

// Draw the unaffected half
	if(config.split)
	{
		glBegin(GL_TRIANGLES);
		glNormal3f(0, 0, 1.0);


		glTexCoord2f(0.0 / get_output()->get_texture_w(), 
			0.0 / get_output()->get_texture_h());
		glVertex3f(0.0, -(float)get_output()->get_h(), 0);

		glTexCoord2f((float)get_output()->get_w() / get_output()->get_texture_w(), 
			0.0 / get_output()->get_texture_h());
		glVertex3f((float)get_output()->get_w(), 
			-(float)get_output()->get_h(), 0);

		glTexCoord2f((float)get_output()->get_w() / get_output()->get_texture_w(), 
			(float)get_output()->get_h() / get_output()->get_texture_h());
		glVertex3f((float)get_output()->get_w(), -0.0, 0);


 		glEnd();
	}

	get_output()->set_opengl_state(VFrame::SCREEN);
#endif
    return 0;
}












HistogramPackage::HistogramPackage()
 : LoadPackage()
{
}




HistogramUnit::HistogramUnit(HistogramEngine *server, 
	HistogramMain *plugin)
 : LoadClient(server)
{
	this->plugin = plugin;
	this->server = server;
	for(int i = 0; i < HISTOGRAM_MODES; i++)
	{
    	accum[i] = new int[HISTOGRAM_SLOTS];
    }
}

HistogramUnit::~HistogramUnit()
{
	for(int i = 0; i < HISTOGRAM_MODES; i++)
		delete [] accum[i];
}

void HistogramUnit::process_package(LoadPackage *package)
{
	HistogramPackage *pkg = (HistogramPackage*)package;

	if(server->operation == HistogramEngine::HISTOGRAM)
	{
		int do_value = server->do_value;


#define HISTOGRAM_HEAD(type) \
{ \
	for(int i = pkg->start; i < pkg->end; i++) \
	{ \
		type *row = (type*)data->get_rows()[i]; \
		for(int j = 0; j < w; j++) \
		{

#define HISTOGRAM_TAIL(components) \
/* Value takes the maximum of the output RGB values */ \
			if(do_value) \
			{ \
				CLAMP(r, 0, HISTOGRAM_SLOTS - 1); \
				CLAMP(g, 0, HISTOGRAM_SLOTS - 1); \
				CLAMP(b, 0, HISTOGRAM_SLOTS - 1); \
				r_out = lookup_r[r]; \
				g_out = lookup_g[g]; \
				b_out = lookup_b[b]; \
/*				v = (r * 76 + g * 150 + b * 29) >> 8; */ \
				v = MAX(r_out, g_out); \
				v = MAX(v, b_out); \
				v += -HISTOGRAM_MIN * 0xffff / 100; \
				CLAMP(v, 0, HISTOGRAM_SLOTS - 1); \
				accum_v[v]++; \
			} \
 \
			r += -HISTOGRAM_MIN * 0xffff / 100; \
			g += -HISTOGRAM_MIN * 0xffff / 100; \
			b += -HISTOGRAM_MIN * 0xffff / 100; \
			CLAMP(r, 0, HISTOGRAM_SLOTS - 1); \
			CLAMP(g, 0, HISTOGRAM_SLOTS - 1); \
			CLAMP(b, 0, HISTOGRAM_SLOTS - 1); \
			accum_r[r]++; \
			accum_g[g]++; \
			accum_b[b]++; \
			row += components; \
		} \
	} \
}




		VFrame *data = server->data;
		int w = data->get_w();
		int h = data->get_h();
		int *accum_r = accum[HISTOGRAM_RED];
		int *accum_g = accum[HISTOGRAM_GREEN];
		int *accum_b = accum[HISTOGRAM_BLUE];
		int *accum_v = accum[HISTOGRAM_VALUE];
		int32_t r, g, b, a, y, u, v;
		int r_out, g_out, b_out;
		int *lookup_r = plugin->preview_lookup[HISTOGRAM_RED];
		int *lookup_g = plugin->preview_lookup[HISTOGRAM_GREEN];
		int *lookup_b = plugin->preview_lookup[HISTOGRAM_BLUE];

		switch(data->get_color_model())
		{
			case BC_RGB888:
				HISTOGRAM_HEAD(unsigned char)
				r = (row[0] << 8) | row[0];
				g = (row[1] << 8) | row[1];
				b = (row[2] << 8) | row[2];
				HISTOGRAM_TAIL(3)
				break;
			case BC_RGB_FLOAT:
				HISTOGRAM_HEAD(float)
				r = (int)(row[0] * 0xffff);
				g = (int)(row[1] * 0xffff);
				b = (int)(row[2] * 0xffff);
				HISTOGRAM_TAIL(3)
				break;
			case BC_YUV888:
				HISTOGRAM_HEAD(unsigned char)
				y = (row[0] << 8) | row[0];
				u = (row[1] << 8) | row[1];
				v = (row[2] << 8) | row[2];
				plugin->yuv.yuv_to_rgb_16(r, g, b, y, u, v);
				HISTOGRAM_TAIL(3)
				break;
			case BC_RGBA8888:
				HISTOGRAM_HEAD(unsigned char)
				r = (row[0] << 8) | row[0];
				g = (row[1] << 8) | row[1];
				b = (row[2] << 8) | row[2];
				HISTOGRAM_TAIL(4)
				break;
			case BC_RGBA_FLOAT:
				HISTOGRAM_HEAD(float)
				r = (int)(row[0] * 0xffff);
				g = (int)(row[1] * 0xffff);
				b = (int)(row[2] * 0xffff);
				HISTOGRAM_TAIL(4)
				break;
			case BC_YUVA8888:
				HISTOGRAM_HEAD(unsigned char)
				y = (row[0] << 8) | row[0];
				u = (row[1] << 8) | row[1];
				v = (row[2] << 8) | row[2];
				plugin->yuv.yuv_to_rgb_16(r, g, b, y, u, v);
				HISTOGRAM_TAIL(4)
				break;
		}
	}
	else
	if(server->operation == HistogramEngine::APPLY)
	{



#define PROCESS(type, components) \
{ \
	for(int i = pkg->start; i < pkg->end; i++) \
	{ \
		type *row = (type*)input->get_rows()[i]; \
		for(int j = 0; j < w; j++) \
		{ \
			if ( plugin->config.split && ((j + i * w / h) < w) ) \
		    	continue; \
			row[0] = lookup_r[row[0]]; \
			row[1] = lookup_g[row[1]]; \
			row[2] = lookup_b[row[2]]; \
			row += components; \
		} \
	} \
}

#define PROCESS_YUV(type, components, max) \
{ \
	for(int i = pkg->start; i < pkg->end; i++) \
	{ \
		type *row = (type*)input->get_rows()[i]; \
		for(int j = 0; j < w; j++) \
		{ \
			if ( plugin->config.split && ((j + i * w / h) < w) ) \
		    { \
            	continue; \
            } \
 \
/* Convert to 16 bit RGB */ \
			if(max == 0xff) \
			{ \
				y = (row[0] << 8) | row[0]; \
				u = (row[1] << 8) | row[1]; \
				v = (row[2] << 8) | row[2]; \
			} \
			else \
			{ \
				y = row[0]; \
				u = row[1]; \
				v = row[2]; \
			} \
 \
			plugin->yuv.yuv_to_rgb_16(r, g, b, y, u, v); \
 \
/* Look up in RGB domain */ \
			r = lookup_r[r]; \
			g = lookup_g[g]; \
			b = lookup_b[b]; \
 \
/* Convert to 16 bit YUV */ \
			plugin->yuv.rgb_to_yuv_16(r, g, b, y, u, v); \
 \
			if(max == 0xff) \
			{ \
				row[0] = y >> 8; \
				row[1] = u >> 8; \
				row[2] = v >> 8; \
			} \
			else \
			{ \
				row[0] = y; \
				row[1] = u; \
				row[2] = v; \
			} \
			row += components; \
		} \
	} \
}

#define PROCESS_FLOAT(components) \
{ \
	for(int i = pkg->start; i < pkg->end; i++) \
	{ \
		float *row = (float*)input->get_rows()[i]; \
		for(int j = 0; j < w; j++) \
		{ \
			if ( plugin->config.split && ((j + i * w / h) < w) ) \
		    	continue; \
			float r = row[0]; \
			float g = row[1]; \
			float b = row[2]; \
 \
			r = plugin->calculate_level(r, HISTOGRAM_RED, 1); \
			g = plugin->calculate_level(g, HISTOGRAM_GREEN, 1); \
			b = plugin->calculate_level(b, HISTOGRAM_BLUE, 1); \
 \
 			row[0] = r; \
			row[1] = g; \
			row[2] = b; \
 \
			row += components; \
		} \
	} \
}


		VFrame *input = plugin->input;
		VFrame *output = plugin->output;
		int w = input->get_w();
		int h = input->get_h();
		int *lookup_r = plugin->lookup[0];
		int *lookup_g = plugin->lookup[1];
		int *lookup_b = plugin->lookup[2];
        if(input->get_color_model() == BC_YUV888 ||
            input->get_color_model() == BC_YUVA8888)
        {
		    lookup_r = plugin->lookup16[0];
		    lookup_g = plugin->lookup16[1];
		    lookup_b = plugin->lookup16[2];
        }
		int r, g, b, y, u, v, a;
		switch(input->get_color_model())
		{
			case BC_RGB888:
				PROCESS(unsigned char, 3)
				break;
			case BC_RGB_FLOAT:
				PROCESS_FLOAT(3);
				break;
			case BC_RGBA8888:
				PROCESS(unsigned char, 4)
				break;
			case BC_RGBA_FLOAT:
				PROCESS_FLOAT(4);
				break;
			case BC_YUV888:
				PROCESS_YUV(unsigned char, 3, 0xff)
				break;
			case BC_YUVA8888:
				PROCESS_YUV(unsigned char, 4, 0xff)
				break;
		}
	}
}






HistogramEngine::HistogramEngine(HistogramMain *plugin, 
	int total_clients, 
	int total_packages)
 : LoadServer(total_clients, total_packages)
// : LoadServer(1, 1)
{
	this->plugin = plugin;
}

void HistogramEngine::init_packages()
{
	switch(operation)
	{
		case HISTOGRAM:
			total_size = data->get_h();
			break;
		case APPLY:
			total_size = data->get_h();
			break;
	}


	int package_size = (int)((float)total_size / 
			get_total_packages() + 1);
	int start = 0;

// printf("HistogramEngine::HistogramEngine %d %d %d\n", 
// __LINE__,
// get_total_packages(),
// get_total_clients());

	for(int i = 0; i < get_total_packages(); i++)
	{
		HistogramPackage *package = (HistogramPackage*)get_package(i);
		package->start = total_size * i / get_total_packages();
		package->end = total_size * (i + 1) / get_total_packages();
	}

// Initialize clients here in case some don't get run.
	for(int i = 0; i < get_total_clients(); i++)
	{
		HistogramUnit *unit = (HistogramUnit*)get_client(i);
		for(int i = 0; i < HISTOGRAM_MODES; i++)
			bzero(unit->accum[i], sizeof(int) * HISTOGRAM_SLOTS);
	}

}

LoadClient* HistogramEngine::new_client()
{
	return new HistogramUnit(this, plugin);
}

LoadPackage* HistogramEngine::new_package()
{
	return new HistogramPackage;
}

void HistogramEngine::process_packages(int operation, VFrame *data, int do_value)
{
	this->data = data;
	this->operation = operation;
	this->do_value = do_value;
	LoadServer::process_packages();
}



