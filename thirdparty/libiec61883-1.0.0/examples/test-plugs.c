/*
 * libiec61883 - Linux IEEE 1394 streaming media library.
 * Copyright (C) 2004 Dan Dennedy
 *
 * This example demonstrates creating a process to host plug control registers
 * used for connection management procedures as described in IEC 61883-1.
 * This can be expanded to run as a daemon, use syslog, and define some
 * interprocess communication for the addition and removal of plug control
 * registers. libiec61883 plug functions are not thread safe and manipulations
 * should be protected.
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

int main (int argc, char *argv[])
{
	raw1394handle_t handle = raw1394_new_handle_on_port (0);
	
	if (handle) {
		int fd = raw1394_get_fd (handle);
		struct timeval tv;
		fd_set rfds;
		int result;
		
		signal (SIGINT, sighandler);
		signal (SIGTERM, sighandler);
		
		if ((result = iec61883_plug_impr_init (handle, IEC61883_DATARATE_400)) < 0) {
			fprintf (stderr, "iec61883_plug_impr_init failed: %d\n", result);
			raw1394_destroy_handle (handle);
			return 1;
		}

		if ((result = iec61883_plug_ipcr_add (handle, 1 /* online */)) < 0) {
			fprintf (stderr, "iec61883_plug_add_ipcr  failed: %d\n", result);
			raw1394_destroy_handle (handle);
			return 1;
		}
		
		if ((result = iec61883_plug_ompr_init (handle, IEC61883_DATARATE_400, 63)) < 0) {
			fprintf (stderr, "iec61883_plug_ompr_init failed: %d\n", result);
			raw1394_destroy_handle (handle);
			return 1;
		}

		/* 968 = 5 transport stream packets + CIP header, payload is in quadlets */
		/* even after reading the spec I still don't understand the overhead
		   parameter. Besides, the iso packet headers, where does the overhead
		   come from? */
		if ((result = iec61883_plug_opcr_add (handle, 1 /* online */,
			IEC61883_OVERHEAD_512, 968/4)) < 0) {
			fprintf (stderr, "iec61883_plug_add_opcr failed: %d\n", result);
			raw1394_destroy_handle (handle);
			return 1;
		}
		
		do {
			FD_ZERO (&rfds);
			FD_SET (fd, &rfds);
			tv.tv_sec = 0;
			tv.tv_usec = 20000;
			
			if (select (fd + 1, &rfds, NULL, NULL, &tv) > 0)
				raw1394_loop_iterate (handle);
			
		} while (g_done == 0);
		
		fprintf (stderr, "done.\n");

		iec61883_plug_impr_close (handle);
		iec61883_plug_ompr_close (handle);

		raw1394_destroy_handle (handle);
		
	} else {
		fprintf (stderr, "Failed to get libraw1394 handle\n");
		return 1;
	}
	
	return 0;
}
