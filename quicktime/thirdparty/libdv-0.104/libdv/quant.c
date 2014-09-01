/* 
 *  quant.c
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

/** @file
 *  @ingroup quant
 *  @brief Implementation for @link quant Coefficient Quantization @endlink
 */

/** @weakgroup quant Coefficient Quantization
 *
 *  Quantization is the primary means of controlling
 *  compression-quality tradeoff in modern hybrid coders.
 *  
 *  @{
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <math.h>

#include "idct_248.h"
#include "quant.h"

#if ARCH_X86 || ARCH_X86_64
#include <mmx.h>
#endif

static uint8_t dv_88_areas[64] = {
-1,0,0,1,1,1,2,2, 
 0,0,1,1,1,2,2,2, 
 0,1,1,1,2,2,2,3, 
 1,1,1,2,2,2,3,3, 

 1,1,2,2,2,3,3,3, 
 1,2,2,2,3,3,3,3, 
 2,2,2,3,3,3,3,3, 
 2,2,3,3,3,3,3,3 };

static uint8_t dv_248_areas[64] = {
-1,0,1,1,1,2,2,3, 
 0,1,1,2,2,2,3,3, 
 1,1,2,2,2,3,3,3, 
 1,2,2,2,3,3,3,3,

 0,0,1,1,2,2,2,3, 
 0,1,1,2,2,2,3,3, 
 1,1,2,2,2,3,3,3, 
 1,2,2,3,3,3,3,3 };

#if 0
static uint8_t dv_quant_steps[22][4] = {
  { 8,8,16,16 }, 
  { 8,8,16,16 }, 

  { 4,8,8,16 }, 
  { 4,8,8,16 },

  { 4,4,8,8 }, 
  { 4,4,8,8 }, 

  { 2,4,4,8 }, 
  { 2,4,4,8 }, 

  { 2,2,4,4 }, 
  { 2,2,4,4 }, 

  { 1,2,2,4 }, 
  { 1,2,2,4 }, 

  { 1,1,2,2 }, 
  { 1,1,2,2 },

  { 1,1,1,2 }, 

  { 1,1,1,1 }, 
  { 1,1,1,1 }, 
  { 1,1,1,1 }, 
  { 1,1,1,1 }, 
  { 1,1,1,1 }, 
  { 1,1,1,1 }, 
  { 1,1,1,1 }
};
#endif

uint8_t dv_quant_shifts[22][4] = {
  { 3,3,4,4 }, 
  { 3,3,4,4 }, 

  { 2,3,3,4 }, 
  { 2,3,3,4 },

  { 2,2,3,3 }, 
  { 2,2,3,3 }, 

  { 1,2,2,3 }, 
  { 1,2,2,3 }, 

  { 1,1,2,2 }, 
  { 1,1,2,2 }, 

  { 0,1,1,2 }, 
  { 0,1,1,2 }, 

  { 0,0,1,1 }, 
  { 0,0,1,1 },

  { 0,0,0,1 }, 

  { 0,0,0,0 }, 
  { 0,0,0,0 }, 
  { 0,0,0,0 }, 
  { 0,0,0,0 }, 
  { 0,0,0,0 }, 
  { 0,0,0,0 }, 
  { 0,0,0,0 }
};

uint8_t  dv_quant_offset[4] = { 6,3,0,1 };
uint32_t	dv_quant_248_mul_tab [2] [22] [64];
uint32_t dv_quant_88_mul_tab [2] [22] [64];

extern void             _dv_quant_x86(dv_coeff_t *block,int qno,int klass);
extern void             _dv_quant_x86_64(dv_coeff_t *block,int qno,int klass);
static void quant_248_inverse_std(dv_coeff_t *block,int qno,int klass,dv_248_coeff_t *co);
static void quant_248_inverse_mmx(dv_coeff_t *block,int qno,int klass,dv_248_coeff_t *co);

void (*_dv_quant_248_inverse) (dv_coeff_t *block,int qno,int klass,dv_248_coeff_t *co);

void
dv_quant_init (void) 
{
  int	ex, qno, i;
 
  for (ex = 0; ex < 2; ++ex) {
    for (qno = 0; qno < 22; ++qno) {
      for (i = 0; i < 64; ++i) {
 	dv_quant_248_mul_tab [ex] [qno] [i] =
	  (1 << (dv_quant_shifts [qno] [dv_248_areas [i]] + ex)) * dv_idct_248_prescale[i];
      }
      dv_quant_248_mul_tab [ex] [qno] [0] = dv_idct_248_prescale[0];
    }
  }
  _dv_quant_248_inverse = quant_248_inverse_std;
#if ARCH_X86
  if (dv_use_mmx) {
    _dv_quant_248_inverse = quant_248_inverse_mmx;
  }
#elif ARCH_X86_64
    _dv_quant_248_inverse = quant_248_inverse_mmx;
#endif
}

void _dv_quant(dv_coeff_t *block,int qno,int klass) 
{
	if (!(qno == 15 && klass != 3)) { /* Nothing to be done ? */
