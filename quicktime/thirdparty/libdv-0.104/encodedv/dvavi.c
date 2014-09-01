/* 
 *  dvavi.c
 *
 *     Copyright (C) Peter Schlaile - January 2002
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include "libdv/dv_types.h"

#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <setjmp.h>
#include <string.h>

#if HAVE_LIBPOPT
#include <popt.h>
#endif

#include <stdlib.h>
#include <stdarg.h>

#include "libdv/headers.h"
#include "libdv/enc_audio_input.h"
#include "libdv/enc_input.h"
#include "libdv/enc_output.h"

static jmp_buf error_jmp_env;

unsigned long read_long(FILE* in_wav)
{
        unsigned char buf[4];
	unsigned long rval;

        if (fread(buf, 1, 4, in_wav) != 4) {
                fprintf(stderr, "AVI: Short read!\n");
                longjmp(error_jmp_env, 1);
        }

        rval = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
	return rval;
}

unsigned long read_short(FILE* in_wav)
{
        unsigned char buf[2];
        if (fread(buf, 1, 2, in_wav) != 2) {
                fprintf(stderr, "AVI: Short read!\n");
                longjmp(error_jmp_env, 1);
        }

        return buf[0] + (buf[1] << 8);
}

int read_header(FILE* in_wav, ...)
{
        unsigned char buf[4];
	char* header;
	int i = 1;

	va_list ap;
	va_start(ap, in_wav);

        if (fread(buf, 1, 4, in_wav) != 4) {
                fprintf(stderr, "AVI: Short read!\n");
                longjmp(error_jmp_env, 1);
        }

	for (;;) {
		header = va_arg(ap, char *);
		if (!header) break;
		if (memcmp(buf, header, 4) == 0) {
			return i;
		}
		i++;
	}
	return 0;
}

void read_header_excp(FILE* in_wav, char* header)
{
	if (!read_header(in_wav, header, NULL)) {
                fprintf(stderr, "AVI: No %s header!\n", header);
                longjmp(error_jmp_env, 1);
	}
}

void skip(FILE* in_avi, unsigned long amount)
{
	if (!amount) {
		return;
	}
	if (fseek(in_avi, amount, SEEK_CUR) == -1) { 
		char buf[1024];

		while (amount > 1024) {
			if (fread(buf, 1, 1024, in_avi) < 1024) {
				fprintf(stderr, "AVI: short read in skip!\n");
				longjmp(error_jmp_env, 1);
			}
			amount-=1024;
		}
		if (fread(buf, 1, amount, in_avi) < amount) {
			fprintf(stderr, "AVI: short read in skip!\n");
			longjmp(error_jmp_env, 1);
		}
	}
}

#define PAL_SIZE  144000
#define NODE_SIZE PAL_SIZE

struct buf_node {
	unsigned char buffer[NODE_SIZE];
	int usage;
	int processed;
	struct buf_node* next;
};

struct buf_list {
	struct buf_node * first;
	struct buf_node * last;
	int usage;
};

static struct buf_list free_list  = { NULL, NULL, 0 };
static struct buf_list video_bufs = { NULL, NULL, 0 };
static struct buf_list audio_bufs = { NULL, NULL, 0 };

int max_buffer_blocks = 25;

void push_back(struct buf_list * l, struct buf_node * elem)
{
	if (l->last) {
		l->last->next = elem;
	} else {
		l->first = elem;
	}
	elem->next = NULL;
	l->last = elem;
	l->usage++;
}

struct buf_node * pop_front(struct buf_list * l)
{
	struct buf_node * rval;

	rval = l->first;

	if (l->first == l->last) {
		l->last = NULL;
	}
	if (rval) {
		l->first = rval->next;
		l->usage--;
	} else {
		l->first = NULL;
	}

	return rval;
}

struct buf_node * get_free_block()
{
	struct buf_node * f = pop_front(&free_list);
	if (!f) {
		if (audio_bufs.usage >= max_buffer_blocks
		    || video_bufs.usage >= max_buffer_blocks) {
			fprintf(stderr, "AVI: max buffer limit reached!\n");
			longjmp(error_jmp_env, 1);
		}
		f = (struct buf_node*) malloc(sizeof(struct buf_node));
	}
	return f;
}

dv_enc_audio_info_t audio_info;

void get_audio_header(FILE* in_avi, int * len_, 
		      dv_enc_audio_info_t * res)
{
	int len = *len_;
	int head_len;
	int avihead_len;

	len -= 4;
	if (!read_header(in_avi, "avih", NULL)) {
		*len_ = len;
		return;
	}
	len -= 4; 
	avihead_len = read_long(in_avi); 

	skip(in_avi, avihead_len);
	len -= avihead_len;

	while (len > 0) {
		int strl_len;

		len -= 4;
		if (!read_header(in_avi, "LIST", NULL)) {
			*len_ = len;
			return;
		}

		len -= 4;
		strl_len = read_long(in_avi);

		len -= 4;
		strl_len -= 4;
		if (!read_header(in_avi, "strl", NULL)) {
			*len_ = len;
			return;
		}

		len -= 4;
		strl_len -= 4;
		if (!read_header(in_avi, "strh", NULL)) {
			*len_ = len;
			return;
		}

		head_len = read_long(in_avi);
		len -= 8;
		strl_len -= 8;
		if (!read_header(in_avi, "auds", NULL)) {
			skip(in_avi, strl_len);
			len -= strl_len;
		} else {
			break;
		}
	}
	if (len <= 0) {
		*len_ = len;
		return;
	}

	skip(in_avi, head_len - 4);
	len -= head_len - 4;

	len -= 4;
	if (read_header(in_avi, "strf", NULL)) {
		int ft;
		len -= 4;
		read_long(in_avi); /* len */
		ft = read_short(in_avi); /* format tag */
		fprintf(stderr, 
			"Found audio channel: format: ft=%d\n", ft);
		len -= 2;
		if (ft != 1) {
			fprintf(stderr, "Not supported!!!\n");
			*len_ = len;
			return;
		}
		res->channels = read_short(in_avi);
		res->frequency = read_long(in_avi);
		res->bytespersecond = read_long(in_avi); /* bytes per second */
		res->bytealignment = read_short(in_avi); /* byte alignment */
		res->bitspersample = read_short(in_avi);

		fprintf(stderr, "Opening audio source with:\n"
			"Channels: %d\n"
			"Frequency: %d\n"
			"Bytes per second: %d\n"
			"Byte alignment: %d\n"
			"Bits per sample: %d\n",
			res->channels, res->frequency, 
			res->bytespersecond,
			res->bytealignment, 
			res->bitspersample);

		len -= 2 + 4 + 4 + 2 + 2;
	}
	*len_ = len;
}

