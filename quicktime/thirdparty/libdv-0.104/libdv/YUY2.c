/*
 *  YUY2.c
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

/* Most of this file is derived from patch 101018 submitted by Stefan
 * Lucke */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if HAVE_ENDIAN_H
#include <endian.h>
#elif HAVE_MACHINE_ENDIAN_H
#include <machine/endian.h>
#endif
#include <stdlib.h>

#include "YUY2.h"

#if ARCH_X86 || ARCH_X86_64
#include "mmx.h"
#endif // ARCH_X68 | ARCH_X86_64

/* Lookup tables for mapping signed to unsigned, and clamping */
static unsigned char	real_uvlut[256], *uvlut;
static unsigned char	real_ylut[768],  *ylut;
static unsigned char	real_ylut_setup[768],  *ylut_setup;

#if ARCH_X86 || ARCH_X86_64
/* Define some constants used in MMX range mapping and clamping logic */
static mmx_t		mmx_0x0010s = (mmx_t) 0x0010001000100010LL,
			mmx_0x8080s = (mmx_t) 0x8080808080808080LL,
			mmx_zero = (mmx_t) 0x0000000000000000LL,
			mmx_cch  = (mmx_t) 0x0f000f000f000f00LL,
			mmx_ccl  = (mmx_t) 0x1f001f001f001f00LL,
			mmx_ccb  = (mmx_t) 0x1000100010001000LL,
			mmx_clh  = (mmx_t) 0x0014001400140014LL,
			mmx_cll  = (mmx_t) 0x0024002400240024LL,
			mmx_clb  = (mmx_t) 0x0010001000100010LL,
			mmx_cbh  = (mmx_t) 0x0f140f140f140f14LL,
			mmx_cbl  = (mmx_t) 0x1f241f241f241f24LL,
			mmx_cbb  = (mmx_t) 0x1010101010101010LL;

#endif // ARCH_X86 || ARCH_X86_64

/* ----------------------------------------------------------------------------
 */
void
dv_YUY2_init(int clamp_luma, int clamp_chroma) {
  int i;
  int value;

  uvlut = real_uvlut + 128; // index from -128 .. 127
  for(i=-128;
      i<128;
      ++i) {
    value = i + 128;
    if (clamp_chroma == TRUE) value = CLAMP(value, 16, 240);
    uvlut[i] = value;
  } /* for */

  ylut = real_ylut + 256; // index from -256 .. 511
  ylut_setup = real_ylut_setup + 256;
  for(i=-256;
      i<512;
      ++i) {
	value = i + 128;
	if (clamp_luma == TRUE) value = CLAMP(value, 16, 235);
	else value = CLAMP(value, 0, 255);
	ylut[i] = value;
	value += 16;
	ylut_setup[i] = CLAMP(value, 0, 255);
  } /* for */
} /* dv_YUY2_init */

/* ----------------------------------------------------------------------------
 */
void
dv_mb411_YUY2(dv_macroblock_t *mb, uint8_t **pixels, int *pitches, int add_ntsc_setup) {
  dv_coeff_t		*Y[4], *cr_frame, *cb_frame;
  unsigned char	        *pyuv, *pwyuv, cb, cr, *my_ylut;
  int			i, j, row;

  Y [0] = mb->b[0].coeffs;
  Y [1] = mb->b[1].coeffs;
  Y [2] = mb->b[2].coeffs;
  Y [3] = mb->b[3].coeffs;
  cr_frame = mb->b[4].coeffs;
  cb_frame = mb->b[5].coeffs;

  pyuv = pixels[0] + (mb->x * 2) + (mb->y * pitches[0]);
  my_ylut = (add_ntsc_setup == TRUE ? ylut_setup : ylut);

  for (row = 0; row < 8; ++row) { // Eight rows
    pwyuv = pyuv;

    for (i = 0; i < 4; ++i) {     // Four Y blocks
      dv_coeff_t *Ytmp = Y[i];   // less indexing in inner loop speedup?


      for (j = 0; j < 2; ++j) {   // two 4-pixel spans per Y block

        cb = uvlut[CLAMP(*cb_frame, -128, 127)];
        cr = uvlut[CLAMP(*cr_frame, -128, 127)];
        cb_frame++;
        cr_frame++;

#if 0 /* (BYTE_ORDER == BIG_ENDIAN) */
       *pwyuv++ = cb;
       *pwyuv++ = my_ylut[CLAMP(*Ytmp, -256, 511)];
       Ytmp++;
       *pwyuv++ = cr;
	*pwyuv++ = my_ylut[CLAMP(*Ytmp, -256, 511)];
	Ytmp++;
       
	*pwyuv++ = cb;
	*pwyuv++ = my_ylut[CLAMP(*Ytmp, -256, 511)];
	Ytmp++;
	*pwyuv++ = cr;
	*pwyuv++ = my_ylut[CLAMP(*Ytmp, -256, 511)];
	Ytmp++;
#else
	*pwyuv++ = my_ylut[CLAMP(*Ytmp, -256, 511)];
       *pwyuv++ = cb;
	Ytmp++;
       *pwyuv++ = my_ylut[CLAMP(*Ytmp, -256, 511)];
	*pwyuv++ = cr;
       Ytmp++;

       *pwyuv++ = my_ylut[CLAMP(*Ytmp, -256, 511)];
       *pwyuv++ = cb;
       Ytmp++;
       *pwyuv++ = my_ylut[CLAMP(*Ytmp, -256, 511)];
       *pwyuv++ = cr;
       Ytmp++;
#endif
      } /* for j */

      Y[i] = Ytmp;
    } /* for i */

    pyuv += pitches[0];
  } /* for row */
} /* dv_mb411_YUY2 */

