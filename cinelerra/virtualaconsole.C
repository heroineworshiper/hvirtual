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
    bzero(fastfwd_tail, sizeof(Samples*) * MAX_CHANNELS);
}

VirtualAConsole::~VirtualAConsole()
{
	if(output_temp) delete output_temp;
    for(int i = 0; i < MAX_CHANNELS; i++)
    {
        delete fastfwd_tail[i];
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
if(debug) printf("VirtualAConsole::process_buffer %d this=%p len=%lld\n", 
__LINE__, 
this,
(long long)len);


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
			double *current_buffer = arender->audio_out[i]->get_data();


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
					double *sample = &current_buffer[j];


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
		int real_output_len;
// output sample
		double sample;
		int k;
		double *audio_out_planar[MAX_CHANNELS];
		int audio_channels = renderengine->get_edl()->session->audio_channels;

		for(int i = 0; 
			i < audio_channels; 
			i++)
		{
			int in, out;
			int fragment_end;

			audio_out_planar[i] = arender->audio_out[i]->get_data();
			double *current_buffer = audio_out_planar[i];

// Time stretch the fragment to the real_output size
			if(renderengine->command->get_speed() > 1)
			{
// printf("VirtualAConsole::process_buffer %d scrub_chop=%d\n", 
// __LINE__,
// renderengine->preferences->scrub_chop);
// Number of samples in real output buffer for each to sample rendered.
                if(renderengine->preferences->scrub_chop)
                {
// chop the buffer length
                    real_output_len = len / renderengine->command->get_speed();
                    if(real_output_len <= 0) real_output_len = 1;

// split output len into smaller windows to make the chopping intelligible
                    int fastfwd_window = real_output_len;
                    while(fastfwd_window >= sample_rate / 40 &&
                        fastfwd_window > 1) fastfwd_window /= 2;

                    if(fastfwd_tail[i] && fastfwd_tail[i]->get_allocated() < fastfwd_window)
                    {
                        delete fastfwd_tail[i];
                        fastfwd_tail[i] = 0;
                    }
                    if(!fastfwd_tail[i])
                    {
                        fastfwd_tail[i] = new Samples(fastfwd_window, 0);
                        bzero(fastfwd_tail[i]->get_data(), fastfwd_window * sizeof(double));
                    }
                    double *current_tail = fastfwd_tail[i]->get_data();

                    for(in = 0, out = 0; 
                        in + fastfwd_window <= len && out + fastfwd_window <= real_output_len; 
                        in += (int)(fastfwd_window * renderengine->command->get_speed()))
                    {
                        for(int j = 0; j < fastfwd_window; j++)
                        {
// fade out end of previous window while 
// fading in start of next window
                            float fraction = (float)j / fastfwd_window;
                            sample = current_buffer[in + j] * fraction;
                            sample += current_tail[j] * (1.0 - fraction);
                            current_buffer[out++] = sample;
                        }

                        if(in + fastfwd_window * 2 <= len)
                            memcpy(current_tail, 
                                current_buffer + in + fastfwd_window, 
                                fastfwd_window * sizeof(double));
                    }
                }
                else
                {
				    int interpolate_len = (int)renderengine->command->get_speed();
				    for(in = 0, out = 0; in < len; )
				    {
					    sample = 0;
					    for(k = 0; k < interpolate_len; k++)
					    {
						    sample += current_buffer[in++];
					    }

					    sample /= renderengine->command->get_speed();
					    current_buffer[out++] = sample;
				    }
				    real_output_len = out;
                }
			}
			else
			if(renderengine->command->get_speed() < 1)
			{
// number of samples to skip
 				int interpolate_len = (int)(1.0 / renderengine->command->get_speed());
				real_output_len = len * interpolate_len;

				for(in = len - 1, out = real_output_len - 1; in >= 0; )
				{
					for(k = 0; k < interpolate_len; k++)
					{
						current_buffer[out--] = current_buffer[in];
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
if(debug) printf("VirtualAConsole::process_buffer %d\n", __LINE__);
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

