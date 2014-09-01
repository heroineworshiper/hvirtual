/* 
 *  insert_audio.c
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
#include <libdv/enc_audio_input.h>
#include <libdv/enc_input.h>
#include <libdv/enc_output.h>

void generate_empty_frame(unsigned char* frame_buf, int isPAL)
{
	static time_t t = 0;
	static int frame_count = -1;
	if (!t) {
		t = time(NULL);
	}
	if (frame_count == -1) {
		frame_count = isPAL ? 25 : 30;
	}
	if (!--frame_count) {
		frame_count = isPAL ? 25 : 30;
		t++;
	}
	memset(frame_buf, 0, isPAL ? 144000 : 120000);
	_dv_write_meta_data(frame_buf, 0, isPAL, 0, &t);
}

int read_frame(FILE* in_vid, unsigned char* frame_buf, int * isPAL)
{
	if (fread(frame_buf, 1, 120000, in_vid) != 120000) {
		generate_empty_frame(frame_buf, *isPAL);
		return 0;
	}

	*isPAL = (frame_buf[3] & 0x80);

	if (*isPAL) {
		if (fread(frame_buf + 120000, 1, 144000 - 120000, in_vid) !=
		    144000 - 120000) {
			generate_empty_frame(frame_buf, *isPAL);
			return 0;
		}
	}
	return 1;
}

#define OPT_VERSION         0
#define OPT_VERBOSE         1
#define OPT_AUDIO_INPUT     2
#define OPT_OUTPUT          3
#define OPT_TRUNCATE        4
#define OPT_REPEAT          5
#define OPT_AUTOHELP        6
#define NUM_OPTS            7

int main(int argc, const char** argv)
{
	FILE* in_vid;

	const char* filename = NULL;
	const char* audio_filename = NULL;

	unsigned char frame_buf[144000];
	int isPAL = 1;
	int gotframe = 0;
	int have_pipes = 0;
	int verbose_mode = 0;
	int truncate_mode = 0;
	int repeat_count = 0;
	int count;
	const char* audio_input_filter_str = "wav";
	const char* output_filter_str = "raw";

	dv_enc_audio_info_t audio_info_;
	dv_enc_audio_info_t* audio_info;

	dv_enc_audio_input_filter_t * audio_input_filter;
	dv_enc_output_filter_t * output_filter;

	time_t now;

#if HAVE_LIBPOPT
	struct poptOption option_table[NUM_OPTS+1]; 
	int rc;             /* return code from popt */
	poptContext optCon; /* context for parsing command-line options */
	option_table[OPT_VERSION] = (struct poptOption) {
		longName: "version", 
		val: 'v',
		descrip: "show dubdv version number"
	}; /* version */

	option_table[OPT_VERBOSE] = (struct poptOption) {
		longName:   "verbose", 
		shortName:  'v', 
		arg:        &verbose_mode,
		descrip:    "show encoder statistics / status information"
	}; /* verbose mode */

	option_table[OPT_AUDIO_INPUT] = (struct poptOption) {
		longName:   "audio-input", 
		shortName:  'a', 
		arg:        &audio_input_filter_str,
		argInfo:    POPT_ARG_STRING, 
		descrip:    "choose audio-input-filter [none, >wav<, dsp]"
	}; /* audio input */

	option_table[OPT_OUTPUT] = (struct poptOption) {
		longName:   "output", 
		shortName:  'o', 
		arg:        &output_filter_str,
		argInfo:    POPT_ARG_STRING, 
		descrip:    "choose output-filter [>raw<]"
	}; /* output */

 	option_table[OPT_TRUNCATE] = (struct poptOption) {
 		longName:   "truncate", 
 		shortName:  't', 
 		arg:        &truncate_mode,
 		descrip:    "truncate output at end of input"
 	}; /* truncate */

 	option_table[OPT_REPEAT] = (struct poptOption) {
 		longName:   "repeat", 
 		shortName:  'r', 
 		arg:        &repeat_count,
 		argInfo:    POPT_ARG_INT,
 		descrip:    "repeat the first video frame this many times"
 	}; /* repeat */

	option_table[OPT_AUTOHELP] = (struct poptOption) {
		argInfo: POPT_ARG_INCLUDE_TABLE,
		arg:     poptHelpOptions,
		descrip: "Help options",
	}; /* autohelp */

	option_table[NUM_OPTS] = (struct poptOption) { 
		NULL, 0, 0, NULL, 0 };

	optCon = poptGetContext(NULL, argc, 
				(const char **)argv, option_table, 0);
	poptSetOtherOptionHelp(optCon, 
		"<audio input> "
		"[<filename or - for stdin>]");

	while ((rc = poptGetNextOpt(optCon)) > 0) {
		switch (rc) {
		case 'v':
			fprintf(stderr,"insert_audio: version 0.0.2 "
				"http://libdv.sourceforge.net/\n");
			exit(0);
			break;
		default:
			break;
		} /* switch */
	} /* while */

	if (rc < -1) {
		/* an error occurred during option processing */
		fprintf(stderr, "%s: %s\n",
			poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
			poptStrerror(rc));
		exit(-1);
	}

	audio_filename = poptGetArg(optCon);
	if(audio_filename == NULL) {
		poptPrintUsage(optCon, stderr, 0);
		fprintf(stderr, 
			"\nYou have to specify at least a audio input file\n");
		exit(-1);
	}
	filename = poptGetArg(optCon);
	poptFreeContext(optCon);

