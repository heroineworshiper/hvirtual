/*
 * libiec61883 - Linux IEEE 1394 streaming media library.
 * Copyright (C) 2004 Kristian Hogsberg, Dan Dennedy, and Dan Maas.
 * This file written by Dan Dennedy.
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

#include "iec61883.h"
#include "iec61883-private.h"
#include "tsbuffer.h"

#include <errno.h>
#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <assert.h>

#define MAX_PACKET_SIZE 2048 /* max 1394 iso packet size */
#define TSP_SPH_SIZE 192 /* size of transport stream packet plus source packet header */

iec61883_mpeg2_t
iec61883_mpeg2_xmit_init(raw1394handle_t handle, 
		iec61883_mpeg2_xmit_t get_data,
		void *callback_data)
{
	struct iec61883_mpeg2 *mpeg;

	assert (handle != NULL);
	mpeg = malloc(sizeof(struct iec61883_mpeg2));
	if (!mpeg) {
		errno = ENOMEM;
		return NULL;
	}

	mpeg->tsbuffer = NULL;
	mpeg->handle = handle;
	mpeg->put_data = NULL;
	mpeg->get_data = get_data;
	mpeg->callback_data = callback_data;
	mpeg->buffer_packets = 1000;
	mpeg->prebuffer_packets = 1000;
	mpeg->irq_interval = 250;
	mpeg->synch = 0;
	mpeg->speed = RAW1394_ISO_SPEED_200;

	raw1394_set_userdata (handle, mpeg);
	
	return mpeg;
}

iec61883_mpeg2_t
iec61883_mpeg2_recv_init(raw1394handle_t handle, 
		iec61883_mpeg2_recv_t put_data,
		void *callback_data)
{
	struct iec61883_mpeg2 *mpeg;

	mpeg = malloc(sizeof(struct iec61883_mpeg2));
	if (!mpeg) {
		errno = ENOMEM;
		return NULL;
	}

	mpeg->tsbuffer = NULL;
	mpeg->handle = handle;
	mpeg->put_data = put_data;
	mpeg->get_data = NULL;
	mpeg->callback_data = callback_data;
	mpeg->buffer_packets = 1000;
	mpeg->prebuffer_packets = 1000;
	mpeg->irq_interval = 250;
	mpeg->synch = 0;
	mpeg->speed = RAW1394_ISO_SPEED_200;

	raw1394_set_userdata (handle, mpeg);
	
	return mpeg;
}

static enum raw1394_iso_disposition
mpeg2_recv_handler (raw1394handle_t handle, 
		unsigned char *data,
		unsigned int len, 
		unsigned char channel,
		unsigned char tag, 
		unsigned char sy,
		unsigned int cycle, 
		unsigned int dropped)
{
	struct iec61883_mpeg2 *mpeg = raw1394_get_userdata (handle);
	enum raw1394_iso_disposition result = RAW1394_ISO_OK;
	
	/* check fields of CIP header for valid packet */
	unsigned short dbs_fn_qpc_sph = (htonl (* (unsigned long*) (data)) >> 10) & 0x3fff;
	unsigned char fmt = (htonl (* (unsigned long*) (data + 4)) >> 24) & 0x3f;
	
	assert (mpeg != NULL);
	mpeg->total_dropped += dropped;

	if (mpeg->put_data != NULL && /* only if callback registered */
		channel == mpeg->channel &&    /* only for selected channel */
		len >= TSP_SPH_SIZE + 8 &&     /* no empty packets */
		dbs_fn_qpc_sph == 0x01b1 &&    /* valid CIP header */
		fmt == 0x20 )
	{
		/* skip over CIP header and SPH */
		data += 12;

		/* write each TSP in the iso packet minus SPH */
		for (; len > IEC61883_MPEG2_TSP_SIZE; len -= TSP_SPH_SIZE, data += TSP_SPH_SIZE) {
			if (mpeg->put_data (data, IEC61883_MPEG2_TSP_SIZE, dropped, mpeg->callback_data) < 0) {
				result = RAW1394_ISO_ERROR;
				break;
			}
			dropped = 0; /* do not repeatedly report dropped */
		}
	}
	if (result == RAW1394_ISO_OK && dropped)
		result = RAW1394_ISO_DEFER;
			
	return result;
}

int
iec61883_mpeg2_recv_start(struct iec61883_mpeg2 *mpeg, int channel)
{
	int result = 0;
	
	assert (mpeg != NULL);
	result = raw1394_iso_recv_init (mpeg->handle, 
		mpeg2_recv_handler,
		mpeg->buffer_packets, 
		MAX_PACKET_SIZE + 8,
		channel,
		RAW1394_DMA_PACKET_PER_BUFFER,
		mpeg->irq_interval);
	
	if (result == 0) {
		mpeg->total_dropped = 0;
		mpeg->channel = channel;
		result = raw1394_iso_recv_start (mpeg->handle, -1, -1, 0);
	}
	return result;
}

