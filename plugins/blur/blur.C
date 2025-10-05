/*
 * CINELERRA
 * Copyright (C) 2010-2024 Adam Williams <broadcast at earthling dot net>
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

#include "filexml.h"
#include "blur.h"
#include "blurwindow.h"
#include "bchash.h"
#include "clip.h"
#include "keyframe.h"
#include "language.h"
#include "picon_png.h"
#include "vframe.h"

#include <math.h>
#include <stdint.h>
#include <string.h>


BlurConstants::BlurConstants()
{
    curve2 = 0;
    sum2 = 0;
}

BlurConstants::~BlurConstants()
{
    delete [] curve2;
    delete [] sum2;
}




BlurConfig::BlurConfig()
{
	vertical = MIN_RADIUS;
	horizontal = MIN_RADIUS;
//	radius = 5;
	a_key = 0;
	a = r = g = b = 1;
}

int BlurConfig::equivalent(BlurConfig &that)
{
	return (vertical == that.vertical && 
		horizontal == that.horizontal && 
//		radius == that.radius &&
		a_key == that.a_key &&
		a == that.a &&
		r == that.r &&
		g == that.g &&
		b == that.b);
}

void BlurConfig::copy_from(BlurConfig &that)
{
	vertical = that.vertical;
	horizontal = that.horizontal;
//	radius = that.radius;
	a_key = that.a_key;
	a = that.a;
	r = that.r;
	g = that.g;
	b = that.b;
}

void BlurConfig::interpolate(BlurConfig &prev, 
	BlurConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);


//printf("BlurConfig::interpolate %d %d %d\n", prev_frame, next_frame, current_frame);
	this->vertical = (float)(prev.vertical * prev_scale + next.vertical * next_scale);
	this->horizontal = (float)(prev.horizontal * prev_scale + next.horizontal * next_scale);
//	this->radius = (float)(prev.radius * prev_scale + next.radius * next_scale);
	a_key = prev.a_key;
	a = prev.a;
	r = prev.r;
	g = prev.g;
	b = prev.b;
}






REGISTER_PLUGIN(BlurMain)








BlurMain::BlurMain(PluginServer *server)
 : PluginVClient(server)
{
	need_reconfigure = 1;
	engine = 0;
    lock = 1;
    cpus = 0;
}

BlurMain::~BlurMain()
{
//printf("BlurMain::~BlurMain 1\n");

	if(engine)
	{
		for(int i = 0; i < cpus; i++)
			delete engine[i];
		delete [] engine;
	}
}

const char* BlurMain::plugin_title() { return N_("Blur"); }
int BlurMain::is_realtime() { return 1; }


NEW_WINDOW_MACRO(BlurMain, BlurWindow)
NEW_PICON_MACRO(BlurMain)

LOAD_CONFIGURATION_MACRO(BlurMain, BlurConfig)




int BlurMain::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	int i, j, k, l;
	unsigned char **input_rows, **output_rows;
    cpus = get_project_smp() + 1;
// DEBUG
//    cpus = 1;

	need_reconfigure |= load_configuration();

	read_frame(frame, 
		0, 
		start_position, 
		frame_rate,
		0);

// printf("BlurMain::process_realtime %d horizontal=%f vertical=%f a_key=%d\n", 
// __LINE__, config.horizontal, config.vertical, config.a_key);

// always use RLE for these modes
    if(config.horizontal < IIR_RADIUS ||
        config.vertical < IIR_RADIUS ||
        config.a_key)
        use_rle = 1;
    else
        use_rle = 0;

// DEBUG
//    use_rle = 1;
//printf("BlurMain::process_realtime %d use_rle=%d\n", __LINE__, use_rle);

	if(!engine)
	{
		engine = new BlurEngine*[cpus];
		for(int i = 0; i < cpus; i++)
		{
			engine[i] = new BlurEngine(this);
			engine[i]->start();
		}
	}

// do nothing
	if(config.horizontal < MIN_RADIUS &&
		config.vertical < MIN_RADIUS)
	{
// Data never processed
// discard alpha for consistent results
        if(config.a_key)
        {
    		int w = frame->get_w();
	    	int h = frame->get_h();
#define DISCARD_ALPHA(type, max) \
{ \
	type **rows = (type**)frame->get_rows(); \
    for(int i = 0; i < h; i++) \
    { \
        type *row = rows[i]; \
        for(int j = 0; j < w; j++) \
        { \
            row[3] = max; \
            row += 4; \
        } \
    } \
}

	        switch(frame->get_color_model())
	        {
		        case BC_YUVA8888:
		        case BC_RGBA8888:
			        DISCARD_ALPHA(unsigned char, 0xff);
			        break;
		        case BC_RGBA_FLOAT:
			        DISCARD_ALPHA(float, 1.0);
			        break;
            }
        }
	}
	else
	{

		PluginVClient::new_temp(frame->get_w(),
				frame->get_h(),
				frame->get_color_model());
		input_frame = frame;


// set up constants
// Process blur
	    for(i = 0; i < cpus; i++)
	    {
		    engine[i]->reconfigure(&engine[i]->constants_h, 
                config.horizontal);
		    engine[i]->reconfigure(&engine[i]->constants_v,
                config.vertical);
	    }

// Need to blur vertically to a temp and 
// horizontally to the output in 2 discrete passes.
		for(i = 0; i < cpus; i++)
		{
			engine[i]->set_range(
				input_frame->get_h() * i / cpus, 
				input_frame->get_h() * (i + 1) / cpus,
				input_frame->get_w() * i / cpus,
				input_frame->get_w() * (i + 1) / cpus);
		}

		for(i = 0; i < cpus; i++)
		{
			engine[i]->do_horizontal = 0;
			engine[i]->start_process_frame(input_frame);
		}

		for(i = 0; i < cpus; i++)
		{
			engine[i]->wait_process_frame();
		}

		for(i = 0; i < cpus; i++)
		{
			engine[i]->do_horizontal = 1;
			engine[i]->start_process_frame(input_frame);
		}

		for(i = 0; i < cpus; i++)
		{
			engine[i]->wait_process_frame();
		}
	}


	return 0;
}


void BlurMain::update_gui()
{
	if(thread)
	{
		int reconfigure = load_configuration();
		if(reconfigure) 
		{
			((BlurWindow*)thread->window)->lock_window("BlurMain::update_gui");
			((BlurWindow*)thread->window)->h->update(config.horizontal);
			((BlurWindow*)thread->window)->v->update(config.vertical);
			((BlurWindow*)thread->window)->h_text->update(config.horizontal);
			((BlurWindow*)thread->window)->v_text->update(config.vertical);
			((BlurWindow*)thread->window)->a_key->update(config.a_key);
			((BlurWindow*)thread->window)->a->update(config.a);
			((BlurWindow*)thread->window)->r->update(config.r);
			((BlurWindow*)thread->window)->g->update(config.g);
			((BlurWindow*)thread->window)->b->update(config.b);
			((BlurWindow*)thread->window)->unlock_window();
		}
	}
}





void BlurMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->get_data());
	output.tag.set_title("BLUR");
	output.tag.set_property("VERTICAL", config.vertical);
	output.tag.set_property("HORIZONTAL", config.horizontal);
//	output.tag.set_property("RADIUS", config.radius);
	output.tag.set_property("R", config.r);
	output.tag.set_property("G", config.g);
	output.tag.set_property("B", config.b);
	output.tag.set_property("A", config.a);
	output.tag.set_property("A_KEY", config.a_key);
	output.append_tag();
	output.terminate_string();
}

void BlurMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data());

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("BLUR"))
			{
                int radius = input.tag.get_property("RADIUS", 1);
                if(input.tag.has_property("VERTICAL"))
    				config.vertical = input.tag.get_property("VERTICAL", config.vertical);
				else
                    config.vertical = radius;
                if(input.tag.has_property("HORIZONTAL"))
                    config.horizontal = input.tag.get_property("HORIZONTAL", config.horizontal);
                else
                    config.horizontal = radius;
//				config.radius = input.tag.get_property("RADIUS", config.radius);
//printf("BlurMain::read_data 1 %d %d %s\n", get_source_position(), keyframe->position, keyframe->get_data());
				config.r = input.tag.get_property("R", config.r);
				config.g = input.tag.get_property("G", config.g);
				config.b = input.tag.get_property("B", config.b);
				config.a = input.tag.get_property("A", config.a);
				config.a_key = input.tag.get_property("A_KEY", config.a_key);
			}
		}
	}
}









BlurEngine::BlurEngine(BlurMain *plugin)
 : Thread(1, 0, 0)
{
	this->plugin = plugin;
	last_frame = 0;

// Strip size
	size = MAX(plugin->get_input()->get_w(), plugin->get_input()->get_h());
// IIR arrays
	val_p = new pixel_f[size];
	val_m = new pixel_f[size];
	radius = new float[size];
	src = new pixel_f[size];
	dst = new pixel_f[size];
// maximum possible RLE length from make_rle_curve
    float max_radius = MAX_RADIUS + 1.0;
    float sigma = sqrt (-(max_radius * max_radius) / (2 * log (1.0 / 255.0)));
    float sigma2 = 2 * sigma * sigma;
    float l      = sqrt (-sigma2 * log (1.0 / 255.0));
    int n = ceil (l) * 2;
    if ((n % 2) == 0)
        n += 1;
    int max_length = n / 2;
// RLE arrays
    rle2 = new pixel_f[size + 2 * max_length];
    pix2 = new pixel_f[size + 2 * max_length];
    rle = rle2 + max_length;
    pix = pix2 + max_length;


	set_synchronous(1);
	input_lock.lock();
	output_lock.lock();
}

BlurEngine::~BlurEngine()
{
	last_frame = 1;
	input_lock.unlock();
	join();
	delete [] val_p;
	delete [] val_m;
	delete [] src;
	delete [] dst;
	delete [] radius;
    delete [] rle2;
    delete [] pix2;
}

void BlurEngine::set_range(int start_y, 
	int end_y,
	int start_x,
	int end_x)
{
	this->start_y = start_y;
	this->end_y = end_y;
	this->start_x = start_x;
	this->end_x = end_x;
}

int BlurEngine::start_process_frame(VFrame *frame)
{
	this->frame = frame;
	input_lock.unlock();
	return 0;
}

int BlurEngine::wait_process_frame()
{
	output_lock.lock();
	return 0;
}

void BlurEngine::run()
{
	int i, j, k;


	while(1)
	{
		input_lock.lock();
		if(last_frame)
		{
			output_lock.unlock();
			return;
		}





		color_model = frame->get_color_model();
		int w = frame->get_w();
		int h = frame->get_h();
// Force recalculation of filter
		prev_radius = -1;




#define BLUR(type, max, components) \
{ \
	type **input_rows = (type **)frame->get_rows(); \
	type **output_rows = (type **)frame->get_rows(); \
	type **current_input = input_rows; \
	type **current_output = output_rows; \
	vmax = max; \
 \
	if(!do_horizontal && plugin->config.vertical) \
	{ \
/* Vertical pass */ \
/* Render to temp if a horizontal pass comes next */ \
/*		if(plugin->config.horizontal) */ \
/*		{ */ \
/*			current_output = (type **)plugin->get_temp()->get_rows(); */ \
/*		} */ \
 \
		for(j = start_x; j < end_x; j++) \
		{ \
			bzero(val_p, sizeof(pixel_f) * h); \
			bzero(val_m, sizeof(pixel_f) * h); \
 \
			for(k = 0; k < h; k++) \
			{ \
				if(plugin->config.r) src[k].r = (float)current_input[k][j * components]; \
				if(plugin->config.g) src[k].g = (float)current_input[k][j * components + 1]; \
				if(plugin->config.b) src[k].b = (float)current_input[k][j * components + 2]; \
				if(components == 4) \
				{ \
					if(plugin->config.a || plugin->config.a_key) src[k].a = (float)current_input[k][j * components + 3]; \
				} \
			} \
 \
			if(components == 4) \
				blur_strip4(h, plugin->config.vertical, &constants_v); \
			else \
				blur_strip3(h, plugin->config.vertical, &constants_v); \
 \
			for(k = 0; k < h; k++) \
			{ \
				if(plugin->config.r) current_output[k][j * components] = (type)dst[k].r; \
				if(plugin->config.g) current_output[k][j * components + 1] = (type)dst[k].g; \
				if(plugin->config.b) current_output[k][j * components + 2] = (type)dst[k].b; \
				if(components == 4) \
				{ \
/* don't keep the alpha */ \
				    if(plugin->config.a_key && !plugin->config.horizontal) \
					    current_output[k][j * components + 3] = max; \
                	else \
                    if(plugin->config.a || plugin->config.a_key) \
                        current_output[k][j * components + 3] = (type)dst[k].a; \
                } \
			} \
		} \
 \
