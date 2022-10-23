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

#include "audiopulse.h"
#include "mutex.h"
#include "bctimer.h"
#include "playbackconfig.h"
#include "recordconfig.h"


#include <pulse/simple.h>
#include <pulse/error.h>


AudioPulse::AudioPulse(AudioDevice *device)
 : AudioLowLevel(device)
{
	samples_written = 0;
	timer = new Timer;
    timer_lock = new Mutex("AudioPulse::timer_lock");
    dsp_in = 0;
    dsp_out = 0;
    delay = 0;
}

AudioPulse::~AudioPulse()
{
    delete timer_lock;
	delete timer;
    close_all();
}



int AudioPulse::open_input()
{
    pa_sample_spec ss;
    int error;

    ss.format = PA_SAMPLE_S16LE;
    ss.rate = device->in_samplerate; 
    ss.channels = device->get_ichannels();   
	device->in_bits = 16;

    char *server = 0;
    if(device->in_config->pulse_in_server[0])
    {
        server = device->in_config->pulse_in_server;
    }
    
    dsp_in = pa_simple_new(server, 
        PROGRAM_NAME, 
        PA_STREAM_RECORD, 
        NULL, 
        "recording", 
        &ss, 
        NULL, 
        NULL, 
        &error);
    if(!dsp_in)
    {
        printf("AudioPulse::open_input %d: failed server=%s %s\n", 
            __LINE__, 
            server, 
            pa_strerror(error));
        return 1;
    }

    return 0;
}

int AudioPulse::open_output()
{
    pa_sample_spec ss;
    int error;

    ss.format = PA_SAMPLE_S16LE;
    ss.rate = device->out_samplerate;
    ss.channels = device->get_ochannels();
	device->out_bits = 16;

    char *server = 0;
    if(device->out_config->pulse_out_server[0])
    {
        server = device->out_config->pulse_out_server;
    }

    dsp_out = pa_simple_new(server, 
        PROGRAM_NAME, 
        PA_STREAM_PLAYBACK, 
        NULL, 
        "playback", 
        &ss, 
        NULL, 
        NULL, 
        &error);
    if(!dsp_out)
    {
        printf("AudioPulse::open_output %d: failed server=%s %s\n", 
            __LINE__, 
            server,
            pa_strerror(error));
        return 1;
    }


    pa_usec_t latency;
    latency = pa_simple_get_latency((pa_simple*)dsp_out, &error);
    if(latency < 0)
    {
        printf("AudioPulse::open_output %d: failed %s\n", 
            __LINE__, 
            pa_strerror(error));
    }
    delay = latency * device->out_samplerate / 1000000;
// supposed to be latency but seems to be dead
    device->device_buffer = 0;
//printf("AudioPulse::open_output %d latency=%ld delay=%ld\n", __LINE__, latency, delay);

    timer->update();
    return 0;
}


int AudioPulse::close_all()
{
    if(dsp_out)
    {
        pa_simple_free((pa_simple*)dsp_out);
    }
    
    if(dsp_in)
    {
        pa_simple_free((pa_simple*)dsp_in);
    }
    
    dsp_out = 0;
    dsp_in = 0;
    samples_written = 0;
    delay = 0;
    return 0;
}

int64_t AudioPulse::device_position()
{
	timer_lock->lock("AudioPulse::device_position");

// printf("AudioPulse::device_position %d: %d\n", 
// __LINE__,
// device->out_samplerate);

	int64_t result = samples_written + 
		timer->get_scaled_difference(device->out_samplerate) - 
		delay;
	timer_lock->unlock();
	return result;
}

int AudioPulse::write_buffer(char *buffer, int size)
{
    if(!dsp_out)
    {
        return 1;
    }

// printf("AudioPulse::write_buffer %d: %d %d\n", 
// __LINE__,
// device->out_bits,
// device->get_ochannels());
    int samples = size / (device->out_bits / 8) / device->get_ochannels();
    int error;
    int result = pa_simple_write((pa_simple*)dsp_out, buffer, (size_t)size, &error);
    if(result < 0)
    {
        printf("AudioPulse::write_buffer %d: %s\n", 
            __LINE__,
            pa_strerror(error));
        return 1;
    }
    
    timer_lock->lock("AudioPulse::write_buffer");
    samples_written += samples;
    timer->update();
    timer_lock->unlock();
    return 0;
}

int AudioPulse::read_buffer(char *buffer, int size)
{
    if(!dsp_in)
    {
        return 1;
    }
    
    int error;
    int result = pa_simple_read((pa_simple*)dsp_in, buffer, size, &error);
    if(result < 0)
    {
        printf("AudioPulse::read_buffer %d: %s\n", 
            __LINE__,
            pa_strerror(error));
        return 1;
    }
    
//printf("AudioPulse::read_buffer %d %d\n", __LINE__, size);
    
    return 0;
}

int AudioPulse::flush_device()
{
    if(dsp_out)
    {
        int error;
        pa_simple_drain((pa_simple*)dsp_out, &error);
    }
    return 0;
}

int AudioPulse::interrupt_playback()
{
    if(!dsp_out)
    {
        return 1;
    }
    
    int error;
    int result = pa_simple_flush((pa_simple*)dsp_out, &error);
    if(result < 0)
    {
        printf("AudioPulse::interrupt_playback %d: %s\n", 
            __LINE__,
            pa_strerror(error));
        return 1;
    }
    
    return 0;
}



