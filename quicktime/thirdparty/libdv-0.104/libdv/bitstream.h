/* 
 *  bitstream.h
 *
 *	Copyright (C) Aaron Holtzman - Dec 1999
 * 	Modified by Erik Walthinsen - Feb 2000
 *      Modified by Charles 'Buck' Krasic - April 2000
 *
 *  This file is part of libdv, a free DV (IEC 61834/SMPTE 314M)
 *  codec.
 *
 *  This file was originally part of mpeg2dec, a free MPEG-2 video
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
 * */

#ifndef DV_BITSTREAM_H
#define DV_BITSTREAM_H

#include "dv_types.h"
#if HAVE_ENDIAN_H
#include <endian.h>
#elif HAVE_MACHINE_ENDIAN_H
#include <machine/endian.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

//My new and improved vego-matic endian swapping routine
//(stolen from the kernel)
#if (BYTE_ORDER == BIG_ENDIAN)
#define swab32(x) (x)
#else // LITTLE_ENDIAN
#    define swab32(x)\
((((uint8_t*)&x)[0] << 24) | (((uint8_t*)&x)[1] << 16) |  \
 (((uint8_t*)&x)[2] << 8)  | (((uint8_t*)&x)[3]))
#endif // LITTLE_ENDIAN

bitstream_t *_dv_bitstream_init();
void _dv_bitstream_set_fill_func(bitstream_t *bs,uint32_t (*next_function) (uint8_t **,void *),void *priv);
void _dv_bitstream_next_buffer(bitstream_t * bs);
void _dv_bitstream_new_buffer(bitstream_t *bs,uint8_t *buf,uint32_t len);
void _dv_bitstream_byte_align(bitstream_t *bs);

static void bitstream_next_word(bitstream_t *bs) {
  uint32_t diff = bs->buflen - bs->bufoffset;

  if (diff == 0)
    _dv_bitstream_next_buffer(bs);

  if ((bs->buflen - bs->bufoffset) >=4 ) {
    bs->next_word = *(uint32_t *)(bs->buf + bs->bufoffset);
    bs->next_word = swab32(bs->next_word);
    bs->next_bits = 32;
//    fprintf(stderr,"next_word is %08x at %d\n",bs->next_word,bs->bufoffset);
    bs->bufoffset += 4;
  } else {
    bs->next_word = *(uint32_t *)(bs->buf + bs->buflen - 4);
    bs->next_bits = (bs->buflen - bs->bufoffset) * 8;
//    fprintf(stdout,"end of buffer, have %d bits\n",bs->next_bits);
    _dv_bitstream_next_buffer(bs);
//    fprintf(stderr,"next_word is %08x at %d\n",bs->next_word,bs->bufoffset);
  }
}

//
// The fast paths for _show, _flush, and _get are in the
// bitstream.h header file so they can be inlined.
//
// The "bottom half" of these routines are suffixed _bh
//
// -ah
//

uint32_t static inline bitstream_show_bh(bitstream_t *bs,uint32_t num_bits) {
  uint32_t result;

  result = (bs->current_word << (32 - bs->bits_left)) >> (32 - bs->bits_left);
  num_bits -= bs->bits_left;
  result = (result << num_bits) | (bs->next_word >> (32 - num_bits));

  return result;
}

uint32_t static inline bitstream_get_bh(bitstream_t *bs,uint32_t num_bits) {
  uint32_t result;

  num_bits -= bs->bits_left;
  result = (bs->current_word << (32 - bs->bits_left)) >> (32 - bs->bits_left);

  if (num_bits != 0)
    result = (result << num_bits) | (bs->next_word >> (32 - num_bits));

  bs->current_word = bs->next_word;
  bs->bits_left = 32 - num_bits;
  bitstream_next_word(bs);

  return result;
}

void static inline bitstream_flush_bh(bitstream_t *bs,uint32_t num_bits) {
  //fprintf(stderr,"(flush) current_word 0x%08x, next_word 0x%08x, bits_left %d, num_bits %d\n",current_word,next_word,bits_left,num_bits);

  bs->current_word = bs->next_word;
  bs->bits_left = (32 + bs->bits_left) - num_bits;
  bitstream_next_word(bs);
}

static inline uint32_t bitstream_show(bitstream_t * bs, uint32_t num_bits) {
  if (num_bits <= bs->bits_left)
    return (bs->current_word >> (bs->bits_left - num_bits));

  return bitstream_show_bh(bs,num_bits);
}

static inline uint32_t bitstream_get(bitstream_t * bs, uint32_t num_bits) {
  uint32_t result;

  bs->bitsread += num_bits;

  if (num_bits < bs->bits_left) {
    result = (bs->current_word << (32 - bs->bits_left)) >> (32 - num_bits);
    bs->bits_left -= num_bits;
    return result;
  }

  return bitstream_get_bh(bs,num_bits);
}

// This will fail unpredictably if you try to put more than 32 bits back
static inline void bitstream_unget(bitstream_t *bs, uint32_t data, uint8_t num_bits)
{
  unsigned int high_bits;
  uint32_t mask = (1<<num_bits)-1;

  if (!((num_bits <= 32) && (num_bits >0) && (!(data & (~mask)))))
    return;

  bs->bitsread -= num_bits;
  if(num_bits <= (32 - bs->bits_left)) {
    bs->current_word &= (~(mask << bs->bits_left));
    bs->current_word |= (data << bs->bits_left);
    bs->bits_left += num_bits;
  } else if(bs->bits_left == 32) {
    bs->next_word = bs->current_word;
    bs->current_word = data;
    bs->bits_left = num_bits;
    bs->bufoffset -= 4;
  } else {
    high_bits = 32 - bs->bits_left;
    bs->next_word = 
      (data << bs->bits_left) | 
      ((bs->current_word << high_bits) >> high_bits);
    bs->current_word = (data >> high_bits);
    bs->bits_left = num_bits - high_bits;
    bs->bufoffset -= 4;
  }
}

static inline void bitstream_flush(bitstream_t * bs, uint32_t num_bits) {
  if (num_bits < bs->bits_left)
    bs->bits_left -= num_bits;
  else
    bitstream_flush_bh(bs,num_bits);

  bs->bitsread += num_bits;
}

static inline void bitstream_flush_large(bitstream_t *bs,uint32_t num_bits) {
  int bits = num_bits;

  while (bits > 32) {
    bitstream_flush(bs,32);
    bits -= 32;
  }
  bitstream_flush(bs,bits);
}

static inline void bitstream_seek_set(bitstream_t *bs, uint32_t offset) {
  bs->bufoffset = ((offset & (~0x1f)) >> 5) << 2;
  bs->current_word = *(uint32_t *)(bs->buf + bs->bufoffset);
  bs->current_word = swab32(bs->current_word);
  bs->bufoffset += 4;
  bs->next_word = *(uint32_t *)(bs->buf + bs->bufoffset);
  bs->next_word = swab32(bs->next_word);
  bs->bufoffset += 4;
  bs->bits_left = 32 - (offset & 0x1f);
  bs->next_bits = 32;
  bs->bitsread = offset;
} 


#ifdef __cplusplus
}
#endif

#endif /* DV_BITSTREAM_H */
