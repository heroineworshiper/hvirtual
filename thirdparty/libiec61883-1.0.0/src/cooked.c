/*
 * libiec61883 - Linux IEEE 1394 streaming media library.
 * Copyright (C) 2004 Kristian Hogsberg, Dan Dennedy, and Dan Maas.
 * This code originally written by Andreas Micklei, then revised over the years
 * by Dan Dennedy.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "cooked.h"

#include <errno.h>
#include <time.h>

/*
 * constants for use with raw1394 ARM
 * The following probably should be in raw1394.h
 */


/* maximum number of retry attempts on raw1394 async transactions */
#define MAXTRIES 20

/* amount of delay in nanoseconds to wait before retrying raw1394 async transaction */
#define RETRY_DELAY 20000

int
iec61883_cooked_read(raw1394handle_t handle, nodeid_t node, nodeaddr_t addr,
                    size_t length, quadlet_t *buffer)
{
    int retval, i;
	struct timespec ts = {0, RETRY_DELAY};
    for (i = 0; i < MAXTRIES; i++) {
        retval = raw1394_read (handle, node, addr, length, buffer);
        if (retval < 0 && errno == EAGAIN)
            nanosleep(&ts, NULL);
        else
            return retval;
    }
    return -1;
}

int
iec61883_cooked_write(raw1394handle_t handle, nodeid_t node, nodeaddr_t addr,
                     size_t length, quadlet_t *data)
{
    int retval, i;
	struct timespec ts = {0, RETRY_DELAY};
    for (i = 0; i < MAXTRIES; i++) {
        retval = raw1394_write (handle, node, addr, length, data);
        if (retval < 0 && errno == EAGAIN)
            nanosleep(&ts, NULL);
        else
            return retval;
    }
    return -1;
}
