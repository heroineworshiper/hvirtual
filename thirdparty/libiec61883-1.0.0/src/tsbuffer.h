/*
 * libiec61883 - Linux IEEE 1394 streaming media library.
 * Copyright (C) 2004 Kristian Hogsberg, Dan Dennedy, and Dan Maas.
 * This file orginially written in C++ by Dan Maas; ported to C by Dan Dennedy.
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
 
#ifndef _TSBUFFER_H
#define _TSBUFFER_H

#include "iec61883.h"

struct tsbuffer;
typedef struct tsbuffer* tsbuffer_t;

#ifdef __cplusplus
extern "C" {
#endif

tsbuffer_t
tsbuffer_init (iec61883_mpeg2_xmit_t read_cb, void *callback_data, int pid);
	
void
tsbuffer_close (tsbuffer_t self);
	
void 
tsbuffer_set_pid (tsbuffer_t self, int pid);
	
// read one MPEG-2 TS packet from the input fd
// and stick it on the end of ts_queue
int
tsbuffer_read_ts (tsbuffer_t self);

// read packets into ts_queue until we find one with a PCR
int
tsbuffer_read_to_next_pcr (tsbuffer_t self);

// refill the queue and update timestamper state
int
tsbuffer_refill (tsbuffer_t self);

// output one ISO cycle's worth of packets
// returns the total length of the ISO data
unsigned int tsbuffer_send_iso_cycle (tsbuffer_t self, void *data, 
	unsigned int iso_cycle, unsigned char src_node_id, unsigned int dropped);

#ifdef __cplusplus
}
#endif

#endif /* _TSBUFFER_H */
