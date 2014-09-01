/*
 *  rgb.c
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


#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <math.h>

#include "rgb.h"

#define COLOR_FRACTION_BITS 10
#define COLOR_FRACTION_MUL  (1 << COLOR_FRACTION_BITS)

/* yuv -> rgb converion lookup tables.  These tables are constructed
 * from the standard formulas, plus the following assumptions:
 *
 *  - yuv index values are signed (i.e. offset by -128)
 *  - yuv index values may be out of range and should be clamped
 *    to 16-235 for y, and 16-240 for uv.
 *
 * The standard formulas are:
 *
 *    G = 1.164(Y-16) - 0.391(U-128)  0.813(V-128)
 *    R = 1.164(Y-16) - 1.596(V-128)
 *    B = 1.164(Y-16) - 2.018(U-128)
 * 
 * Note that you need to clamp values both before and after
 * the formula.  Given the limits above for YUV, the range of 
 * values from the formula are:
 *    
 *   -179 < R < 433
 *   -135 < G < 390
 *   -227 < B < 365
 *
 * These must be clamped to 0 .. 255 for the correct final 
 * 8-bit pixel components.
 */  

/* lookups for the terms in the formula above.  Note we use
 * pointer arithmetic to make use of negative index values legal.
 */

static int32_t real_table_2_018[256];
static int32_t real_table_0_813[256];
static int32_t real_table_0_391[256];
static int32_t real_table_1_596[256];

static int32_t *table_2_018;
static int32_t *table_0_813;
static int32_t *table_0_391;
static int32_t *table_1_596;

static int32_t real_ylut[768], *ylut;
static int32_t real_ylut_setup[768], *ylut_setup;

/* rgb lookup - clamps values in range -256 .. 511 to 0 .. 255 */
static uint8_t real_rgblut[768], *rgblut;

/* ---------------------------------------------------------------------------
 */
static inline int
clamp (int low, int val, int high) {
  return ((val > high) ? high : (val < low) ? low :val);
}

/* ---------------------------------------------------------------------------
 */
void
dv_rgb_init(int clamp_luma, int clamp_chroma) {
  int i;
  int clamped_offset;
  table_2_018 = real_table_2_018 + 128;
  table_0_813 = real_table_0_813 + 128;
  table_0_391 = real_table_0_391 + 128;
  table_1_596 = real_table_1_596 + 128;

  for(i=-128;
      i<128;
      ++i) {
    if ((clamp_chroma == TRUE) && (i < (16-128))) {
      clamped_offset = (16-128);
    } else if ((clamp_chroma == TRUE) && (i > (240-128))) {
      clamped_offset = (240-128);
    } else {
      clamped_offset = i;
    } // else
    table_2_018[i] = (int32_t)rint(2.018 * COLOR_FRACTION_MUL * clamped_offset);
    table_0_813[i] = (int32_t)rint(0.813 * COLOR_FRACTION_MUL * clamped_offset);
    table_0_391[i] = (int32_t)rint(0.391 * COLOR_FRACTION_MUL * clamped_offset);
    table_1_596[i] = (int32_t)rint(1.596 * COLOR_FRACTION_MUL * clamped_offset);
  } // for

  ylut = real_ylut + 256;
  ylut_setup = real_ylut_setup + 256;
  for(i=-256; i < 512; i++) {
    clamped_offset = i + 128 - 16;
    if (clamp_luma == TRUE) clamped_offset = CLAMP(clamped_offset, 16, 235);
    ylut[i] = (int32_t)rint(1.164 * COLOR_FRACTION_MUL * clamped_offset);
    ylut_setup[i] = (int32_t)rint(1.164 * COLOR_FRACTION_MUL * (clamped_offset+16));
  } // for

  rgblut = real_rgblut + 256;
  for(i=-256; i < 512; i++) {
    rgblut[i] = CLAMP(i, 0, 255);
  } // for
} /* dv_rgb_init */

/* ---------------------------------------------------------------------------
 */