static enum raw1394_iso_disposition
mpeg2_xmit_handler (raw1394handle_t handle, unsigned char *data,
                    unsigned int *len, unsigned char *tag,
                    unsigned char *sy, int cycle,
                    unsigned int dropped )
{
	struct iec61883_mpeg2 *mpeg = raw1394_get_userdata (handle);
	enum raw1394_iso_disposition result = RAW1394_ISO_OK;
	
	assert (mpeg != NULL);
	mpeg->total_dropped += dropped;
	
	if ( mpeg->tsbuffer != NULL ) {
		*len = tsbuffer_send_iso_cycle (mpeg->tsbuffer, data, cycle, 
			(raw1394_get_local_id (handle) & 0x3f), dropped);
		if (*len == 0)
			result = RAW1394_ISO_ERROR;
	}
	else
		result = RAW1394_ISO_ERROR;

	*tag = IEC61883_TAG_WITH_CIP;
	*sy = 0;

	return result;
}


int
iec61883_mpeg2_xmit_start (struct iec61883_mpeg2 *mpeg, int pid, int channel)
{
	int result = 0;
	
	assert (mpeg != NULL);
	if (mpeg->get_data != NULL) {
		mpeg->tsbuffer = tsbuffer_init (mpeg->get_data, mpeg->callback_data, pid);
		if (mpeg->tsbuffer != NULL) {
			if (raw1394_iso_xmit_init (mpeg->handle,
										mpeg2_xmit_handler,
										mpeg->buffer_packets,
										968,  /* max packets size  = 5 * 192 + 8 */
										channel,
										mpeg->speed,
										mpeg->irq_interval) == 0) {
				mpeg->total_dropped = 0;
				result = raw1394_iso_xmit_start (mpeg->handle, -1, mpeg->prebuffer_packets);
			} else
				result = -1;
		} else
			result = -1;
	} else
		result = -1;
	
	return result;
}

void
iec61883_mpeg2_xmit_stop (struct iec61883_mpeg2 *mpeg)
{
	assert (mpeg != NULL);
	if (mpeg->synch)
		raw1394_iso_xmit_sync (mpeg->handle);
	raw1394_iso_shutdown (mpeg->handle);
	tsbuffer_close (mpeg->tsbuffer);
	mpeg->tsbuffer = NULL;
}

void
iec61883_mpeg2_recv_stop (struct iec61883_mpeg2 *mpeg)
{
	assert (mpeg != NULL);
	if (mpeg->synch)
		raw1394_iso_recv_flush (mpeg->handle);
	raw1394_iso_shutdown (mpeg->handle);
}

void
iec61883_mpeg2_close (struct iec61883_mpeg2 *mpeg)
{
	assert (mpeg != NULL);
	if (mpeg->put_data)
		iec61883_mpeg2_recv_stop (mpeg);
	else if (mpeg->get_data)
		iec61883_mpeg2_xmit_stop (mpeg);
	free (mpeg);
}

unsigned int
iec61883_mpeg2_get_buffers(iec61883_mpeg2_t mpeg2)
{
	assert (mpeg2 != NULL);
	return mpeg2->buffer_packets;
}

void
iec61883_mpeg2_set_buffers(iec61883_mpeg2_t mpeg2, unsigned int packets)
{
	assert (mpeg2 != NULL);
	mpeg2->buffer_packets = packets;
}

unsigned int
iec61883_mpeg2_get_prebuffers(iec61883_mpeg2_t mpeg2)
{
	assert (mpeg2 != NULL);
	return mpeg2->prebuffer_packets;
}

void
iec61883_mpeg2_set_prebuffers(iec61883_mpeg2_t mpeg2, unsigned int packets)
{
	assert (mpeg2 != NULL);
	mpeg2->prebuffer_packets = packets;
}

unsigned int
iec61883_mpeg2_get_irq_interval(iec61883_mpeg2_t mpeg2)
{
	assert (mpeg2 != NULL);
	return mpeg2->irq_interval;
}

void
iec61883_mpeg2_set_irq_interval(iec61883_mpeg2_t mpeg2, unsigned int packets)
{
	assert (mpeg2 != NULL);
	mpeg2->irq_interval = packets;
}

int
iec61883_mpeg2_get_synch(iec61883_mpeg2_t mpeg2)
{
	assert (mpeg2 != NULL);
	return mpeg2->synch;
}

void
iec61883_mpeg2_set_synch(iec61883_mpeg2_t mpeg2, int synch)
{
	assert (mpeg2 != NULL);
	mpeg2->synch = synch;
}

int
iec61883_mpeg2_get_speed(iec61883_mpeg2_t mpeg2)
{
	assert (mpeg2 != NULL);
	return mpeg2->speed;
}

void
iec61883_mpeg2_set_speed(iec61883_mpeg2_t mpeg2, int speed)
{
	assert (mpeg2 != NULL);
	mpeg2->speed = speed;
}

unsigned int
iec61883_mpeg2_get_dropped(iec61883_mpeg2_t mpeg2)
{
	assert (mpeg2 != NULL);
	return mpeg2->total_dropped;
}

void *
iec61883_mpeg2_get_callback_data (iec61883_mpeg2_t mpeg2)
{
	assert (mpeg2 != NULL);
	return mpeg2->callback_data;
}
