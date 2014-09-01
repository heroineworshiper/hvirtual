/* 
 *  scan_packet_headers.c
 *
 *     Copyright (C) Peter Schlaile - January 2001
 *
 *  This file is part of libdv, a free DV (IEC 61834/SMPTE 314M)
 *  codec.
 *
 *  libdv is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser Public License as published by
 *  the Free Software Foundation; either version 2.1, or (at your
 *  option) any later version.
 *   
 *  libdv is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser Public License for more details.
 *   
 *  You should have received a copy of the GNU Lesser Public License
 *  along with libdv; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 *  The libdv homepage is http://libdv.sourceforge.net/.  
 */

#include "libdv/dv_types.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


int main(int argc, const char** argv)
{
	unsigned char buf[80];
	int dif_count = -1;
	int packet = -1;
	int frame = 0;

	while (read(STDIN_FILENO, buf, 80) == 80) {
		int i;
		dif_count++;
		if ((dif_count % 6) == 0) {
			if (buf[0] == 0x1f && buf[1] == 0x07 && packet != -1) {
				packet = -1;
				dif_count = 0;
				frame++;
				printf("------------------------\n");
			}
			packet++;
		}
		printf("%04d/%03d/%03d :", dif_count, packet, frame);
		for (i = 0; i < 40; i++) {
			printf("%02x ", buf[i]);
		}
		printf("| ");
		for (i = 40; i < 80; i++) {
			printf("%02x ", buf[i]);
		}
		printf("\n");
	}

	exit(0);
}
