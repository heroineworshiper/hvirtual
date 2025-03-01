
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
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

#include "audiocine.h"



AudioCine::AudioCine(AudioDevice *device)
 : AudioLowLevel(device)
{
}


AudioCine::~AudioCine()
{
}


int AudioCine::open_input()
{
    return 0;
}

int AudioCine::open_output()
{
    return 0;
}

int AudioCine::write_buffer(char *buffer, int size)
{
    return 0;
}

int AudioCine::read_buffer(char *buffer, int size)
{
    return 0;
}

int AudioCine::close_all()
{
    return 0;
}

int64_t AudioCine::device_position()
{
    return 0;
}

int AudioCine::flush_device()
{
    return 0;
}

int AudioCine::interrupt_playback()
{
    return 0;
}





