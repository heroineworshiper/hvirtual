/* 
 *  encode.h
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
 
#ifndef DV_ENCODE_H
#define DV_ENCODE_H

#include "dct.h"
#include "enc_input.h"
#include "enc_output.h"

#define DV_WIDTH       720
#define DV_PAL_HEIGHT  576
#define DV_NTSC_HEIGHT 480

/* FIXME: Just guessed! */
#define DCT_248_THRESHOLD  (17 * 65536 /10)

extern void _dv_init_vlc_test_lookup(void);
extern void _dv_init_vlc_encode_lookup(void);
extern void _dv_init_qno_start(void);
extern void _dv_prepare_reorder_tables(void);
extern void dv_show_statistics(void);
extern int  dv_encoder_loop(dv_enc_input_filter_t * input,
			 dv_enc_audio_input_filter_t * audio_input,
			 dv_enc_output_filter_t * output,
			 int start, int end, const char* filename,
			 const char* audio_filename,
			 int vlc_encode_passes, int static_qno, 
			 int verbose_mode,
			 int fps, int is16x9);

#endif /* DV_ENCODE_H */