/* ----------------------------------------------------------------------------
 */
void
dv_mb411_right_YUY2(dv_macroblock_t *mb, uint8_t **pixels, int *pitches, int add_ntsc_setup) {

  dv_coeff_t		*Y[4], *Ytmp, *cr_frame, *cb_frame;
  unsigned char	        *pyuv, *pwyuv, cb, cr, *my_ylut;
  int			i, j, col, row;


  Y[0] = mb->b[0].coeffs;
  Y[1] = mb->b[1].coeffs;
  Y[2] = mb->b[2].coeffs;
  Y[3] = mb->b[3].coeffs;

  pyuv = pixels[0] + (mb->x * 2) + (mb->y * pitches[0]);
  my_ylut = (add_ntsc_setup == TRUE ? ylut_setup : ylut);

  for (j = 0; j < 4; j += 2) { // Two rows of blocks
    cr_frame = mb->b[4].coeffs + (j * 2);
    cb_frame = mb->b[5].coeffs + (j * 2);

    for (row = 0; row < 8; row++) {
      pwyuv = pyuv;

      for (i = 0; i < 2; ++i) { // Two columns of blocks
        Ytmp = Y[j + i];

        for (col = 0; col < 8; col+=4) {  // two 4-pixel spans per Y block

          cb = uvlut[*cb_frame];
          cr = uvlut[*cr_frame];
	  cb_frame++;
	  cr_frame++;

#if 0 /* (BYTE_ORDER == BIG_ENDIAN) */
         *pwyuv++ = cb;
         *pwyuv++ = my_ylut[CLAMP(*Ytmp, -256, 511)];
         Ytmp++;
         *pwyuv++ = cr;
         *pwyuv++ = my_ylut[CLAMP(*Ytmp, -256, 511)];
         Ytmp++;
       
         *pwyuv++ = cb;
         *pwyuv++ = my_ylut[CLAMP(*Ytmp, -256, 511)];
         Ytmp++;
         *pwyuv++ = cr;
         *pwyuv++ = my_ylut[CLAMP(*Ytmp, -256, 511)];
         Ytmp++;
#else
	  *pwyuv++ = my_ylut[CLAMP(*Ytmp, -256, 511)];
	  Ytmp++;
	  *pwyuv++ = cb;
	  *pwyuv++ = my_ylut[CLAMP(*Ytmp, -256, 511)];
	  Ytmp++;
	  *pwyuv++ = cr;

	  *pwyuv++ = my_ylut[CLAMP(*Ytmp, -256, 511)];
	  Ytmp++;
	  *pwyuv++ = cb;
	  *pwyuv++ = my_ylut[CLAMP(*Ytmp, -256, 511)];
	  Ytmp++;
	  *pwyuv++ = cr;
#endif
        } /* for col */
        Y[j + i] = Ytmp;

      } /* for i */

      cb_frame += 4;
      cr_frame += 4;
      pyuv += pitches[0];
    } /* for row */

  } /* for j */
} /* dv_mb411_right_YUY2 */

/* ----------------------------------------------------------------------------
 */
