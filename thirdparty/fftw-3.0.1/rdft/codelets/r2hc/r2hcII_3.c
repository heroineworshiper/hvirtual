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
/* Generated on Sat Jul  5 21:58:02 EDT 2003 */

#include "codelet-rdft.h"

/* Generated by: /homee/stevenj/cvs/fftw3.0.1/genfft/gen_r2hc -compact -variables 4 -n 3 -name r2hcII_3 -dft-II -include r2hcII.h */

/*
 * This function contains 4 FP additions, 2 FP multiplications,
 * (or, 3 additions, 1 multiplications, 1 fused multiply/add),
 * 7 stack variables, and 6 memory accesses
 */
/*
 * Generator Id's : 
 * $Id: algsimp.ml,v 1.7 2003/03/15 20:29:42 stevenj Exp $
 * $Id: fft.ml,v 1.2 2003/03/15 20:29:42 stevenj Exp $
 * $Id: gen_r2hc.ml,v 1.13 2003/04/17 19:25:50 athena Exp $
 */

#include "r2hcII.h"

static void r2hcII_3(const R *I, R *ro, R *io, stride is, stride ros, stride ios, int v, int ivs, int ovs)
{
     DK(KP500000000, +0.500000000000000000000000000000000000000000000);
     DK(KP866025403, +0.866025403784438646763723170752936183471402627);
     int i;
     for (i = v; i > 0; i = i - 1, I = I + ivs, ro = ro + ovs, io = io + ovs) {
	  E T1, T2, T3, T4;
	  T1 = I[0];
	  T2 = I[WS(is, 1)];
	  T3 = I[WS(is, 2)];
	  T4 = T2 - T3;
	  ro[WS(ros, 1)] = T1 - T4;
	  io[0] = -(KP866025403 * (T2 + T3));
	  ro[0] = FMA(KP500000000, T4, T1);
     }
}

static const kr2hc_desc desc = { 3, "r2hcII_3", {3, 1, 1, 0}, &GENUS, 0, 0, 0, 0, 0 };

void X(codelet_r2hcII_3) (planner *p) {
     X(kr2hcII_register) (p, r2hcII_3, &desc);
}
