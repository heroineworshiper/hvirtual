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
/* Generated on Sat Jul  5 21:40:07 EDT 2003 */

#include "codelet-dft.h"

/* Generated by: /homee/stevenj/cvs/fftw3.0.1/genfft/gen_notw_c -simd -compact -variables 4 -n 5 -name n1fv_5 -include n1f.h */

/*
 * This function contains 16 FP additions, 6 FP multiplications,
 * (or, 13 additions, 3 multiplications, 3 fused multiply/add),
 * 18 stack variables, and 10 memory accesses
 */
/*
 * Generator Id's : 
 * $Id: algsimp.ml,v 1.7 2003/03/15 20:29:42 stevenj Exp $
 * $Id: fft.ml,v 1.2 2003/03/15 20:29:42 stevenj Exp $
 * $Id: gen_notw_c.ml,v 1.9 2003/04/16 21:21:53 athena Exp $
 */

#include "n1f.h"

static void n1fv_5(const R *ri, const R *ii, R *ro, R *io, stride is, stride os, int v, int ivs, int ovs)
{
     DVK(KP250000000, +0.250000000000000000000000000000000000000000000);
     DVK(KP587785252, +0.587785252292473129168705954639072768597652438);
     DVK(KP951056516, +0.951056516295153572116439333379382143405698634);
     DVK(KP559016994, +0.559016994374947424102293417182819058860154590);
     int i;
     const R *xi;
     R *xo;
     xi = ri;
     xo = ro;
     BEGIN_SIMD();
     for (i = v; i > 0; i = i - VL, xi = xi + (VL * ivs), xo = xo + (VL * ovs)) {
	  V T8, T7, Td, T9, Tc;
	  T8 = LD(&(xi[0]), ivs, &(xi[0]));
	  {
	       V T1, T2, T3, T4, T5, T6;
	       T1 = LD(&(xi[WS(is, 1)]), ivs, &(xi[WS(is, 1)]));
	       T2 = LD(&(xi[WS(is, 4)]), ivs, &(xi[0]));
	       T3 = VADD(T1, T2);
	       T4 = LD(&(xi[WS(is, 2)]), ivs, &(xi[0]));
	       T5 = LD(&(xi[WS(is, 3)]), ivs, &(xi[WS(is, 1)]));
	       T6 = VADD(T4, T5);
	       T7 = VMUL(LDK(KP559016994), VSUB(T3, T6));
	       Td = VSUB(T4, T5);
	       T9 = VADD(T3, T6);
	       Tc = VSUB(T1, T2);
	  }
	  ST(&(xo[0]), VADD(T8, T9), ovs, &(xo[0]));
	  {
	       V Te, Tf, Tb, Tg, Ta;
	       Te = VBYI(VFMA(LDK(KP951056516), Tc, VMUL(LDK(KP587785252), Td)));
	       Tf = VBYI(VFNMS(LDK(KP587785252), Tc, VMUL(LDK(KP951056516), Td)));
	       Ta = VFNMS(LDK(KP250000000), T9, T8);
	       Tb = VADD(T7, Ta);
	       Tg = VSUB(Ta, T7);
	       ST(&(xo[WS(os, 1)]), VSUB(Tb, Te), ovs, &(xo[WS(os, 1)]));
	       ST(&(xo[WS(os, 3)]), VSUB(Tg, Tf), ovs, &(xo[WS(os, 1)]));
	       ST(&(xo[WS(os, 4)]), VADD(Te, Tb), ovs, &(xo[0]));
	       ST(&(xo[WS(os, 2)]), VADD(Tf, Tg), ovs, &(xo[0]));
	  }
     }
     END_SIMD();
}

static const kdft_desc desc = { 5, "n1fv_5", {13, 3, 3, 0}, &GENUS, 0, 0, 0, 0 };
void X(codelet_n1fv_5) (planner *p) {
     X(kdft_register) (p, n1fv_5, &desc);
}