void
dv_mb420_YUY2 (dv_macroblock_t *mb, uint8_t **pixels, int *pitches) {
    dv_coeff_t    *Y [4], *Ytmp0, *cr_frame, *cb_frame;
    unsigned char *pyuv,*pwyuv0, *pwyuv1,
                  cb, cr;
    int           i, j, col, row, inc_l2, inc_l4;

  pyuv = pixels[0] + (mb->x * 2) + (mb->y * pitches[0]);

  Y [0] = mb->b[0].coeffs;
  Y [1] = mb->b[1].coeffs;
  Y [2] = mb->b[2].coeffs;
  Y [3] = mb->b[3].coeffs;
  cr_frame = mb->b[4].coeffs;
  cb_frame = mb->b[5].coeffs;
  inc_l2 = pitches[0];
  inc_l4 = pitches[0]*2;

  for (j = 0; j < 4; j += 2) { // Two rows of blocks j, j+1
    for (row = 0; row < 4; row++) { // 4 pairs of two rows
      pwyuv0 = pyuv;
      pwyuv1 = pyuv + inc_l4;
      for (i = 0; i < 2; ++i) { // Two columns of blocks
        Ytmp0 = Y[j + i];
        for (col = 0; col < 4; ++col) {  // 4 spans of 2x2 pixels
          cb = uvlut[CLAMP(*cb_frame, -128, 127)];
          cr = uvlut[CLAMP(*cr_frame, -128, 127)];
          cb_frame++;
          cr_frame++;

#if (BYTE_ORDER == LITTLE_ENDIAN)
          *pwyuv0++ = ylut[CLAMP(*(Ytmp0 + 0), -256, 511)];
          *pwyuv0++ = cb;
          *pwyuv0++ = ylut[CLAMP(*(Ytmp0 + 1), -256, 511)];
          *pwyuv0++ = cr;

          *pwyuv1++ = ylut[CLAMP(*(Ytmp0 + 16), -256, 511)];
          *pwyuv1++ = cb;
          *pwyuv1++ = ylut[CLAMP(*(Ytmp0 + 17), -256, 511)];
          *pwyuv1++ = cr;
#else
          *pwyuv0++ = cr;
          *pwyuv0++ = ylut[CLAMP(*(Ytmp0 + 1), -256, 511)];
          *pwyuv0++ = cb;
          *pwyuv0++ = ylut[CLAMP(*(Ytmp0 + 0), -256, 511)];

          *pwyuv1++ = cr;
          *pwyuv1++ = ylut[CLAMP(*(Ytmp0 + 17), -256, 511)];
          *pwyuv1++ = cb;
          *pwyuv1++ = ylut[CLAMP(*(Ytmp0 + 16), -256, 511)];
#endif
          Ytmp0 += 2;
        }
        if (row & 1) {
          Ytmp0 += 16;
        }
        Y[j + i] = Ytmp0;
      }
      pyuv += inc_l2;
      if (row & 1) {
        pyuv += inc_l4;
      }
    }
  }
}

#if ARCH_X86 || ARCH_X86_64

/* TODO (by Buck):
 *
 *   When testing YV12.c, I discovered that my video card (RAGE 128)
 *   doesn't care that pixel components are strictly clamped to Y
 *   (16..235), UV (16..240).  So I was able to use MMX pack with
 *   unsigned saturation to go from 16 to 8 bit representation.  This
 *   clamps to (0..255).  Applying this to the code here might allow
 *   us to reduce instruction count below.
 *
 *   Question:  do other video cards behave the same way?
 *   DRD> No, your video card will not care about clamping to these
 *   ranges. This issue only applies to analog NTSC output.
 * */
void
dv_mb411_YUY2_mmx(dv_macroblock_t *mb, uint8_t **pixels, int *pitches,
                  int add_ntsc_setup, int clamp_luma, int clamp_chroma) {
    dv_coeff_t		*Y[4], *cr_frame, *cb_frame;
    unsigned char	*pyuv, *pwyuv;
    int			i, row;

    Y[0] = mb->b[0].coeffs;
    Y[1] = mb->b[1].coeffs;
    Y[2] = mb->b[2].coeffs;
    Y[3] = mb->b[3].coeffs;
    cr_frame = mb->b[4].coeffs;
    cb_frame = mb->b[5].coeffs;

    pyuv = pixels[0] + (mb->x * 2) + (mb->y * pitches[0]);

    if (clamp_luma && clamp_chroma) {
      movq_m2r (mmx_cbh, mm5);
      movq_m2r (mmx_cbl, mm6);
      movq_m2r (mmx_cbb, mm7);
    } else if (clamp_luma) {
      movq_m2r (mmx_clh, mm5);
      movq_m2r (mmx_cll, mm6);
      movq_m2r (mmx_clb, mm7);
    } else if (clamp_chroma) {
      movq_m2r (mmx_cch, mm5);
      movq_m2r (mmx_ccl, mm6);
      movq_m2r (mmx_ccb, mm7);
    } else {
      movq_m2r (mmx_zero, mm5);
      movq_m2r (mmx_zero, mm6);
      movq_m2r (mmx_zero, mm7);
    }

    if (add_ntsc_setup)
      paddusb_m2r (mmx_0x0010s, mm5);	/* add setup to hi clamp	*/

    for (row = 0; row < 8; ++row) { // Eight rows
      pwyuv = pyuv;
      for (i = 0; i < 4; ++i) {     // Four Y blocks
	dv_coeff_t *Ytmp = Y [i];   // less indexing in inner loop speedup?
	/* ---------------------------------------------------------------------
	 */
	movq_m2r (*cb_frame, mm2);    // cb0 cb1 cb2 cb3
	movq_m2r (*cr_frame, mm3);    // cr0 cr1 cr2 cr3
	punpcklwd_r2r (mm3, mm2); // cb0cr0 cb1cr1
	movq_r2r (mm2, mm3);
	punpckldq_r2r (mm2, mm2); // cb0cr0 cb0cr0
	movq_m2r (Ytmp [0], mm0);
	movq_r2r (mm0, mm1);
	punpcklwd_r2r (mm2, mm0);
	punpckhwd_r2r (mm2, mm1);

	packsswb_r2r (mm1, mm0);
	paddb_m2r (mmx_0x8080s, mm0);
	paddusb_r2r (mm5, mm0);		/* clamp high		*/
	psubusb_r2r (mm6, mm0);		/* clamp low		*/
	paddusb_r2r (mm7, mm0);		/* to black level	*/
	movq_r2m (mm0, pwyuv [0]);

	/* ---------------------------------------------------------------------
	 */
	movq_m2r (Ytmp [4], mm0);
	punpckhdq_r2r (mm3, mm3);
	movq_r2r (mm0, mm1);
	punpcklwd_r2r (mm3, mm0);
	punpckhwd_r2r (mm3, mm1);

	packsswb_r2r (mm1, mm0);
	paddb_m2r (mmx_0x8080s, mm0);
	paddusb_r2r (mm5, mm0);		/* clamp high		*/
	psubusb_r2r (mm6, mm0);		/* clamp low		*/
	paddusb_r2r (mm7, mm0);		/* to black level	*/
	movq_r2m (mm0, pwyuv [8]);

	pwyuv += 16;
	cr_frame += 2;
	cb_frame += 2;
	Y [i] = Ytmp + 8;
      } /* for i */
      pyuv += pitches[0];
    } /* for j */
    emms ();
} /* dv_mb411_YUY2_mmx */

