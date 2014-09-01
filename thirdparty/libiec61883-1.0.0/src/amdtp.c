/*
 * libiec61883 - Linux IEEE 1394 streaming media library.
 * Copyright (C) 2004 Kristian Hogsberg, Dan Dennedy, and Dan Maas.
 *
 * This file written by Kristian Hogsberg.
 * Update receive to use new raw1394 API by Dan Dennedy.
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
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <assert.h>

#include "iec61883.h"
#include "iec61883-private.h"

#define AMDTP_MAX_PACKET_SIZE 2048	//XXX: what is max packet size in AMDTP?

iec61883_amdtp_t
iec61883_amdtp_xmit_init (raw1394handle_t handle,
		int rate,
		int format,
		int sample_format,
		int mode,
		int dimension,
		iec61883_amdtp_xmit_t get_data, void *callback_data)
{
	int fdf, syt_interval;
	struct iec61883_amdtp *amdtp;

	amdtp = malloc (sizeof (struct iec61883_amdtp));
	if (!amdtp) {
		errno = ENOMEM;
		return NULL;
	}
	amdtp->channel = -1;

	if (format <= IEC61883_AMDTP_FORMAT_IEC958_AC3)
		amdtp->format = format;
	else {
		free (amdtp);
		return NULL;
	}

	switch (rate) {
	case 32000:
		syt_interval = 8;
		fdf = IEC61883_FDF_SFC_32KHZ;
		amdtp->iec958_rate_code = 0x0c;
		break;
	case 44100:
		syt_interval = 8;
		fdf = IEC61883_FDF_SFC_44K1HZ;
		amdtp->iec958_rate_code = 0x00;
		break;
	case 48000:
		syt_interval = 8;
		fdf = IEC61883_FDF_SFC_48KHZ;
		amdtp->iec958_rate_code = 0x04;
		break;
	case 88200:
		syt_interval = 16;
		fdf = IEC61883_FDF_SFC_88K2HZ;
		amdtp->iec958_rate_code = 0x00;
		break;
	case 96000:
		syt_interval = 16;
		fdf = IEC61883_FDF_SFC_96KHZ;
		amdtp->iec958_rate_code = 0x00;
		break;
	case 176400:
		syt_interval = 32;
		fdf = IEC61883_FDF_SFC_176K4HZ;
		amdtp->iec958_rate_code = 0x00;
		break;
	case 192000:
		syt_interval = 32;
		fdf = IEC61883_FDF_SFC_192KHZ;
		amdtp->iec958_rate_code = 0x00;
		break;

	default:
		free (amdtp);
		return NULL;
	}

	/* When using the AM824 raw subformat we can stream signals of
	 * any dimension.  The IEC958 subformat, however, only
	 * supports 2 channels.
	 */
	if (amdtp->format == IEC61883_AMDTP_FORMAT_IEC958_PCM && dimension > 2) {
		free (amdtp);
		return NULL;
	}
	else {
		amdtp->dimension = dimension;
	}

	amdtp->sample_format = sample_format;
	amdtp->get_data = get_data;
	amdtp->callback_data = callback_data;
	amdtp->handle = handle;
	amdtp->dimension = dimension;
	amdtp->buffer_packets = 1000;
	amdtp->prebuffer_packets = 1000;
	amdtp->irq_interval = 250;
	amdtp->synch = 0;
	amdtp->speed = RAW1394_ISO_SPEED_100;

	iec61883_cip_init (&amdtp->cip, IEC61883_FMT_AMDTP, fdf,
			   rate, dimension, syt_interval);
	iec61883_cip_set_transmission_mode (&amdtp->cip, mode);

	raw1394_set_userdata (handle, amdtp);

	return amdtp;
}