#if (!ARCH_X86) && (!ARCH_X86_64)
		int i;
		int extra = (klass == 3) ? 1 : 0;
		int factor;
		uint8_t *pq;	/* pointer to the four quantization
				   factors that we'll use */

		pq = dv_quant_shifts[qno+dv_quant_offset[klass]];
		factor = 1 << (pq[0] + extra); 
                /* There is a little difference between
		   shifts and divisions:
		   if you try to quantizise -1 you will 
		   definitely notice it... */
		for (i = 1; i < 1+2+3; i++) {
			block[i] /= factor;
		}
		factor = 1 << (pq[1] + extra);
		for (; i < 1+2+3+4+5+6; i++) {
			block[i] /= factor;
		}
		factor = 1 << (pq[2] + extra);
		for (; i < 1+2+3+4+5+6+7+8+7; i++) {
			block[i] /= factor;
		}
		factor = 1 << (pq[3] + extra);
		for (; i < 64; i++) {
			block[i] /= factor;
		}
#elif ARCH_X86_64
		_dv_quant_x86_64(block, qno, klass);
		emms();
#else
		_dv_quant_x86(block, qno, klass);
		emms();
#endif
	}
}

void _dv_quant_88_inverse(dv_coeff_t *block,int qno,int klass) {
  int i;
  uint8_t *pq;			/* pointer to the four quantization
                                   factors that we'll use */
  int extra;

  extra = (klass == 3);	/* if klass==3, shift
			   everything left one
			   more place */
  pq = dv_quant_shifts[qno + dv_quant_offset[klass]];
  for (i = 1; i < 64; i++)
    block[i] <<= (pq[dv_88_areas[i]] + extra);
}

static void
quant_248_inverse_std(dv_coeff_t *block,int qno,int klass,dv_248_coeff_t *co) {
  int i;
  uint8_t *pq;			/* pointer to the four quantization
                                   factors that we'll use */
  int extra;

  extra = (klass == 3);		/* if klass==3, shift everything left
                                   one more place */
  pq = dv_quant_shifts[qno + dv_quant_offset[klass]];
  co [0] = block [0] * dv_idct_248_prescale[0];
  for (i = 1; i < 64; i++)
    co [i] = (block[i] << (pq[dv_248_areas[i]] + extra)) * dv_idct_248_prescale[i];
}

static void
quant_248_inverse_mmx(dv_coeff_t *block,int qno,int klass,dv_248_coeff_t *co) {
  int i;
  uint32_t *pm;

  pm = dv_quant_248_mul_tab [klass == 3] [qno + dv_quant_offset[klass]];
  for (i = 0; i < 64; i++) {
    co [i] = block [i] * pm [i];
  }
}

/*@}*/