/* ----------------------------------------------------------------------------
 */
void
dv_mb411_right_YUY2_mmx(dv_macroblock_t *mb, uint8_t **pixels, int *pitches,
                        int add_ntsc_setup, int clamp_luma, int clamp_chroma) {

  dv_coeff_t		*Y[4], *Ytmp, *cr_frame, *cb_frame;
  unsigned char	        *pyuv;
  int			j, row;

  Y[0] = mb->b[0].coeffs;
  Y[1] = mb->b[1].coeffs;
  Y[2] = mb->b[2].coeffs;
  Y[3] = mb->b[3].coeffs;

  pyuv = pixels[0] + (mb->x * 2) + (mb->y * pitches[0]);

  if (clamp_luma && clamp_chroma) {
    movq_m2r (mmx_cbh, mm5);
    movq_m2r (mmx_cbl, mm6);
    movq_m2r (mmx_cbb, mm7);
  } else if (clamp_luma) {
    movq_m2r (mmx_clh, mm5);
    movq_m2r (mmx_cll, mm6);
    movq_m2r (mmx_clb, mm7);
  } else if (clamp_chroma) {
    movq_m2r (mmx_cch, mm5);
    movq_m2r (mmx_ccl, mm6);
    movq_m2r (mmx_ccb, mm7);
  } else {
    movq_m2r (mmx_zero, mm5);
    movq_m2r (mmx_zero, mm6);
    movq_m2r (mmx_zero, mm7);
  }

  if (add_ntsc_setup)
    paddusb_m2r (mmx_0x0010s, mm5);	/* add setup to hi clamp	*/

  for (j = 0; j < 4; j += 2) { // Two rows of blocks
    cr_frame = mb->b[4].coeffs + (j * 2);
    cb_frame = mb->b[5].coeffs + (j * 2);

    for (row = 0; row < 8; row++) {

      movq_m2r(*cb_frame, mm0);
      packsswb_r2r(mm0,mm0);
      movq_m2r(*cr_frame, mm1);
      packsswb_r2r(mm1,mm1);
      punpcklbw_r2r(mm1,mm0);
      movq_r2r(mm0,mm1);

      punpcklwd_r2r(mm0,mm0); // pack doubled low cb and crs
      punpckhwd_r2r(mm1,mm1); // pack doubled high cb and crs

      Ytmp = Y[j];

      movq_m2r(Ytmp [0],mm2);
      movq_m2r(Ytmp [4],mm3);

      packsswb_r2r(mm3,mm2);  // pack Ys from signed 16-bit to signed 8-bit
      movq_r2r(mm2,mm3);

      punpcklbw_r2r (mm0, mm3); // interlieve low Ys with crcbs
      paddb_m2r (mmx_0x8080s, mm3);
      paddusb_r2r (mm5, mm3);		/* clamp high		*/
      psubusb_r2r (mm6, mm3);		/* clamp low		*/
      paddusb_r2r (mm7, mm3);		/* to black level	*/
      movq_r2m (mm3, pyuv [0]);

      punpckhbw_r2r (mm0, mm2); // interlieve high Ys with crcbs
      paddb_m2r (mmx_0x8080s, mm2);
      paddusb_r2r (mm5, mm2);		/* clamp high		*/
      psubusb_r2r (mm6, mm2);		/* clamp low		*/
      paddusb_r2r (mm7, mm2);		/* to black level	*/
      movq_r2m (mm2, pyuv [8]);

      Y[j] += 8;

      Ytmp = Y[j+1];

      movq_m2r(Ytmp [0],mm2);
      movq_m2r(Ytmp [4],mm3);

      packsswb_r2r(mm3,mm2); // pack Ys from signed 16-bit to signed 8-bit
      movq_r2r(mm2,mm3);

      punpcklbw_r2r(mm1,mm3);
      paddb_m2r (mmx_0x8080s, mm3);
      paddusb_r2r (mm5, mm3);		/* clamp high		*/
      psubusb_r2r (mm6, mm3);		/* clamp low		*/
      paddusb_r2r (mm7, mm3);		/* to black level	*/
      movq_r2m(mm3, pyuv [16]);

      punpckhbw_r2r(mm1,mm2);  // interlieve low Ys with crcbs
      paddb_m2r (mmx_0x8080s, mm2);
      paddusb_r2r (mm5, mm2);		/* clamp high		*/
      psubusb_r2r (mm6, mm2);		/* clamp low		*/
      paddusb_r2r (mm7, mm2);		/* to black level	*/
      movq_r2m (mm2, pyuv [24]); // interlieve high Ys with crcbs

      Y[j+1] += 8;
      cr_frame += 8;
      cb_frame += 8;

      pyuv += pitches[0];
    } /* for row */

  } /* for j */
  emms();
} /* dv_mb411_right_YUY2_mmx */