/*		current_input = current_output; */ \
/*		current_output = output_rows; */ \
	} \
 \
 \
	if(do_horizontal && plugin->config.horizontal) \
	{ \
/* Vertical pass */ \
/*		if(plugin->config.vertical) */ \
/*		{ */ \
/*			current_input = (type **)plugin->get_temp()->get_rows(); */ \
/*		} */ \
 \
/* Horizontal pass */ \
		for(j = start_y; j < end_y; j++) \
		{ \
			bzero(val_p, sizeof(pixel_f) * w); \
			bzero(val_m, sizeof(pixel_f) * w); \
 \
			for(k = 0; k < w; k++) \
			{ \
				if(plugin->config.r) src[k].r = (float)current_input[j][k * components]; \
				if(plugin->config.g) src[k].g = (float)current_input[j][k * components + 1]; \
				if(plugin->config.b) src[k].b = (float)current_input[j][k * components + 2]; \
				if(components == 4) \
					if(plugin->config.a || plugin->config.a_key) src[k].a = (float)current_input[j][k * components + 3]; \
			} \
 \
 			if(components == 4) \
				blur_strip4(w, plugin->config.horizontal, &constants_h); \
			else \
				blur_strip3(w, plugin->config.horizontal, &constants_h); \
 \
			for(k = 0; k < w; k++) \
			{ \
				if(plugin->config.r) current_output[j][k * components] = (type)dst[k].r; \
				if(plugin->config.g) current_output[j][k * components + 1] = (type)dst[k].g; \
				if(plugin->config.b) current_output[j][k * components + 2] = (type)dst[k].b; \
				if(components == 4) \
                { \
/* don't keep the alpha */ \
					if(plugin->config.a_key) \
						current_output[j][k * components + 3] = max; \
					else \
                    if(plugin->config.a) \
						current_output[j][k * components + 3] = (type)dst[k].a; \
                } \
			} \
		} \
	} \
}



		switch(color_model)
		{
			case BC_RGB888:
			case BC_YUV888:
				BLUR(unsigned char, 0xff, 3);
				break;

			case BC_RGB_FLOAT:
				BLUR(float, 1.0, 3);
				break;

			case BC_RGBA8888:
			case BC_YUVA8888:
				BLUR(unsigned char, 0xff, 4);
				break;

			case BC_RGBA_FLOAT:
				BLUR(float, 1.0, 4);
				break;
		}

		output_lock.unlock();
	}
}

