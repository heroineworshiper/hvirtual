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
/* Generated on Sat Jul  5 21:40:10 EDT 2003 */

#include "codelet-dft.h"

/* Generated by: /homee/stevenj/cvs/fftw3.0.1/genfft/gen_notw_c -simd -compact -variables 4 -n 16 -name n1fv_16 -include n1f.h */

/*
 * This function contains 72 FP additions, 12 FP multiplications,
 * (or, 68 additions, 8 multiplications, 4 fused multiply/add),
 * 30 stack variables, and 32 memory accesses
 */
/*
 * Generator Id's : 
 * $Id: algsimp.ml,v 1.7 2003/03/15 20:29:42 stevenj Exp $
 * $Id: fft.ml,v 1.2 2003/03/15 20:29:42 stevenj Exp $
 * $Id: gen_notw_c.ml,v 1.9 2003/04/16 21:21:53 athena Exp $
 */

#include "n1f.h"

static void n1fv_16(const R *ri, const R *ii, R *ro, R *io, stride is, stride os, int v, int ivs, int ovs)
{
     DVK(KP923879532, +0.923879532511286756128183189396788286822416626);
     DVK(KP382683432, +0.382683432365089771728459984030398866761344562);
     DVK(KP707106781, +0.707106781186547524400844362104849039284835938);
     int i;
     const R *xi;
     R *xo;
     xi = ri;
     xo = ro;
     BEGIN_SIMD();
     for (i = v; i > 0; i = i - VL, xi = xi + (VL * ivs), xo = xo + (VL * ovs)) {
	  V Tp, T13, Tu, TN, Tm, T14, Tv, TY, T7, T17, Ty, TT, Te, T16, Tx;
	  V TQ;
	  {
	       V Tn, To, TM, Ts, Tt, TL;
	       Tn = LD(&(xi[WS(is, 4)]), ivs, &(xi[0]));
	       To = LD(&(xi[WS(is, 12)]), ivs, &(xi[0]));
	       TM = VADD(Tn, To);
	       Ts = LD(&(xi[0]), ivs, &(xi[0]));
	       Tt = LD(&(xi[WS(is, 8)]), ivs, &(xi[0]));
	       TL = VADD(Ts, Tt);
	       Tp = VSUB(Tn, To);
	       T13 = VADD(TL, TM);
	       Tu = VSUB(Ts, Tt);
	       TN = VSUB(TL, TM);
	  }
	  {
	       V Ti, TW, Tl, TX;
	       {
		    V Tg, Th, Tj, Tk;
		    Tg = LD(&(xi[WS(is, 14)]), ivs, &(xi[0]));
		    Th = LD(&(xi[WS(is, 6)]), ivs, &(xi[0]));
		    Ti = VSUB(Tg, Th);
		    TW = VADD(Tg, Th);
		    Tj = LD(&(xi[WS(is, 2)]), ivs, &(xi[0]));
		    Tk = LD(&(xi[WS(is, 10)]), ivs, &(xi[0]));
		    Tl = VSUB(Tj, Tk);
		    TX = VADD(Tj, Tk);
	       }
	       Tm = VMUL(LDK(KP707106781), VSUB(Ti, Tl));
	       T14 = VADD(TX, TW);
	       Tv = VMUL(LDK(KP707106781), VADD(Tl, Ti));
	       TY = VSUB(TW, TX);
	  }
	  {
	       V T3, TR, T6, TS;
	       {
		    V T1, T2, T4, T5;
		    T1 = LD(&(xi[WS(is, 15)]), ivs, &(xi[WS(is, 1)]));
		    T2 = LD(&(xi[WS(is, 7)]), ivs, &(xi[WS(is, 1)]));
		    T3 = VSUB(T1, T2);
		    TR = VADD(T1, T2);
		    T4 = LD(&(xi[WS(is, 3)]), ivs, &(xi[WS(is, 1)]));
		    T5 = LD(&(xi[WS(is, 11)]), ivs, &(xi[WS(is, 1)]));
		    T6 = VSUB(T4, T5);
		    TS = VADD(T4, T5);
	       }
	       T7 = VFNMS(LDK(KP923879532), T6, VMUL(LDK(KP382683432), T3));
	       T17 = VADD(TR, TS);
	       Ty = VFMA(LDK(KP923879532), T3, VMUL(LDK(KP382683432), T6));
	       TT = VSUB(TR, TS);
	  }
	  {
	       V Ta, TO, Td, TP;
	       {
		    V T8, T9, Tb, Tc;
		    T8 = LD(&(xi[WS(is, 1)]), ivs, &(xi[WS(is, 1)]));
		    T9 = LD(&(xi[WS(is, 9)]), ivs, &(xi[WS(is, 1)]));
		    Ta = VSUB(T8, T9);
		    TO = VADD(T8, T9);
		    Tb = LD(&(xi[WS(is, 5)]), ivs, &(xi[WS(is, 1)]));
		    Tc = LD(&(xi[WS(is, 13)]), ivs, &(xi[WS(is, 1)]));
		    Td = VSUB(Tb, Tc);
		    TP = VADD(Tb, Tc);
	       }
	       Te = VFMA(LDK(KP382683432), Ta, VMUL(LDK(KP923879532), Td));
	       T16 = VADD(TO, TP);
	       Tx = VFNMS(LDK(KP382683432), Td, VMUL(LDK(KP923879532), Ta));
	       TQ = VSUB(TO, TP);
	  }
	  {
	       V T15, T18, T19, T1a;
	       T15 = VADD(T13, T14);
	       T18 = VADD(T16, T17);
	       ST(&(xo[WS(os, 8)]), VSUB(T15, T18), ovs, &(xo[0]));
	       ST(&(xo[0]), VADD(T15, T18), ovs, &(xo[0]));
	       T19 = VSUB(T13, T14);
	       T1a = VBYI(VSUB(T17, T16));
	       ST(&(xo[WS(os, 12)]), VSUB(T19, T1a), ovs, &(xo[0]));
	       ST(&(xo[WS(os, 4)]), VADD(T19, T1a), ovs, &(xo[0]));
	  }
	  {
	       V TV, T11, T10, T12, TU, TZ;
	       TU = VMUL(LDK(KP707106781), VADD(TQ, TT));
	       TV = VADD(TN, TU);
	       T11 = VSUB(TN, TU);
	       TZ = VMUL(LDK(KP707106781), VSUB(TT, TQ));
	       T10 = VBYI(VADD(TY, TZ));
	       T12 = VBYI(VSUB(TZ, TY));
	       ST(&(xo[WS(os, 14)]), VSUB(TV, T10), ovs, &(xo[0]));
	       ST(&(xo[WS(os, 6)]), VADD(T11, T12), ovs, &(xo[0]));
	       ST(&(xo[WS(os, 2)]), VADD(TV, T10), ovs, &(xo[0]));
	       ST(&(xo[WS(os, 10)]), VSUB(T11, T12), ovs, &(xo[0]));
	  }
	  {
	       V Tr, TB, TA, TC;
	       {
		    V Tf, Tq, Tw, Tz;
		    Tf = VSUB(T7, Te);
		    Tq = VSUB(Tm, Tp);
		    Tr = VBYI(VSUB(Tf, Tq));
		    TB = VBYI(VADD(Tq, Tf));
		    Tw = VADD(Tu, Tv);
		    Tz = VADD(Tx, Ty);
		    TA = VSUB(Tw, Tz);
		    TC = VADD(Tw, Tz);
	       }
	       ST(&(xo[WS(os, 7)]), VADD(Tr, TA), ovs, &(xo[WS(os, 1)]));
	       ST(&(xo[WS(os, 15)]), VSUB(TC, TB), ovs, &(xo[WS(os, 1)]));
	       ST(&(xo[WS(os, 9)]), VSUB(TA, Tr), ovs, &(xo[WS(os, 1)]));
	       ST(&(xo[WS(os, 1)]), VADD(TB, TC), ovs, &(xo[WS(os, 1)]));
	  }
	  {
	       V TF, TJ, TI, TK;
	       {
		    V TD, TE, TG, TH;
		    TD = VSUB(Tu, Tv);
		    TE = VADD(Te, T7);
		    TF = VADD(TD, TE);
		    TJ = VSUB(TD, TE);
		    TG = VADD(Tp, Tm);
		    TH = VSUB(Ty, Tx);
		    TI = VBYI(VADD(TG, TH));
		    TK = VBYI(VSUB(TH, TG));
	       }
	       ST(&(xo[WS(os, 13)]), VSUB(TF, TI), ovs, &(xo[WS(os, 1)]));
	       ST(&(xo[WS(os, 5)]), VADD(TJ, TK), ovs, &(xo[WS(os, 1)]));
	       ST(&(xo[WS(os, 3)]), VADD(TF, TI), ovs, &(xo[WS(os, 1)]));
	       ST(&(xo[WS(os, 11)]), VSUB(TJ, TK), ovs, &(xo[WS(os, 1)]));
	  }
     }
     END_SIMD();
}

static const kdft_desc desc = { 16, "n1fv_16", {68, 8, 4, 0}, &GENUS, 0, 0, 0, 0 };
void X(codelet_n1fv_16) (planner *p) {
     X(kdft_register) (p, n1fv_16, &desc);
}