/* ----------------------------------------------------------------------------
 */
void
dv_mb420_YUY2_mmx (dv_macroblock_t *mb, uint8_t **pixels, int *pitches,
                   int clamp_luma, int clamp_chroma) {
    dv_coeff_t		*Y [4], *Ytmp0, *cr_frame, *cb_frame;
    unsigned char	*pyuv,
			*pwyuv0, *pwyuv1;
    int			i, j, row, inc_l2, inc_l4;

  pyuv = pixels[0] + (mb->x * 2) + (mb->y * pitches[0]);

  Y [0] = mb->b[0].coeffs;
  Y [1] = mb->b[1].coeffs;
  Y [2] = mb->b[2].coeffs;
  Y [3] = mb->b[3].coeffs;
  cr_frame = mb->b[4].coeffs;
  cb_frame = mb->b[5].coeffs;
  inc_l2 = pitches[0];
  inc_l4 = pitches[0]*2;

  if (clamp_luma && clamp_chroma) {
    movq_m2r (mmx_cbh, mm5);
    movq_m2r (mmx_cbl, mm6);
    movq_m2r (mmx_cbb, mm7);
  } else if (clamp_luma) {
    movq_m2r (mmx_clh, mm5);
    movq_m2r (mmx_cll, mm6);
    movq_m2r (mmx_clb, mm7);
  } else if (clamp_chroma) {
    movq_m2r (mmx_cch, mm5);
    movq_m2r (mmx_ccl, mm6);
    movq_m2r (mmx_ccb, mm7);
  } else {
    movq_m2r (mmx_zero, mm5);
    movq_m2r (mmx_zero, mm6);
    movq_m2r (mmx_zero, mm7);
  }

  for (j = 0; j < 4; j += 2) { // Two rows of blocks j, j+1
    for (row = 0; row < 4; row++) { // 4 pairs of two rows
      pwyuv0 = pyuv;
      pwyuv1 = pyuv + inc_l4;
      for (i = 0; i < 2; ++i) { // Two columns of blocks
        Ytmp0 = Y[j + i];

	/* -------------------------------------------------------------------
	 */
	movq_m2r (*cb_frame, mm2);	/* mm2 = b1 b2 b3 b4	*/
	movq_m2r (*cr_frame, mm3);	/* mm3 = r1 r2 r3 r4	*/
	movq_r2r (mm2, mm4);		/* mm4 = b1 b2 b3 b4	*/
	punpcklwd_r2r (mm3, mm4);	/* mm4 = b3 r3 b4 r4	*/

	movq_m2r (Ytmp0[0], mm0);	/* mm0 = y1 y2 y3 y4	*/
	movq_r2r (mm0, mm1);

	punpcklwd_r2r (mm4, mm0);	/* mm0 = b4 y3 r4 y4	*/
	punpckhwd_r2r (mm4, mm1);	/* mm1 = b3 y1 r3 y2	*/

	packsswb_r2r (mm1, mm0);	/* mm0 = b3 y1 r3 y2 b4 y3 r4 y4	*/
	paddb_m2r (mmx_0x8080s, mm0);
	paddusb_r2r (mm5, mm0);		/* clamp high		*/
	psubusb_r2r (mm6, mm0);		/* clamp low		*/
	paddusb_r2r (mm7, mm0);		/* to black level	*/
	movq_r2m (mm0, pwyuv0[0]);

	movq_m2r (Ytmp0[8+8], mm0);
	movq_r2r (mm0, mm1);

	punpcklwd_r2r (mm4, mm0);
	punpckhwd_r2r (mm4, mm1);
	packsswb_r2r (mm1, mm0);
	paddb_m2r (mmx_0x8080s, mm0);
	paddusb_r2r (mm5, mm0);		/* clamp high		*/
	psubusb_r2r (mm6, mm0);		/* clamp low		*/
	paddusb_r2r (mm7, mm0);		/* to black level	*/
	movq_r2m (mm0, pwyuv1[0]);

	movq_r2r (mm2, mm4);
	punpckhwd_r2r (mm3, mm4);
	movq_m2r (Ytmp0[4], mm0);
	movq_r2r (mm0, mm1);
	punpcklwd_r2r (mm4, mm0);
	punpckhwd_r2r (mm4, mm1);
	packsswb_r2r (mm1, mm0);
	paddb_m2r (mmx_0x8080s, mm0);
	paddusb_r2r (mm5, mm0);		/* clamp high		*/
	psubusb_r2r (mm6, mm0);		/* clamp low		*/
	paddusb_r2r (mm7, mm0);		/* to black level	*/
	movq_r2m (mm0, pwyuv0[8]);

	movq_m2r (Ytmp0[8+12], mm0);
	movq_r2r (mm0, mm1);
	punpcklwd_r2r (mm4, mm0);
	punpckhwd_r2r (mm4, mm1);
	packsswb_r2r (mm1, mm0);
	paddb_m2r (mmx_0x8080s, mm0);
	paddusb_r2r (mm5, mm0);		/* clamp high		*/
	psubusb_r2r (mm6, mm0);		/* clamp low		*/
	paddusb_r2r (mm7, mm0);		/* to black level	*/
	movq_r2m (mm0, pwyuv1[8]);

        pwyuv0 += 16;
        pwyuv1 += 16;
        cb_frame += 4;
        cr_frame += 4;
        if (row & 1) {
          Ytmp0 += 24;
        } else {
          Ytmp0 += 8;
        }
        Y[j + i] = Ytmp0;
      }
      pyuv += inc_l2;
      if (row & 1)
        pyuv += inc_l4;
    }
  }
  emms ();
}

