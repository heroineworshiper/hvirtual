/* jquant_x86simd.c 
   Quantization / inverse quantization    
   In compiler (gcc) embdeed assembly language...
*/

/* Copyright (C) 2000 Andrew Stevens */

/* This program is free software; you can redistribute it
 *  and/or modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2 of
 *  the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 */



/* 
 * 3DNow 8*8 block Quantiser
 */

#include "jconfig.h"
#include "jmorecfg.h"
#include "mmx.h"
	

/*
 * 3D-Now version: simply truncates to zero rather than round.
 * We could easily.
 */
 
void jcquant_3dnow( INT16 *psrc, INT16 *pdst, FLOAT32 *piqf )

{
	int i;
	for (i=0; i < 64 ; i+=4)
	{

		/* TODO: For maximum efficiency this should be unrolled to allow
		   f.p. and int MMX to be interleaved... 
		*/

		/* Load 4 words, unpack into mm2 and mm3 (with sign extension!)
		 */

		movq_m2r( *(mmx_t *)&psrc[0], mm2 );
		movq_r2r( mm2, mm7 );
		psraw_i2r( 16, mm7 );	/* Replicate sign bits mm2 in mm7 */
		movq_r2r( mm2, mm3 );
		punpcklwd_r2r( mm7, mm2 ); /* Unpack with sign extensions */
		punpckhwd_r2r( mm7, mm3);

		/*
		   Load the inverse quantisation factors from the 
		   table in to mm4 and mm5
		   Interleaved with converting mm2 and mm3 to float's
		   to (hopefully) maximise parallelism.
		 */
		movq_m2r( *(mmx_t*)&piqf[0], mm4);
		pi2fd_r2r( mm2, mm2);
		movq_m2r( *(mmx_t*)&piqf[2], mm5);
		pi2fd_r2r( mm3, mm3);

		/* "Divide" by multiplying by inverse quantisation
		 and convert back to integers*/
		pfmul_r2r( mm4, mm2 );
		pfmul_r2r( mm5, mm3);
		pf2id_r2r( mm2, mm2);
		pf2id_r2r( mm3, mm3);


		/* Convert the two pairs of double words into four words */
		packssdw_r2r(  mm3, mm2);

		piqf += 4;
		psrc += 4;
		movq_r2m( mm2, *(mmx_t*)pdst );
		pdst += 4;
	}
	femms();

}

/*
 * SSE version: this version rounds and so should produce nicer results
 * than the 3D-Now! Version.
 *
 */

static int trunc_mxcsr = 0x1f80;
 
void jcquant_sse( INT16 *psrc, INT16 *pdst, FLOAT32 *piqf )
{
	int i;

	/* Initialise zero block flags */
	/* Set up SSE rounding mode */
	__asm__ ( "ldmxcsr %0\n" : : "m" (trunc_mxcsr) );

	for (i=0; i < 64 ; i+=4)
	{
		/* Load 4 words, unpack into mm2 and mm3 (with sign extension!)
		 */

		movq_m2r( psrc[i], mm2 );
		movq_r2r( mm2, mm7 );
		psraw_i2r( 16, mm7 );	/* Replicate sign bits mm2 in mm7 */
		movq_r2r( mm2, mm3 );
		punpcklwd_r2r( mm7, mm2 ); /* Unpack with sign extensions */
		punpckhwd_r2r( mm7, mm3);

		
		/*
		  Convert mm2 and mm3 to float's  in xmm2 and xmm3
		 */
		cvtpi2ps_r2r( mm2, xmm2 );
		cvtpi2ps_r2r( mm3, xmm3 );
		shufps_r2ri(  xmm3, xmm2, 0*1 + 1*4 + 0 * 16 + 1 * 64 );

		/* "Divide" by multiplying by inverse quantisation
		 and convert back to integers*/
		mulps_m2r( piqf[i], xmm2 );
		cvtps2pi_r2r( xmm2, mm2 );
		shufps_r2ri( xmm2, xmm2, 2*1 + 3*4 + 0 * 16 + 1 * 64 );
		cvtps2pi_r2r( xmm2, mm3 );

		/* Convert the two pairs of double words into four words */
		packssdw_r2r(  mm3, mm2);

		/*piqf += 4;
		  psrc += 4;*/

		movq_r2m( mm2, pdst[i] );

		/*pdst += 4;*/
			
	}
	emms();
}
