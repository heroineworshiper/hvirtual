/* 
 *  headers.c
 *
 *     Copyright (C) Peter Schlaile - Feb 2001
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

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "headers.h"

static void write_header_block(unsigned char* target, int ds, int isPAL)
{
	target[0] = 0x1f; /* header magic */
	target[1] = 0x07 | (ds << 4);
	target[2] = 0x00;

	target[3] = ( isPAL ? 0xbf : 0x3f);
	target[4] = 0x68; /* FIXME ? */
	target[5] = 0x78; /* FIXME ? */
	target[6] = 0x78; /* FIXME ? */
	target[7] = 0x78; /* FIXME ? */

	memset(target + 8, 0xff, 80 - 8);
}

static inline void write_bcd(unsigned char* target, int val)
{
	*target = ((val / 10) << 4) + (val % 10);
}

static void write_timecode_13(unsigned char* target, struct tm * now, int frame,
		       int isPAL)
{
	target[0] = 0x13;
	write_bcd(target+1, frame % (isPAL ? 25 : 30));
	write_bcd(target+2, now->tm_sec);
	write_bcd(target+3, now->tm_min);
	write_bcd(target+4, now->tm_hour);
}

/*
  60 ff ff 20 ff 61 33 c8 fd ff 62 ff d0 e1 01 63 ff b8 c6 c9
*/

static void write_timecode_60(unsigned char* target, struct tm * now, int isPAL)
{
	target[0] = 0x60;
	target[1] = 0xff;
	target[2] = 0xff;
	target[3] = isPAL ? 0x20 : 0x00;
	target[4] = 0xff;
}

static void write_timecode_61(unsigned char* target, struct tm * now,
			      int is16x9)
{
	target[0] = 0x61; /* FIXME: What's this? */

	target[1] = 0x33;
	target[2] = 0xc8 | (is16x9 ? 0x7 : 0x0);
	target[3] = 0xfd;

	target[4] = 0xff;
}

static void write_timecode_62(unsigned char* target, struct tm * now)
{
	target[0] = 0x62;
	target[1] = 0xff;
	write_bcd(target + 2, now->tm_mday);
	write_bcd(target + 3, now->tm_mon+1);
	write_bcd(target + 4, now->tm_year % 100);
}

static void write_timecode_63(unsigned char* target, struct tm * now)
{
	target[0] = 0x63;
	target[1] = 0xff;
	write_bcd(target + 2, now->tm_sec);
	write_bcd(target + 3, now->tm_min);
	write_bcd(target + 4, now->tm_hour);
}

static void write_subcode_blocks(unsigned char* target, int ds, int frame, 
				 struct tm * now, int isPAL)
{
	static int block_count = 0;

	memset(target, 0xff, 2*80);

	target[0] = 0x3f; /* subcode magic */
	target[1] = 0x07 | (ds << 4);
	target[2] = 0x00;

	target[80 + 0] = 0x3f; /* subcode magic */
	target[80 + 1] = 0x07 | (ds << 4);
	target[80 + 2] = 0x01;
	
	target[5] = target[80 + 5] = 0xff;

	if (ds >= 6) {
		target[3] = 0x80 | (block_count >> 8);
		target[4] = block_count;

		target[80 + 3] = 0x80 | (block_count >> 8);
		target[80 + 4] = block_count + 6;

		write_timecode_13(target + 6, now, frame, isPAL);
		write_timecode_13(target + 80 + 6, now, frame, isPAL);

		write_timecode_62(target + 6 + 8, now);
		write_timecode_62(target + 80 + 6 + 8, now);

		write_timecode_63(target + 6 + 2*8, now);
		write_timecode_63(target + 80 + 6 + 2*8, now);

		write_timecode_13(target + 6 + 3*8, now, frame, isPAL);
		write_timecode_13(target + 80 + 6 + 3*8, now, frame, isPAL);

		write_timecode_62(target + 6 + 4*8, now);
		write_timecode_62(target + 80 + 6 + 4*8, now);

		write_timecode_63(target + 6 + 5*8, now);
		write_timecode_63(target + 80 + 6 + 5*8, now);
	} else {
		target[3] = (block_count >> 8);
		target[4] = block_count;

		target[80 + 3] = (block_count >> 8);
		target[80 + 4] = block_count + 6;
		
	}
	block_count += 0x20;
	block_count &= 0xfff;
}

static void write_vaux_blocks(unsigned char* target, int ds, struct tm* now,
			      int isPAL, int is16x9)
{
	memset(target, 0xff, 3*80);

	target[0] = 0x5f; /* vaux magic */
	target[1] = 0x07 | (ds << 4);
	target[2] = 0x00;

	target[0 + 80] = 0x5f; /* vaux magic */
	target[1 + 80] = 0x07 | (ds << 4);
	target[2 + 80] = 0x01;

	target[0 + 2*80] = 0x5f; /* vaux magic */
	target[1 + 2*80] = 0x07 | (ds << 4);
	target[2 + 2*80] = 0x02;


	if ((ds & 1) == 0) {
		if (ds == 0) {
			target[3] = 0x70; /* FIXME: What's this? */
			target[4] = 0xc5;
			target[5] = 0x41; /* 42 ? */
			target[6] = 0x20;
			target[7] = 0xff;
			target[8] = 0x71;
			target[9] = 0xff;
			target[10] = 0x7f;
			target[11] = 0xff;
			target[12] = 0xff;
			target[13] = 0x7f;
			target[14] = 0xff;
			target[15] = 0xff;
			target[16] = 0x38;
			target[17] = 0x81;
		}
	} else {
		write_timecode_60(target + 3, now, isPAL);
		write_timecode_61(target + 3 + 5, now, is16x9);
		write_timecode_62(target + 3 + 2*5, now);
		write_timecode_63(target + 3 + 3*5, now);
	}
	write_timecode_60(target + 2*80+ 48, now, isPAL);
	write_timecode_61(target + 2*80+ 48 + 5, now, is16x9);
	write_timecode_62(target + 2*80+ 48 + 2*5, now);
	write_timecode_63(target + 2*80+ 48 + 3*5, now);
}

