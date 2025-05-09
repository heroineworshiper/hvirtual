/*
 * CINELERRA
 * Copyright (C) 1997-2017 Adam Williams <broadcast at earthling dot net>
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
#include "filexml.h"
#include "freezeframe.h"
#include "language.h"
#include "picon_png.h"
#include "transportque.inc"



#include <string.h>


REGISTER_PLUGIN(FreezeFrameMain)





FreezeFrameConfig::FreezeFrameConfig()
{
	enabled = 0;
	line_double = 0;
}

void FreezeFrameConfig::copy_from(FreezeFrameConfig &that)
{
	enabled = that.enabled;
	line_double = that.line_double;
}

int FreezeFrameConfig::equivalent(FreezeFrameConfig &that)
{
	return enabled == that.enabled &&
		line_double == that.line_double;
}

void FreezeFrameConfig::interpolate(FreezeFrameConfig &prev, 
	FreezeFrameConfig &next, 
	long prev_frame, 
	long next_frame, 
	long current_frame)
{
	this->enabled = prev.enabled;
	this->line_double = prev.line_double;
}










FreezeFrameWindow::FreezeFrameWindow(FreezeFrameMain *client)
 : PluginClientWindow(client,
	DP(200),
	DP(100),
	DP(200),
	DP(100),
	0)
{
	this->client = client; 
}

FreezeFrameWindow::~FreezeFrameWindow()
{
}

void FreezeFrameWindow::create_objects()
{
	int x = DP(10), y = DP(10);
	add_tool(enabled = new FreezeFrameToggle(client, 
		&client->config.enabled,
		x, 
		y,
		_("Enabled")));
// Try using extra effect for the line double since it doesn't
// change the overhead.
// 	y += 30;
// 	add_tool(line_double = new FreezeFrameToggle(client, 
// 		&client->config.line_double,
// 		x, 
// 		y,
// 		_("Line double")));
	show_window();
	flush();
}






FreezeFrameToggle::FreezeFrameToggle(FreezeFrameMain *client, 
	int *value, 
	int x, 
	int y,
	char *text)
 : BC_CheckBox(x, y, *value, text)
{
	this->client = client;
	this->value = value;
}
FreezeFrameToggle::~FreezeFrameToggle()
{
}
int FreezeFrameToggle::handle_event()
{
	*value = get_value();
	client->send_configure_change();
	return 1;
}












FreezeFrameMain::FreezeFrameMain(PluginServer *server)
 : PluginVClient(server)
{
	
	first_frame = 0;
	first_frame_position = -1;
}

FreezeFrameMain::~FreezeFrameMain()
{
	
	if(first_frame) delete first_frame;
}

const char* FreezeFrameMain::plugin_title() { return N_("Freeze Frame"); }
int FreezeFrameMain::is_synthesis() { return 1; }
int FreezeFrameMain::is_realtime() { return 1; }


NEW_WINDOW_MACRO(FreezeFrameMain, FreezeFrameWindow)
NEW_PICON_MACRO(FreezeFrameMain)

int FreezeFrameMain::load_configuration()
{
	KeyFrame *prev_keyframe = get_prev_keyframe(get_source_position());
	int64_t prev_position = edl_to_local(prev_keyframe->position);
	if(prev_position < get_source_start()) prev_position = get_source_start();
	read_data(prev_keyframe);
// Invalidate stored frame
	if(config.enabled) first_frame_position = prev_position;
	return 0;
}

void FreezeFrameMain::update_gui()
{
	if(thread)
	{
		load_configuration();
		((FreezeFrameWindow*)thread->window)->lock_window();
		((FreezeFrameWindow*)thread->window)->enabled->update(config.enabled);
//		thread->window->line_double->update(config.line_double);
		thread->window->unlock_window();
	}
}

void FreezeFrameMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->get_data(), MESSAGESIZE);
	output.tag.set_title("FREEZEFRAME");
	output.append_tag();
	if(config.enabled)
	{
		output.tag.set_title("ENABLED");
		output.append_tag();
	}
	if(config.line_double)
	{
		output.tag.set_title("LINE_DOUBLE");
		output.append_tag();
	}
	output.terminate_string();
// data is now in *text
}

void FreezeFrameMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), strlen(keyframe->get_data()));

	int result = 0;
	config.enabled = 0;
	config.line_double = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("ENABLED"))
			{
				config.enabled = 1;
			}
			if(input.tag.title_is("LINE_DOUBLE"))
			{
				config.line_double = 1;
			}
		}
	}
}







int FreezeFrameMain::process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate)
{
	int64_t previous_first_frame = first_frame_position;
	load_configuration();

// Just entered frozen range
	if(!first_frame && config.enabled)
	{
		if(!first_frame)
			first_frame = new VFrame(0, 
				-1,
				frame->get_w(), 
				frame->get_h(),
				frame->get_color_model(),
				-1);

		read_frame(first_frame, 
				0, 
				get_direction() == PLAY_REVERSE ? first_frame_position + 1 : first_frame_position,
				frame_rate,
				get_use_opengl());
printf("FreezeFrameMain::process_buffer %d first_frame_position=%ld\n", 
__LINE__, 
(long)first_frame_position);
		if(get_use_opengl()) return run_opengl();

		frame->copy_from(first_frame);
	}
	else
// Still not frozen
	if(!first_frame && !config.enabled)
	{
		read_frame(frame, 
			0, 
			start_position,
			frame_rate,
			get_use_opengl());
	}
	else
// Just left frozen range
	if(first_frame && !config.enabled)
	{
		delete first_frame;
		first_frame = 0;
		read_frame(frame, 
			0, 
			start_position,
			frame_rate,
			get_use_opengl());
	}
	else
// Still frozen
	if(first_frame && config.enabled)
	{
// Had a keyframe in frozen range.  Load new first frame
		if(previous_first_frame != first_frame_position)
		{
printf("FreezeFrameMain::process_buffer %d reloading first frame from %ld\n", 
__LINE__, 
(long)first_frame_position);

			read_frame(first_frame, 
				0, 
				get_direction() == PLAY_REVERSE ? first_frame_position + 1 : first_frame_position,
				frame_rate,
				get_use_opengl());
		}

// printf("FreezeFrameMain::process_buffer %d opengl_state=%d\n", 
// __LINE__,
// first_frame->get_opengl_state());

		if(get_use_opengl()) return run_opengl();

		frame->copy_from(first_frame);
	}


// Line double to support interlacing
// 	if(config.line_double && config.enabled)
// 	{
// 		for(int i = 0; i < frame->get_h() - 1; i += 2)
// 		{
// 			memcpy(frame->get_rows()[i + 1], 
// 				frame->get_rows()[i], 
// 				frame->get_bytes_per_line());
// 		}
// 	}



	return 0;
}

int FreezeFrameMain::handle_opengl()
{
#ifdef HAVE_GL
// printf("FreezeFrameMain::handle_opengl %d opengl_state=%d\n", 
// __LINE__,
// first_frame->get_opengl_state());
    first_frame->to_texture();
	get_output()->enable_opengl();
	get_output()->init_screen();
	first_frame->bind_texture(0);
	first_frame->draw_texture();
	get_output()->set_opengl_state(VFrame::SCREEN);
#endif
	return 0;
}


