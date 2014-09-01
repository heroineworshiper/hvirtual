/* 
 *  encodedv.c
 *
 *     Copyright (C) James Bowman  - July 2000
 *                   Peter Schlaile - Jan 2001
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

#include <string.h>
#include "libdv/dv_types.h"
#include "libdv/dv.h"
#include "libdv/encode.h"
#include "libdv/enc_input.h"
#include "libdv/enc_output.h"
#include "libdv/headers.h"

#define DV_ENCODER_OPT_VERSION         0
#define DV_ENCODER_OPT_START_FRAME     1
#define DV_ENCODER_OPT_END_FRAME       2
#define DV_ENCODER_OPT_WRONG_INTERLACE 3
#define DV_ENCODER_OPT_ENCODE_PASSES   4
#define DV_ENCODER_OPT_VERBOSE         5
#define DV_ENCODER_OPT_INPUT           6
#define DV_ENCODER_OPT_AUDIO_INPUT     7
#define DV_ENCODER_OPT_OUTPUT          8
#define DV_ENCODER_OPT_AUTOHELP        9
#define DV_ENCODER_OPT_STATIC_QNO      10
#define DV_ENCODER_OPT_FPS             11
#define DV_ENCODER_OPT_FORCE_DCT       12
#define DV_ENCODER_OPT_16X9            13
#define DV_ENCODER_OPT_STDIN           14
#define DV_ENCODER_NUM_OPTS            15

int main(int argc, char *argv[])
{
	unsigned long start = 0;
	unsigned long end = 0xfffffff;
	int wrong_interlace = 0;
	const char* filename = NULL;
	const char* audio_filename = NULL;
	int vlc_encode_passes = 3;
	int static_qno = 0;
	int verbose_mode = 0;
	const char* audio_input_filter_str = "none";
	const char* input_filter_str = "ppm";
	const char* output_filter_str = "raw";
	dv_enc_audio_input_filter_t * audio_input_filter = NULL;
	dv_enc_input_filter_t * input_filter;
	dv_enc_output_filter_t * output_filter;
	int count = 0;
	int fps = 0;
	int is16x9 = 0;
	int err_code;
	int force_dct = -1;
	int	isStdin = 0;

#if HAVE_LIBPOPT
	struct poptOption option_table[DV_ENCODER_NUM_OPTS+1]; 
	int rc;             /* return code from popt */
	poptContext optCon; /* context for parsing command-line options */
	option_table[DV_ENCODER_OPT_VERSION] = (struct poptOption) {
		longName: "version", 
		val: 'v', 
		descrip: "show encode version number"
	}; /* version */

	option_table[DV_ENCODER_OPT_START_FRAME] = (struct poptOption) {
		longName:   "start-frame", 
		shortName:  's', 
		argInfo:    POPT_ARG_INT, 
		arg:        &start,
		argDescrip: "count",
		descrip:    "start at <count> frame (defaults to 0)"
	}; /* start-frame */

	option_table[DV_ENCODER_OPT_END_FRAME] = (struct poptOption) {
		longName:   "end-frame", 
		shortName:  'e', 
		argInfo:    POPT_ARG_INT, 
		arg:        &end,
		argDescrip: "count",
		descrip:    "end at <count> frame (defaults to unlimited)"
	}; /* end-frames */

	option_table[DV_ENCODER_OPT_WRONG_INTERLACE] = (struct poptOption) {
		longName:   "wrong-interlace", 
		shortName:  'l', 
		arg:        &wrong_interlace,
		descrip:    "flip lines to compensate for wrong interlacing"
	}; /* wrong-interlace */

	option_table[DV_ENCODER_OPT_ENCODE_PASSES] = (struct poptOption) {
		longName:   "vlc-passes", 
		shortName:  'p', 
		argInfo:    POPT_ARG_INT, 
		arg:        &vlc_encode_passes,
		descrip:    "vlc code distribution passes (1-3) "
		"greater values = better quality but "
		"not necessarily slower encoding!"
	}; /* vlc encoder passes */

	option_table[DV_ENCODER_OPT_VERBOSE] = (struct poptOption) {
		longName:   "verbose", 
		shortName:  'v', 
		arg:        &verbose_mode,
		descrip:    "show encoder statistics / status information"
	}; /* verbose mode */

	option_table[DV_ENCODER_OPT_INPUT] = (struct poptOption) {
		longName:   "input", 
		shortName:  'i', 
		arg:        &input_filter_str,
		argInfo:    POPT_ARG_STRING, 
		descrip:    "choose input-filter [>ppm<, pgm, video]"
	}; /* input */

	option_table[DV_ENCODER_OPT_AUDIO_INPUT] = (struct poptOption) {
		longName:   "audio-input", 
		shortName:  'a', 
		arg:        &audio_input_filter_str,
		argInfo:    POPT_ARG_STRING, 
		descrip:    "choose audio-input-filter [>none<, wav, dsp]"
	}; /* audio input */

	option_table[DV_ENCODER_OPT_OUTPUT] = (struct poptOption) {
		longName:   "output", 
		shortName:  'o', 
		arg:        &output_filter_str,
		argInfo:    POPT_ARG_STRING, 
		descrip:    "choose output-filter [>raw<]"
	}; /* output */

	option_table[DV_ENCODER_OPT_STATIC_QNO] = (struct poptOption) {
		longName:   "static-qno", 
		shortName:  'q', 
		arg:        &static_qno,
		argInfo:    POPT_ARG_INT, 
		descrip:    "static qno tables for quantisation on 2 VLC "
		"passes. For turbo (but somewhat lossy encoding) try "
		"-q [1,2] -p 2"
	}; /* start qno */

	option_table[DV_ENCODER_OPT_FPS] = (struct poptOption) {
		longName:   "fps", 
		shortName:  'f', 
		arg:        &fps,
		argInfo:    POPT_ARG_INT, 
		descrip:    "set frames per second (default: use all frames)"
	}; /* fps */

	option_table[DV_ENCODER_OPT_FORCE_DCT] = (struct poptOption) {
		longName:   "force-dct", 
		shortName:  'd', 
		arg:        &force_dct,
		argInfo:    POPT_ARG_INT, 
		descrip:    "force dct mode (88 or 248) for whole picture"
	}; /* force dct */

	option_table[DV_ENCODER_OPT_16X9] = (struct poptOption) {
		longName:   "wide", 
		shortName:  'w', 
		arg:        &is16x9,
		argInfo:    POPT_ARG_INT, 
		descrip:    "set wide 16 x 9 format"
	}; /* 16x9 */

	option_table[DV_ENCODER_OPT_STDIN] = (struct poptOption) {
		longName:   "", 
		shortName:  '-', 
		arg:        &isStdin,
		descrip:    "set stdin input"
	}; /* stdin */

	option_table[DV_ENCODER_OPT_AUTOHELP] = (struct poptOption) {
		argInfo: POPT_ARG_INCLUDE_TABLE,
		arg:     poptHelpOptions,
		descrip: "Help options",
	}; /* autohelp */

	option_table[DV_ENCODER_NUM_OPTS] = (struct poptOption) { 
		NULL, 0, 0, NULL, 0 };

	optCon = poptGetContext(NULL, argc, 
				(const char **)argv, option_table, 0);
	poptSetOtherOptionHelp(optCon, "<filename pattern or - for stdin>"
		" [<audio input>]");

	while ((rc = poptGetNextOpt(optCon)) > 0) {
		switch (rc) {
		case 'v':
			fprintf(stderr,"encode: version %s, "
				"http://libdv.sourceforge.net/\n",
				"CVS 01/14/2001");
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

	if ( !isStdin ) {
		filename = poptGetArg(optCon);
		if(filename == NULL) {
			poptPrintUsage(optCon, stderr, 0);
			fprintf(stderr, 
				"\nSpecify a single <filename pattern> argument; "
				"e.g. pond%%05d.ppm or - for stdin\n"
				"(For audio input specify additionally "
				"the audio source.)\n");
			exit(-1);
		}
	}
	else {
		filename = "-";
	}
 
	audio_filename = poptGetArg(optCon);
	poptFreeContext(optCon);

#else
	/* No popt, no usage and no options!  HINT: get popt if you don't
	 * have it yet, it's at: ftp://ftp.redhat.com/pub/redhat/code/popt 
	 */
	filename = argv[1];
#endif /* HAVE_LIBPOPT */

	dv_enc_get_input_filters(&input_filter, &count);
	while (count 
	       && strcmp(input_filter->filter_name, input_filter_str) != 0) {
		input_filter++;
		count--;
	}
	if (!count) {
		fprintf(stderr, "Unknown input filter selected: %s!\n"
			"The following filters are supported:\n",
			input_filter_str);
		dv_enc_get_input_filters(&input_filter, &count);
		while (count--) {
			fprintf(stderr, "%s\n", input_filter->filter_name);
			input_filter++;
		}
		return(-1);
	}

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
	if (static_qno < 0 || static_qno > 2) {
		fprintf(stderr, "There are only two static qno tables "
			"registered right now:\n"
			"1 : for sharp DV pictures\n"
			"2 : for somewhat noisy satelite television signal\n"
			"\nIf you want to add some more, go ahead ;-)\n");
		return(-1);
	}
	if (static_qno && vlc_encode_passes == 1) {
		fprintf(stderr, "Warning! static_qno enabled but "
			"vlc_passes == 1.\n"
			"This won't gain you anything for now !!!\n");
	}
	if (force_dct != -1) {
		switch (force_dct) {
		case 88:
			force_dct = DV_DCT_88;
			break;
		case 248:
			force_dct = DV_DCT_248;
			break;
		default:
			fprintf(stderr, "--force-dct has to be 88 or 248!\n");
			return(-1);
		}
	} 
 
        dv_init(FALSE, FALSE);

	if (input_filter->init(wrong_interlace, force_dct) < 0) {
		return -1;
	}
	if (output_filter->init(stdout) < 0) {
		return -1;
	}
	/* audio_input_filter->init() is in dv_encoder_loop! */

	err_code = dv_encoder_loop(input_filter,audio_input_filter, output_filter,
				start, end, filename, audio_filename,
				vlc_encode_passes, static_qno,
				verbose_mode, fps, is16x9);

	input_filter->finish();
	if (audio_input_filter) {
		audio_input_filter->finish();
	}
	output_filter->finish();
  
	if (verbose_mode) {
		dv_show_statistics();
	}
	return 0;
}


