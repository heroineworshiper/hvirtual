/* 
 *  fix_headers.c
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
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include <libdv/headers.h>

int read_frame(FILE* in_fp, unsigned char* frame_buf, int * isPAL)
{
	if (fread(frame_buf, 1, 120000, in_fp ) != 120000) {
		return 0;
	}

	*isPAL = (frame_buf[3] & 0x80);

	if (*isPAL) {
		if (fread(frame_buf + 120000, 1, 144000 - 120000, in_fp) !=
		    144000 - 120000) {
			return 0;
		}
	}
	return 1;
}

int main(int argc, const char** argv)
{
	unsigned char frame_buf[144000];
	int isPAL;
	int frame_count = 0;
	time_t t = time(NULL);
	int wide = 0;
	if (argc == 2 && strcmp(argv[1], "-w") == 0) {
		wide = 1;
	}

	while (read_frame(stdin, frame_buf, &isPAL)) {
		_dv_write_meta_data(frame_buf, frame_count++, isPAL, wide, &t);
		fwrite(frame_buf, 1, isPAL ? 144000 : 120000, stdout);
	}
	return 0;
}
