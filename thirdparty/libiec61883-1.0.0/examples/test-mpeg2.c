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

static int write_packet (unsigned char *data, int len, unsigned int dropped, void *callback_data)
{
	FILE *f = (FILE*) callback_data;

	if (dropped)
		fprintf (stderr, "\a%d packets dropped!\n", dropped);
	return (fwrite (data, len, 1, f) < 1) ? -1 : 0;
}

static int read_packet (unsigned char *data, int n_packets, unsigned int dropped, void *callback_data)
{
	FILE *f = (FILE*) callback_data;
	return (fread (data, IEC61883_MPEG2_TSP_SIZE * n_packets, 1, f) < 1) ? -1 : 0;
}

static void sighandler (int sig)
{
	g_done = 1;
}

static void mpeg2_receive (raw1394handle_t handle, FILE *f, int channel)
{	
	iec61883_mpeg2_t mpeg = iec61883_mpeg2_recv_init (handle, write_packet,
		(void *)f );
	
	if ( mpeg && iec61883_mpeg2_recv_start (mpeg, channel) == 0)
	{
		int fd = raw1394_get_fd (handle);
		struct timeval tv;
		fd_set rfds;
		int result = 0;
		
		signal (SIGINT, sighandler);
		signal (SIGTERM, sighandler);
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
	iec61883_mpeg2_close (mpeg);
}

static void mpeg2_transmit (raw1394handle_t handle, FILE *f, int pid, int channel)
{	
	iec61883_mpeg2_t mpeg;
	
	mpeg = iec61883_mpeg2_xmit_init (handle, read_packet, (void *)f );
	if ( mpeg && iec61883_mpeg2_xmit_start (mpeg, pid, channel) == 0)
	{
		int fd = raw1394_get_fd (handle);
		struct timeval tv;
		fd_set wfds;
		int result = 0;
		
		signal (SIGINT, sighandler);
		signal (SIGTERM, sighandler);
		signal (SIGPIPE, sighandler);
		fprintf (stderr, "Starting to transmit\n");

		do {
			FD_ZERO (&wfds);
			FD_SET (fd, &wfds);
			tv.tv_sec = 0;
			tv.tv_usec = 20000;
			
			if (select (fd + 1, NULL, &wfds, NULL, &tv) > 0)
				result = raw1394_loop_iterate (handle);
			
		} while (g_done == 0 && result == 0);
		
		fprintf (stderr, "done.\n");
	}
	iec61883_mpeg2_close (mpeg);
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
	int pid = -1;
	
	for (i = 1; i < argc; i++) {
		if (strncmp (argv[i], "-h", 2) == 0 || 
			strncmp (argv[i], "--h", 3) == 0)
		{
			fprintf (stderr, 
			"usage: %s [[-r | -t] node-id] [-p pid] [- | file]\n"
			"       Use - to transmit MPEG2-TS from stdin, or\n"
			"       supply a filename to transmit from a MPEG2-TS file.\n"
			"       Otherwise, capture MPEG2-TS to stdout.\n"
			"       The default PID for transmit is -1 (use first found).\n",
				argv[0]);
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
		} else if (strncmp (argv[i], "-p", 2) == 0) {
			pid |= atoi (argv[++i]);
			is_transmit = 1;
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
					mpeg2_transmit (handle, f, pid, channel);
					iec61883_cmp_disconnect (handle,
						raw1394_get_local_id (handle), oplug, node, iplug,
						channel, bandwidth);
				} else {
					fprintf (stderr, "Connect failed, reverting to broadcast channel 63.\n");
					mpeg2_transmit (handle, f, pid, 63);
				}
			} else {
				mpeg2_transmit (handle, f, pid, 63);
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
					mpeg2_receive (handle, f, channel);
					iec61883_cmp_disconnect (handle, node, oplug,
						raw1394_get_local_id (handle), iplug,
						channel, bandwidth);
				} else {
					fprintf (stderr, "Connect failed, reverting to broadcast channel 63.\n");
					mpeg2_receive (handle, f, 63);
				}
			} else {
				mpeg2_receive (handle, f, 63);
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
