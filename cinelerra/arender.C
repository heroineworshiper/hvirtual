/*
 * CINELERRA
 * Copyright (C) 2009-2024 Adam Williams <broadcast at earthling dot net>
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

#include "amodule.h"
#include "arender.h"
#include "atrack.h"
#include "atrack.h"
#include "audiodevice.h"
#include "auto.h"
#include "autos.h"
#include "bcsignals.h"
#include "cache.h"
#include "condition.h"
#include "edit.h"
#include "edl.h"
#include "edlsession.h"
#include "levelwindow.h"
#include "mainsession.h"
#include "playabletracks.h"
#include "playbackengine.h"
#include "preferences.h"
#include "renderengine.h"
#include "samples.h"
#include "tracks.h"
#include "transportque.h"
#include "virtualaconsole.h"
#include "virtualconsole.h"
#include "virtualnode.h"

ARender::ARender(RenderEngine *renderengine)
 : CommonRender(renderengine)
{
// Clear output buffers
	for(int i = 0; i < MAXCHANNELS; i++)
	{
		buffer[i] = 0;
		audio_out[i] = 0;
		buffer_allocated[i] = 0;
		level_history[i] = 0;
	}
	level_samples = 0;
	total_peaks = 0;

	data_type = TRACK_AUDIO;
}

ARender::~ARender()
{
	for(int i = 0; i < MAXCHANNELS; i++)
	{
		if(buffer[i]) delete buffer[i];
		if(level_history[i]) delete [] level_history[i];
	}
	if(level_samples) delete [] level_samples;
}

void ARender::arm_command()
{
// Need the meter history now so AModule can allocate its own history
	calculate_history_size();
	CommonRender::arm_command();
	asynchronous = 1;
	init_meters();
}


int ARender::get_total_tracks()
{
	return renderengine->get_edl()->tracks->total_audio_tracks();
}

Module* ARender::new_module(Track *track)
{
	return new AModule(renderengine, this, 0, track);
}

int ARender::calculate_history_size()
{
	if(total_peaks > 0)
		return total_peaks;
	else
	{
		meter_render_fragment = renderengine->fragment_len;
// This number and the timer in tracking.C determine the rate
		while(meter_render_fragment > 
			renderengine->get_edl()->session->sample_rate / TRACKING_RATE) 
			meter_render_fragment /= 2;
		total_peaks = 16 * 
			renderengine->fragment_len / 
			meter_render_fragment;
		return total_peaks;
	}
}

int ARender::init_meters()
{
// not providing enough peaks results in peaks that are ahead of the sound
	if(level_samples) delete [] level_samples;
	calculate_history_size();
	level_samples = new int64_t[total_peaks];

	for(int i = 0; i < MAXCHANNELS;i++)
	{
		current_level[i] = 0;
		if(buffer[i] && !level_history[i]) 
			level_history[i] = new double[total_peaks];
	}

	for(int i = 0; i < total_peaks; i++)
	{
		level_samples[i] = -1;
	}
	
	for(int j = 0; j < MAXCHANNELS; j++)
	{
		if(buffer[j])
			for(int i = 0; i < total_peaks; i++)
				level_history[j][i] = 0;
	}
	return 0;
}

void ARender::allocate_buffers(int samples)
{
	for(int i = 0; i < MAXCHANNELS; i++)
	{
// Reset the output buffers in case speed changed
		if(buffer_allocated[i] < samples)
		{
			delete buffer[i];
			buffer[i] = 0;
		}

		if(i < renderengine->get_edl()->session->audio_channels)
		{
			buffer[i] = new Samples(samples);
			buffer_allocated[i] = samples;
			audio_out[i] = buffer[i];
		}
	}
}

void ARender::init_output_buffers()
{
	allocate_buffers(renderengine->adjusted_fragment_len);
}


VirtualConsole* ARender::new_vconsole_object() 
{ 
	return new VirtualAConsole(renderengine, this);
}

int64_t ARender::tounits(double position, int round)
{
	if(round)
		return Units::round(position * renderengine->get_edl()->session->sample_rate);
	else
		return (int64_t)(position * renderengine->get_edl()->session->sample_rate);
}

double ARender::fromunits(int64_t position)
{
	return (double)position / renderengine->get_edl()->session->sample_rate;
}


int ARender::process_buffer(Samples **buffer_out, 
	int64_t input_len,
	int64_t input_position)
{
	int result = 0;

	int64_t fragment_position = 0;
	int64_t fragment_len = input_len;
	int reconfigure = 0;
	current_position = input_position;

// Process in fragments
	int start_offset = buffer_out[0]->get_offset();
	while(fragment_position < input_len)
	{
// Set pointers for destination data
		for(int i = 0; i < MAXCHANNELS; i++)
		{
			if(buffer_out[i])
			{
				this->audio_out[i] = buffer_out[i];
				this->audio_out[i]->set_offset(start_offset + fragment_position);
			}
			else
				this->audio_out[i] = 0;
		}

		fragment_len = input_len;
		if(fragment_position + fragment_len > input_len)
			fragment_len = input_len - fragment_position;

		reconfigure = vconsole->test_reconfigure(input_position, 
			fragment_len);

printf("ARender::process_buffer %d input_position=%d fragment_len=%d reconfigure=%d\n", 
__LINE__, (int)input_position, (int)fragment_len, reconfigure);

		if(reconfigure) restart_playback();

		result = process_buffer(fragment_len, input_position);

		fragment_position += fragment_len;
		input_position += fragment_len;
		current_position = input_position;
	}

// Reset offsets
	for(int i = 0; i < MAXCHANNELS; i++)
	{
		if(buffer_out[i]) buffer_out[i]->set_offset(start_offset);
	}



	return result;
}


int ARender::process_buffer(int64_t input_len, int64_t input_position)
{
// printf("ARender::process_buffer %d %p edl->nested_depth=%d\n", 
// __LINE__, 
// this, 
// renderengine->get_edl()->nested_depth);
	int result = ((VirtualAConsole*)vconsole)->process_buffer(input_len,
		input_position);
//printf("ARender::process_buffer %d input_len=%d input_position=%d\n", __LINE__, (int)input_len, (int)input_position);
	return result;
}

int ARender::get_history_number(int64_t *table, int64_t position)
{
// Get the entry closest to position
	int result = 0;
	int64_t min_difference = 0x7fffffff;
	for(int i = 0; i < total_peaks; i++)
	{

//printf("%lld ", table[i]);
		if(labs(table[i] - position) < min_difference)
		{
			min_difference = labs(table[i] - position);
			result = i;
		}
	}
//printf("\n");
//printf("ARender::get_history_number %lld %d\n", position, result);
	return result;
}

void ARender::send_last_buffer()
{
	renderengine->adevice->set_last_buffer();
}

int ARender::wait_device_completion()
{
// audio device should be entirely cleaned up by vconsole
	renderengine->adevice->wait_for_completion();
	return 0;
}

void ARender::run()
{
	int64_t current_input_length;
	int reconfigure = 0;
const int debug = 0;

	first_buffer = 1;

	start_lock->unlock();
if(debug) printf("ARender::run %d %d\n", __LINE__, Thread::calculate_realtime());

	while(!done && !interrupt)
	{
		current_input_length = renderengine->fragment_len;

if(debug) printf("ARender::run %d %d %d\n", __LINE__, (int)current_position, (int)current_input_length);
		get_boundaries(current_input_length);

if(debug) printf("ARender::run %d %d %d\n", __LINE__, (int)current_position, (int)current_input_length);
		if(current_input_length)
		{
			reconfigure = vconsole->test_reconfigure(current_position, 
				current_input_length);
			if(reconfigure) restart_playback();
		}
if(debug) printf("ARender::run %d %lld %lld\n", __LINE__, (long long)current_position, (long long)current_input_length);


// Update tracking if no video is playing.
		if(renderengine->command->realtime && 
			renderengine->playback_engine &&
			!renderengine->do_video)
		{
			double position = (double)renderengine->adevice->current_position() / 
				renderengine->get_edl()->session->sample_rate * 
				renderengine->command->get_speed();

			if(renderengine->command->get_direction() == PLAY_FORWARD) 
				position += renderengine->command->playbackstart;
			else
				position = renderengine->command->playbackstart - position;

// This number is not compensated for looping.  It's compensated in 
// PlaybackEngine::get_tracking_position when interpolation also happens.
			renderengine->playback_engine->update_tracking(position);
		}


if(debug) printf("ARender::run %d current_input_length=%d current_position=%d\n", 
__LINE__, 
(int)current_input_length,
(int)current_position);



		process_buffer(current_input_length, current_position);
if(debug) printf("ARender::run %d\n", __LINE__);


		advance_position(current_input_length);
if(debug) printf("ARender::run %d current_position=%d current_input_length=%d\n", 
__LINE__, 
(int)current_position,
(int)current_input_length);


		if(vconsole->interrupt) interrupt = 1;
	}

if(debug) printf("ARender::run %d\n", __LINE__);
	if(!interrupt) send_last_buffer();
	if(renderengine->command->realtime) wait_device_completion();
	vconsole->stop_rendering(0);
	stop_plugins();
}


int ARender::get_next_peak(int current_peak)
{
	current_peak++;
	if(current_peak >= total_peaks) current_peak = 0;
	return current_peak;
}