/* ----------------------------------------------------------------------------
 * start of half high render functions
 */

/* ---------------------------------------------------------------------------
 */
void
dv_mb411_YUY2_hh_mmx(dv_macroblock_t *mb, uint8_t **pixels, int *pitches,
                     int add_ntsc_setup, int clamp_luma, int clamp_chroma) {
    dv_coeff_t		*Y[4], *cr_frame, *cb_frame;
    unsigned char	*pyuv, *pwyuv;
    int			i, row;

    Y[0] = mb->b[0].coeffs;
    Y[1] = mb->b[1].coeffs;
    Y[2] = mb->b[2].coeffs;
    Y[3] = mb->b[3].coeffs;
    cr_frame = mb->b[4].coeffs;
    cb_frame = mb->b[5].coeffs;

    pyuv = pixels[0] + (mb->x * 2) + ((mb->y * pitches[0]) / 2);

    if (clamp_luma && clamp_chroma) {
      movq_m2r (mmx_cbh, mm5);
      movq_m2r (mmx_cbl, mm6);
      movq_m2r (mmx_cbb, mm7);
    } else if (clamp_luma) {
      movq_m2r (mmx_clh, mm5);
      movq_m2r (mmx_cll, mm6);
      movq_m2r (mmx_clb, mm7);
    } else if (clamp_chroma) {
      movq_m2r (mmx_cch, mm5);
      movq_m2r (mmx_ccl, mm6);
      movq_m2r (mmx_ccb, mm7);
    } else {
      movq_m2r (mmx_zero, mm5);
      movq_m2r (mmx_zero, mm6);
      movq_m2r (mmx_zero, mm7);
    }

    if (add_ntsc_setup)
      paddusb_m2r (mmx_0x0010s, mm5);	/* add setup to hi clamp	*/

    for (row = 0; row < 4; ++row) { // Eight rows
      pwyuv = pyuv;
      for (i = 0; i < 4; ++i) {     // Four Y blocks
	dv_coeff_t *Ytmp = Y [i];   // less indexing in inner loop speedup?
	/* ---------------------------------------------------------------------
	 */
	movq_m2r (*cb_frame, mm2);	// cb0 cb1 cb2 cb3
	movq_m2r (*cr_frame, mm3);	// cr0 cr1 cr2 cr3
	punpcklwd_r2r (mm3, mm2);	// cb0cr0 cb1cr1
	movq_r2r (mm2, mm3);
	punpckldq_r2r (mm2, mm2);	// cb0cr0 cb0cr0
	movq_m2r (Ytmp [0], mm0);
	movq_r2r (mm0, mm1);
	punpcklwd_r2r (mm2, mm0);	/* mm0 = b4 y3 r4 y4	*/
	punpckhwd_r2r (mm2, mm1);	/* mm1 = b3 y1 r3 y2	*/

	packsswb_r2r (mm1, mm0);	/* mm0 = b3 y1 r3 y2 b4 y3 r4 y4	*/
	paddb_m2r (mmx_0x8080s, mm0);
	paddusb_r2r (mm5, mm0);		/* clamp high		*/
	psubusb_r2r (mm6, mm0);		/* clamp low		*/
	paddusb_r2r (mm7, mm0);		/* to black level	*/
	movq_r2m (mm0, pwyuv [0]);

	/* ---------------------------------------------------------------------
	 */
	movq_m2r (Ytmp [4], mm0);
	punpckhdq_r2r (mm3, mm3);
	movq_r2r (mm0, mm1);
	punpcklwd_r2r (mm3, mm0);	/* mm0 = b4 y3 r4 y4	*/
	punpckhwd_r2r (mm3, mm1);	/* mm1 = b3 y1 r3 y2	*/

	packsswb_r2r (mm1, mm0);	/* mm0 = b3 y1 r3 y2 b4 y3 r4 y4	*/
	paddb_m2r (mmx_0x8080s, mm0);
	paddusb_r2r (mm5, mm0);		/* clamp high		*/
	psubusb_r2r (mm6, mm0);		/* clamp low		*/
	paddusb_r2r (mm7, mm0);		/* to black level	*/
	movq_r2m (mm0, pwyuv [8]);

	pwyuv += 16;

	cr_frame += 2;
	cb_frame += 2;
	Y [i] = Ytmp + 16;
      } /* for i */
      cr_frame += 8;
      cb_frame += 8;
      pyuv += pitches[0];
    } /* for j */
    emms ();
} /* dv_mb411_YUY2_mmx */