static enum raw1394_iso_disposition
amdtp_xmit_handler (raw1394handle_t handle,
		unsigned char *data, unsigned int *len,
		unsigned char *tag, unsigned char *sy,
		int cycle, unsigned int dropped)
{
	struct iec61883_amdtp *amdtp = raw1394_get_userdata (handle);
	struct iec61883_packet *packet = (struct iec61883_packet *) data;
	int nevents = iec61883_cip_fill_header (handle, &amdtp->cip, packet);
	quadlet_t *event = (quadlet_t *) packet->data;
	enum raw1394_iso_disposition result = RAW1394_ISO_OK;
	int nsamples;
	
	assert (amdtp != NULL);
	amdtp->total_dropped += dropped;

	if (nevents > 0) {
		nsamples = nevents;
	}
	else {
		if (amdtp->cip.mode == IEC61883_MODE_BLOCKING_EMPTY) {
			nsamples = 0;
		}
		else {
			nsamples = amdtp->cip.syt_interval;
		}
	}

	memset (packet->data, '\0', nsamples * amdtp->dimension * sizeof (quadlet_t));

	if (nevents > 0) {
		if( amdtp->get_data (amdtp, packet->data, nevents, packet->dbc, dropped, 
				     amdtp->callback_data) < 0 ) {
			result = RAW1394_ISO_ERROR;
		}
	}

	if (result == RAW1394_ISO_OK ) {
		quadlet_t label = 0;
		int i;
		
		if (amdtp->format == IEC61883_AMDTP_FORMAT_RAW) {
			label = IEC61883_AM824_LABEL;
			switch (amdtp->sample_format) {
				case IEC61883_AMDTP_INPUT_LE24:
					label |= IEC61883_AM824_VBL_24BITS;
					break;
				case IEC61883_AMDTP_INPUT_LE20:
					label |= IEC61883_AM824_VBL_20BITS;
					break;
				case IEC61883_AMDTP_INPUT_LE16:
					label |= IEC61883_AM824_VBL_16BITS;
					break;
				default:
					break;
			}
			label = label << 24;

			for (i = 0; i < nsamples * amdtp->dimension; i++) {
				event[i] = htonl (event[i] | label);
			}
		}
		else if (amdtp->format == IEC61883_AMDTP_FORMAT_IEC958_PCM) {
			struct iec60958_data *sample;

			for (i = 0; i < nsamples * amdtp->dimension; i++) {
				sample = (struct iec60958_data *) &event[i];
				sample->label = IEC60958_LABEL;
				if (nevents == 0) {
					sample->validity = IEC60958_DATA_INVALID;
					/* Using reseved value for old
					 * SoftAcoustik SA2.0 speakers. */
					sample->pac = IEC60958_PAC_RSV;
				}
				else {
					sample->validity = IEC60958_DATA_VALID;
					if( i % 2 == 0 || amdtp->dimension == 1 ) {
						/* even --> CHANNEL 1 */
						sample->pac = IEC60958_PAC_M;
					}
					else {
						/* Odd --> CHANNEL 2 */
						sample->pac = IEC60958_PAC_W;
					}
				}

				/* The parity bit should be computed here... */
				event[i] = htonl (event[i]);
			}
		}
		else {
			/* Unsupported format. */
			result = RAW1394_ISO_ERROR;
		}

		*len = nsamples * amdtp->dimension * sizeof (quadlet_t) + 8;
		*tag = IEC61883_TAG_WITH_CIP;
		*sy = 0;
	}

	return result;
}

int
iec61883_amdtp_xmit_start (struct iec61883_amdtp *amdtp, int channel)
{
	int result = 0;
	unsigned int max_packet_size;

	assert (amdtp != NULL);
	max_packet_size = iec61883_cip_get_max_packet_size (&amdtp->cip);

	result = raw1394_iso_xmit_init (amdtp->handle, amdtp_xmit_handler,
					amdtp->buffer_packets,
					max_packet_size, channel,
					amdtp->speed, amdtp->irq_interval);
	if (result == 0) {
		amdtp->total_dropped = 0;
		amdtp->channel = channel;
		result = raw1394_iso_xmit_start (amdtp->handle, 0,
						 amdtp->prebuffer_packets);
	}

	return result;
}

iec61883_amdtp_t
iec61883_amdtp_recv_init (raw1394handle_t handle,
		iec61883_amdtp_recv_t put_data, void *callback_data)
{
	struct iec61883_amdtp *amdtp;
	
	amdtp = malloc (sizeof (struct iec61883_amdtp));
	if (!amdtp) {
		errno = ENOMEM;
		return NULL;
	}

	amdtp->channel = -1;
	amdtp->handle = handle;
	amdtp->put_data = put_data;
	amdtp->callback_data = callback_data;
	amdtp->buffer_packets = 1000;
	amdtp->prebuffer_packets = 1000;
	amdtp->irq_interval = 250;
	amdtp->synch = 0;

	raw1394_set_userdata (handle, amdtp);

	return amdtp;
}

