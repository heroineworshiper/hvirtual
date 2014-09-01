/* 
 *  ppmtodv.c
 *
 *     Copyright (C) Dan Dennedy 
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

#include "dv.h"
#include "dv_types.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define TIMES 5

int main( int argc, char **argv)
{
	unsigned char dv_buffer[480 * 300];
	unsigned char video_buffer[720 * 576 * 3];
	int pitches[3];
	unsigned char *pixels[3];
	FILE *f;
	int imagefile;
	int height, width, i;
	char line[200];
	dv_decoder_t *decoder = NULL;
	dv_encoder_t *encoder = NULL;

	if (argc < 3) exit(-1);

	pitches[0] = 720 * 3;
	pixels[0] = video_buffer;
	
	f = fopen(argv[1], "r");
	fgets(line, sizeof(line), f);
	if (feof(f)) {
		return -1;
	}
	do {
		fgets(line, sizeof(line), f); /* P6 */
	} while ((line[0] == '#'||(line[0] == '\n')) && !feof(f));
	if (sscanf(line, "%d %d\n", &width, &height) != 2) {
		fprintf(stderr, "Bad PPM file!\n");
		return -1;
	}
	fgets(line, sizeof(line), f);	/* 255 */
	
	fread(pixels[0], 1, 3 * width * height, f);
	
	decoder = dv_decoder_new(FALSE, FALSE, FALSE);
	decoder->quality = DV_QUALITY_BEST;

	encoder = dv_encoder_new(FALSE, FALSE, FALSE);
	encoder->isPAL = (height == 576);
	encoder->is16x9 = FALSE;
	encoder->vlc_encode_passes = 3;
	encoder->static_qno = 0;
	encoder->force_dct = DV_DCT_AUTO;
	
	for (i = 0; i < TIMES; i++) {
		time_t now = time(NULL);
		dv_encode_full_frame(encoder, pixels, e_dv_color_rgb, dv_buffer);
		dv_encode_metadata(dv_buffer, encoder->isPAL, encoder->is16x9, &now, 0);
		dv_encode_timecode(dv_buffer, encoder->isPAL, 0);
	
		memset(video_buffer,0,sizeof(video_buffer));
		dv_parse_header(decoder, dv_buffer);
		dv_decode_full_frame(decoder, dv_buffer, e_dv_color_rgb, 
				     pixels, pitches);
	}

	/* write RGB images to PPMs */
	imagefile=open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 00600);
	write(imagefile,"P6\n720 480\n255\n", 15);
	write(imagefile, video_buffer, width*height*3);
	close(imagefile);
	
	dv_decoder_free(decoder);
	dv_encoder_free(encoder);

	exit(0);
}
