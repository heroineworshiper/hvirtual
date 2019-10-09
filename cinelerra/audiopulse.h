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

#ifndef AUDIOPULSE_H
#define AUDIOPULSE_H

#include "audiodevice.h"
#include "bctimer.inc"

class AudioPulse : public AudioLowLevel
{
public:
    AudioPulse(AudioDevice *device);
    ~AudioPulse();
    
    int open_input();
    int open_output();
    int close_all();
    int64_t device_position();
    int write_buffer(char *buffer, int size);
    int read_buffer(char *buffer, int size);
    int flush_device();
    int interrupt_playback();

// the pulse audio handles
    void *dsp_out;
    void *dsp_in;
	int64_t samples_written;
    int64_t delay;
	Timer *timer;
    Mutex *timer_lock;
};





#endif


