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

#include "bctimer.h"
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

Timer::Timer(int interruptable)
{
    this->interruptable = interruptable;
    pipefd[0] = -1;
    pipefd[1] = -1;
    if(interruptable) reset_delay();

	update();
//printf("Timer::Timer %d %p\n", __LINE__, this);
}

Timer::~Timer()
{
//printf("Timer::~Timer %d %d %d\n", __LINE__, pipefd[0], pipefd[1]);
//printf("Timer::~Timer %d %p\n", __LINE__, this);
    if(interruptable) 
    {
        close(pipefd[0]);
        close(pipefd[1]);
    }
}

int Timer::update()
{
	gettimeofday(&current_time, 0);
	return 0;
}

void Timer::subtract(int64_t value)
{
	current_time.tv_usec += 1000 * (value % 1000);
	current_time.tv_sec += value / 1000 + current_time.tv_usec / 1000000;
	current_time.tv_usec %= 1000000;
	if(get_difference() < 0) current_time = new_time;
}

int64_t Timer::get_difference(struct timeval *result)
{
	gettimeofday(&new_time, 0);
	
	result->tv_usec = new_time.tv_usec - current_time.tv_usec;
	result->tv_sec = new_time.tv_sec - current_time.tv_sec;
	if(result->tv_usec < 0) 
	{
		result->tv_usec += 1000000; 
		result->tv_sec--; 
	}
	
	return (int64_t)result->tv_sec * 1000 + (int64_t)result->tv_usec / 1000;
}

int64_t Timer::get_difference(int update_it)
{
	gettimeofday(&new_time, 0);
    struct timeval new_time2 = new_time;

	new_time.tv_usec -= current_time.tv_usec;
	new_time.tv_sec -= current_time.tv_sec;
	if(new_time.tv_usec < 0)
	{
		new_time.tv_usec += 1000000;
		new_time.tv_sec--;
	}
    if(update_it) current_time = new_time2;

	return (int64_t)new_time.tv_sec * 1000 + 
		(int64_t)new_time.tv_usec / 1000;
}

int64_t Timer::get_scaled_difference(long denominator)
{
	get_difference(&new_time);
	return (int64_t)new_time.tv_sec * denominator + 
		(int64_t)((double)new_time.tv_usec / 1000000 * denominator);
}

int Timer::delay(int64_t milliseconds)
{
    if(interruptable) 
    {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(pipefd[0], &read_fds); // Monitor pipe read end
	    struct timeval timeout;
	    timeout.tv_sec = 0;
	    timeout.tv_usec = milliseconds * 1000;

//printf("Timer::delay %d %d %d\n", __LINE__, pipefd[0], pipefd[1]);
	    int result = select(pipefd[0] + 1, &read_fds,  NULL, NULL, &timeout);
//printf("Timer::delay %d result=%d %d %d\n", __LINE__, result, pipefd[0], pipefd[1]);
// timed out
        if(result == 0) return 0;
        return 1;
    }
    else
    {
        usleep(milliseconds * 1000);
    }
    return 0;
}

void Timer::cancel_delay()
{
    if(interruptable)
    {
//printf("Timer::cancel_delay %d\n", __LINE__);
        uint8_t buf = 'x';
        int _ = write(pipefd[1], &buf, 1);
    }
}

void Timer::reset_delay()
{
    if(interruptable)
    {
        if(pipefd[0] >= 0)
        {
            close(pipefd[0]);
            close(pipefd[1]);
        }

    // create a new pipe
        int result = pipe(pipefd);
//printf("Timer::reset_delay %d result=%d %d %d\n", 
//__LINE__, result, pipefd[0], pipefd[1]);
    }
}