void
dv_mb411_rgb(dv_macroblock_t *mb, uint8_t **pixels, int *pitches, int add_ntsc_setup) {
  dv_coeff_t *Y[4], *cr_frame, *cb_frame;
  uint8_t *prgb, *pwrgb;
  int i,j,k, row;

  Y[0] = mb->b[0].coeffs;
  Y[1] = mb->b[1].coeffs;
  Y[2] = mb->b[2].coeffs;
  Y[3] = mb->b[3].coeffs;
  cr_frame = mb->b[4].coeffs;
  cb_frame = mb->b[5].coeffs;

  prgb = pixels[0] + (mb->x * 3)  + (mb->y * pitches[0]);

  for (row = 0; row < 8; ++row) { // Eight rows
    pwrgb = prgb;
    for (i = 0; i < 4; ++i) {     // Four Y blocks
      dv_coeff_t *Ytmp = Y[i]; // less indexing in inner loop speedup?
      for (j = 0; j < 2; ++j) {   // two 4-pixel spans per Y block
        int8_t cb = clamp (-128, *cb_frame++, 127);  /* -128,-1  => 0x80,0xff */
        int8_t cr = clamp (-128, *cr_frame++, 127);
        int ro = table_1_596[cr];
        int go = table_0_813[cr] + table_0_391[cb];
        int bo =                   table_2_018[cb];

        for (k = 0; k < 4; ++k) { // 4-pixel span
          int32_t y = (add_ntsc_setup == TRUE) ?
	  		ylut_setup[clamp (-256, *Ytmp++, 511)] :
				ylut[clamp (-256, *Ytmp++, 511)];
          int32_t r = (y + ro) >> COLOR_FRACTION_BITS;
          int32_t g = (y - go) >> COLOR_FRACTION_BITS;
          int32_t b = (y + bo) >> COLOR_FRACTION_BITS;
          *pwrgb++ = rgblut[r];
          *pwrgb++ = rgblut[g];
          *pwrgb++ = rgblut[b];
        } // for k
      } // for j
      Y[i] = Ytmp;
    } // for i
    prgb += pitches[0];
  } // for row
} // dv_mb411_rgb

/* ---------------------------------------------------------------------------
 */
void
dv_mb411_right_rgb(dv_macroblock_t *mb, uint8_t **pixels, int *pitches, int add_ntsc_setup) {
  dv_coeff_t *Ytmp;
  dv_coeff_t *Y[4], *cr_frame, *cb_frame;
  uint8_t *prgb, *pwrgb;
  int i, j, k, row, col;

  Y[0] = mb->b[0].coeffs;
  Y[1] = mb->b[1].coeffs;
  Y[2] = mb->b[2].coeffs;
  Y[3] = mb->b[3].coeffs;

  prgb = pixels[0] + (mb->x * 3) + (mb->y * pitches[0]);

  for (j = 0; j < 4; j += 2) { // Two rows of blocks j, j+1

    cr_frame = mb->b[4].coeffs + (j*2);
    cb_frame = mb->b[5].coeffs + (j*2);

    for (row = 0; row < 8; row++) {
      pwrgb = prgb;

      for (i = 0; i < 2; ++i) { // Two columns of blocks
	Ytmp = Y[j+i];

        for (col = 0; col < 8; col+=4) {  // 4 spans of 2x2 pixels

          int8_t cb = clamp (-128, *cb_frame++, 127); // +128;
          int8_t cr = clamp (-128, *cr_frame++, 127); // +128

	  int ro = table_1_596[cr];
	  int go = table_0_813[cr] + table_0_391[cb];
	  int bo =                   table_2_018[cb];

          for (k = 0; k < 4; ++k) { // 4x1 pixels

            int32_t y = (add_ntsc_setup == TRUE) ?
	    			ylut_setup[clamp (-256, *Ytmp++, 511)] :
					ylut[clamp (-256, *Ytmp++, 511)];
            int32_t r = (y + ro) >> COLOR_FRACTION_BITS;
            int32_t g = (y - go) >> COLOR_FRACTION_BITS;
            int32_t b = (y + bo) >> COLOR_FRACTION_BITS;

	    *pwrgb++ = rgblut[r];
	    *pwrgb++ = rgblut[g];
	    *pwrgb++ = rgblut[b];
          } // for k

        } // for col

        Y[j+i] = Ytmp;
      } // for i

      cb_frame += 4;
      cr_frame += 4;
      prgb += pitches[0];
    } // for row

  } // for j
} /* dv_mb411_right_rgb */

/* ---------------------------------------------------------------------------
 */
