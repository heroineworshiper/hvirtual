/* 
 *  enc_output
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
 
#ifndef DV_ENC_OUTPUT_H
#define DV_ENC_OUTPUT_H

#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include "enc_audio_input.h"

#ifdef __cplusplus
extern "C" {
#endif
	#define DV_ENC_MAX_OUTPUT_FILTERS     32

	typedef struct dv_enc_output_filter_s {
		int (*init)(FILE* fp);
		void (*finish)();
		int (*store)(unsigned char* encoded_data, 
			     dv_enc_audio_info_t* audio_data,/* may be null */
			     int keep_meta_headers,
			     int isPAL, int is16x9, time_t now);

		const char* filter_name;
	} dv_enc_output_filter_t;

	extern int _dv_raw_insert_audio(unsigned char * frame_buf, 
		     dv_enc_audio_info_t * audio, int isPAL);

	extern void dv_enc_register_output_filter(dv_enc_output_filter_t 
						  filter);
	extern int dv_enc_get_output_filters(dv_enc_output_filter_t ** filters,
					     int * count);

#ifdef __cplusplus
}
#endif

#endif // DV_ENC_OUTPUT_H
