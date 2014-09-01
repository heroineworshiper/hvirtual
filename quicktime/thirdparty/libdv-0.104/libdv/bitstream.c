/* 
 *  bitstream.c
 *
 *	Copyright (C) Aaron Holtzman - Dec 1999
 *	Modified by Erik Walthinsen - Feb 2000
 *      Modified by Charles 'Buck' Krasic - April 2000
 *
 *  This file is part of libdv, a free DV (IEC 61834/SMPTE 314M)
 *  codec.
 *
 *  This file was originally part of mpeg2dec, a free MPEG-2 video
 *  decoder.
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
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitstream.h"

void _dv_bitstream_next_buffer(bitstream_t * bs) {
  if (bs->bitstream_next_buffer) {
    bs->buflen = bs->bitstream_next_buffer(&bs->buf,bs->priv);
    bs->bufoffset = 0;
  }
}

void _dv_bitstream_byte_align(bitstream_t *bs) {
  //byte align the bitstream
  bs->bitsread += bs->bits_left & 7;
  bs->bits_left = bs->bits_left & ~7;
  if (!bs->bits_left) {
    bs->current_word = bs->next_word;
    bs->bits_left = bs->next_bits;
    bitstream_next_word(bs);
  }
}

bitstream_t *_dv_bitstream_init() {
  bitstream_t *bs = (bitstream_t *)malloc(sizeof(bitstream_t));
  memset(bs,0,sizeof(bitstream_t));

  return bs;
}

void _dv_bitstream_set_fill_func(bitstream_t *bs,uint32_t (*next_function) (uint8_t **,void *),void *priv) {
  bs->bitstream_next_buffer = next_function;
  bs->priv = priv;

  _dv_bitstream_next_buffer(bs);

  bitstream_next_word(bs);
  bs->current_word = bs->next_word;
  bs->bits_left = bs->next_bits;
  bitstream_next_word(bs);
  bs->bitsread = 0;
}

void _dv_bitstream_new_buffer(bitstream_t *bs,uint8_t *buf,uint32_t len) {
  bs->buf = buf;
  bs->buflen = len;
  bs->bufoffset = 0;

  bitstream_next_word(bs);
  bs->current_word = bs->next_word;
  bs->bits_left = bs->next_bits;
  bitstream_next_word(bs);
  bs->bitsread = 0;
}

uint32_t _dv_bitstream_done(bitstream_t *bs) {
  //FIXME
  return 0;
}

#if 0
void bitstream_seek_set(bitstream_t *bs, uint32_t offset) {
  uint32_t unaligned_bits;
  if((offset >> 3) > bs->buflen) {
    g_return_if_fail((offset >> 3) <= bs->buflen);
  }
  bs->bufoffset = (offset & (~0x1f)) >> 3;
  bitstream_next_word(bs);
  bs->current_word = bs->next_word;
  bs->bits_left = bs->next_bits;
  bitstream_next_word(bs);
  if((unaligned_bits = offset & 0x1f)) 
    bitstream_flush(bs,unaligned_bits);
  bs->bitsread = offset;
} 
#endif