void
dv_mb420_rgb(dv_macroblock_t *mb, uint8_t **pixels, int *pitches) {
  dv_coeff_t *Y[4], *cr_frame, *cb_frame;
  uint8_t *prgb, *pwrgb0, *pwrgb1;
  int i, j, k, row, col;

  Y[0] = mb->b[0].coeffs;
  Y[1] = mb->b[1].coeffs;
  Y[2] = mb->b[2].coeffs;
  Y[3] = mb->b[3].coeffs;
  cr_frame = mb->b[4].coeffs;
  cb_frame = mb->b[5].coeffs;

  prgb = pixels[0] + (mb->x * 3) + (mb->y * pitches[0]);

  for (j = 0; j < 4; j += 2) { // Two rows of blocks j, j+1

    for (row = 0; row < 4; row++) { // 4 pairs of two rows
      pwrgb0 = prgb;
      pwrgb1 = prgb + pitches[0] * 2;

      for (i = 0; i < 2; ++i) { // Two columns of blocks
        int yindex = j + i;
        dv_coeff_t *Ytmp0 = Y[yindex];
        dv_coeff_t *Ytmp1 = Y[yindex] + 16;
        for (col = 0; col < 4; ++col) {  // 4 spans of 2x2 pixels
          int8_t cb = clamp (-128, *cb_frame++, 127); // +128;
          int8_t cr = clamp (-128, *cr_frame++, 127); // +128
	  int ro = table_1_596[cr];
	  int go = table_0_813[cr] + table_0_391[cb];
	  int bo =                   table_2_018[cb];

          for (k = 0; k < 2; ++k) { // 2x2 pixel
            int32_t y = ylut[clamp (-256, *Ytmp0++, 511)];
            int32_t r = (y + ro) >> COLOR_FRACTION_BITS;
            int32_t g = (y - go) >> COLOR_FRACTION_BITS;
            int32_t b = (y + bo) >> COLOR_FRACTION_BITS;
	    *pwrgb0++ = rgblut[r];
	    *pwrgb0++ = rgblut[g];
	    *pwrgb0++ = rgblut[b];

            y = ylut[clamp (-256, *Ytmp1++, 511)];
            r = (y + ro) >> COLOR_FRACTION_BITS;
            g = (y - go) >> COLOR_FRACTION_BITS;
            b = (y + bo) >> COLOR_FRACTION_BITS;
	    *pwrgb1++ = rgblut[r];
	    *pwrgb1++ = rgblut[g];
	    *pwrgb1++ = rgblut[b];
          } // for k

        } // for col
        if (row & 1)
          Ytmp0 += 16;
        Y[yindex] = Ytmp0;
      } // for i

      prgb += (row & 1) ? 3 * pitches [0] : pitches [0];
    } // for row

  } // for j
} /* dv_mb420_rgb */

/* ---------------------------------------------------------------------------
 */
void
dv_mb411_bgr0(dv_macroblock_t *mb, uint8_t **pixels, int *pitches, int add_ntsc_setup) {
  dv_coeff_t *Y[4], *cr_frame, *cb_frame;
  uint8_t *prgb, *pwrgb;
  int i,j,k, row;

  Y[0] = mb->b[0].coeffs;
  Y[1] = mb->b[1].coeffs;
  Y[2] = mb->b[2].coeffs;
  Y[3] = mb->b[3].coeffs;
  cr_frame = mb->b[4].coeffs;
  cb_frame = mb->b[5].coeffs;

  prgb = pixels[0] + (mb->x * 4)  + (mb->y * pitches[0]);

  for (row = 0; row < 8; ++row) { // Eight rows
    pwrgb = prgb;
    for (i = 0; i < 4; ++i) {     // Four Y blocks
      dv_coeff_t *Ytmp = Y[i]; // less indexing in inner loop speedup?
      for (j = 0; j < 2; ++j) {   // two 4-pixel spans per Y block
        int8_t cb = clamp (-128, *cb_frame++, 127);  /* -128,-1  => 0x80,0xff */
        int8_t cr = clamp (-128, *cr_frame++, 127);
        int ro = table_1_596[cr];
        int go = table_0_813[cr] + table_0_391[cb];
        int bo =                   table_2_018[cb];

        for (k = 0; k < 4; ++k) { // 4-pixel span
          int32_t y = (add_ntsc_setup == TRUE) ?
	  		ylut_setup[clamp (-256, *Ytmp++, 511)] :
				ylut[clamp (-256, *Ytmp++, 511)];
          int32_t r = (y + ro) >> COLOR_FRACTION_BITS;
          int32_t g = (y - go) >> COLOR_FRACTION_BITS;
          int32_t b = (y + bo) >> COLOR_FRACTION_BITS;
          *pwrgb++ = rgblut[b];
          *pwrgb++ = rgblut[g];
          *pwrgb++ = rgblut[r];
          *pwrgb++ = 0;
        } // for k
      } // for j
      Y[i] = Ytmp;
    } // for i
    prgb += pitches[0];
  } // for row
} // dv_mb411_bgr0

/* ---------------------------------------------------------------------------
 */
