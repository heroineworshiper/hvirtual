/*
 * libiec61883 - Linux IEEE 1394 streaming media library.
 * Copyright (C) 2004 Kristian Hogsberg and Dan Dennedy
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

#include "../src/iec61883.h"
#include <stdio.h>
#include <sys/select.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

static int g_done = 0;

static void sighandler (int sig)
{
	g_done = 1;
}

int fill_packet (iec61883_amdtp_t amdtp, char *data, int nevents,
	unsigned int dbc, unsigned int dropped, void *callback_data)
{
	FILE *fp = (FILE *) callback_data;
	static int total_packets = 0;
	int nsamples = nevents * 2; // stereo 16bit PCM
	char buffer [nsamples * 2];
	
	if (fread (buffer, 2, nsamples, fp) != nsamples) {
		return -1;
	} else {
		// TODO: convert from additional formats (20 or 24-bits).
		char *p = buffer;
		quadlet_t *event = (quadlet_t*) data;
		int i;

		for (i = 0; i < nsamples; i++) {
			// Shift to allow room for label
			event[i] = (p[1] << 16) | (p[0] << 8);
			p += 2;
		}
	}

	total_packets++;
	if ((total_packets & 0xfff) == 0) {
		fprintf (stderr, "\r%10d packets", total_packets);
		fflush (stderr);
	}

	return 0;
}

static int read_packet (iec61883_amdtp_t amdtp, char *data, int nsamples,
	unsigned int dbc, unsigned int dropped, void *callback_data)
{
	FILE *f = (FILE*) callback_data;
	static int total_packets = 0;
	
	if (total_packets == 0)
		fprintf (stderr, "format=0x%x sample_format=0x%x channels=%d rate=%d\n",
			iec61883_amdtp_get_format (amdtp), 
			iec61883_amdtp_get_sample_format (amdtp), 
			iec61883_amdtp_get_dimension (amdtp), 
			iec61883_amdtp_get_rate (amdtp));
	
	// TODO: convert to additional formats (20 or 24-bits).
	if (iec61883_amdtp_get_format (amdtp) == IEC61883_AMDTP_FORMAT_RAW &&
		iec61883_amdtp_get_sample_format (amdtp) == IEC61883_AMDTP_INPUT_LE16 &&
		iec61883_amdtp_get_dimension (amdtp) == 2)
	{
		unsigned char buffer [nsamples * 2];
		unsigned char *p = buffer;
		quadlet_t *sample = (quadlet_t *) data;
		int i;
		
		for (i = 0; i < nsamples; i++) {
			// the first byte contains a label - skip it
			*p++ = (sample[i] >> 8);
			*p++ = (sample[i] >> 16);
		}
		
		if (fwrite (buffer, 2, nsamples, f) != nsamples) {
			return -1;
		} else {
			total_packets++;
			if ((total_packets & 0xfff) == 0) {
				fprintf (stderr, "\r%10d packets", total_packets);
				fflush (stderr);
			}
		}
	}
	return 0;
}

static void amdtp_receive (raw1394handle_t handle, FILE *f, int channel)
{	
	iec61883_amdtp_t amdtp = iec61883_amdtp_recv_init (handle, read_packet, (void *)f );
		
	if ( amdtp && iec61883_amdtp_recv_start (amdtp, channel) == 0)
	{
		int fd = raw1394_get_fd (handle);
		struct timeval tv;
		fd_set rfds;
		int result = 0;
		
		signal (SIGINT, sighandler);
		signal (SIGPIPE, sighandler);
		fprintf (stderr, "Starting to receive\n");

		do {
			FD_ZERO (&rfds);
			FD_SET (fd, &rfds);
			tv.tv_sec = 0;
			tv.tv_usec = 20000;
			
			if (select (fd + 1, &rfds, NULL, NULL, &tv) > 0)
				result = raw1394_loop_iterate (handle);
			
		} while (g_done == 0 && result == 0);
		
		fprintf (stderr, "\ndone.\n");
	}
	iec61883_amdtp_close (amdtp);
}

static void amdtp_transmit (raw1394handle_t handle, FILE *f, int channel)
{	
	iec61883_amdtp_t amdtp;
	
	amdtp = iec61883_amdtp_xmit_init (handle, 44100, IEC61883_AMDTP_FORMAT_RAW,
		IEC61883_AMDTP_INPUT_LE16, IEC61883_MODE_BLOCKING_EMPTY, 2, fill_packet,
		(void *)f );
	
	if (amdtp && iec61883_amdtp_xmit_start (amdtp, channel) == 0)
	{
		int fd = raw1394_get_fd (handle);
		struct timeval tv;
		fd_set rfds;
		int result = 0;
		
		signal (SIGINT, sighandler);
		signal (SIGPIPE, sighandler);
		fprintf (stderr, "Starting to transmit\n");

		do {
			FD_ZERO (&rfds);
			FD_SET (fd, &rfds);
			tv.tv_sec = 0;
			tv.tv_usec = 20000;
			
			if (select (fd + 1, &rfds, NULL, NULL, &tv) > 0)
				result = raw1394_loop_iterate (handle);
			
		} while (g_done == 0 && result == 0);
		
		fprintf (stderr, "\ndone.\n");
	}
	iec61883_amdtp_close (amdtp);
}

int main (int argc, char *argv[])
{
	raw1394handle_t handle = raw1394_new_handle_on_port (0);
	nodeid_t node = 0xffc0;
	FILE *f = NULL;
	int is_transmit = 0;
	int node_specified = 0;
	int i;
	int channel;
	int bandwidth = -1;
	
	for (i = 1; i < argc; i++) {
		
		if (strncmp (argv[i], "-h", 2) == 0 || 
			strncmp (argv[i], "--h", 3) == 0)
		{
			fprintf (stderr, 
			"usage: %s [[-r | -t] node-id] [- | file]\n"
			"       All audio data must be signed 16bit 44.1KHz stereo PCM\n"
			"       Use - to transmit raw PCM from stdin, or\n"
			"       supply a filename to to transmit from a raw PCM file.\n"
			"       Otherwise, capture raw PCM to stdout.\n", argv[0]);
			raw1394_destroy_handle (handle);
			return 1;
		} else if (strncmp (argv[i], "-t", 2) == 0) {
			node |= atoi (argv[++i]);
			is_transmit = 1;
			node_specified = 1;
		} else if (strncmp (argv[i], "-r", 2) == 0) {
			node |= atoi (argv[++i]);
			is_transmit = 0;
			node_specified = 1;
		} else if (strcmp (argv[i], "-") != 0) {
			if (node_specified && !is_transmit)
				f = fopen (argv[i], "wb");
			else {
				f = fopen (argv[i], "rb");
				is_transmit = 1;
			}
		} else if (!node_specified) {
			is_transmit = 1;
		}
	}
		
	if (handle) {
		int oplug = -1, iplug = -1;
		
		if (is_transmit) {
			if (f == NULL)
				f = stdin;
			if (node_specified) {
				channel = iec61883_cmp_connect (handle,
					raw1394_get_local_id (handle), &oplug, node, &iplug,
					&bandwidth);
				if (channel > -1) {
					amdtp_transmit (handle, f, channel);
					iec61883_cmp_disconnect (handle,
						raw1394_get_local_id (handle), oplug, node, iplug,
						channel, bandwidth);
				} else {
					fprintf (stderr, "Connect failed, reverting to broadcast channel 63.\n");
					amdtp_transmit (handle, f, 63);
				}
			} else {
				amdtp_transmit (handle, f, 63);
			}
			if (f != stdin)
				fclose (f);
		} else {
			if (f == NULL)
				f = stdout;
			if (node_specified) {
				channel = iec61883_cmp_connect (handle, node, &oplug,
					raw1394_get_local_id (handle), &iplug, &bandwidth);
				if (channel > -1) {
					amdtp_receive (handle, f, channel);
					iec61883_cmp_disconnect (handle, node, oplug,
						raw1394_get_local_id (handle), iplug,
						channel, bandwidth);
				} else {
					fprintf (stderr, "Connect failed, reverting to broadcast channel 63.\n");
					amdtp_receive (handle, f, 63);
				}
			} else {
				amdtp_receive (handle, f, 63);
			}
			if (f != stdout)
				fclose (f);
		}
		raw1394_destroy_handle (handle);
	} else {
		fprintf (stderr, "Failed to get libraw1394 handle\n");
		return -1;
	}
	
	return 0;
}