int BlurEngine::reconfigure(BlurConstants *constants, float radius)
{
// from blur-gauss.c: gauss_iir
    radius = fabs(radius) + 1.0;


	float std_dev = sqrt(-(float)(radius * radius) / 
		(2 * log (1.0 / 255.0)));


    if(!plugin->use_rle)
    {
// from blur-gauss.c: gauss_iir
        get_iir_constants(constants, std_dev);
    }
    else
    {
// from blur-gauss.c: gauss_rle
        make_rle_curve(constants, std_dev);
    }
    return 0;
}

void BlurEngine::make_rle_curve(BlurConstants *ptr, float sigma)
{
    delete [] ptr->curve2;
    delete [] ptr->sum2;
    
    double sigma2 = 2 * sigma * sigma;
    double l      = sqrt (-sigma2 * log (1.0 / 255.0));
    int n = ceil (l) * 2;
    if ((n % 2) == 0)
        n += 1;
    
    ptr->curve2 = new float[n];
    ptr->length = n / 2;
    ptr->curve = ptr->curve2 + ptr->length;
    ptr->curve[0] = 1.0;

    for(int i = 1; i <= ptr->length; i++)
    {
        float temp = exp (- (i * i) / sigma2);
        ptr->curve[-i] = temp;
        ptr->curve[i] = temp;
    }
    
    ptr->sum2 = new float[ptr->length * 2 + 1];
    ptr->sum2[0] = 0;
    for (int i = 1; i <= ptr->length*2; i++)
    {
        ptr->sum2[i] = ptr->curve[i-ptr->length-1] + ptr->sum2[i-1];
    }
    
    ptr->sum = ptr->sum2 + ptr->length; /* 'center' the sum[] */
    ptr->total = ptr->sum[ptr->length] - ptr->sum[-ptr->length];
}


