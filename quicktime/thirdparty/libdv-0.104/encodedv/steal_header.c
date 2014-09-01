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
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, const char** argv)
{
	int in_ren;
	int in_cap;
	unsigned char buf_ren[80];
	unsigned char buf_cap[80];
	int dif_block_count = 0;
	int vid_count = 0;

	if (argc != 3) {
		fprintf(stderr, "Usage: steal_header in_rendered in_captured "
			"> out.dv\n");
		return -1;
	}

	in_ren = open(argv[1], O_RDONLY);
	in_cap = open(argv[2], O_RDONLY);

	assert(in_ren != -1 && in_cap != -1);

	while (read(in_ren, buf_ren, 80) == 80 &&
	       read(in_cap, buf_cap, 80) == 80) {
		if (dif_block_count < 6) {
			write(STDOUT_FILENO, buf_cap, 80);
		} else {
			if (!vid_count) {
				write(STDOUT_FILENO, buf_ren, 80);
			} else {
				buf_ren[0] = buf_cap[0];
				buf_ren[1] = buf_cap[1];
				buf_ren[2] = buf_cap[2];
				write(STDOUT_FILENO, buf_ren, 80);
			}
			vid_count++;
			vid_count %= 16;
		}

		dif_block_count++;
		dif_block_count %= 150;
	}
	return 0;
}
