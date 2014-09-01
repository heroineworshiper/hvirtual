/*
 * libiec61883 - Linux IEEE 1394 streaming media library.
 * Copyright (C) 2004 Dan Dennedy
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

static int write_frame (unsigned char *data, int len, int complete, void *callback_data)
{
	FILE *f = (FILE*) callback_data;

	if (complete == 0)
		fprintf (stderr, "Error: incomplete frame received!\n");
	return (fwrite (data, len, 1, f) < 1) ? -1 : 0;
}

static int read_frame (unsigned char *data, int n, unsigned int dropped, void *callback_data)
{
	FILE *f = (FILE*) callback_data;

	if (n == 1)
		if (fread (data, 480, 1, f) < 1) {
			return -1;
		} else
			return 0;
	else
		return 0;
}

static void sighandler (int sig)
{
	g_done = 1;
}

static void dv_receive( raw1394handle_t handle, FILE *f, int channel)
{	
	iec61883_dv_fb_t frame = iec61883_dv_fb_init (handle, write_frame, (void *)f );
		
	if (frame && iec61883_dv_fb_start (frame, channel) == 0)
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
		
		fprintf (stderr, "done.\n");
	}
	iec61883_dv_fb_close (frame);
}

static void dv_transmit( raw1394handle_t handle, FILE *f, int channel)
{	
	iec61883_dv_t dv;
	unsigned char data[480];
	int ispal;
	
	fread (data, 480, 1, f);
	ispal = (data[ 3 ] & 0x80) != 0;
	dv = iec61883_dv_xmit_init (handle, ispal, read_frame, (void *)f );
	
	if (dv && iec61883_dv_xmit_start (dv, channel) == 0)
	{
		int fd = raw1394_get_fd (handle);
		struct timeval tv;
		fd_set rfds;
		int result = 0;
		
		signal (SIGINT, sighandler);
		signal (SIGPIPE, sighandler);
		fprintf (stderr, "Starting to transmit %s\n", ispal ? "PAL" : "NTSC");

		do {
			FD_ZERO (&rfds);
			FD_SET (fd, &rfds);
			tv.tv_sec = 0;
			tv.tv_usec = 20000;
			
			if (select (fd + 1, &rfds, NULL, NULL, &tv) > 0)
				result = raw1394_loop_iterate (handle);
			
		} while (g_done == 0 && result == 0);
		
		fprintf (stderr, "done.\n");
	}
	iec61883_dv_close (dv);
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
			"       Use - to transmit raw DV from stdin, or\n"
			"       supply a filename to to transmit from a raw DV file.\n"
			"       Otherwise, capture raw DV to stdout.\n", argv[0]);
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
					dv_transmit (handle, f, channel);
					iec61883_cmp_disconnect (handle,
						raw1394_get_local_id (handle), oplug, node, iplug,
						channel, bandwidth);
				} else {
					fprintf (stderr, "Connect failed, reverting to broadcast channel 63.\n");
					dv_transmit (handle, f, 63);
				}
			} else {
				dv_transmit (handle, f, 63);
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
					dv_receive (handle, f, channel);
					iec61883_cmp_disconnect (handle, node, oplug, 
						raw1394_get_local_id (handle), iplug,
						channel, bandwidth);
				} else {
					fprintf (stderr, "Connect failed, reverting to broadcast channel 63.\n");
					dv_receive (handle, f, 63);
				}
			} else {
				dv_receive (handle, f, 63);
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
