/*
 * Copyright (c) 2003 Matteo Frigo
 * Copyright (c) 2003 Massachusetts Institute of Technology
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/* This file was automatically generated --- DO NOT EDIT */
/* Generated on Sat Jul  5 22:11:22 EDT 2003 */

#include "codelet-rdft.h"

/* Generated by: /homee/stevenj/cvs/fftw3.0.1/genfft/gen_hc2hc -compact -variables 4 -sign 1 -n 3 -dif -name hb_3 -include hb.h */

/*
 * This function contains 16 FP additions, 12 FP multiplications,
 * (or, 10 additions, 6 multiplications, 6 fused multiply/add),
 * 15 stack variables, and 12 memory accesses
 */
/*
 * Generator Id's : 
 * $Id: algsimp.ml,v 1.7 2003/03/15 20:29:42 stevenj Exp $
 * $Id: fft.ml,v 1.2 2003/03/15 20:29:42 stevenj Exp $
 * $Id: gen_hc2hc.ml,v 1.9 2003/04/17 19:25:50 athena Exp $
 */

#include "hb.h"

static const R *hb_3(R *rio, R *iio, const R *W, stride ios, int m, int dist)
{
     DK(KP866025403, +0.866025403784438646763723170752936183471402627);
     DK(KP500000000, +0.500000000000000000000000000000000000000000000);
     int i;
     for (i = m - 2; i > 0; i = i - 2, rio = rio + dist, iio = iio - dist, W = W + 4) {
	  E T1, T4, Ta, Te, T5, T8, Tb, Tf;
	  {
	       E T2, T3, T6, T7;
	       T1 = rio[0];
	       T2 = rio[WS(ios, 1)];
	       T3 = iio[-WS(ios, 2)];
	       T4 = T2 + T3;
	       Ta = FNMS(KP500000000, T4, T1);
	       Te = KP866025403 * (T2 - T3);
	       T5 = iio[0];
	       T6 = rio[WS(ios, 2)];
	       T7 = iio[-WS(ios, 1)];
	       T8 = T6 - T7;
	       Tb = KP866025403 * (T6 + T7);
	       Tf = FMA(KP500000000, T8, T5);
	  }
	  rio[0] = T1 + T4;
	  iio[-WS(ios, 2)] = T5 - T8;
	  {
	       E Ti, Tk, Th, Tj;
	       Ti = Tf - Te;
	       Tk = Ta + Tb;
	       Th = W[2];
	       Tj = W[3];
	       iio[0] = FMA(Th, Ti, Tj * Tk);
	       rio[WS(ios, 2)] = FNMS(Tj, Ti, Th * Tk);
	  }
	  {
	       E Tc, Tg, T9, Td;
	       Tc = Ta - Tb;
	       Tg = Te + Tf;
	       T9 = W[0];
	       Td = W[1];
	       rio[WS(ios, 1)] = FNMS(Td, Tg, T9 * Tc);
	       iio[-WS(ios, 1)] = FMA(T9, Tg, Td * Tc);
	  }
     }
     return W;
}

static const tw_instr twinstr[] = {
     {TW_FULL, 0, 3},
     {TW_NEXT, 1, 0}
};

static const hc2hc_desc desc = { 3, "hb_3", twinstr, {10, 6, 6, 0}, &GENUS, 0, 0, 0 };

void X(codelet_hb_3) (planner *p) {
     X(khc2hc_dif_register) (p, hb_3, &desc);
}
