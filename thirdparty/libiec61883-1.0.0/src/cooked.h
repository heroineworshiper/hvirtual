/*
 * libiec61883 - Linux IEEE 1394 streaming media library.
 * Copyright (C) 2004 Kristian Hogsberg, Dan Dennedy, and Dan Maas.
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

#include <libraw1394/raw1394.h>

int
iec61883_cooked_read(raw1394handle_t handle, nodeid_t node, nodeaddr_t addr,
        	size_t length, quadlet_t *buffer);

int
iec61883_cooked_write(raw1394handle_t handle, nodeid_t node, nodeaddr_t addr,
            size_t length, quadlet_t *data);

/*
 * constants for use with raw1394 ARM
 * The following probably should be in raw1394.h
 */

/* extended transaction codes (lock-request-response) */
#define EXTCODE_MASK_SWAP        0x1
/* IEC 61883 says plugs should only be manipulated using
 * compare_swap lock. */
#define EXTCODE_COMPARE_SWAP     0x2
#define EXTCODE_FETCH_ADD        0x3
#define EXTCODE_LITTLE_ADD       0x4
#define EXTCODE_BOUNDED_ADD      0x5
#define EXTCODE_WRAP_ADD         0x6

/* response codes */
#define RCODE_COMPLETE           0x0
#define RCODE_CONFLICT_ERROR     0x4
#define RCODE_DATA_ERROR         0x5
#define RCODE_TYPE_ERROR         0x6
#define RCODE_ADDRESS_ERROR      0x7
