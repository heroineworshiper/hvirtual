/* 
 *  recode.c
 *
 *     Copyright (C) Dan Dennedy, Peter Schlaile
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

#include <libdv/dv.h>
#include <libdv/dv_types.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define TIMES 5

int read_frame(FILE* in_vid, unsigned char* frame_buf, int * isPAL)
{
        if (fread(frame_buf, 1, 120000, in_vid) != 120000) {
                return 0;
        }

        *isPAL = (frame_buf[3] & 0x80);

        if (*isPAL) {
                if (fread(frame_buf + 120000, 1, 144000 - 120000, in_vid) !=
                    144000 - 120000) {
                        return 0;
                }
        }
        return 1;
}

int main( int argc, char **argv)
{
	int infile = 0;
	unsigned char dv_buffer[144000];
	unsigned char video_buffer[720 * 576 * 3];
	int16_t *audio_bufs[4];
	dv_decoder_t *decoder = NULL;
	dv_encoder_t *encoder = NULL;
	int pitches[3];
	unsigned char *pixels[3];
	int i = 0, j;
	int isPAL = FALSE;

	pitches[0] = 720 * 2;
	pixels[0] = video_buffer;
	
	for(i = 0; i < 4; i++) {
		audio_bufs[i] = malloc(DV_AUDIO_MAX_SAMPLES*sizeof(int16_t));
	}

	/* assume NTSC for now, switch to PAL later if needed */
	decoder = dv_decoder_new(FALSE, FALSE, FALSE);
	encoder = dv_encoder_new(FALSE, FALSE, FALSE);
	
	decoder->quality = DV_QUALITY_BEST;
	encoder->vlc_encode_passes = 3;
	encoder->static_qno = 0;
	encoder->force_dct = DV_DCT_AUTO;

	i = 0;
	while (read_frame(stdin, dv_buffer, &isPAL)) {
		dv_parse_header(decoder, dv_buffer);
		if (isPAL != encoder->isPAL && isPAL == TRUE) {
			decoder->clamp_luma = FALSE;
			decoder->clamp_chroma = FALSE;
			encoder->clamp_luma = FALSE;
			encoder->clamp_chroma = FALSE;
			dv_reconfigure(FALSE, FALSE);
		} else if (isPAL != encoder->isPAL) {
			decoder->clamp_luma = TRUE;
			decoder->clamp_chroma = TRUE;
			decoder->add_ntsc_setup = TRUE;
			encoder->clamp_luma = TRUE;
			encoder->clamp_chroma = TRUE;
			encoder->rem_ntsc_setup = TRUE;
			dv_reconfigure(TRUE, TRUE);
		}
		encoder->isPAL = isPAL;
		encoder->is16x9 = (dv_format_wide(decoder)>0);
		dv_decode_full_audio(decoder, dv_buffer, audio_bufs);
		for (j = 0; j < TIMES; j++) {
			dv_decode_full_frame(decoder, dv_buffer, e_dv_color_yuv, 
					     pixels, pitches);
	
			dv_encode_full_frame(encoder, pixels, e_dv_color_yuv, dv_buffer);
		}
		dv_encode_full_audio(encoder, audio_bufs, 2, 48000, dv_buffer);
		fwrite(dv_buffer, 1, (isPAL ? 144000 : 120000), stdout);
	}

	close(infile);
	
	for(i=0; i < 4; i++) free(audio_bufs[i]);
	dv_decoder_free(decoder);
	dv_encoder_free(encoder);

	return 0;
}



