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


#ifndef TIMESTRETCH_H
#define TIMESTRETCH_H

#include "samples.inc"


class TimeStretch
{
public:
    TimeStretch();
    ~TimeStretch();
// returns the stretched output length
    int process(double *audio_out,
        double speed,
        int len,
        int scrub_chop,
        int sample_rate);


// temporaries for crossfaded fast forward buffers
    Samples *chopper_buf;
// current position in the chopper window
    int chopper_count;
// current length of the sections
    int dissolve_count;
    int drop_count;
// accumulated remaneders
    double drop_remane;
    double dissolve_remane;
// total length of the sections with fractional parts
    double dissolve_count0;
    double drop_count0;

// temporaries for interpolating fast forward
    double fastfwd_accum;
    double fastfwd_count;
};





#endif // TIMESTRETCH_INC
