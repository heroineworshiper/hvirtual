/*
 * CINELERRA
 * Copyright (C) 2025 Adam Williams <broadcast at earthling dot net>
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


// asynchronously read files

#ifndef FILESPOOLER_H
#define FILESPOOLER_H

#include "condition.inc"
#include "mutex.inc"
#include "thread.h"
#include <stdint.h>
#include <stdio.h>


class FileSpooler : public Thread
{
public:
    FileSpooler();
    ~FileSpooler();

    int open(const char *path);
    int read(uint8_t *buffer, int size);
    void seek(int64_t offset);
    void run();

    FILE *fd;
    Mutex *buffer_lock;
    Condition *command_lock;
    Condition *output_lock; 
    int command;
    int64_t command_offset;
    uint8_t *buffer;
// position in the buffer of the input
    int64_t in_ptr;
// position in the buffer of the output
    int64_t out_ptr;
// position in the file of out_ptr
    int64_t out_position;
    int buffer_filled;
    int error;
};




#endif

















