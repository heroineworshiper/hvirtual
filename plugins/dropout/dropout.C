/*
 * CINELERRA
 * Copyright (C) 2024 Adam Williams <broadcast at earthling dot net>
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

// A crummy transition just to test an audio transition with a GUI
// Set the balance to 0 or 100 to just fade 1 of the edits.
// Set the balance to 50 to fade out & fade in.


#include "dropout.h"
#include "edl.inc"
#include "filexml.h"
#include "language.h"
#include "samples.h"
#include "theme.h"
#include <string.h>


REGISTER_PLUGIN(DropoutMain)


DropoutSlider::DropoutSlider(DropoutMain *plugin, 
	DropoutWindow *window,
	int x,
	int y,
    int w,
    int *output)
 : BC_ISlider(x,
    y,
    0,
    w,
    w,
    0, 
    100,
    *output)
{
    this->plugin = plugin;
    this->window = window;
    this->output = output;
}

int DropoutSlider::handle_event()
{
    *output = get_value();
    plugin->send_configure_change();
    return 1;
}



DropoutWindow::DropoutWindow(DropoutMain *plugin)
 : PluginClientWindow(plugin, 
	DP(320), 
	DP(100), 
	DP(320), 
	DP(100), 
	0)
{
	this->plugin = plugin;
}




void DropoutWindow::create_objects()
{
	int widget_border = plugin->get_theme()->widget_border;
	int window_border = plugin->get_theme()->window_border;
	int x = window_border;
    int y = window_border;
	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, _("Balance:")));
	y += title->get_h() + widget_border;
	add_subwindow(balance = new DropoutSlider(plugin, 
		this,
		x,
		y,
        get_w() - window_border * 2,
        &plugin->balance));
	show_window();
}






DropoutMain::DropoutMain(PluginServer *server)
 : PluginAClient(server)
{
    balance = 50;
}

DropoutMain::~DropoutMain()
{
}

const char* DropoutMain::plugin_title() { return N_("Dropout"); }
int DropoutMain::is_transition() { return 1; }
int DropoutMain::uses_gui() { return 1; }

NEW_WINDOW_MACRO(DropoutMain, DropoutWindow)



void DropoutMain::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_string(keyframe->get_data());
	output.tag.set_title("DROPOUT");
	output.tag.set_property("BALANCE", balance);
	output.append_tag();
	output.terminate_string();
}

void DropoutMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

//printf("DropoutMain::read_data %d %s\n", __LINE__, keyframe->get_data());
	input.set_shared_string(keyframe->get_data());

	while(!input.read_tag())
	{
		if(input.tag.title_is("DROPOUT"))
		{
			balance = input.tag.get_property("BALANCE", balance);
		}
	}
}

int DropoutMain::load_configuration()
{
	read_data(get_prev_keyframe(get_source_position()));
	return 1;
}

void DropoutMain::update_gui()
{
	if(thread)
	{
        load_configuration();
//printf("DropoutMain::update_gui %d balance=%d\n", __LINE__, balance);
        thread->window->lock_window("DropoutMain::update_gui 1");
        ((DropoutWindow*)thread->window)->balance->update(balance);
        thread->window->unlock_window();
    }
}


int DropoutMain::process_realtime(int64_t size, 
	Samples *outgoing, 
	Samples *incoming)
{
	load_configuration();

// printf("DropoutMain::process_realtime %d: start=%ld len=%ld size=%ld\n", 
// __LINE__,
// (long)PluginClient::get_source_position(), 
// (long)PluginClient::get_total_len(),
// (long)size);
    int64_t len = PluginClient::get_total_len();
    int64_t midpoint = len * balance / 100;

// process in fragments
    int64_t fragment = size;
    int64_t start = PluginClient::get_source_position();
    double *in_ptr = incoming->get_data();
    double *out_ptr = outgoing->get_data();

    for(int64_t offset = start; offset < start + size; offset += fragment)
    {
        int start2 = offset - start;
        fragment = size;
        if(offset < midpoint)
        {
            if(offset + fragment > midpoint)
                fragment = midpoint - offset;
        }
        else
        if(offset + fragment > start + size)
            fragment = start + size - offset;

// only outgoing
        if(offset < midpoint)
        {
            double intercept = (double)offset / midpoint;
            double slope = (double)1 / midpoint;
            for(int i = 0; i < fragment; i++)
            {
                in_ptr[start2 + i] = out_ptr[start2 + i] *
                    ((double)1 - (slope * i + intercept));
            }
        }
        else
// only incoming
        {
            double intercept = (double)(offset - midpoint) / (len - midpoint);
            double slope = (double)1 / (len - midpoint);
            for(int i = 0; i < fragment; i++)
            {
                in_ptr[start2 + i] = in_ptr[start2 + i] *
                    (slope * i + intercept);
            }
        }
    }

	return 0;
}