static void write_video_headers(unsigned char* target, int frame, int ds)
{
	int i,j, z;
	z = 0;
	for(i = 0; i < 9; i++) {
		target += 80;
		for (j = 1; j < 16; j++) { /* j = 0: audio blocks */
			target[0] = 0x90 | ((frame + 0xb) % 12);
			target[1] = 0x07 | (ds << 4);
			target[2] = z++;
			target += 80;
		}
	}
}

static void write_audio_headers(unsigned char* target, int frame, int ds)
{
	int i, z;
	z = 0;
	for(i = 0; i < 9; i++) {
		memset(target, 0xff, 80);

		target[0] = 0x70 | ((frame + 0xb) % 12);
		target[1] = 0x07 | (ds << 4);
		target[2] = z++;

		target += 16 * 80;
	}
}


void _dv_write_meta_data(unsigned char* target, int frame, int isPAL, 
		     int is16x9, time_t * now)
{
	int numDIFseq;
	int ds;
	struct tm * now_t = NULL;

	numDIFseq = isPAL ? 12 : 10;

	if (frame % (isPAL ? 25 : 30) == 0) {
		(*now)++;
	}

	now_t = localtime(now);

	for (ds = 0; ds < numDIFseq; ds++) { 
		write_header_block(target, ds, isPAL);
		target +=   1 * 80;
		write_subcode_blocks(target, ds, frame, now_t, isPAL);
		target +=   2 * 80;
		write_vaux_blocks(target, ds, now_t, isPAL, is16x9);
		target +=   3 * 80;
		write_video_headers(target, frame, ds);
		write_audio_headers(target, frame, ds);
		target += 144 * 80;
	}
}


/** @brief Write the recording datetime and timecode into a frame of DV video.
 *
 * @param target A pointer to a buffer containing one DV frame,
 *        which should already be allocated width*height*2 bytes.
 * @param isPAL Set true (non-zero) to encode the data in PAL format.
 * @param is16x9 Set true (non-zero) to set the flag bits in the DV
 *          header to indicate widescreen video.
 * @param timecode A time structure containing the date and time to write.
 * @param frame A zero-based running frame counter that is used both in the
 *        frame field of timecode as well as the timestamp on video and audio
 *        DIF blocks.
 */
void dv_encode_metadata(uint8_t *target, int isPAL, int is16x9, time_t *datetime, int frame)
{
	int numDIFseq;
	int ds;
	struct tm now_t;

	numDIFseq = isPAL ? 12 : 10;

	if (frame % (isPAL ? 25 : 30) == 0) {
		(*datetime)++;
	}

	if (localtime_r(datetime, &now_t) != NULL )
	{
		for (ds = 0; ds < numDIFseq; ds++) { 
			target +=   1 * 80;
			write_subcode_blocks(target, ds, frame, &now_t, isPAL);
			target +=   2 * 80;
			write_vaux_blocks(target, ds, &now_t, isPAL, is16x9);
			target +=   3 * 80;
			target += 144 * 80;
		}
	}
}


/** @brief Convert a frame count into a timecode and write into DV frame.
 *
 * @param target A pointer to a buffer containing the DV frame to affect.
 * @param isPAL Set true (non-zero) to base the timecode on PAL framerate,
 *              or false (zero) to use NTSC timecode base.
 * @param frame A zero-based running frame counter.
 */
void dv_encode_timecode(uint8_t* target, int isPAL, int frame)
{
	int numDIFseq  = isPAL ? 12 : 10;
	unsigned char* buf = target;
	int ds;
	struct tm time;
	int cur = frame;
	int fps = isPAL ? 25 : 30;

	if (cur == 0) {
		time.tm_hour = 0;
		time.tm_min = 0;
		time.tm_sec = 0;
	} else {
		time.tm_hour = cur / (fps * 3600);
		cur -= time.tm_hour * (fps * 3600);
		time.tm_min = cur / (fps * 60);
		cur -= time.tm_min * (fps * 60);
		time.tm_sec = cur / fps;
		cur -= time.tm_sec * fps;
	}

	for (ds = 0; ds < numDIFseq; ds++) { 
		buf +=   1 * 80;
		if (ds >= 6) {
			write_timecode_13(buf + 6, &time, cur, isPAL);
			write_timecode_13(buf + 80 + 6, &time, cur, isPAL);
			write_timecode_13(buf + 6 + 3*8, &time, cur, isPAL);
			write_timecode_13(buf + 80 + 6 + 3*8, &time, cur, isPAL);
		}
		buf +=   5 * 80;
		buf += 144 * 80;
	}
}
