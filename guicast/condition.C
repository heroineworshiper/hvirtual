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

#ifndef NO_GUICAST
#include "bcsignals.h"
#endif
#include "condition.h"

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>
#include <unistd.h>

Condition::Condition(int init_value, const char *title, int is_binary)
{
	this->is_binary = is_binary;
	this->title = title;
	pthread_mutex_init(&mutex, 0);
	pthread_condattr_t cond_attr;
	pthread_condattr_init(&cond_attr);
	pthread_condattr_setclock(&cond_attr, CLOCK_MONOTONIC);
	pthread_cond_init(&cond, &cond_attr);
	this->value = this->init_value = init_value;
}

Condition:: ~Condition()
{
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);
#ifndef NO_GUICAST
	UNSET_ALL_LOCKS(this);
#endif
}

void Condition::reset()
{
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);
	pthread_mutex_init(&mutex, 0);
	pthread_cond_init(&cond, NULL);
	value = init_value;
}

void Condition::lock(const char *location)
{
#ifndef NO_GUICAST
	SET_LOCK(this, title, location);
#endif
    pthread_mutex_lock(&mutex);
    while(value <= 0) pthread_cond_wait(&cond, &mutex);
#ifndef NO_GUICAST
	UNSET_LOCK2
#endif
	if(is_binary)
		value = 0;
	else
		value--;
    pthread_mutex_unlock(&mutex);
}

void Condition::unlock()
{
// The lock trace is created and removed by the acquirer
//#ifndef NO_GUICAST
//	UNSET_LOCK(this);
//#endif
    pthread_mutex_lock(&mutex);
    if(is_binary)
		value = 1;
	else
		value++;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
}

int Condition::timed_lock(int microseconds, const char *location)
{
#ifndef NO_GUICAST
	SET_LOCK(this, title, location);
#endif
    struct timespec after;
    struct timespec before;
    clock_gettime(CLOCK_MONOTONIC, &before);
    int64_t nsec2 = (int64_t)before.tv_nsec + (int64_t)microseconds * (int64_t)1000;
    int64_t sec2 = (int64_t)before.tv_sec + (int64_t)nsec2 / (int64_t)1000000000;
    nsec2 %= (int64_t)1000000000;
    after.tv_nsec = nsec2;
    after.tv_sec = sec2;
    int result = 0;

    pthread_mutex_lock(&mutex);
    while(value <= 0)
	{
		result = pthread_cond_timedwait(&cond, &mutex, &after);

        if(result == ETIMEDOUT) 
	    {
            pthread_mutex_unlock(&mutex);
#ifndef NO_GUICAST
		    UNSET_LOCK2
#endif
		    return 1;
        } 
    }

	if(is_binary)
		value = 0;
	else
		value--;
    pthread_mutex_unlock(&mutex);

	result = 0;
#ifndef NO_GUICAST
	UNSET_LOCK2
#endif
	return result;
}


int Condition::get_value()
{
	return value;
}
