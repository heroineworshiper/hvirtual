
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

#ifndef COMPRESSOR_H
#define COMPRESSOR_H



#include "bchash.inc"
#include "compressortools.h"
#include "guicast.h"
#include "mutex.h"
#include "pluginaclient.h"
#include "samples.inc"
#include "vframe.inc"

class CompressorEffect;
class CompressorWindow;




class CompressorCanvas : public CompressorCanvasBase
{
public:
	CompressorCanvas(CompressorEffect *plugin, 
        CompressorWindow *window,
        int x, 
        int y, 
        int w, 
        int h);
    void update_window();
};


class CompressorLookAhead : public BC_TumbleTextBox
{
public:
	CompressorLookAhead(CompressorEffect *plugin, 
        CompressorWindow *window, 
        int x, 
        int y);
	int handle_event();
	CompressorEffect *plugin;
};

class CompressorAttack : public BC_TumbleTextBox
{
public:
	CompressorAttack(CompressorEffect *plugin, 
        CompressorWindow *window, 
        int x, 
        int y);
	int handle_event();
	CompressorEffect *plugin;
};

class CompressorRelease : public BC_TumbleTextBox
{
public:
	CompressorRelease(CompressorEffect *plugin, 
        CompressorWindow *window, 
        int x, 
        int y);
	int handle_event();
	CompressorEffect *plugin;
};


class CompressorClear : public BC_GenericButton
{
public:
	CompressorClear(CompressorEffect *plugin, int x, int y);
	int handle_event();
	CompressorEffect *plugin;
};

class CompressorX : public BC_TumbleTextBox
{
public:
	CompressorX(CompressorEffect *plugin, 
        CompressorWindow *window, 
        int x, 
        int y);
	int handle_event();
	CompressorEffect *plugin;
};

class CompressorY : public BC_TumbleTextBox
{
public:
	CompressorY(CompressorEffect *plugin, 
        CompressorWindow *window, 
        int x, 
        int y);
	int handle_event();
	CompressorEffect *plugin;
};

class CompressorTrigger : public BC_TumbleTextBox
{
public:
	CompressorTrigger(CompressorEffect *plugin, 
        CompressorWindow *window, 
        int x, 
        int y);
	int handle_event();
	CompressorEffect *plugin;
};

class CompressorSmooth : public BC_CheckBox
{
public:
	CompressorSmooth(CompressorEffect *plugin, int x, int y);
	int handle_event();
	CompressorEffect *plugin;
};

class CompressorInput : public BC_PopupMenu
{
public:
	CompressorInput(CompressorEffect *plugin, int x, int y);
	void create_objects();
	int handle_event();
	static const char* value_to_text(int value);
	static int text_to_value(char *text);
	CompressorEffect *plugin;
};



class CompressorWindow : public PluginClientWindow
{
public:
	CompressorWindow(CompressorEffect *plugin);
    ~CompressorWindow();

	void create_objects();
	void update();
	void update_textboxes();
	void draw_scales();
    void update_meter(CompressorFrame *frame);
	int resize_event(int w, int h);	
	
	CompressorCanvas *canvas;
	CompressorLookAhead *readahead;
	CompressorAttack *attack;
	CompressorClear *clear;
	CompressorX *x_text;
	CompressorY *y_text;
	CompressorTrigger *trigger;
	CompressorRelease *release;
	CompressorSmooth *smooth;
	CompressorInput *input;
    BC_Meter *in;
    BC_Meter *gain_change;
	CompressorEffect *plugin;
};



class CompressorConfig : public CompressorConfigBase
{
public:
	CompressorConfig();

	void copy_from(CompressorConfig &that);
	int equivalent(CompressorConfig &that);
	void interpolate(CompressorConfig &prev, 
		CompressorConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);


};




class CompressorEffect : public PluginAClient
{
public:
	CompressorEffect(PluginServer *server);
	~CompressorEffect();

	int is_multichannel();
	int is_realtime();
	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);
	int process_buffer(int64_t size, 
		Samples **buffer,
		int64_t start_position,
		int sample_rate);
    void allocate_input(int size);


	void update_gui();
    void render_stop();

	PLUGIN_CLASS_MEMBERS(CompressorConfig)

// Input data + read ahead for each channel
	Samples **input_buffer;

// Number of samples in the input buffer 
	int64_t input_size;
// Number of samples allocated in the input buffer
	int64_t input_allocated;
// Starting sample of input buffer relative to project in requested rate.
	int64_t input_start;
    int64_t last_position;
    int need_reconfigure;


    CompressorEngine *engine;

// Temporaries for linear transfer
	ArrayList<compressor_point_t> levels;
//	double min_x, min_y;
//	double max_x, max_y;
};


#endif
