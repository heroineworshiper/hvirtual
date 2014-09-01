/*
 *  audio.h
 *
 *     Copyright (C) Charles 'Buck' Krasic - January 2001
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

#ifndef DV_AUDIO_H
#define DV_AUDIO_H

#include "dv_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Low-level routines */
extern dv_audio_t *dv_audio_new(void);
extern int         dv_parse_audio_header(dv_decoder_t *decoder, const uint8_t *inbuf);
extern int         dv_update_num_samples(dv_audio_t *dv_audio, const uint8_t *inbuf);
extern int         dv_decode_audio_block(dv_audio_t *dv_audio, const uint8_t *buffer, int ds, int audio_dif, int16_t **outbufs);
extern void        dv_audio_deemphasis(dv_audio_t *dv_audio, int16_t **outbuf);
extern void        dv_dump_aaux_as(void *buffer, int ds, int audio_dif);
extern void        dv_audio_correct_errors (dv_audio_t *dv_audio, int16_t **outbufs);
extern void        dv_audio_mix4ch (dv_audio_t *dv_audio, int16_t **outbufs);

#ifdef __cplusplus
}
#endif

#endif // DV_AUDIO_H
