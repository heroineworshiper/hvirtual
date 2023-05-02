
/*
 * CINELERRA
 * Copyright (C) 2017-2019 Adam Williams <broadcast at earthling dot net>
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
#include "confirmsave.h"
#include "bchash.h"
#include "bcsignals.h"
#include "errorbox.h"
#include "filexml.h"
#include "language.h"
#include "samples.h"
#include "theme.h"
#include "transportque.inc"
#include "tremolo.h"
#include "units.h"

#include "vframe.h"

#include <math.h>
#include <string.h>
#include <time.h>
#include <unistd.h>


// min rate for the GUI
#define MIN_RATE 0.0
// min rate to avoid division by zero
#define MIN_RATE2 0.10
#define MAX_RATE 10.0
#define MIN_OFFSET 0.0
#define MAX_OFFSET 100.0
#define MIN_DEPTH 0.0
#define MAX_DEPTH (-INFINITYGAIN)



PluginClient* new_plugin(PluginServer *server)
{
	return new Tremolo(server);
}



Tremolo::Tremolo(PluginServer *server)
 : PluginAClient(server)
{
	need_reconfigure = 1;
    table = 0;
    table_size = 0;
    table_offset = 0;
    last_position = 0;
}

Tremolo::~Tremolo()
{
    delete [] table;
}

const char* Tremolo::plugin_title() { return N_("Tremolo"); }
int Tremolo::is_realtime() { return 1; }
int Tremolo::is_multichannel() { return 0; }
int Tremolo::is_synthesis() { return 0; }
VFrame* Tremolo::new_picon() { return 0; }


int Tremolo::process_buffer(int64_t size, 
	Samples *buffer, 
	int64_t start_position,
	int sample_rate)
{
    need_reconfigure |= load_configuration();
// printf("Tremolo::process_buffer %d start_position=%ld size=%ld need_reconfigure=%d\n",
// __LINE__,
// start_position, 
// size,
// need_reconfigure);

// reset after configuring
    if(last_position != start_position ||
        need_reconfigure)
    {
        need_reconfigure = 0;

        if(table)
        {
            delete [] table;
        }

// waveform is a whole number of samples that repeats
        if(config.rate < MIN_RATE2)
        {
            table_size = 256;
        }
        else
        {
            table_size = (int)((double)sample_rate / config.rate);
        }


        table = new double[table_size];
        double depth = 1.0 - DB::fromdb(-config.depth);

// printf("Tremolo::process_buffer %d table_size=%d depth=%f\n", 
// __LINE__, 
// table_size,
// depth);

        for(int i = 0; i < table_size; i++)
        {
            double value = 0;
            
            switch(config.waveform)
            {
                case SINE:
                    value = (sin((double)i * 2 * M_PI / table_size + 3.0 * M_PI / 2) + 1) / 2;
                    break;
                case SAWTOOTH:
                    value = (double)(table_size - i) / table_size;
                    break;
                case SAWTOOTH2:
                    value = (double)i / table_size;
                    break;
                case SQUARE:
                    if(i < table_size / 2)
                    {
                        value = 0;
                    }
                    else
                    {
                        value = 1;
                    }
                    break;
                case TRIANGLE:
                    if(i < table_size / 2)
                    {
                        value = (double)(i * 2) / table_size;
                    }
                    else
                    {
                        value = 1.0 - 
                            (double)(i - table_size / 2) / 
                            (table_size / 2);
                    }
                    break;
            }
// value is -1 ... 0
            value = 1.0 - value * depth;
// printf("Tremolo::process_buffer %d i=%d value=%f\n", 
// __LINE__, 
// i, 
// value);
            table[i] = value;
        }


// compute the phase position from the keyframe position & the phase offset
		int64_t prev_position = edl_to_local(
			get_prev_keyframe(
				get_source_position())->position);

		if(prev_position == 0)
		{
			prev_position = get_source_start();
		}

        int64_t starting_offset = (int64_t)(config.offset * table_size / 100);
        table_offset = (int64_t)(start_position - 
            prev_position +
            starting_offset) %
            table_size;
// printf("Tremolo::process_buffer %d table_offet=%d table_size=%d\n",
// __LINE__,
// table_offset,
// table_size);

// printf("Tremolo::process_buffer %d i=%d src=%d dst=%d input_sample=%f\n",
// __LINE__,
// i,
// voice->src_channel,
// voice->dst_channel,
// flanging_table[voice->table_offset].input_sample);
    }


// read the input
	read_samples(buffer,
		0,
		sample_rate,
		start_position,
		size);

// input signal
    double *in = buffer->get_data();
    double *out = buffer->get_data();
    for(int j = 0; j < size; j++)
    {
        out[j] = in[j] * table[table_offset++];
        table_offset %= table_size;
    }



    if(get_direction() == PLAY_FORWARD)
    {
        last_position = start_position + size;
    }
    else
    {
        last_position = start_position - size;
    }
//printf("Tremolo::process_buffer %d\n", __LINE__);

    

    return 0;
}



NEW_WINDOW_MACRO(Tremolo, TremoloWindow)


LOAD_CONFIGURATION_MACRO(Tremolo, TremoloConfig)


void Tremolo::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause xml file to store data directly in text
	output.set_shared_string(keyframe->get_data(), MESSAGESIZE);

	output.tag.set_title("TREMOLO");
	output.tag.set_property("OFFSET", config.offset);
	output.tag.set_property("DEPTH", config.depth);
	output.tag.set_property("RATE", config.rate);
	output.tag.set_property("WAVEFORM", config.waveform);
	output.append_tag();
	output.append_newline();

	output.terminate_string();
}

void Tremolo::read_data(KeyFrame *keyframe)
{
	FileXML input;
// cause xml file to read directly from text
	input.set_shared_string(keyframe->get_data(), strlen(keyframe->get_data()));
	int result = 0;

	result = input.read_tag();

	if(!result)
	{
		if(input.tag.title_is("TREMOLO"))
		{
			config.offset = input.tag.get_property("OFFSET", config.offset);
			config.depth = input.tag.get_property("DEPTH", config.depth);
			config.rate = input.tag.get_property("RATE", config.rate);
			config.waveform = input.tag.get_property("WAVEFORM", config.waveform);
		}
	}

	config.boundaries();
}

void Tremolo::update_gui()
{
	if(thread)
	{
		if(load_configuration())
		{
			thread->window->lock_window("Tremolo::update_gui 1");
            ((TremoloWindow*)thread->window)->update();
			thread->window->unlock_window();
        }
	}
}









TremoloConfig::TremoloConfig()
{
	offset = 0.00;
	depth = 10.0;
	rate = 0.20;
	waveform = SINE;
}

int TremoloConfig::equivalent(TremoloConfig &that)
{
	return EQUIV(offset, that.offset) &&
		EQUIV(depth, that.depth) &&
		EQUIV(rate, that.rate) &&
		waveform == that.waveform;
}

void TremoloConfig::copy_from(TremoloConfig &that)
{
	offset = that.offset;
	depth = that.depth;
	rate = that.rate;
	waveform = that.waveform;
}

void TremoloConfig::interpolate(TremoloConfig &prev, 
	TremoloConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	copy_from(prev);
}

void TremoloConfig::boundaries()
{
	CLAMP(offset, MIN_OFFSET, MAX_OFFSET);
	CLAMP(depth, MIN_DEPTH, MAX_DEPTH);
	CLAMP(rate, MIN_RATE, MAX_RATE);
	CLAMP(waveform, 0, TOTAL_WAVEFORMS - 1);
}











#define WINDOW_W DP(400)
#define WINDOW_H DP(200)

TremoloWindow::TremoloWindow(Tremolo *plugin)
 : PluginClientWindow(plugin, 
	WINDOW_W, 
	WINDOW_H, 
	WINDOW_W, 
	WINDOW_H, 
	0)
{ 
	this->plugin = plugin; 
}

TremoloWindow::~TremoloWindow()
{
    delete offset;
    delete depth;
    delete rate;
    delete waveform;
}

void TremoloWindow::create_objects()
{
	int margin = plugin->get_theme()->widget_border;
    int x1 = margin;
	int x2 = DP(200), y = margin;
    int x3 = x2 + BC_Pot::calculate_w() + margin;
    int x4 = x3 + BC_Pot::calculate_w() + margin;
    int text_w = get_w() - margin - x4;
    int height = BC_TextBox::calculate_h(this, MEDIUMFONT, 1, 1) + margin;


    offset = new PluginParam(plugin,
        this,
        x1, 
        x3,
        x4,
        y, 
        text_w,
        0,  // output_i
        &plugin->config.offset, // output_f
        0, // output_q
        "Phase offset (%):",
        MIN_OFFSET, // min
        MAX_OFFSET); // max
    offset->set_precision(3);
    offset->initialize();
    y += height;


    depth = new PluginParam(plugin,
        this,
        x1, 
        x2,
        x4,
        y, 
        text_w,
        0,  // output_i
        &plugin->config.depth, // output_f
        0, // output_q
        "Depth (dB):",
        MIN_DEPTH, // min
        MAX_DEPTH); // max
    depth->set_precision(3);
    depth->initialize();
    y += height;



    rate = new PluginParam(plugin,
        this,
        x1, 
        x3,
        x4,
        y, 
        text_w,
        0,  // output_i
        &plugin->config.rate, // output_f
        0, // output_q
        "Rate (Hz):",
        MIN_RATE, // min
        MAX_RATE); // max
    rate->set_precision(3);
    rate->initialize();
    y += height;

    char string[BCTEXTLEN];
    int y2 = y + BC_Pot::calculate_h() / 2;
    add_subwindow(new BC_Title(x1, y2, _("Waveform:")));
    add_tool(waveform = new TremoloWaveForm(plugin,
        x2, 
        y2,
        waveform_to_text(string, plugin->config.waveform)));
    waveform->create_objects();

	show_window();
}

void TremoloWindow::update()
{
    offset->update(0, 0);
    depth->update(0, 0);
    rate->update(0, 0);
    char string[BCTEXTLEN];
	waveform->set_text(waveform_to_text(string, plugin->config.waveform));
}



char* TremoloWindow::waveform_to_text(char *text, int waveform)
{
	switch(waveform)
	{
		case SINE:            sprintf(text, _("Sine"));           break;
		case SAWTOOTH:        sprintf(text, _("Sawtooth"));       break;
		case SAWTOOTH2:        sprintf(text, _("Rev Sawtooth"));       break;
		case SQUARE:          sprintf(text, _("Square"));         break;
		case TRIANGLE:        sprintf(text, _("Triangle"));       break;
	}
	return text;
}


void TremoloWindow::param_updated()
{
}





TremoloWaveForm::TremoloWaveForm(Tremolo *plugin, int x, int y, char *text)
 : BC_PopupMenu(x, y, DP(120), text)
{
    this->plugin = plugin;
}


TremoloWaveForm::~TremoloWaveForm()
{
}


void TremoloWaveForm::create_objects()
{
    char string[BCTEXTLEN];
	add_item(new TremoloWaveFormItem(plugin, 
        TremoloWindow::waveform_to_text(string, SINE), 
        SINE));
	add_item(new TremoloWaveFormItem(plugin, 
        TremoloWindow::waveform_to_text(string, SAWTOOTH), 
        SAWTOOTH));
	add_item(new TremoloWaveFormItem(plugin, 
        TremoloWindow::waveform_to_text(string, SAWTOOTH2), 
        SAWTOOTH2));
	add_item(new TremoloWaveFormItem(plugin, 
        TremoloWindow::waveform_to_text(string, SQUARE), 
        SQUARE));
	add_item(new TremoloWaveFormItem(plugin, 
        TremoloWindow::waveform_to_text(string, TRIANGLE), 
        TRIANGLE));
}



TremoloWaveFormItem::TremoloWaveFormItem(Tremolo *plugin, char *text, int value)
 : BC_MenuItem(text)
{
    this->plugin = plugin;
    this->value = value;
}

TremoloWaveFormItem::~TremoloWaveFormItem()
{
}

int TremoloWaveFormItem::handle_event()
{
    plugin->config.waveform = value;
    get_popup_menu()->set_text(get_text());
    plugin->send_configure_change();
    return 1;
}