// from blur-gauss.c: find_iir_constants
int BlurEngine::get_iir_constants(BlurConstants *ptr, float std_dev)
{
	int i;

	float div = sqrt(2 * M_PI) * std_dev;
	float x0 = -1.783 / std_dev;
	float x1 = -1.723 / std_dev;
	float x2 = 0.6318 / std_dev;
	float x3 = 1.997  / std_dev;
	float x4 = 1.6803 / div;
	float x5 = 3.735 / div;
	float x6 = -0.6803 / div;
	float x7 = -0.2598 / div;

	ptr->n_p[0] = x4 + x6;
	ptr->n_p[1] = exp(x1) *
				(x7 * sin(x3) -
				(x6 + 2 * x4) * cos(x3)) +
				exp(x0) *
				(x5 * sin(x2) -
				(2 * x6 + x4) * cos(x2));

	ptr->n_p[2] = 2 * exp(x0 + x1) *
				((x4 + x6) * cos(x3) * 
				cos(x2) - x5 * 
				cos(x3) * sin(x2) -
				x7 * cos(x2) * sin(x3)) +
				x6 * exp(2 * x0) +
				x4 * exp(2 * x1);

	ptr->n_p[3] = exp(x1 + 2 * x0) *
				(x7 * sin(x3) - 
				x6 * cos(x3)) +
				exp(x0 + 2 * x1) *
				(x5 * sin(x2) - x4 * 
				cos(x2));
	ptr->n_p[4] = 0.0;

	ptr->d_p[0] = 0.0;
	ptr->d_p[1] = -2 * exp(x1) * cos(x3) -
				2 * exp(x0) * cos(x2);

	ptr->d_p[2] = 4 * cos(x3) * cos(x2) * 
				exp(x0 + x1) +
				exp(2 * x1) + exp (2 * x0);

	ptr->d_p[3] = -2 * cos(x2) * exp(x0 + 2 * x1) -
				2 * cos(x3) * exp(x1 + 2 * x0);

	ptr->d_p[4] = exp(2 * x0 + 2 * x1);

	for(i = 0; i < 5; i++) ptr->d_m[i] = ptr->d_p[i];

	ptr->n_m[0] = 0.0;
	for(i = 1; i <= 4; i++)
		ptr->n_m[i] = ptr->n_p[i] - ptr->d_p[i] * ptr->n_p[0];

	float sum_n_p, sum_n_m, sum_d;
	float a, b;

	sum_n_p = 0.0;
	sum_n_m = 0.0;
	sum_d = 0.0;
	for(i = 0; i < 5; i++)
	{
		sum_n_p += ptr->n_p[i];
		sum_n_m += ptr->n_m[i];
		sum_d += ptr->d_p[i];
	}

	a = sum_n_p / (1.0 + sum_d);
	b = sum_n_m / (1.0 + sum_d);

	for (i = 0; i < 5; i++)
	{
		ptr->bd_p[i] = ptr->d_p[i] * a;
		ptr->bd_m[i] = ptr->d_m[i] * b;
	}

	return 0;
}