static enum raw1394_iso_disposition
amdtp_recv_handler (raw1394handle_t handle,
		unsigned char *data,
		unsigned int len,
		unsigned char channel,
		unsigned char tag,
		unsigned char sy,
		unsigned int cycle, 
		unsigned int dropped)
{
	struct iec61883_amdtp *amdtp = raw1394_get_userdata (handle);
	enum raw1394_iso_disposition result = RAW1394_ISO_OK;
	struct iec61883_packet *packet = (struct iec61883_packet *) data;
	int label;

	assert (amdtp != NULL);
	amdtp->total_dropped += dropped;
	
	/* We only support AM824 data for the moment. */
	/* We should check the DBC value to make sure we don't miss any packet. */
	if (tag == IEC61883_TAG_WITH_CIP &&
	    packet->fmt == IEC61883_FMT_AMDTP && (
	    ((packet->fdf & ~IEC61883_FDF_SFC_MASK) == IEC61883_FDF_AM824) ||
	    ((packet->fdf & ~IEC61883_FDF_SFC_MASK) == IEC61883_FDF_AM824_CONTROLLED))) {

		/* We fill amdtp structure fields upon reception of first packet. */
		if ((amdtp->dimension < 0) && (packet->syt != 0xFFFF)) {
			amdtp->dimension = packet->dbs; /* Number of audio channels. */

			DEBUG ("FDF code = %d.", packet->fdf);

			/* Obtaining sampling rate from SFC code. */
			switch (packet->fdf & IEC61883_FDF_SFC_MASK)
			{
			case IEC61883_FDF_SFC_32KHZ:
				amdtp->rate = 32000;
				break;
			case IEC61883_FDF_SFC_44K1HZ:
				amdtp->rate = 44100;
				break;
			case IEC61883_FDF_SFC_48KHZ:
				amdtp->rate = 48000;
				break;
			case IEC61883_FDF_SFC_88K2HZ:
				amdtp->rate = 88200;
				break;
			case IEC61883_FDF_SFC_96KHZ:
				amdtp->rate = 96000;
				break;
			case IEC61883_FDF_SFC_176K4HZ:
				amdtp->rate = 176400;
				break;
			case IEC61883_FDF_SFC_192KHZ:
				amdtp->rate = 192000;
			default:
				WARN ("Unsupported SFC code (%d).",
				      packet->fdf & IEC61883_FDF_SFC_MASK);
				return RAW1394_ISO_ERROR;
			}

			label = htonl (*((quadlet_t *) packet->data)) >> 24;

			/* Checking label. */
			if ((label & ~0x03) == IEC61883_AM824_LABEL) {
				DEBUG ("Multi-bit Linear Audio (MBLA) samples.");
				/* Multi-bit Linear Audio (MBLA). */
				amdtp->format = IEC61883_AMDTP_FORMAT_RAW;

				/* Checking Valid Bit Length code. */
				switch (label & 0x03) {
				case IEC61883_AM824_VBL_24BITS:
					DEBUG ("24-bits samples.");
					amdtp->sample_format =
						IEC61883_AMDTP_INPUT_LE24;
					break;
				case IEC61883_AM824_VBL_20BITS:
					DEBUG ("20-bits samples.");
					amdtp->sample_format =
						IEC61883_AMDTP_INPUT_LE20;
					break;
				case IEC61883_AM824_VBL_16BITS:
					DEBUG ("16-bits samples.");
					amdtp->sample_format =
						IEC61883_AMDTP_INPUT_LE16;
					break;
				default:
					WARN ("Unsupported valid bit length code (%d).", label & 0x03);
					result = RAW1394_ISO_ERROR;
				}
			} else if ((label >= 0) && (label <= 0x3F)) {
				/* IEC60958 Conformant Data. */
				/* TODO: how to determine AC3 */
				amdtp->format = IEC61883_AMDTP_FORMAT_IEC958_PCM;
				amdtp->sample_format = IEC61883_AMDTP_INPUT_NA;
			} else {
				WARN ("Unsupported data format label (%d).",
				      label);
				result = RAW1394_ISO_ERROR;
			}
		}

		if ((result == RAW1394_ISO_OK) && 
				(amdtp->dimension > 0) && (packet->syt != 0xFFFF)) {
			int nsamples = (len / sizeof (quadlet_t)) - 2; /* Substracting CIP headers. */
			quadlet_t *event = (quadlet_t *) packet->data;
			int i;

			for (i = 0; i < nsamples; i++)
				event[i] = ntohl (event[i]);
				
			if (amdtp->put_data (amdtp, packet->data, nsamples, packet->dbc, dropped,
				amdtp->callback_data) < 0)
				result = RAW1394_ISO_ERROR;
		}
	}
	if (result == RAW1394_ISO_OK && dropped)
		result = RAW1394_ISO_DEFER;

	return result;
}