#else
#warning dubdv does not work without libpopt!

#endif

	if (strcmp(audio_input_filter_str, "none") != 0) {
		dv_enc_get_audio_input_filters(&audio_input_filter, &count);
		while (count 
		       && strcmp(audio_input_filter->filter_name, 
				 audio_input_filter_str) != 0) {
			audio_input_filter++;
			count--;
		}
		if (!count) {
			fprintf(stderr, 
				"Unknown audio input filter selected: %s!\n"
				"The following filters are supported:\n",
				audio_input_filter_str);
			dv_enc_get_audio_input_filters(&audio_input_filter, 
						       &count);
			while (count--) {
				fprintf(stderr, "%s\n", 
					audio_input_filter->filter_name);
				audio_input_filter++;
			}
			return(-1);
		}
		if (!audio_filename) {
			fprintf(stderr, "Audio input selected but no filename "
				"or device given!\n");
			return(-1);
		}
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

	audio_info = &audio_info_;
	if (audio_input_filter->init(audio_filename, audio_info) != 0) {
		return(-1);
	}
	if (verbose_mode) {
		fprintf(stderr, "Opening audio source with:\n"
			"Channels: %d\n"
			"Frequency: %d\n"
			"Bytes per second: %d\n"
			"Byte alignment: %d\n"
			"Bits per sample: %d\n",
			audio_info->channels, audio_info->frequency, 
			audio_info->bytespersecond,
			audio_info->bytealignment, 
			audio_info->bitspersample);
	}

	have_pipes = (filename == NULL || strcmp(filename, "-") == 0);

	if (have_pipes) {
		if (output_filter->init(stdout) != 0) {
			return -1;
		}
		in_vid = stdin;
	} else {
		FILE* out_vid = fopen(filename, "r+");
		in_vid = fopen(filename, "r");
		if (output_filter->init(out_vid) != 0) {
			return -1;
		}

	}

	now = time(NULL);

 	/* repeat the first frame if needed */
 	gotframe = read_frame(in_vid, frame_buf, &isPAL);
 	if (gotframe)
		while (repeat_count--)
			if (output_filter->store(frame_buf, audio_info, 1,
 						isPAL, 0, now) != 0)
 				return -1;
 
	for (;;) {
		if (!gotframe && truncate_mode)
			return 0; /* truncate audio at end of video */

		if (audio_input_filter->load(audio_info, isPAL) != 0) {
			/* end of audio stream */
			if (!truncate_mode) {
				/* copy the rest with original audio */
				while (gotframe) {
					fwrite(frame_buf, 1,
					       isPAL ? 144000 : 120000,
					       stdout);
					gotframe = read_frame(
						in_vid, frame_buf, &isPAL);
				}
			}
			return 0;
		}
		if (output_filter->store(frame_buf, audio_info, 1,
					 isPAL, 0, now) != 0) {
			return -1;
		}
		gotframe = read_frame(in_vid, frame_buf, &isPAL);
	}
}