#define BOUNDARY(x) if((x) > vmax) (x) = vmax; else if((x) < 0) (x) = 0;




int BlurEngine::transfer_pixels(pixel_f *src1, 
	pixel_f *src2, 
	pixel_f *src,
//	float *radius,
	pixel_f *dest, 
	int size)
{
	int i;
	float sum;

// printf("BlurEngine::transfer_pixels %d %d %d %d\n", 
// plugin->config.r, 
// plugin->config.g, 
// plugin->config.b, 
// plugin->config.a);

	for(i = 0; i < size; i++)
    {
		sum = src1[i].r + src2[i].r;
		BOUNDARY(sum);
		dest[i].r = sum;

		sum = src1[i].g + src2[i].g;
		BOUNDARY(sum);
		dest[i].g = sum;

		sum = src1[i].b + src2[i].b;
		BOUNDARY(sum);
		dest[i].b = sum;

		sum = src1[i].a + src2[i].a;
		BOUNDARY(sum);
		dest[i].a = sum;

// 		if(radius[i] < 2)
// 		{
// 			float scale = 2.0 - (radius[i] * radius[i] - 2.0);
// 			dest[i].r /= scale;
// 			dest[i].g /= scale;
// 			dest[i].b /= scale;
// 			dest[i].a /= scale;
// 		}
    }
	return 0;
}


