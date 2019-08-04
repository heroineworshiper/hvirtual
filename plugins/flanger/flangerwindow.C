
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

#include "bcdisplayinfo.h"
#include "bchash.h"
#include "bcsignals.h"
#include "language.h"
#include "flanger.h"
#include "flangerwindow.h"
#include "theme.h"

#include <string.h>

#define TEXT_W DP(90)
#define WINDOW_W DP(400)
#define WINDOW_H DP(200)

FlangerWindow::FlangerWindow(Flanger *plugin)
 : PluginClientWindow(plugin, 
	WINDOW_W, 
	WINDOW_H, 
	WINDOW_W, 
	WINDOW_H, 
	0)
{ 
	this->plugin = plugin; 
}

FlangerWindow::~FlangerWindow()
{
    delete voices;
    delete offset;
    delete starting_phase;
    delete depth;
    delete rate;
    delete wetness;
}

void FlangerWindow::create_objects()
{
	int margin = client->get_theme()->widget_border;
    int x1 = margin;
	int x2 = DP(200), y = margin;
    int x3 = x2 + BC_Pot::calculate_w() + margin;
    int x4 = x3 + BC_Pot::calculate_w() + margin;
    int height = BC_TextBox::calculate_h(this, MEDIUMFONT, 1, 1) + margin;


    voices = new PluginParam(plugin,
        this,
        x1, 
        x2,
        x4,
        y, 
        TEXT_W,
        &plugin->config.voices,  // output_i
        0, // output_f
        0, // output_q
        "Voices:",
        1, // min
        MAX_VOICES); // max
    voices->initialize();
    y += height;

    offset = new PluginParam(plugin,
        this,
        x1, 
        x3,
        x4,
        y, 
        TEXT_W,
        0,  // output_i
        &plugin->config.offset, // output_f
        0, // output_q
        "Phase offset (seconds):",
        0, // min
        1); // max
    offset->set_precision(4);
    offset->initialize();
    y += height;


    starting_phase = new PluginParam(plugin,
        this,
        x1, 
        x2,
        x4,
        y, 
        TEXT_W,
        0,  // output_i
        &plugin->config.starting_phase, // output_f
        0, // output_q
        "Starting phase (%):",
        0, // min
        100); // max
    starting_phase->initialize();
    y += height;



    depth = new PluginParam(plugin,
        this,
        x1, 
        x3,
        x4,
        y, 
        TEXT_W,
        0,  // output_i
        &plugin->config.depth, // output_f
        0, // output_q
        "Depth (%):",
        0, // min
        100); // max
    depth->initialize();
    y += height;



    rate = new PluginParam(plugin,
        this,
        x1, 
        x2,
        x4,
        y, 
        TEXT_W,
        0,  // output_i
        &plugin->config.rate, // output_f
        0, // output_q
        "Rate (Hz):",
        0, // min
        1); // max
    rate->initialize();
    y += height;



    wetness = new PluginParam(plugin,
        this,
        x1, 
        x3,
        x4,
        y, 
        TEXT_W,
        0,  // output_i
        &plugin->config.wetness, // output_f
        0, // output_q
        "Wetness (db):",
        INFINITYGAIN, // min
        0); // max
    wetness->initialize();
    y += height;

	show_window();
}

void FlangerWindow::update()
{
    voices->update(1, 1);
    offset->update(1, 1);
    starting_phase->update(1, 1);
    depth->update(1, 1);
    rate->update(1, 1);
    wetness->update(1, 1);
}

void FlangerWindow::param_updated()
{
}

