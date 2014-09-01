/*
 *  vlc.h
 *
 *     Copyright (C) Charles 'Buck' Krasic - April 2000
 *     Copyright (C) Erik Walthinsen - April 2000
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

#ifndef DV_VLC_H
#define DV_VLC_H

#include "bitstream.h"

#define VLC_NOBITS (-1)
#define VLC_ERROR (-2)

typedef struct dv_vlc_s {
  int8_t run;
  int8_t len;
  int16_t amp;
} dv_vlc_t;

#if 1
typedef dv_vlc_t dv_vlc_tab_t;
#else
typedef struct dv_vlc_tab_s {
  int8_t run;
  int8_t len;
  int16_t amp;
} dv_vlc_tab_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern int8_t *dv_vlc_classes[];
extern int dv_vlc_class_index_mask[];
extern int dv_vlc_class_index_rshift[];
extern const dv_vlc_tab_t dv_vlc_broken[1];
extern const dv_vlc_tab_t *dv_vlc_lookups[6];
extern const int dv_vlc_index_mask[6];
extern const int dv_vlc_index_rshift[6];
extern const int sign_lookup[2];
extern const int sign_mask[17];
extern const int sign_rshift[17];
extern void dv_construct_vlc_table();

// Note we assume bits is right (lsb) aligned, 0 < maxbits < 17
// This may look crazy, but there are no branches here.
extern void dv_decode_vlc(int bits,int maxbits, dv_vlc_t *result);
extern void __dv_decode_vlc(int bits, dv_vlc_t *result);

static inline void dv_peek_vlc(bitstream_t *bs,int maxbits, dv_vlc_t *result) {
  if(maxbits < 16)
    dv_decode_vlc(bitstream_show(bs,16),maxbits,result);
  else
    __dv_decode_vlc(bitstream_show(bs,16),result);
} // dv_peek_vlc

#ifdef __cplusplus
}
#endif

#endif // DV_VLC_H