int BlurEngine::blur_strip3(int size, 
    float radius2, 
    BlurConstants *constants)
{

// Gimp uses run_length_encode+do_encoded_lre for repetitive data but it
// only works for int values.  It uses do_full_lre for unique data.
// For floating point, treat every pixel as unique.
    if(plugin->use_rle)
    {
        float *curve = constants->curve;
        float ctotal = constants->total;
        int length = constants->length;
// printf("BlurEngine::blur_strip3 %d size=%d radius2=%f length=%d total=%f\n", 
// __LINE__, size, radius2, length, constants->total);
        for(int col = 0; col < size; col++)
        {
            float val_r = ctotal / 2;
            float val_g = ctotal / 2;
            float val_b = ctotal / 2;
            float *c = &curve[0];
            pixel_f *x1;
            pixel_f *x2;
            x1 = x2 = &src[col];
            
            if(plugin->config.r) val_r = x1->r * c[0];
            if(plugin->config.g) val_g = x1->g * c[0];
            if(plugin->config.b) val_b = x1->b * c[0];
            
            c++;
            if(x1 < src + size - 1) x1++;
            if(x2 > src) x2--;
            for(int i = length; i >= 1; i--)
            {
                if(plugin->config.r) val_r += (x1->r + x2->r) * c[0];
                if(plugin->config.g) val_g += (x1->g + x2->g) * c[0];
                if(plugin->config.b) val_b += (x1->b + x2->b) * c[0];
                c++;
                if(x1 < src + size - 1) x1++;
                if(x2 > src) x2--;
            }

            if(plugin->config.r) val_r /= ctotal;
            if(plugin->config.g) val_g /= ctotal;
            if(plugin->config.b) val_b /= ctotal;
            if(plugin->config.r) dst[col].r = MIN(val_r, vmax);
            if(plugin->config.g) dst[col].g = MIN(val_g, vmax);
            if(plugin->config.b) dst[col].b = MIN(val_b, vmax);
        }
    }
    else
    {

	    pixel_f *sp_p = src;
	    pixel_f *sp_m = src + size - 1;
	    pixel_f *vp = val_p;
	    pixel_f *vm = val_m + size - 1;

	    initial_p = sp_p[0];
	    initial_m = sp_m[0];

	    int l;
	    for(int k = 0; k < size; k++)
	    {
		    terms = (k < 4) ? k : 4;

		    radius[k] = radius2;

		    for(l = 0; l <= terms; l++)
		    {
			    if(plugin->config.r)
			    {
				    vp->r += constants->n_p[l] * sp_p[-l].r - constants->d_p[l] * vp[-l].r;
				    vm->r += constants->n_m[l] * sp_m[l].r - constants->d_m[l] * vm[l].r;
			    }
			    if(plugin->config.g)
			    {
				    vp->g += constants->n_p[l] * sp_p[-l].g - constants->d_p[l] * vp[-l].g;
				    vm->g += constants->n_m[l] * sp_m[l].g - constants->d_m[l] * vm[l].g;
			    }
			    if(plugin->config.b)
			    {
				    vp->b += constants->n_p[l] * sp_p[-l].b - constants->d_p[l] * vp[-l].b;
				    vm->b += constants->n_m[l] * sp_m[l].b - constants->d_m[l] * vm[l].b;
			    }
		    }

		    for( ; l <= 4; l++)
		    {
			    if(plugin->config.r)
			    {
				    vp->r += (constants->n_p[l] - constants->bd_p[l]) * initial_p.r;
				    vm->r += (constants->n_m[l] - constants->bd_m[l]) * initial_m.r;
			    }
			    if(plugin->config.g)
			    {
				    vp->g += (constants->n_p[l] - constants->bd_p[l]) * initial_p.g;
				    vm->g += (constants->n_m[l] - constants->bd_m[l]) * initial_m.g;
			    }
			    if(plugin->config.b)
			    {
				    vp->b += (constants->n_p[l] - constants->bd_p[l]) * initial_p.b;
				    vm->b += (constants->n_m[l] - constants->bd_m[l]) * initial_m.b;
			    }
		    }
		    sp_p++;
		    sp_m--;
		    vp++;
		    vm--;
	    }

	    transfer_pixels(val_p, val_m, src, dst, size);
    }
	return 0;
}