void
dv_mb411_right_bgr0(dv_macroblock_t *mb, uint8_t **pixels, int *pitches, int add_ntsc_setup) {
  dv_coeff_t *Ytmp;
  dv_coeff_t *Y[4], *cr_frame, *cb_frame;
  uint8_t *prgb, *pwrgb;
  int i, j, k, row, col;

  Y[0] = mb->b[0].coeffs;
  Y[1] = mb->b[1].coeffs;
  Y[2] = mb->b[2].coeffs;
  Y[3] = mb->b[3].coeffs;

  prgb = pixels[0] + (mb->x * 4) + (mb->y * pitches[0]);

  for (j = 0; j < 4; j += 2) { // Two rows of blocks j, j+1

    cr_frame = mb->b[4].coeffs + (j*2);
    cb_frame = mb->b[5].coeffs + (j*2);

    for (row = 0; row < 8; row++) {
      pwrgb = prgb;

      for (i = 0; i < 2; ++i) { // Two columns of blocks
	Ytmp = Y[j+i];

        for (col = 0; col < 8; col+=4) {  // 4 spans of 2x2 pixels

          int8_t cb = clamp (-128, *cb_frame++, 127); // +128;
          int8_t cr = clamp (-128, *cr_frame++, 127); // +128

	  int ro = table_1_596[cr];
	  int go = table_0_813[cr] + table_0_391[cb];
	  int bo =                   table_2_018[cb];

          for (k = 0; k < 4; ++k) { // 4x1 pixels

            int32_t y = (add_ntsc_setup == TRUE) ?
	    			ylut_setup[clamp (-256, *Ytmp++, 511)] :
					ylut[clamp (-256, *Ytmp++, 511)];
            int32_t r = (y + ro) >> COLOR_FRACTION_BITS;
            int32_t g = (y - go) >> COLOR_FRACTION_BITS;
            int32_t b = (y + bo) >> COLOR_FRACTION_BITS;

	    *pwrgb++ = rgblut[b];
	    *pwrgb++ = rgblut[g];
	    *pwrgb++ = rgblut[r];
	    *pwrgb++ = 0;
          } // for k

        } // for col

        Y[j+i] = Ytmp;
      } // for i

      cb_frame += 4;
      cr_frame += 4;
      prgb += pitches[0];
    } // for row

  } // for j
} /* dv_mb411_right_bgr0 */

/* ---------------------------------------------------------------------------
 */
void
dv_mb420_bgr0(dv_macroblock_t *mb, uint8_t **pixels, int *pitches) {
  dv_coeff_t *Y[4], *cr_frame, *cb_frame;
  uint8_t *prgb, *pwrgb0, *pwrgb1;
  int i, j, k, row, col;

  Y[0] = mb->b[0].coeffs;
  Y[1] = mb->b[1].coeffs;
  Y[2] = mb->b[2].coeffs;
  Y[3] = mb->b[3].coeffs;
  cr_frame = mb->b[4].coeffs;
  cb_frame = mb->b[5].coeffs;

  prgb = pixels[0] + (mb->x * 4) + (mb->y * pitches[0]);

  for (j = 0; j < 4; j += 2) { // Two rows of blocks j, j+1

    for (row = 0; row < 4; row++) { // 4 pairs of two rows
      pwrgb0 = prgb;
      pwrgb1 = prgb + pitches[0] * 2;

      for (i = 0; i < 2; ++i) { // Two columns of blocks
        int yindex = j + i;
        dv_coeff_t *Ytmp0 = Y[yindex];
        dv_coeff_t *Ytmp1 = Y[yindex] + 16;
        for (col = 0; col < 4; ++col) {  // 4 spans of 2x2 pixels
          int8_t cb = clamp (-128, *cb_frame++, 127); // +128;
          int8_t cr = clamp (-128, *cr_frame++, 127); // +128
	  int ro = table_1_596[cr];
	  int go = table_0_813[cr] + table_0_391[cb];
	  int bo =                   table_2_018[cb];

          for (k = 0; k < 2; ++k) { // 2x2 pixel
            int32_t y = ylut[clamp (-256, *Ytmp0++, 511)];
            int32_t r = (y + ro) >> COLOR_FRACTION_BITS;
            int32_t g = (y - go) >> COLOR_FRACTION_BITS;
            int32_t b = (y + bo) >> COLOR_FRACTION_BITS;
	    *pwrgb0++ = rgblut[b];
	    *pwrgb0++ = rgblut[g];
	    *pwrgb0++ = rgblut[r];
	    *pwrgb0++ = 0;

            y = ylut[clamp (-256, *Ytmp1++, 511)];
            r = (y + ro) >> COLOR_FRACTION_BITS;
            g = (y - go) >> COLOR_FRACTION_BITS;
            b = (y + bo) >> COLOR_FRACTION_BITS;
	    *pwrgb1++ = rgblut[b];
	    *pwrgb1++ = rgblut[g];
	    *pwrgb1++ = rgblut[r];
	    *pwrgb1++ = 0;
          } // for k

        } // for col
        if (row & 1)
          Ytmp0 += 16;
        Y[yindex] = Ytmp0;
      } // for i

      prgb += (row & 1) ? 3 * pitches [0] : pitches [0];
    } // for row

  } // for j
} /* dv_mb420_bgr0 */


/* TODO: MMX versions */
