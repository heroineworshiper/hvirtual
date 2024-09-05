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

#ifndef CHROMAKEY_H
#define CHROMAKEY_H




#include "colorpicker.h"
#include "guicast.h"
#include "loadbalance.h"
#include "pluginvclient.h"


class ChromaKeyHSV;
class ChromaKeyHSV;
class ChromaKeyWindow;

enum {
	CHROMAKEY_POSTPROCESS_NONE,
	CHROMAKEY_POSTPROCESS_BLUR,
	CHROMAKEY_POSTPROCESS_DILATE
};

class ChromaKeyConfig
{
public:
	ChromaKeyConfig();

	void copy_from(ChromaKeyConfig &src);
	int equivalent(ChromaKeyConfig &src);
	void interpolate(ChromaKeyConfig &prev, 
		ChromaKeyConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);
	int get_color();

	// Output mode
	bool  show_mask;
	// Key color definition
	float red;
	float green;
	float blue;
	// Key shade definition
	float min_brightness;
	float max_brightness;
// distance of wedge point from the center
	float saturation_start;
// minimum saturation after wedge point
	float saturation_line;
// hue range
	float tolerance;
	// Mask feathering
	float in_slope;
	float out_slope;
	float alpha_offset;
	// Spill light compensation
	float spill_saturation;
	float spill_angle;
    bool desaturate_only;
};

class ChromaKeyColor : public BC_GenericButton
{
public:
	ChromaKeyColor(ChromaKeyHSV *plugin, 
		ChromaKeyWindow *gui, 
		int x, 
		int y);

	int handle_event();

	ChromaKeyHSV *plugin;
	ChromaKeyWindow *gui;
};

class ChromaKeySlider : public BC_FSlider
{
public:
    ChromaKeySlider(ChromaKeyHSV *plugin, int x, int y, float min, float max, float *output);
    int handle_event();
    ChromaKeyHSV *plugin;
    float *output;
};


class ChromaKeyUseColorPicker : public BC_GenericButton
{
public:
	ChromaKeyUseColorPicker(ChromaKeyHSV *plugin, ChromaKeyWindow *gui, int x, int y);
	int handle_event();
	ChromaKeyHSV *plugin;
	ChromaKeyWindow *gui;
};


class ChromaKeyColorThread : public ColorThread
{
public:
	ChromaKeyColorThread(ChromaKeyHSV *plugin, ChromaKeyWindow *gui);
	int handle_new_color(int output, int alpha);
	ChromaKeyHSV *plugin;
	ChromaKeyWindow *gui;
};

class ChromaKeyToggle : public BC_CheckBox
{
public:
	ChromaKeyToggle(ChromaKeyHSV *plugin, 
        int x, 
        int y,
        bool *output,
        const char *caption);
	int handle_event();
	ChromaKeyHSV *plugin;
    bool *output;
};



class ChromaKeyWindow : public PluginClientWindow
{
public:
	ChromaKeyWindow(ChromaKeyHSV *plugin);
	~ChromaKeyWindow();

	void create_objects();
	void update_sample();

	ChromaKeyColor *color;
	ChromaKeyUseColorPicker *use_colorpicker;
	ChromaKeySlider *min_brightness;
	ChromaKeySlider *max_brightness;
	ChromaKeySlider *saturation_start;
	ChromaKeySlider *saturation_line;
	ChromaKeySlider *tolerance;
	ChromaKeySlider *in_slope;
	ChromaKeySlider *out_slope;
	ChromaKeySlider *alpha_offset;
	ChromaKeySlider *spill_saturation;
	ChromaKeySlider *spill_angle;
    ChromaKeyToggle *desaturate_only;
	ChromaKeyToggle *show_mask;
	BC_SubWindow *sample;
	ChromaKeyHSV *plugin;
	ChromaKeyColorThread *color_thread;
};







class ChromaKeyServer : public LoadServer
{
public:
	ChromaKeyServer(ChromaKeyHSV *plugin);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	ChromaKeyHSV *plugin;
};

class ChromaKeyPackage : public LoadPackage
{
public:
	ChromaKeyPackage();
	int y1, y2;
};

class ChromaKeyUnit : public LoadClient
{
public:
	ChromaKeyUnit(ChromaKeyHSV *plugin, ChromaKeyServer *server);
	void process_package(LoadPackage *package);
	template <typename component_type> void process_chromakey(int components, component_type max, bool use_yuv, ChromaKeyPackage *pkg);
	bool is_same_color(float r, float g, float b, float rk,float bk,float gk, float color_threshold, float light_threshold, int key_main_component);

	ChromaKeyHSV *plugin;

};




class ChromaKeyHSV : public PluginVClient
{
public:
	ChromaKeyHSV(PluginServer *server);
	~ChromaKeyHSV();
	
	PLUGIN_CLASS_MEMBERS(ChromaKeyConfig);
	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int handle_opengl();
	int is_realtime();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();

	VFrame *input, *output;
	ChromaKeyServer *engine;
};








#endif