int BlurEngine::blur_strip4(int size, 
    float radius2, 
    BlurConstants *constants)
{

    if(plugin->use_rle)
    {
// set the radius based on each pixel's alpha
        if(plugin->config.a_key)
	    {
	        for(int k = 0; k < size; k++)
	        {
		        if(plugin->config.a_key)
			        radius[k] = radius2 * src[k].a / vmax;
		        else
			        radius[k] = radius2;
	        }
        }

        float *curve = constants->curve;
        float ctotal = constants->total;
        int length = constants->length;
        for(int col = 0; col < size; col++)
        {
            int skip = 0;
		    if(plugin->config.a_key)
		    {
			    if(radius[col] != prev_radius)
			    {
				    if(radius[col] >= MIN_RADIUS)
				    {
				        prev_radius = radius[col];
					    reconfigure(constants, radius[col]);
                        curve = constants->curve;
                        ctotal = constants->total;
                        length = constants->length;
				    }
				    else
				    {
                        skip = 1;
				    }
			    }

// Copy alpha
			    dst[col].a = src[col].a;
		    }

            if(skip)
            {
                dst[col].r = src[col].r;
                dst[col].g = src[col].g;
                dst[col].b = src[col].b;
            }
            else
            {
                float val_r = ctotal / 2;
                float val_g = ctotal / 2;
                float val_b = ctotal / 2;
                float val_a = ctotal / 2;
                float *c = &curve[0];
                pixel_f *x1;
                pixel_f *x2;
                x1 = x2 = &src[col];

                if(plugin->config.r) val_r = x1->r * c[0];
                if(plugin->config.g) val_g = x1->g * c[0];
                if(plugin->config.b) val_b = x1->b * c[0];
                if(plugin->config.a && !plugin->config.a_key) val_a = x1->a * c[0];

                c++;
                if(x1 < src + size - 1) x1++;
                if(x2 > src) x2--;
                for(int i = length; i >= 1; i--)
                {
                    if(plugin->config.r) val_r += (x1->r + x2->r) * c[0];
                    if(plugin->config.g) val_g += (x1->g + x2->g) * c[0];
                    if(plugin->config.b) val_b += (x1->b + x2->b) * c[0];
                    if(plugin->config.a && !plugin->config.a_key) val_a += (x1->a + x2->a) * c[0];
                    c++;
                    if(x1 < src + size - 1) x1++;
                    if(x2 > src) x2--;
                }

                if(plugin->config.r) val_r /= ctotal;
                if(plugin->config.g) val_g /= ctotal;
                if(plugin->config.b) val_b /= ctotal;
                if(plugin->config.a && !plugin->config.a_key) val_a /= ctotal;
                if(plugin->config.r) dst[col].r = MIN(val_r, vmax);
                if(plugin->config.g) dst[col].g = MIN(val_g, vmax);
                if(plugin->config.b) dst[col].b = MIN(val_b, vmax);
                if(plugin->config.a && !plugin->config.a_key) dst[col].a = MIN(val_a, vmax);
            }
        }
    }
    else
    {
// IIR
	    int l;
	    pixel_f *sp_p = src;
	    pixel_f *sp_m = src + size - 1;
	    pixel_f *vp = val_p;
	    pixel_f *vm = val_m + size - 1;

	    initial_p = sp_p[0];
	    initial_m = sp_m[0];

	    for(int k = 0; k < size; k++)
	    {
		    terms = (k < 4) ? k : 4;

		    for(l = 0; l <= terms; l++)
		    {
			    if(plugin->config.r)
			    {
				    vp->r += constants->n_p[l] * sp_p[-l].r - constants->d_p[l] * vp[-l].r;
				    vm->r += constants->n_m[l] * sp_m[l].r - constants->d_m[l] * vm[l].r;
			    }
			    if(plugin->config.g)
			    {
				    vp->g += constants->n_p[l] * sp_p[-l].g - constants->d_p[l] * vp[-l].g;
				    vm->g += constants->n_m[l] * sp_m[l].g - constants->d_m[l] * vm[l].g;
			    }
			    if(plugin->config.b)
			    {
				    vp->b += constants->n_p[l] * sp_p[-l].b - constants->d_p[l] * vp[-l].b;
				    vm->b += constants->n_m[l] * sp_m[l].b - constants->d_m[l] * vm[l].b;
			    }
			    if(plugin->config.a)
			    {
				    vp->a += constants->n_p[l] * sp_p[-l].a - constants->d_p[l] * vp[-l].a;
				    vm->a += constants->n_m[l] * sp_m[l].a - constants->d_m[l] * vm[l].a;
			    }
		    }

		    for( ; l <= 4; l++)
		    {
			    if(plugin->config.r)
			    {
				    vp->r += (constants->n_p[l] - constants->bd_p[l]) * initial_p.r;
				    vm->r += (constants->n_m[l] - constants->bd_m[l]) * initial_m.r;
			    }
			    if(plugin->config.g)
			    {
				    vp->g += (constants->n_p[l] - constants->bd_p[l]) * initial_p.g;
				    vm->g += (constants->n_m[l] - constants->bd_m[l]) * initial_m.g;
			    }
			    if(plugin->config.b)
			    {
				    vp->b += (constants->n_p[l] - constants->bd_p[l]) * initial_p.b;
				    vm->b += (constants->n_m[l] - constants->bd_m[l]) * initial_m.b;
			    }
			    if(plugin->config.a)
			    {
				    vp->a += (constants->n_p[l] - constants->bd_p[l]) * initial_p.a;
				    vm->a += (constants->n_m[l] - constants->bd_m[l]) * initial_m.a;
			    }
		    }

		    sp_p++;
		    sp_m--;
		    vp++;
		    vm--;
	    }

	    transfer_pixels(val_p, val_m, src, dst, size);
    }
	return 0;
}