int
iec61883_amdtp_recv_start (struct iec61883_amdtp *amdtp, int channel)
{
	int result = 0;

	assert (amdtp != NULL);
	result = raw1394_iso_recv_init (amdtp->handle,
		amdtp_recv_handler,
		amdtp->buffer_packets,
		AMDTP_MAX_PACKET_SIZE,
		channel,
		RAW1394_DMA_PACKET_PER_BUFFER,
		amdtp->irq_interval);

	if (result == 0) {
		amdtp->total_dropped = 0;
		amdtp->channel = channel;
		amdtp->dimension = -1;	/* The audio informations in amdtp structure will be
					 * filled-in upon reception of the first isochronous
					 * packet. */

		result = raw1394_iso_recv_start (amdtp->handle, -1, -1, 0);
	}
	return result;
}

void
iec61883_amdtp_recv_stop (struct iec61883_amdtp *amdtp)
{
	assert (amdtp != NULL);
	if (amdtp->synch)
		raw1394_iso_recv_flush (amdtp->handle);
	raw1394_iso_shutdown (amdtp->handle);
}

void
iec61883_amdtp_xmit_stop (struct iec61883_amdtp *amdtp)
{
	assert (amdtp != NULL);
	if (amdtp->synch)
		raw1394_iso_xmit_sync (amdtp->handle);
	raw1394_iso_shutdown (amdtp->handle);
}

void
iec61883_amdtp_close (struct iec61883_amdtp *amdtp)
{
	assert (amdtp != NULL);
	if (amdtp->put_data)
		iec61883_amdtp_recv_stop (amdtp);
	if (amdtp->get_data)
		iec61883_amdtp_xmit_stop (amdtp);
	free (amdtp);
}

unsigned int
iec61883_amdtp_get_buffers (iec61883_amdtp_t amdtp)
{
	assert (amdtp != NULL);
	return amdtp->buffer_packets;
}

void
iec61883_amdtp_set_buffers (iec61883_amdtp_t amdtp, unsigned int packets)
{
	assert (amdtp != NULL);
	amdtp->buffer_packets = packets;
}

unsigned int
iec61883_amdtp_get_prebuffers (iec61883_amdtp_t amdtp)
{
	assert (amdtp != NULL);
	return amdtp->prebuffer_packets;
}

void
iec61883_amdtp_set_prebuffers (iec61883_amdtp_t amdtp, unsigned int packets)
{
	assert (amdtp != NULL);
	amdtp->prebuffer_packets = packets;
}

unsigned int
iec61883_amdtp_get_irq_interval (iec61883_amdtp_t amdtp)
{
	assert (amdtp != NULL);
	return amdtp->irq_interval;
}

void
iec61883_amdtp_set_irq_interval (iec61883_amdtp_t amdtp, unsigned int packets)
{
	assert (amdtp != NULL);
	amdtp->irq_interval = packets;
}

int
iec61883_amdtp_get_synch (iec61883_amdtp_t amdtp)
{
	assert (amdtp != NULL);
	return amdtp->synch;
}

void
iec61883_amdtp_set_synch (iec61883_amdtp_t amdtp, int synch)
{
	assert (amdtp != NULL);
	amdtp->synch = synch;
}

int
iec61883_amdtp_get_speed (iec61883_amdtp_t amdtp)
{
	assert (amdtp != NULL);
	return amdtp->speed;
}

void
iec61883_amdtp_set_speed (iec61883_amdtp_t amdtp, int speed)
{
	assert (amdtp != NULL);
	amdtp->speed = speed;
}

unsigned int
iec61883_amdtp_get_dropped(iec61883_amdtp_t amdtp)
{
	assert (amdtp != NULL);
	return amdtp->total_dropped;
}

void *
iec61883_amdtp_get_callback_data (iec61883_amdtp_t amdtp)
{
	assert (amdtp != NULL);
	return amdtp->callback_data;
}

int
iec61883_amdtp_get_dimension(iec61883_amdtp_t amdtp)
{
	assert (amdtp != NULL);
	return amdtp->dimension;
}

int
iec61883_amdtp_get_rate(iec61883_amdtp_t amdtp)
{
	assert (amdtp != NULL);
	return amdtp->rate;
}

enum iec61883_amdtp_format
iec61883_amdtp_get_format(iec61883_amdtp_t amdtp)
{
	assert (amdtp != NULL);
	return amdtp->format;
}

enum iec61883_amdtp_sample_format
iec61883_amdtp_get_sample_format(iec61883_amdtp_t amdtp)
{
	assert (amdtp != NULL);
	return amdtp->sample_format;
}