/* ----------------------------------------------------------------------------
 */
void
dv_mb411_right_YUY2_hh_mmx(dv_macroblock_t *mb, uint8_t **pixels, int *pitches,
                           int add_ntsc_setup, int clamp_luma, int clamp_chroma) {

  dv_coeff_t		*Y[4], *Ytmp, *cr_frame, *cb_frame;
  unsigned char	        *pyuv;
  int			j, row;

  Y[0] = mb->b[0].coeffs;
  Y[1] = mb->b[1].coeffs;
  Y[2] = mb->b[2].coeffs;
  Y[3] = mb->b[3].coeffs;

  pyuv = pixels[0] + (mb->x * 2) + (mb->y * pitches[0]) / 2;

  if (clamp_luma && clamp_chroma) {
    movq_m2r (mmx_cbh, mm5);
    movq_m2r (mmx_cbl, mm6);
    movq_m2r (mmx_cbb, mm7);
  } else if (clamp_luma) {
    movq_m2r (mmx_clh, mm5);
    movq_m2r (mmx_cll, mm6);
    movq_m2r (mmx_clb, mm7);
  } else if (clamp_chroma) {
    movq_m2r (mmx_cch, mm5);
    movq_m2r (mmx_ccl, mm6);
    movq_m2r (mmx_ccb, mm7);
  } else {
    movq_m2r (mmx_zero, mm5);
    movq_m2r (mmx_zero, mm6);
    movq_m2r (mmx_zero, mm7);
  }

  if (add_ntsc_setup)
    paddusb_m2r (mmx_0x0010s, mm5);	/* add setup to hi clamp	*/

  for (j = 0; j < 4; j += 2) { // Two rows of blocks
    cr_frame = mb->b[4].coeffs + (j * 2);
    cb_frame = mb->b[5].coeffs + (j * 2);

    for (row = 0; row < 4; row++) {

      movq_m2r(*cb_frame, mm0);
      packsswb_r2r(mm0,mm0);
      movq_m2r(*cr_frame, mm1);
      packsswb_r2r(mm1,mm1);
      punpcklbw_r2r(mm1,mm0);
      movq_r2r(mm0,mm1);

      punpcklwd_r2r(mm0,mm0); // pack doubled low cb and crs
      punpckhwd_r2r(mm1,mm1); // pack doubled high cb and crs

      Ytmp = Y[j];

      movq_m2r(Ytmp [0],mm2);
      movq_m2r(Ytmp [4],mm3);

      packsswb_r2r(mm3,mm2);  // pack Ys from signed 16-bit to unsigned 8-bit
      movq_r2r(mm2,mm3);

      punpcklbw_r2r (mm0, mm3); // interlieve low Ys with crcbs
      paddb_m2r (mmx_0x8080s, mm3);
      paddusb_r2r (mm5, mm3);		/* clamp high		*/
      psubusb_r2r (mm6, mm3);		/* clamp low		*/
      paddusb_r2r (mm7, mm3);		/* to black level	*/
      movq_r2m (mm3, pyuv [0]);

      punpckhbw_r2r (mm0, mm2); // interlieve high Ys with crcbs
      paddb_m2r (mmx_0x8080s, mm2);
      paddusb_r2r (mm5, mm2);		/* clamp high		*/
      psubusb_r2r (mm6, mm2);		/* clamp low		*/
      paddusb_r2r (mm7, mm2);		/* to black level	*/
      movq_r2m (mm2, pyuv [8]);

      Y[j] += 16;

      Ytmp = Y[j+1];

      movq_m2r(Ytmp [0],mm2);
      movq_m2r(Ytmp [4],mm3);

      packsswb_r2r(mm3,mm2); // pack Ys from signed 16-bit to unsigned 8-bit
      movq_r2r(mm2,mm3);

      punpcklbw_r2r(mm1,mm3);
      paddb_m2r (mmx_0x8080s, mm3);
      paddusb_r2r (mm5, mm3);		/* clamp high		*/
      psubusb_r2r (mm6, mm3);		/* clamp low		*/
      paddusb_r2r (mm7, mm3);		/* to black level	*/
      movq_r2m(mm3, pyuv [16]);

      punpckhbw_r2r(mm1,mm2);  // interlieve low Ys with crcbs
      paddb_m2r (mmx_0x8080s, mm2);
      paddusb_r2r (mm5, mm2);		/* clamp high		*/
      psubusb_r2r (mm6, mm2);		/* clamp low		*/
      paddusb_r2r (mm7, mm2);		/* to black level	*/
      movq_r2m (mm2, pyuv [24]); // interlieve high Ys with crcbs

      Y[j+1] += 16;
      cr_frame += 16;
      cb_frame += 16;

      pyuv += pitches[0];
    } /* for row */

  } /* for j */
  emms();
} /* dv_mb411_right_YUY2_mmx */