static void convert_s16_le(unsigned char* in_buf, unsigned char* out_buf,
			   int num_samples)
{
	int i;
	for (i = 0; i < num_samples; i++) {
		char a = in_buf[1];
		char b = in_buf[0];
		*out_buf++ = a;
		*out_buf++ = b;
		in_buf += 2;
	}
}

int run_queue(dv_enc_output_filter_t * output_filter, time_t now, int isPAL)
{
	struct buf_node * it;
	struct buf_node * next;
	int audio_needed;
	unsigned char * p;

	if (!audio_bufs.usage || !video_bufs.usage) {
		return 0;
	}

	audio_needed = audio_info.bytesperframe;

	for (it = audio_bufs.first; it != NULL; it = it->next) {
		audio_needed -= it->usage - it->processed;
		if (audio_needed <= 0) {
			break;
		}
	}
	if (audio_needed > 0) {
		return 0;
	}

	p = audio_info.data;
	audio_needed = audio_info.bytesperframe;

	for (it = audio_bufs.first; it != NULL; it = next) {
		int real_usage = it->usage - it->processed;
		next = it->next;
		if (real_usage <= audio_needed) {
			memcpy(p, it->buffer + it->processed, real_usage);
			p += real_usage;
			audio_needed -= real_usage;
			push_back(&free_list, pop_front(&audio_bufs));
		} else {
			memcpy(p, it->buffer + it->processed, audio_needed);
			it->processed += audio_needed;
			audio_needed = 0;
		}
		if (audio_needed == 0) {
			break;
		}
	}
	
	it = pop_front(&video_bufs);

	convert_s16_le(audio_info.data, audio_info.data, 
		       audio_info.bytesperframe/2);

	if (output_filter->store(it->buffer, &audio_info, 0,
				 isPAL, 0, now) != 0) {
		fprintf(stderr, "AVI: write error!\n");
		longjmp(error_jmp_env, 1);
	}
	
	push_back(&free_list, it);

	return 1;
}

