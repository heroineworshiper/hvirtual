
/*
 * CINELERRA
 * Copyright (C) 2009-2019 Adam Williams <broadcast at earthling dot net>
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

#include "edl.h"
#include "edlsession.h"
#include "pluginaclient.h"
#include "pluginserver.h"
#include "samples.h"
#include "transportque.inc"

#include <string.h>
//#define MAX_FRAME_BUFFER 1024


// PluginClientFrame::PluginClientFrame(int data_size, 
// 	int period_n, 
// 	int period_d)
// {
//     reset();
// 	this->data_size = data_size;
// 	this->period_n = period_n;
// 	this->period_d = period_d;
// }


//static int allocations = 0;
PluginClientFrame::PluginClientFrame()
{
//allocations++;
//printf("PluginClientFrame::PluginClientFrame %d allocations=%d\n", __LINE__, allocations);
    reset();
}

PluginClientFrame::~PluginClientFrame()
{
//allocations--;
//printf("PluginClientFrame::~PluginClientFrame %d allocations=%d\n", __LINE__, allocations);
	if(data)
    {
        delete [] data;
    }
}

void PluginClientFrame::reset()
{
	data_size = 0;
//	period_n = 0;
//	period_d = 0;
    data = 0;
    freq_max = 0;
    time_max = 0;
    nyquist = 0;
    edl_position = -1;
}



PluginAClient::PluginAClient(PluginServer *server)
 : PluginClient(server)
{
	sample_rate = 0;
	if(server &&
		server->edl &&
		server->edl->session) 
	{
		project_sample_rate = server->edl->session->sample_rate;
		sample_rate = project_sample_rate;
	}
	else
	{
		project_sample_rate = 1;
		sample_rate = 1;
	}
}

PluginAClient::~PluginAClient()
{
	frame_buffer.remove_all_objects();
}

int PluginAClient::is_audio()
{
	return 1;
}


// int PluginAClient::get_render_ptrs()
// {
// 	int i, j, double_buffer, fragment_position;
// 
// 	for(i = 0; i < total_in_buffers; i++)
// 	{
// 		double_buffer = double_buffer_in_render.values[i];
// 		fragment_position = offset_in_render.values[i];
// 		input_ptr_render[i] = &input_ptr_master.values[i][double_buffer][fragment_position];
// //printf("PluginAClient::get_render_ptrs %x\n", input_ptr_master.values[i][double_buffer]);
// 	}
// 
// 	for(i = 0; i < total_out_buffers; i++)
// 	{
// 		double_buffer = double_buffer_out_render.values[i];
// 		fragment_position = offset_out_render.values[i];
// 		output_ptr_render[i] = &output_ptr_master.values[i][double_buffer][fragment_position];
// 	}
// //printf("PluginAClient::get_render_ptrs %x %x\n", input_ptr_render[0], output_ptr_render[0]);
// 	return 0;
// }

int PluginAClient::init_realtime_parameters()
{
	project_sample_rate = server->edl->session->sample_rate;
	return 0;
}

int PluginAClient::process_realtime(int64_t size, 
	Samples **input_ptr, 
	Samples **output_ptr)
{
	return 0;
}

int PluginAClient::process_realtime(int64_t size, 
	Samples *input_ptr, 
	Samples *output_ptr)
{
	return 0;
}

int PluginAClient::process_buffer(int64_t size, 
	Samples **buffer,
	int64_t start_position,
	int sample_rate)
{
	for(int i = 0; i < PluginClient::total_in_buffers; i++)
		read_samples(buffer[i], 
			i, 
			sample_rate, 
			source_position, 
			size);
	process_realtime(size, buffer, buffer);
	return 0;
}

int PluginAClient::process_buffer(int64_t size, 
	Samples *buffer,
	int64_t start_position,
	int sample_rate)
{
	read_samples(buffer, 
		0, 
		sample_rate, 
		source_position, 
		size);
	process_realtime(size, buffer, buffer);
	return 0;
}




int PluginAClient::plugin_start_loop(int64_t start, 
	int64_t end, 
	int64_t buffer_size, 
	int total_buffers)
{
	sample_rate = get_project_samplerate();
	return PluginClient::plugin_start_loop(start, 
		end, 
		buffer_size, 
		total_buffers);
}

int PluginAClient::plugin_get_parameters()
{
	sample_rate = get_project_samplerate();
	return PluginClient::plugin_get_parameters();
}


int64_t PluginAClient::local_to_edl(int64_t position)
{
	if(position < 0) return position;
	return (int64_t)(position *
		get_project_samplerate() /
		sample_rate);
	return 0;
}

int64_t PluginAClient::edl_to_local(int64_t position)
{
	if(position < 0) return position;
	return (int64_t)(position *
		sample_rate /
		get_project_samplerate());
}


int PluginAClient::plugin_process_loop(Samples **buffers, int64_t &write_length)
{
	write_length = 0;

	if(is_multichannel())
		return process_loop(buffers, write_length);
	else
		return process_loop(buffers[0], write_length);
}

int PluginAClient::read_samples(Samples *buffer, 
	int channel, 
	int64_t start_position, 
	int64_t total_samples)
{
	return server->read_samples(buffer, 
		channel, 
		start_position, 
		total_samples);
}

int PluginAClient::read_samples(Samples *buffer, 
	int64_t start_position, 
	int64_t total_samples)
{
	return server->read_samples(buffer, 
		0, 
		start_position, 
		total_samples);
}

int PluginAClient::read_samples(Samples *buffer,
		int channel,
		int sample_rate,
		int64_t start_position,
		int64_t len)
{
	return server->read_samples(buffer,
		channel,
		sample_rate,
		start_position,
		len);
}



void PluginAClient::send_reset_gui_frames()
{
    server->send_reset_gui_frames();
}

// void PluginAClient::send_render_gui()
// {
// // send the frame buffer as a void data object to the GUI instance
// 	server->send_render_gui(&frame_buffer);
// }


// used by audio plugins.  runs on the GUI instance
void PluginAClient::reset_gui_frames()
{
    if(thread)
    {
// must lock this to get access to the frame_buffer
	    thread->get_window()->lock_window("PluginClient::render_gui");

//printf("PluginClient::reset_gui_frames %d\n", __LINE__);
        this->frame_buffer.remove_all_objects();

        thread->get_window()->unlock_window();
    }
}

// used by audio plugins
void PluginAClient::plugin_render_gui(void *data)
{
	ArrayList<PluginClientFrame*> *src = 
		(ArrayList<PluginClientFrame*>*)data;
	if(src->size() && thread)
	{
// must lock this to get access to the destination frames
		thread->get_window()->lock_window("PluginClient::render_gui");

// Shift frame pointers to the GUI instance
// order them by playback direction
		while(src->size())
		{
            PluginClientFrame *src_frame = src->get(0);
			src->remove_number(0);
            
            int got_it = 0;
            for(int i = 0; i < this->frame_buffer.size(); i++)
            {
                if(direction == PLAY_FORWARD &&
                    this->frame_buffer.get(i)->edl_position > src_frame->edl_position ||
                    direction == PLAY_REVERSE &&
                    this->frame_buffer.get(i)->edl_position < src_frame->edl_position)
                {
                    this->frame_buffer.insert(src_frame, i);
                    got_it = 1;
                    break;
                }
            }
            if(!got_it)
            {
            	this->frame_buffer.append(src_frame);
            }
		}

	
// Delete unused GUI frames
//	    while(frame_buffer.size() > MAX_FRAME_BUFFER)
//		    frame_buffer.remove_object_number(0);

// printf("PluginClient::render_gui %d direction=%d source_position=%ld ", 
// __LINE__, direction, source_position);
// for(int i = 0; i < this->frame_buffer.size(); i++)
// {
// printf("%ld ", this->frame_buffer.get(i)->edl_position);
// }
// printf("\n");


// Start the timer for the current buffer
//		update_timer->update();
		thread->get_window()->unlock_window();
	}
}

void PluginAClient::add_gui_frame(PluginClientFrame *frame)
{
	frame_buffer.append(frame);
// printf("PluginClient::add_gui_frame %d frame_buffer=%p edl_position=%ld total=%d\n", 
// __LINE__,
// &frame_buffer,
// frame->edl_position,
// frame_buffer.size());
}

int PluginAClient::get_gui_frames()
{
    return frame_buffer.size();
}

int PluginAClient::pending_gui_frames()
{
//printf("PluginAClient::pending_gui_frames %d source_position=%ld frame_buffer.size=%d\n", 
//__LINE__, source_position, frame_buffer.size());
    if(frame_buffer.size())
	{
        int total = 0;
        for(int i = 0; i < frame_buffer.size(); i++)
        {
// in the GUI instance, the source_position is the playhead position in the top
// samplerate
		    PluginClientFrame *frame = frame_buffer.get(i);
//printf("%ld ", frame->edl_position);
            if(direction == PLAY_FORWARD && frame->edl_position <= source_position ||
                direction == PLAY_REVERSE && frame->edl_position >= source_position)
            {
		        total++;
            }
        }
//printf("\n");

        return total;
	}
	else
	{
		return 0;
	}
}

PluginClientFrame* PluginAClient::get_gui_frame()
{
	if(frame_buffer.size())
	{
// in the GUI instance, the source_position is the playhead position in the EDL
// samplerate
		PluginClientFrame *frame = frame_buffer.get(0);

        if(direction == PLAY_FORWARD && frame->edl_position <= source_position ||
            direction == PLAY_REVERSE && frame->edl_position >= source_position)
        {
		    frame_buffer.remove_number(0);
		    return frame;
        }
        
        return 0;
	}
	else
	{
		return 0;
	}
}




void PluginAClient::begin_process_buffer()
{
// Delete all unused GUI frames
	frame_buffer.remove_all_objects();
}


void PluginAClient::end_process_buffer()
{
	if(frame_buffer.size())
	{
// send the frame buffer as a void data object to the GUI instance
    	server->send_render_gui(&frame_buffer);
//		send_render_gui();
	}
}










int PluginAClient::get_project_samplerate()
{
	return project_sample_rate;
}

int PluginAClient::get_samplerate()
{
	return sample_rate;
}

Samples* PluginAClient::get_output(int channel)
{
    return output_buffers[channel];
}




