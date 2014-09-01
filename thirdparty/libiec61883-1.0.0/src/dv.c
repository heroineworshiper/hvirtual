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

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <assert.h>

#define DIF_BLOCK_SIZE 480
#define DV_CUSTOM_CIP 	/* packetizer and syt generation code from Dan Maas */

iec61883_dv_t
iec61883_dv_xmit_init (raw1394handle_t handle, 
		int is_pal,
		iec61883_dv_xmit_t get_data,
		void *callback_data)
{
	/* DV is composed of DIF blocks, each 480 bytes */
	int dbs = DIF_BLOCK_SIZE / sizeof (quadlet_t);
	int fdf = is_pal ? 0x80 : 0x00;
	int syt_interval = is_pal ? 300 : 250;
	int rate = syt_interval * (is_pal ? 25 : 30000.0/1001.0);
	struct iec61883_dv *dv;

	assert (handle != NULL);
	dv = malloc (sizeof (struct iec61883_dv));
	if (!dv) {
		errno = ENOMEM;
		return NULL;
	}
	
	dv->channel = -1;
	dv->handle = handle;
	dv->put_data = NULL;
	dv->get_data = get_data;
	dv->callback_data = callback_data;
	dv->buffer_packets = 1000;
	dv->prebuffer_packets = 1000;
	dv->irq_interval = 250;
	dv->synch = 0;
	dv->speed = RAW1394_ISO_SPEED_100;

	iec61883_cip_init (&dv->cip, IEC61883_FMT_DV, fdf, rate, dbs, syt_interval);

	iec61883_cip_set_transmission_mode (&dv->cip, IEC61883_MODE_NON_BLOCKING);

	raw1394_set_userdata (handle, dv);
	
	return dv;
}

iec61883_dv_t
iec61883_dv_recv_init (raw1394handle_t handle, 
		iec61883_dv_recv_t put_data,
		void *callback_data)
{
	struct iec61883_dv *dv;

	assert (handle != NULL);
	dv = malloc (sizeof (struct iec61883_dv));
	if (!dv) {
		errno = ENOMEM;
		return NULL;
	}
	
	dv->channel = -1;
	dv->handle = handle;
	dv->put_data = put_data;
	dv->get_data = NULL;
	dv->callback_data = callback_data;
	dv->buffer_packets = 775;
	dv->prebuffer_packets = 0;
	dv->irq_interval = 1;
	dv->synch = 0;
	dv->speed = RAW1394_ISO_SPEED_100;

	raw1394_set_userdata (handle, dv);
	
	return dv;
}

static enum raw1394_iso_disposition
dv_xmit_handler (raw1394handle_t handle,
		unsigned char *data, 
		unsigned int *len,
		unsigned char *tag,
		unsigned char *sy,
		int cycle,
		unsigned int dropped)
{
	struct iec61883_dv *dv = raw1394_get_userdata (handle);
	struct iec61883_packet *packet;
	int n_dif_blocks;
	int result = RAW1394_ISO_OK;

	assert (dv != NULL);
	packet = (struct iec61883_packet *) data;
	n_dif_blocks = iec61883_cip_fill_header (handle, &dv->cip, packet);

#ifdef DV_CUSTOM_CIP
	static const int syt_offset = 3;

	static int packet_num = 0;
	static int cip_accum = 0;
	static int continuity_counter = 0;
	
	unsigned int ts;
	int cip_n = 1; /* PAL defaults */
	int cip_d = 16;
	
	if (dv->cip.syt_interval == 250) { /* NTSC */
		cip_n = 68;
		cip_d = 1068;
	}
	/* generate syt */
	if (packet_num == 0) {
		ts = cycle + syt_offset;
		if (ts > 8000)
			ts -= 8000;
		ts = (ts & 0xF) << 12;
	} else {
		ts = 0xFFFF;
	}
	packet->syt = htons(ts);
	packet->dbc = continuity_counter;
	/* num/denom algorithm to determine empty packet rate */
	if (cip_accum > (cip_d - cip_n)) {
		n_dif_blocks = 0;
		cip_accum -= (cip_d - cip_n);
	} else {
		n_dif_blocks = 1;
		cip_accum += cip_n;
		continuity_counter++;
		if (++packet_num >= dv->cip.syt_interval) {
			packet_num = 0;
		}
	}
#endif
	
	dv->total_dropped += dropped;
	
	*len = n_dif_blocks * DIF_BLOCK_SIZE + 8;
	*tag = IEC61883_TAG_WITH_CIP;
	*sy = 0;
		 
	if (dv->get_data != NULL &&
		dv->get_data (packet->data, n_dif_blocks, dropped, dv->callback_data) < 0)
			result = RAW1394_ISO_ERROR;

	return result;
}