int read_avi(FILE* in_avi, dv_enc_output_filter_t * out_filter,
	     FILE* raw_dv_out)
{
        int len;
	int blocktype;
	int frame_count;
	time_t now = time(NULL);
	volatile int isPAL = 0;

        if (setjmp(error_jmp_env)) {
		while (run_queue(out_filter, now, isPAL));
                return -1;
        }

	frame_count = 0;

        read_header_excp(in_avi, "RIFF");
        read_long(in_avi); /* ignore length, this is important,
                              since stream generated avis do not have a
                              valid length! */
	read_header_excp(in_avi, "AVI ");
	len = 0;
	for (;;) {
		skip(in_avi, len);
		while (!read_header(in_avi, "LIST", NULL)) {
			skip(in_avi, read_long(in_avi));
		}
		len = read_long(in_avi) - 4;
		blocktype = read_header(in_avi, "movi", "hdrl", NULL);
		if (blocktype == 1) {
			break;
		} else if (blocktype == 2) {
			get_audio_header(in_avi, &len, &audio_info);
		}
	} 

	for (;;) {
		struct buf_node * bn = NULL;

		for (;;) {
			blocktype = read_header(
				in_avi, "00dc", "01wb", "00__", NULL);
			if (blocktype != 0) {
				break;
			}
			skip(in_avi, read_long(in_avi));
		}
		len = read_long(in_avi);

		switch (blocktype) {
		case 2: /* audio : DV format 2 */
			if (!audio_bufs.usage ||
			    (audio_bufs.last->usage + len > NODE_SIZE)) {
				bn = get_free_block();
				fread(bn->buffer, 1, len, in_avi);
				bn->usage = len;
				bn->processed = 0;
				push_back(&audio_bufs, bn);
			} else {
				bn = audio_bufs.last;
				fread(bn->buffer + bn->usage, 1, len, in_avi);
				bn->usage += len;
			}
			break;
		case 1: /* video : DV format 2 */
			bn = get_free_block();
			if (len > NODE_SIZE) {
				fprintf(stderr, "AVI: DV frame too large!\n");
				longjmp(error_jmp_env, 1);
			}
			isPAL = (len == PAL_SIZE);
			fread(bn->buffer, 1, len, in_avi);
			audio_info.bytesperframe = 
				audio_info.bytespersecond / (isPAL ? 25 : 30);
			bn->usage = len;
			push_back(&video_bufs, bn);
			break;
		case 3: /* video / audio : DV format 1*/
			bn = get_free_block();
			if (len > NODE_SIZE) {
				fprintf(stderr, "AVI: DV frame too large!\n");
				longjmp(error_jmp_env, 1);
			}
			fread(bn->buffer, 1, len, in_avi);
			fwrite(bn->buffer, 1, len, raw_dv_out);

			push_back(&free_list, bn);
			break;
		}
		while (run_queue(out_filter, now, isPAL));
	} 
	while (run_queue(out_filter, now, isPAL));
	return 0;
}

int main(int argc, const char** argv)
{
	FILE* in_avi = NULL;
	const char* output_filter_str = "raw";
	dv_enc_output_filter_t * output_filter;
	int count;

	if (argc < 2) {
		fprintf(stderr, "Usage: dvavi avi-file > raw-dv-file\n");
		exit(0);
	}
	
	if (strcmp(argv[1], "-") == 0) {
		in_avi = stdin;
	} else {
		in_avi = fopen(argv[1], "r");
	}

	if (in_avi == NULL) {
		fprintf(stderr, "AVI: can't open file: %s\n", argv[1]);
		exit(-1);
	}

	dv_enc_get_output_filters(&output_filter, &count);
	while (count && 
	       strcmp(output_filter->filter_name, output_filter_str) != 0) {
		output_filter++;
		count--;
	}
	if (!count) {
		fprintf(stderr, "Unknown output filter selected: %s!\n"
			"The following filters are supported:\n",
			output_filter_str);
		dv_enc_get_output_filters(&output_filter, &count);
		while (count--) {
			fprintf(stderr, "%s\n", output_filter->filter_name);
			output_filter++;
		}
		return(-1);
	}

	output_filter->init(stdout);

	return read_avi(in_avi, output_filter, stdout);
}



