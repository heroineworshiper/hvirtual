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

#include "clip.h"
#include "condition.h"
#include "filespooler.h"
#include "mutex.h"
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>

// commands
#define READ 0
#define SEEK 1
#define CLOSE 2
#define BUFSIZE 0x100000

static __off_t (*lseek_func)(int, __off_t, int) = 0;


FileSpooler::FileSpooler()
{
    fd = 0;
    fileno = -1;
    buffer = new uint8_t[BUFSIZE];
    in_ptr = 0;
    out_ptr = 0;
    out_position = 0;
    buffer_filled = 0;
    command_lock = new Condition;
    buffer_lock = new Mutex;
    output_lock = new Condition;
    error = 0;
}


FileSpooler::~FileSpooler()
{
    if(fd || fileno >= 0)
    {
        command = CLOSE;
        command_lock->unlock();
        Thread::join();
    }

    if(fd)
        fclose(fd);

// return the position to where the reader thinks it is    
    if(fileno >= 0)
    {
 	    if(!lseek_func)
     	    lseek_func = (__off_t(*)(int, __off_t, int))dlsym(RTLD_NEXT, "lseek");
        (*lseek_func)(fileno, out_position, SEEK_SET);
    }

    delete [] buffer;
    delete command_lock;
    delete buffer_lock;
    delete output_lock;
}

int FileSpooler::open(const char *path)
{
    fd = fopen(path, "r");
    if(!fd) 
        return 1;
    else
        Thread::start();
    return 0;
}

int FileSpooler::open(int fileno)
{
    this->fileno = fileno;
    Thread::start();
    return fileno;
}

// unimplemented for fd
int64_t FileSpooler::seek(int64_t offset, int whence)
{
    if(fileno >= 0 || fd)
    {
        command = SEEK;
        seek_offset = offset;
        seek_whence = whence;
        command_lock->unlock();
// wait for it
        output_lock->lock();
        return out_position;
    }
    return 0;
}

int FileSpooler::read(uint8_t *buffer, int size)
{
    int offset = 0;
    if(!fd && fileno < 0) return 0;
    
    while(offset < size)
    {
        buffer_lock->lock();
        if(buffer_filled > 0)
        {
// copy previously read data
            int fragment = MIN(buffer_filled, size - offset);
            if(fragment + out_ptr > BUFSIZE) fragment = BUFSIZE - out_ptr;
            memcpy(buffer + offset, this->buffer + out_ptr, fragment);
            out_ptr += fragment;
            out_position += fragment;
            if(out_ptr >= BUFSIZE) out_ptr = 0;
            offset += fragment;
            buffer_filled -= fragment;
            buffer_lock->unlock();
        }
        else
        if(error)
        {
            buffer_lock->unlock();
//printf("FileSpooler::read %d ERROR size=%d offset=%d\n", __LINE__, size, offset);
            return offset;
        }
        else
        {
            buffer_lock->unlock();
            command = READ;
            command_lock->unlock();
// wait for more data
            output_lock->lock();
        }
    }

// fill in the background
    command = READ;
    command_lock->unlock();

//printf("FileSpooler::read %d size=%d offset=%d\n", __LINE__, size, offset);
    return offset;
}

void FileSpooler::run()
{
    while(1)
    {
        command_lock->lock();

//printf("FileSpooler::run %d command=%d\n", __LINE__, command);
        switch(command)
        {
            case READ:
                buffer_lock->lock();
                if(buffer_filled < BUFSIZE)
                {
                    int fragment = BUFSIZE - buffer_filled;
                    if(in_ptr + fragment > BUFSIZE) fragment = BUFSIZE - in_ptr;
                    buffer_lock->unlock();
                    int result = 0;
                    if(fd)
                        result = fread(buffer + in_ptr, 0, fragment, fd);
                    else
                    if(fileno >= 0)
                    {
                        static ssize_t (*func)(int, void *, size_t) = 0;
 	                    if(!func)
     	                    func = (ssize_t(*)(int, void *, size_t))dlsym(RTLD_NEXT, "read");
                        
                        result = (*func)(fileno, buffer + in_ptr, fragment);
//printf("FileSpooler::run %d fragment=%d\n", __LINE__, fragment);
                    }

                    if(result <= 0) error = 1;

                    if(!error)
                    {
                        buffer_lock->lock();
                        in_ptr += fragment;
                        if(in_ptr >= BUFSIZE) in_ptr = 0;
                        buffer_filled += fragment;
                        buffer_lock->unlock();
                    }
                }
                else
                    buffer_lock->unlock();

// release the user
                output_lock->unlock();
                break;

            case SEEK:
                buffer_lock->lock();
                if(fileno >= 0)
                {
 	                if(!lseek_func)
     	                lseek_func = (__off_t(*)(int, __off_t, int))dlsym(RTLD_NEXT, "lseek");

                    out_position = (*lseek_func)(fileno, seek_offset, seek_whence);
                    buffer_filled = 0;
                    in_ptr = 0;
                    out_ptr = 0;
                    error = 0;
//printf("FileSpooler::run %d: seeked to %ld\n", __LINE__, (long)out_position);
                }
                buffer_lock->unlock();

// release the user
                output_lock->unlock();
                break;

            case CLOSE:
                output_lock->unlock();
                return;
                break;
        }
    }
}






