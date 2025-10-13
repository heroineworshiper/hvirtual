/*
 * CINELERRA
 * Copyright (C) 2008-2025 Adam Williams <broadcast at earthling dot net>
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


#include "samples.h"
#include "timestretch.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>


TimeStretch::TimeStretch()
{
    chopper_buf = 0;
    chopper_count = 0;
    dissolve_count = 0;
    drop_count = 0;
    dissolve_count0 = 0;
    drop_count0 = 0;
    drop_remane = 0;
    dissolve_remane = 0;

// temporaries for interpolating fast forward
    fastfwd_accum = 0;
    fastfwd_count = 0;
}

TimeStretch::~TimeStretch()
{
    delete chopper_buf;
}

int TimeStretch::process(double *audio_out,
    double speed,
    int len,
    int scrub_chop,
    int sample_rate)
{
	int in = 0;
    int out = 0;
	int fragment_end;
    int real_output_len = 0;
// static int debug_in = 0;
// static int debug_out = 0;
// debug_in += len;

// Time stretch the fragment to the real_output size
	if(speed > 1)
	{
// printf("TimeStretch::process_buffer %d scrub_chop=%d\n", 
// __LINE__,
// scrub_chop);
        if(scrub_chop)
        {
// split output len into smaller windows to make the chopping intelligible
// must use longer fragments for lower speeds for intelligibility
            int denominator = 40;
            if(speed < 2.0) denominator = 20;
// sum of drop_count + 2 * dissolve_count must be this
            double chopper_window = (double)sample_rate * speed / denominator;

            double dissolve_count1;
            double drop_count1;

            if(speed >= 2.0)
            {
// for speeds over 2x, drop samples between overlapping windows
                dissolve_count1 = chopper_window / speed;
                drop_count1 = chopper_window - dissolve_count1 * 2;
            }
            else
            {
// for speeds under 2x, all of the input is used & 
// the 2 windows overlap slightly
                dissolve_count1 = chopper_window - chopper_window / speed;
                drop_count1 = chopper_window / speed - dissolve_count1;
            }


// speed changed.  Reset the bits
            if(dissolve_count1 != dissolve_count0 ||
                drop_count1 != drop_count0)
            {
                dissolve_count = (int)dissolve_count1;
                drop_count = (int)drop_count1;
                drop_remane = 0;
                dissolve_remane = 0;
                chopper_count = 0;
            }
            dissolve_count0 = dissolve_count1;
            drop_count0 = drop_count1;
// total remaneders after every window
            double drop_remane0 = drop_count0 - (int)drop_count0;
            double dissolve_remane0 = dissolve_count0 - (int)dissolve_count0;


// printf("TimeStretch::process_buffer %d chopper_window=%f drop_count=%d dissolve_count=%d\n", 
// __LINE__,
// chopper_window,
// drop_count,
// dissolve_count);
            int dissolve_allocate = (int)(dissolve_count0 + 1);
            if(chopper_buf && chopper_buf->get_allocated() < dissolve_allocate)
            {
                delete chopper_buf;
                chopper_buf = 0;
            }
            if(!chopper_buf)
            {
                chopper_buf = new Samples(dissolve_allocate, 0);
                chopper_buf->clear();
            }

            double *current_buf = chopper_buf->get_data();


// printf("TimeStretch::process_buffer %d dissolve_count0=%f drop_count0=%f\n", 
// __LINE__, dissolve_count0, drop_count0);
            int offset;
            for(in = 0; in < len; in++)
            {
// dissolves must apply to the same window so the dissolve buffer can change size
// between window
                if(speed > 2.0)
                {
                    if(chopper_count < dissolve_count)
                    {
// store outgoing dissolve buffer
                        current_buf[chopper_count] = audio_out[in];
                    }
                    else
                    if(chopper_count < dissolve_count + drop_count)
                    {
// drop the drop count
                    }
                    else
                    {
// blend incoming dissolve buffer
                        offset = chopper_count - drop_count - dissolve_count;
                        double fraction = (double)offset /
                            dissolve_count;
                        audio_out[out++] = current_buf[offset] * (1.0 - fraction) +
                            audio_out[in] * fraction;
                    }
                }
                else
                {
                    if(chopper_count < drop_count)
                    {
// copy the drop count
                        audio_out[out++] = audio_out[in];
                    }
                    else
                    if(chopper_count < drop_count + dissolve_count)
                    {
// store outgoing dissolve buffer
                        offset = chopper_count - drop_count;
                        current_buf[offset] = audio_out[in];
                    }
                    else
                    {
// blend incoming dissolve buffer
                        offset = chopper_count - drop_count - dissolve_count;
                        double fraction = (double)offset /
                            dissolve_count;
                        audio_out[out++] = current_buf[offset] * (1.0 - fraction) +
                            audio_out[in] * fraction;
                    }
                }

                chopper_count++;
                if(chopper_count >= drop_count + dissolve_count * 2)
                {
                    chopper_count = 0;
                    drop_count = (int)drop_count0;
                    dissolve_count = (int)dissolve_count0;

// accumulate the remaneders
                    drop_remane += drop_remane0;
                    dissolve_remane += dissolve_remane0;

// apply integer parts of remaneders to their sections
                    if(abs((int)drop_remane) > 0)
                    {
                        drop_count += (int)drop_remane;
                        drop_remane -= (int)drop_remane;
                    }

                    if(abs((int)dissolve_remane) > 0)
                    {
                        dissolve_count += (int)dissolve_remane;
                        dissolve_remane -= (int)dissolve_remane;
                    }
                }
            }

// output drops 1 window in 1.5x & 3x
            real_output_len = out;
//printf("TimeStretch::process_buffer %d real_output_len=%d\n", __LINE__, real_output_len);
// debug_out += real_output_len;
// printf("TimeStretch::process_buffer %d expected out=%d real out=%d error=%d\n", 
// __LINE__, (int)(debug_in / speed), debug_out, (int)(debug_out - debug_in / speed));
        }
        else
        {
            for(in = 0, out = 0; in < len; in++)
			{
                double sample = audio_out[in];
                fastfwd_count += 1.0;
                double remane_count = fastfwd_count - speed;
                if(remane_count >= 0)
                {
// output a sample
                    fastfwd_accum += sample * (1.0 - remane_count);
                    fastfwd_accum /= speed;
                    audio_out[out++] = fastfwd_accum;
                    fastfwd_accum = sample * remane_count;
                    fastfwd_count = remane_count;
                }
                else
                {
// accumulate a sample
                    fastfwd_accum += sample;
                }
			}
			real_output_len = out;
        }
//printf("TimeStretch::process_buffer %d %f %f\n", 
//__LINE__, dissolve_remane, drop_remane);
	}
	else
	if(speed < 1)
	{
// number of samples to skip
 		int interpolate_len = (int)(1.0 / speed);
		real_output_len = len * interpolate_len;

		for(in = len - 1, out = real_output_len - 1; in >= 0; )
		{
			for(int k = 0; k < interpolate_len; k++)
			{
				audio_out[out--] = audio_out[in];
			}
			in--;
		}
	}
	else
		real_output_len = len;


    return real_output_len;
}




