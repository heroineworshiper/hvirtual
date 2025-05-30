/*
 * CINELERRA
 * Copyright (C) 2008-2024 Adam Williams <broadcast at earthling dot net>
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

#include "aedit.h"
#include "amodule.h"
#include "arender.h"
#include "assets.h"
#include "atrack.h"
#include "audiodevice.h"
#include "bcsignals.h"
#include "condition.h"
#include "edit.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "levelwindow.h"

#include "playabletracks.h"
#include "plugin.h"
#include "preferences.h"
#include "renderengine.h"
#include "samples.h"
#include "thread.h"
#include "tracks.h"
#include "transportque.h"
#include "virtualaconsole.h"
#include "virtualanode.h"
#include "virtualnode.h"


VirtualAConsole::VirtualAConsole(RenderEngine *renderengine, ARender *arender)
 : VirtualConsole(renderengine, arender, TRACK_AUDIO)
{
	this->arender = arender;
	output_temp = 0;
    bzero(chopper_buf, sizeof(Samples*) * MAX_CHANNELS);
    bzero(chopper_count, sizeof(double) * MAX_CHANNELS);
    bzero(fastfwd_accum, sizeof(double) * MAX_CHANNELS);
    bzero(fastfwd_count, sizeof(double) * MAX_CHANNELS);
}

VirtualAConsole::~VirtualAConsole()
{
	delete output_temp;
    for(int i = 0; i < MAX_CHANNELS; i++)
    {
        delete chopper_buf[i];
    }
}


// void VirtualAConsole::get_playable_tracks()
// {
// 	if(!playable_tracks)
// 		playable_tracks = new PlayableTracks(renderengine->get_edl(), 
// 			commonrender->current_position, 
// 			renderengine->command->get_direction(),
// 			TRACK_AUDIO,
// 			1);
// }


VirtualNode* VirtualAConsole::new_entry_node(Track *track, 
	Module *module,
	int track_number)
{
	return new VirtualANode(renderengine,
		this, 
		module,
		0,
		track,
		0);
	return 0;
}



int VirtualAConsole::process_buffer(int64_t len,
	int64_t start_position)
{
	int result = 0;
	const int debug = 0;
if(debug) printf("VirtualAConsole::process_buffer %d this=%p len=%d\n", 
__LINE__, 
this,
(int)len);


// clear output buffers
	for(int i = 0; i < MAX_CHANNELS; i++)
	{
// if(debug) printf("VirtualAConsole::process_buffer 2 %d %p %lld\n", 
// i, 
// arender->audio_out[i],
// len);

		if(arender->audio_out[i])
		{
			bzero(arender->audio_out[i]->get_data(), len * sizeof(double));
		}
	}

// Create temporary output
	if(output_temp && output_temp->get_allocated() < len)
	{
		delete output_temp;
		output_temp = 0;
	}

	if(!output_temp)
	{
		output_temp = new Samples(len, 1);
	}
if(debug) printf("VirtualAConsole::process_buffer %d\n", __LINE__);


// Reset plugin rendering status
	reset_attachments();
//printf("VirtualAConsole::process_buffer 1 %p\n", output_temp);

//printf("VirtualAConsole::process_buffer %d\n", __LINE__);


// Render exit nodes
    int sample_rate = renderengine->get_edl()->session->sample_rate;
	for(int i = 0; i < exit_nodes.total; i++)
	{
		VirtualANode *node = (VirtualANode*)exit_nodes.values[i];
		Track *track = node->track;

		result |= node->render(output_temp, 
			len,
			start_position + track->nudge,
			renderengine->get_edl()->session->sample_rate,
            (double)renderengine->arender->current_position / sample_rate);
	}
//printf("VirtualAConsole::process_buffer %d\n", __LINE__);



// get peaks and limit volume in the fragment
	for(int i = 0; i < MAX_CHANNELS; i++)
	{
		if(arender->audio_out[i])
		{
			double *audio_out = arender->audio_out[i]->get_data();


			for(int j = 0; j < len; )
			{
				int meter_render_end;
// Get length to test for meter and limit
				if(renderengine->command->realtime)
				{
                	meter_render_end = j + arender->meter_render_fragment;
				}
                else
				{
                	meter_render_end = len;
                }

				if(meter_render_end > len) 
				{
                	meter_render_end =  len;
                }


				double peak = 0;

				for( ; j < meter_render_end; j++)
				{
// Level history comes before clipping to get over status
					double *sample = &audio_out[j];


					if(fabs(*sample) > peak) peak = fabs(*sample);
// Make the output device clip it
// 					if(*sample > 1) *sample = 1;
// 					else
// 					if(*sample < -1) *sample = -1;
				}


 				if(renderengine->command->realtime)
 				{
					arender->level_history[i][arender->current_level[i]] = peak;
					arender->level_samples[arender->current_level[i]] = 
						renderengine->command->get_direction() == PLAY_REVERSE ? 
						start_position - j : 
						start_position + j;
 					arender->current_level[i] = arender->get_next_peak(arender->current_level[i]);
 				}
			}
		}
	}

if(debug) printf("VirtualAConsole::process_buffer %d\n", __LINE__);





// Fix speed and send to device.
	if(!renderengine->is_nested &&
		renderengine->command->realtime && 
		!interrupt)
	{
// speed parameters
// length compensated for speed
		int real_output_len = 0;
// output sample
		double sample;
		int k;
		double *audio_out_planar[MAX_CHANNELS];
		int audio_channels = renderengine->get_edl()->session->audio_channels;

		for(int i = 0; 
			i < audio_channels; 
			i++)
		{
			int in = 0;
            int out = 0;
			int fragment_end;

			audio_out_planar[i] = arender->audio_out[i]->get_data();
			double *audio_out = audio_out_planar[i];

// Time stretch the fragment to the real_output size
            double speed = renderengine->command->get_speed();
			if(speed > 1)
			{
// printf("VirtualAConsole::process_buffer %d scrub_chop=%d\n", 
// __LINE__,
// renderengine->preferences->scrub_chop);
                if(renderengine->preferences->scrub_chop)
                {
// split output len into smaller windows to make the chopping intelligible
                    double chopper_window = sample_rate * speed / 40;

                    double drop_count = 0;
// length of overlapping section of each window
                    double dissolve_count = 0;
                    double total_in = 0;
                    if(speed >= 2.0)
                    {
// for speeds over 2x, drop samples between overlapping windows
                        dissolve_count = chopper_window / speed;
                        drop_count = chopper_window - dissolve_count * 2;
                    }
                    else
                    {
// for speeds under 2x, all of the input is used & 
// the 2 windows overlap slightly
                        dissolve_count = chopper_window - chopper_window / speed;
                        drop_count = chopper_window / speed - dissolve_count;
                    }
                    total_in = drop_count + dissolve_count * 2;
// amount to dissolve & buffer
                    int dissolve_count_i = (int)ceil(dissolve_count);
// printf("VirtualAConsole::process_buffer %d chopper_window=%f dissolve_count_i=%d drop_count=%f dissolve_count=%f total_in=%f total_out=%f\n", 
// __LINE__,
// chopper_window,
// dissolve_count_i,
// drop_count,
// dissolve_count,
// total_in,
// (speed > 2.0) ? dissolve_count : drop_count + dissolve_count);

                    if(chopper_buf[i] && chopper_buf[i]->get_allocated() < dissolve_count_i)
                    {
                        delete chopper_buf[i];
                        chopper_buf[i] = 0;
                    }
                    if(!chopper_buf[i])
                    {
                        chopper_buf[i] = new Samples(dissolve_count_i, 0);
                        bzero(chopper_buf[i]->get_data(), dissolve_count_i * sizeof(double));
                    }
                    double *current_buf = chopper_buf[i]->get_data();
                    double *current_counter = &chopper_count[i];
                    int offset;
                    for(in = 0; in < len; in++)
                    {
                        if(*current_counter < drop_count)
                        {
                            if(speed < 2.0)
                                audio_out[out++] = audio_out[in];
                        }
                        else
                        {
                            int do_copy = 0;
                            if(*current_counter < drop_count + dissolve_count)
                            {
                                offset = (int)(*current_counter - drop_count);
                                if(speed < 2.0)
                                    do_copy = 1;
                            }
                            else
                            {
                                offset = (int)(*current_counter - drop_count - dissolve_count);
                                if(speed >= 2.0)
                                    do_copy = 1;
                            }

                            if(do_copy)
                            {
// copy new dissolve_count to chopper buffer
                                current_buf[offset] = audio_out[in];
                            }
                            else
                            {
// blend dissolve_count of new samples with chopper buffer & send the samples
                                double fraction = (double)offset /
                                    dissolve_count;
                                current_buf[offset] = current_buf[offset] * (1.0 - fraction) +
                                    audio_out[in] * fraction;
                                audio_out[out++] = current_buf[offset];
                            }
                        }

                        (*current_counter)++;
                        if(*current_counter >= total_in)
                            *current_counter -= total_in;
                    }
// output drops 1 window in 1.5x & 3x
                    real_output_len = out;
//printf("VirtualAConsole::process_buffer %d real_output_len=%d\n", __LINE__, real_output_len);
                }
                else
                {
				    double *current_accum = &fastfwd_accum[i];
                    double *current_count = &fastfwd_count[i];
                    for(in = 0, out = 0; in < len; in++)
				    {
                        double sample = audio_out[in];
                        *current_count += 1.0;
                        double remane_count = *current_count - speed;
                        if(remane_count >= 0)
                        {
// output a sample
                            *current_accum += sample * (1.0 - remane_count);
                            *current_accum /= speed;
                            audio_out[out++] = *current_accum;
                            *current_accum = sample * remane_count;
                            *current_count = remane_count;
                        }
                        else
                        {
// accumulate a sample
                            *current_accum += sample;
                        }
				    }
				    real_output_len = out;
                }
			}
			else
			if(speed < 1)
			{
// number of samples to skip
 				int interpolate_len = (int)(1.0 / speed);
				real_output_len = len * interpolate_len;

				for(in = len - 1, out = real_output_len - 1; in >= 0; )
				{
					for(k = 0; k < interpolate_len; k++)
					{
						audio_out[out--] = audio_out[in];
					}
					in--;
				}
			}
			else
				real_output_len = len;
		}

// Wait until video is ready
		if(arender->first_buffer)
		{
			renderengine->first_frame_lock->lock("VirtualAConsole::process_buffer");
			arender->first_buffer = 0;
		}
		if(!renderengine->adevice->get_interrupted())
		{
if(debug) printf("VirtualAConsole::process_buffer %d real_output_len=%d\n", __LINE__, real_output_len);
			renderengine->adevice->write_buffer(audio_out_planar, 
				real_output_len);
if(debug) printf("VirtualAConsole::process_buffer %d\n", __LINE__);
		}

		if(renderengine->adevice->get_interrupted()) interrupt = 1;
	}

if(debug) printf("VirtualAConsole::process_buffer %d\n", __LINE__);





	return result;
}

























int VirtualAConsole::init_rendering(int duplicate)
{
	return 0;
}


int VirtualAConsole::send_last_output_buffer()
{
	renderengine->adevice->set_last_buffer();
	return 0;
}

