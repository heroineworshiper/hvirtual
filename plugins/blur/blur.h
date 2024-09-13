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

#ifndef BLUR_H
#define BLUR_H

class BlurMain;
class BlurEngine;

#define MAXRADIUS 100

#include "blurwindow.inc"
#include "bchash.inc"
#include "mutex.h"
#include "overlayframe.h"
#include "pluginvclient.h"
#include "thread.h"
#include "vframe.inc"

typedef struct
{
	float r;
    float g;
    float b;
    float a;
} pixel_f;

class BlurConfig
{
public:
	BlurConfig();

	int equivalent(BlurConfig &that);
	void copy_from(BlurConfig &that);
	void interpolate(BlurConfig &prev, 
		BlurConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);

	float vertical;
	float horizontal;
//	int radius;
	int a, r ,g ,b;
// fake tilt shift
	int a_key;
};

class BlurMain : public PluginVClient
{
public:
	BlurMain(PluginServer *server);
	~BlurMain();

// required for all realtime plugins
	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();

	PLUGIN_CLASS_MEMBERS(BlurConfig)

	int need_reconfigure;

private:
	BlurEngine **engine;
	OverlayFrame *overlayer;
	VFrame *input_frame;
};


class BlurConstants
{
public:
    float n_p[5], n_m[5];
    float d_p[5], d_m[5];
    float bd_p[5], bd_m[5];
};

class BlurEngine : public Thread
{
public:
	BlurEngine(BlurMain *plugin);
	~BlurEngine();

	void run();
	void set_range(int start_y, 
		int end_y,
		int start_x,
		int end_x);
	int start_process_frame(VFrame *frame);
	int wait_process_frame();

// parameters needed for blur
	int get_constants(BlurConstants *ptr, 
        float std_dev);
	int reconfigure(BlurConstants *constants, float radius);
	int transfer_pixels(pixel_f *src1, 
		pixel_f *src2, 
		pixel_f *src, 
//		float *radius,
		pixel_f *dest, 
		int size);
	int multiply_alpha(pixel_f *row, int size);
	int separate_alpha(pixel_f *row, int size);
	int blur_strip3(int size, float radius, BlurConstants *reverse, BlurConstants *forward);
	int blur_strip4(int size, float radius, BlurConstants *reverse, BlurConstants *forward);

	int color_model;
	float vmax;
	pixel_f *val_p, *val_m;
    float *radius;
	BlurConstants forward_constants_h;
	BlurConstants reverse_constants_h;
	BlurConstants forward_constants_v;
	BlurConstants reverse_constants_v;
	pixel_f *src, *dst;
    pixel_f initial_p;
    pixel_f initial_m;
	int terms;
	BlurMain *plugin;
// A margin is introduced between the input and output to give a seemless transition between blurs
	int start_y, start_x;
	int end_y, end_x;
	int last_frame;
	int do_horizontal;
// Detect changes in alpha for alpha radius mode
	float prev_forward_radius, prev_reverse_radius;
	Mutex input_lock, output_lock;
	VFrame *frame;
};

#endif