int
iec61883_dv_xmit_start (struct iec61883_dv *dv, int channel)
{
	int result = 0;
	assert (dv != NULL);
	unsigned int max_packet_size = iec61883_cip_get_max_packet_size (&dv->cip);
	
	if ((result = raw1394_iso_xmit_init (dv->handle, 
		dv_xmit_handler,
		dv->buffer_packets, 
		max_packet_size,
		channel,
		dv->speed,
		dv->irq_interval)) == 0)
	
		dv->total_dropped = 0;
		dv->channel = channel;
		result = raw1394_iso_xmit_start (dv->handle, -1, dv->prebuffer_packets);
	
	return result;
}

static enum raw1394_iso_disposition
dv_recv_handler (raw1394handle_t handle, 
		unsigned char *data,
		unsigned int len, 
		unsigned char channel,
		unsigned char tag, 
		unsigned char sy,
		unsigned int cycle, 
		unsigned int dropped)
{
	struct iec61883_dv *dv = raw1394_get_userdata (handle);
	enum raw1394_iso_disposition result = RAW1394_ISO_OK;
	
	assert (dv != NULL);
	dv->total_dropped += dropped;
	
	if (dv->put_data != NULL && /* only if callback registered */
		channel == dv->channel &&    /* only for selected channel */
		len == DIF_BLOCK_SIZE + 8)   /* not empty packets */
	{
		if (dv->put_data (data + 8, DIF_BLOCK_SIZE, dropped, dv->callback_data) < 0)
			result = RAW1394_ISO_ERROR;
	}
static unsigned drop = 0;
drop += dropped;
if (dropped) fprintf( stderr, "total dropped %u\n", drop);
	if (result == RAW1394_ISO_OK && dropped)
		result = RAW1394_ISO_DEFER;
			
	return result;
}

int
iec61883_dv_recv_start (struct iec61883_dv *dv, int channel)
{
	int result = 0;
	
	assert (dv != NULL);
	result = raw1394_iso_recv_init (dv->handle, 
		dv_recv_handler,
		dv->buffer_packets, 
		DIF_BLOCK_SIZE + 8,
		channel,
		RAW1394_DMA_PACKET_PER_BUFFER,
		dv->irq_interval);
	
	if (result == 0) {
		dv->total_dropped = 0;
		dv->channel = channel;
		result = raw1394_iso_recv_start (dv->handle, -1, -1, 0);
	}
	return result;
}

void
iec61883_dv_recv_stop (iec61883_dv_t dv)
{
	assert (dv != NULL);
	if (dv->synch)
		raw1394_iso_recv_flush (dv->handle);
	raw1394_iso_shutdown (dv->handle);
}

void
iec61883_dv_xmit_stop (iec61883_dv_t dv)
{
	assert (dv != NULL);
	if (dv->synch)
		raw1394_iso_xmit_sync (dv->handle);
	raw1394_iso_shutdown (dv->handle);
}

void
iec61883_dv_close (iec61883_dv_t dv)
{
	assert (dv != NULL);
	if (dv->put_data)
		iec61883_dv_recv_stop (dv);
	if (dv->get_data)
		iec61883_dv_xmit_stop (dv);
	free (dv);
}

static int
dv_fb_recv (unsigned char *data, int len, unsigned int dropped, void *callback_data)
{
	struct iec61883_dv_fb *fb = (struct iec61883_dv_fb *)callback_data;
	int section_type = data[0] >> 5; /* section type is in bits 5 - 7 */
	int dif_sequence = data[1] >> 4; /* dif sequence number is in bits 4 - 7 */
	int dif_block = data[2];
	int result = 0;
	
	assert (fb != NULL);
	/* test for start of frame */
	if (section_type == 0 && dif_sequence == 0) {
		/* if not waiting for the first frame */
		if (fb->ff == 0) {
			int total = ((fb->data[3] & 0x80) == 0 ? 250 : 300) * 480;
			if (fb->len != total)
				fb->total_incomplete++;
			result = fb->put_data (fb->data, total, fb->len == total, 
				fb->callback_data);
			fb->len = 0;
		} else {
			/* no longer waiting for first frame */
			fb->ff = 0;
		}
	} 
	/* if not the first frame */
	if (fb->ff == 0) {
		fb->len += len;
		switch ( section_type ) {
			case 0:    /* 1 Header block */
				memcpy( fb->data + dif_sequence * 150 * 80, data, 480 );
				break;
			case 1:    /* 2 Subcode blocks */
				memcpy( fb->data + dif_sequence * 150 * 80 + 
					( 1 + dif_block ) * 80, data, 480 );
				break;
			case 2:    /* 3 VAUX blocks */
				memcpy( fb->data + dif_sequence * 150 * 80 + 
					( 3 + dif_block ) * 80, data, 480 );
				break;
			case 3:    /* 9 Audio blocks interleaved with video */
				memcpy( fb->data + dif_sequence * 150 * 80 + 
					( 6 + dif_block * 16 ) * 80, data, 480 );
				break;
			case 4:    /* 135 Video blocks interleaved with audio */
				memcpy( fb->data + dif_sequence * 150 * 80 + 
					( 7 + ( dif_block / 15 ) + dif_block ) * 80, data, 480 );
				break;
			default:
				break;
		}
	}
		
	return result;
}