/* ---------------------------------------------------------------------------
 */
void
dv_mb420_YUY2_hh_mmx (dv_macroblock_t *mb, uint8_t **pixels, int *pitches,
                   int clamp_luma, int clamp_chroma) {
    dv_coeff_t		*Y [4], *Ytmp0, *cr_frame, *cb_frame;
    unsigned char	*pyuv,
			*pwyuv0;
    int			i, j, row, inc_l2;

  pyuv = pixels[0] + (mb->x * 2) + (mb->y * pitches[0]) / 2;

  Y [0] = mb->b[0].coeffs;
  Y [1] = mb->b[1].coeffs;
  Y [2] = mb->b[2].coeffs;
  Y [3] = mb->b[3].coeffs;
  cr_frame = mb->b[4].coeffs;
  cb_frame = mb->b[5].coeffs;
  inc_l2 = pitches[0];

  if (clamp_luma && clamp_chroma) {
    movq_m2r (mmx_cbh, mm5);
    movq_m2r (mmx_cbl, mm6);
    movq_m2r (mmx_cbb, mm7);
  } else if (clamp_luma) {
    movq_m2r (mmx_clh, mm5);
    movq_m2r (mmx_cll, mm6);
    movq_m2r (mmx_clb, mm7);
  } else if (clamp_chroma) {
    movq_m2r (mmx_cch, mm5);
    movq_m2r (mmx_ccl, mm6);
    movq_m2r (mmx_ccb, mm7);
  } else {
    movq_m2r (mmx_zero, mm5);
    movq_m2r (mmx_zero, mm6);
    movq_m2r (mmx_zero, mm7);
  }

  for (j = 0; j < 4; j += 2) { // Two rows of blocks j, j+1
    for (row = 0; row < 4; row++) { // 4 pairs of two rows
      pwyuv0 = pyuv;
      for (i = 0; i < 2; ++i) { // Two columns of blocks
        Ytmp0 = Y[j + i];

	/* -------------------------------------------------------------------
	 */
	movq_m2r (*cb_frame, mm2);	/* mm2 = b1 b2 b3 b4	*/
	movq_m2r (*cr_frame, mm3);	/* mm3 = r1 r2 r3 r4	*/
	movq_r2r (mm2, mm4);		/* mm4 = b1 b2 b3 b4	*/
	punpcklwd_r2r (mm3, mm4);	/* mm4 = b3 r3 b4 r4	*/

	movq_m2r (Ytmp0[0], mm0);	/* mm0 = y1 y2 y3 y4	*/
	movq_r2r (mm0, mm1);

	punpcklwd_r2r (mm4, mm0);	/* mm0 = b4 y3 r4 y4	*/
	punpckhwd_r2r (mm4, mm1);	/* mm1 = b3 y1 r3 y2	*/

	packsswb_r2r (mm1, mm0);	/* mm4 = b3 y1 r3 y2 b4 y3 r4 y4	*/
	paddb_m2r (mmx_0x8080s, mm0);
	paddusb_r2r (mm5, mm0);		/* clamp high		*/
	psubusb_r2r (mm6, mm0);		/* clamp low		*/
	paddusb_r2r (mm7, mm0);		/* to black level	*/
	movq_r2m (mm0, pwyuv0[0]);

	movq_r2r (mm2, mm4);
	punpckhwd_r2r (mm3, mm4);
	movq_m2r (Ytmp0[4], mm0);
	movq_r2r (mm0, mm1);
	punpcklwd_r2r (mm4, mm0);	/* mm4 = b4 y3 r4 y4	*/
	punpckhwd_r2r (mm4, mm1);	/* mm5 = b3 y1 r3 y2	*/
	packsswb_r2r (mm1, mm0);	/* mm4 = b3 y1 r3 y2 b4 y3 r4 y4	*/
	paddb_m2r (mmx_0x8080s, mm0);
	paddusb_r2r (mm5, mm0);		/* clamp high		*/
	psubusb_r2r (mm6, mm0);		/* clamp low		*/
	paddusb_r2r (mm7, mm0);		/* to black level	*/
	movq_r2m (mm0, pwyuv0[8]);

        pwyuv0 += 16;
        cb_frame += 4;
        cr_frame += 4;
        Y[j + i] = Ytmp0 + 16;
      }
      /* ---------------------------------------------------------------------
       * for odd value of row counter (this is NOT an odd line number)
       * we have to go one additional step forward, and for even value
       * we have to go one step back to use the color information again.
       * Assuming that chroma information is fields based.
       */
      if (row & 1) {
        cb_frame += 8;
        cr_frame += 8;
      } else {
        cb_frame -= 8;
        cr_frame -= 8;
      }
      pyuv += inc_l2;
    }
  }
  emms ();
}
#endif // ARCH_X86 || ARCH_X86_64