iec61883_dv_fb_t
iec61883_dv_fb_init (raw1394handle_t handle, 
		iec61883_dv_fb_recv_t put_data,
		void *callback_data)
{
	struct iec61883_dv_fb *fb;
	
	fb = malloc (sizeof( struct iec61883_dv_fb));
	if (!fb) {
		errno = ENOMEM;
		return NULL;
	}
	
	memset (fb->data, 0, sizeof (fb->data));
	fb->len = 0; /* tracks the bytes collected to determine if frame complete */
	fb->ff = 1;  /* indicates waiting for the start of frame */
	fb->put_data = put_data;
	fb->callback_data = callback_data;
	fb->dv = iec61883_dv_recv_init (handle, dv_fb_recv, (void *)fb );
	if (!fb->dv) {
		free (fb);
		return NULL;
	}
	
	return fb;
}

int
iec61883_dv_fb_start (iec61883_dv_fb_t fb, int channel)
{
	assert (fb != NULL);
	return iec61883_dv_recv_start (fb->dv, channel);
}

void
iec61883_dv_fb_stop (iec61883_dv_fb_t fb)
{
	assert (fb != NULL);
	iec61883_dv_recv_stop (fb->dv);
}

void
iec61883_dv_fb_close (iec61883_dv_fb_t fb)
{
	assert (fb != NULL);
	iec61883_dv_close (fb->dv);
	free (fb);
}

unsigned int
iec61883_dv_get_buffers (iec61883_dv_t dv)
{
	assert (dv != NULL);
	return dv->buffer_packets;
}

void
iec61883_dv_set_buffers (iec61883_dv_t dv, unsigned int packets)
{
	assert (dv != NULL);
	dv->buffer_packets = packets;
}

unsigned int
iec61883_dv_get_prebuffers (iec61883_dv_t dv)
{
	assert (dv != NULL);
	return dv->prebuffer_packets;
}

void
iec61883_dv_set_prebuffers (iec61883_dv_t dv, unsigned int packets)
{
	assert (dv != NULL);
	dv->prebuffer_packets = packets;
}

unsigned int
iec61883_dv_get_irq_interval (iec61883_dv_t dv)
{
	assert (dv != NULL);
	return dv->irq_interval;
}

void
iec61883_dv_set_irq_interval (iec61883_dv_t dv, unsigned int packets)
{
	assert (dv != NULL);
	dv->irq_interval = packets;
}

int
iec61883_dv_get_synch (iec61883_dv_t dv)
{
	assert (dv != NULL);
	return dv->synch;
}

void
iec61883_dv_set_synch (iec61883_dv_t dv, int synch)
{
	assert (dv != NULL);
	dv->synch = synch;
}

int
iec61883_dv_get_speed (iec61883_dv_t dv)
{
	assert (dv != NULL);
	return dv->speed;
}

void
iec61883_dv_set_speed (iec61883_dv_t dv, int speed)
{
	assert (dv != NULL);
	dv->speed = speed;
}

unsigned int
iec61883_dv_get_dropped (iec61883_dv_t dv)
{
	assert (dv != NULL);
	return dv->total_dropped;
}

void *
iec61883_dv_get_callback_data (iec61883_dv_t dv)
{
	assert (dv != NULL);
	return dv->callback_data;
}

unsigned int
iec61883_dv_fb_get_incomplete (iec61883_dv_fb_t fb)
{
	assert (fb != NULL);
	return fb->total_incomplete;
}

void *
iec61883_dv_fb_get_callback_data (iec61883_dv_fb_t fb)
{
	assert (fb != NULL);
	return fb->callback_data;
}
