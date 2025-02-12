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
/* Generated on Sat Jul  5 21:29:32 EDT 2003 */

#include "codelet-dft.h"

/* Generated by: /homee/stevenj/cvs/fftw3.0.1/genfft/gen_notw -compact -variables 4 -n 12 -name n1_12 -include n.h */

/*
 * This function contains 96 FP additions, 16 FP multiplications,
 * (or, 88 additions, 8 multiplications, 8 fused multiply/add),
 * 43 stack variables, and 48 memory accesses
 */
/*
 * Generator Id's : 
 * $Id: algsimp.ml,v 1.7 2003/03/15 20:29:42 stevenj Exp $
 * $Id: fft.ml,v 1.2 2003/03/15 20:29:42 stevenj Exp $
 * $Id: gen_notw.ml,v 1.22 2003/04/17 11:07:19 athena Exp $
 */

#include "n.h"

static void n1_12(const R *ri, const R *ii, R *ro, R *io, stride is, stride os, int v, int ivs, int ovs)
{
     DK(KP866025403, +0.866025403784438646763723170752936183471402627);
     DK(KP500000000, +0.500000000000000000000000000000000000000000000);
     int i;
     for (i = v; i > 0; i = i - 1, ri = ri + ivs, ii = ii + ivs, ro = ro + ovs, io = io + ovs) {
	  E T5, TR, TA, Ts, TS, Tz, Ta, TU, TD, Tx, TV, TC, Tg, T1a, TG;
	  E TJ, T1u, T1d, Tl, T1f, TL, TO, T1v, T1i;
	  {
	       E T1, T2, T3, T4;
	       T1 = ri[0];
	       T2 = ri[WS(is, 4)];
	       T3 = ri[WS(is, 8)];
	       T4 = T2 + T3;
	       T5 = T1 + T4;
	       TR = FNMS(KP500000000, T4, T1);
	       TA = KP866025403 * (T3 - T2);
	  }
	  {
	       E To, Tp, Tq, Tr;
	       To = ii[0];
	       Tp = ii[WS(is, 4)];
	       Tq = ii[WS(is, 8)];
	       Tr = Tp + Tq;
	       Ts = To + Tr;
	       TS = KP866025403 * (Tp - Tq);
	       Tz = FNMS(KP500000000, Tr, To);
	  }
	  {
	       E T6, T7, T8, T9;
	       T6 = ri[WS(is, 6)];
	       T7 = ri[WS(is, 10)];
	       T8 = ri[WS(is, 2)];
	       T9 = T7 + T8;
	       Ta = T6 + T9;
	       TU = FNMS(KP500000000, T9, T6);
	       TD = KP866025403 * (T8 - T7);
	  }
	  {
	       E Tt, Tu, Tv, Tw;
	       Tt = ii[WS(is, 6)];
	       Tu = ii[WS(is, 10)];
	       Tv = ii[WS(is, 2)];
	       Tw = Tu + Tv;
	       Tx = Tt + Tw;
	       TV = KP866025403 * (Tu - Tv);
	       TC = FNMS(KP500000000, Tw, Tt);
	  }
	  {
	       E Tc, Td, Te, Tf;
	       Tc = ri[WS(is, 3)];
	       Td = ri[WS(is, 7)];
	       Te = ri[WS(is, 11)];
	       Tf = Td + Te;
	       Tg = Tc + Tf;
	       T1a = KP866025403 * (Te - Td);
	       TG = FNMS(KP500000000, Tf, Tc);
	  }
	  {
	       E T1b, TH, TI, T1c;
	       T1b = ii[WS(is, 3)];
	       TH = ii[WS(is, 7)];
	       TI = ii[WS(is, 11)];
	       T1c = TH + TI;
	       TJ = KP866025403 * (TH - TI);
	       T1u = T1b + T1c;
	       T1d = FNMS(KP500000000, T1c, T1b);
	  }
	  {
	       E Th, Ti, Tj, Tk;
	       Th = ri[WS(is, 9)];
	       Ti = ri[WS(is, 1)];
	       Tj = ri[WS(is, 5)];
	       Tk = Ti + Tj;
	       Tl = Th + Tk;
	       T1f = KP866025403 * (Tj - Ti);
	       TL = FNMS(KP500000000, Tk, Th);
	  }
	  {
	       E T1g, TM, TN, T1h;
	       T1g = ii[WS(is, 9)];
	       TM = ii[WS(is, 1)];
	       TN = ii[WS(is, 5)];
	       T1h = TM + TN;
	       TO = KP866025403 * (TM - TN);
	       T1v = T1g + T1h;
	       T1i = FNMS(KP500000000, T1h, T1g);
	  }
	  {
	       E Tb, Tm, T1t, T1w;
	       Tb = T5 + Ta;
	       Tm = Tg + Tl;
	       ro[WS(os, 6)] = Tb - Tm;
	       ro[0] = Tb + Tm;
	       {
		    E T1x, T1y, Tn, Ty;
		    T1x = Ts + Tx;
		    T1y = T1u + T1v;
		    io[WS(os, 6)] = T1x - T1y;
		    io[0] = T1x + T1y;
		    Tn = Tg - Tl;
		    Ty = Ts - Tx;
		    io[WS(os, 3)] = Tn + Ty;
		    io[WS(os, 9)] = Ty - Tn;
	       }
	       T1t = T5 - Ta;
	       T1w = T1u - T1v;
	       ro[WS(os, 3)] = T1t - T1w;
	       ro[WS(os, 9)] = T1t + T1w;
	       {
		    E T11, T1l, T1k, T1m, T14, T18, T17, T19;
		    {
			 E TZ, T10, T1e, T1j;
			 TZ = TA + Tz;
			 T10 = TD + TC;
			 T11 = TZ - T10;
			 T1l = TZ + T10;
			 T1e = T1a + T1d;
			 T1j = T1f + T1i;
			 T1k = T1e - T1j;
			 T1m = T1e + T1j;
		    }
		    {
			 E T12, T13, T15, T16;
			 T12 = TG + TJ;
			 T13 = TL + TO;
			 T14 = T12 - T13;
			 T18 = T12 + T13;
			 T15 = TR + TS;
			 T16 = TU + TV;
			 T17 = T15 + T16;
			 T19 = T15 - T16;
		    }
		    io[WS(os, 1)] = T11 - T14;
		    ro[WS(os, 1)] = T19 + T1k;
		    io[WS(os, 7)] = T11 + T14;
		    ro[WS(os, 7)] = T19 - T1k;
		    ro[WS(os, 10)] = T17 - T18;
		    io[WS(os, 10)] = T1l - T1m;
		    ro[WS(os, 4)] = T17 + T18;
		    io[WS(os, 4)] = T1l + T1m;
	       }
	       {
		    E TF, T1r, T1q, T1s, TQ, TY, TX, T1n;
		    {
			 E TB, TE, T1o, T1p;
			 TB = Tz - TA;
			 TE = TC - TD;
			 TF = TB - TE;
			 T1r = TB + TE;
			 T1o = T1d - T1a;
			 T1p = T1i - T1f;
			 T1q = T1o - T1p;
			 T1s = T1o + T1p;
		    }
		    {
			 E TK, TP, TT, TW;
			 TK = TG - TJ;
			 TP = TL - TO;
			 TQ = TK - TP;
			 TY = TK + TP;
			 TT = TR - TS;
			 TW = TU - TV;
			 TX = TT + TW;
			 T1n = TT - TW;
		    }
		    io[WS(os, 5)] = TF - TQ;
		    ro[WS(os, 5)] = T1n + T1q;
		    io[WS(os, 11)] = TF + TQ;
		    ro[WS(os, 11)] = T1n - T1q;
		    ro[WS(os, 2)] = TX - TY;
		    io[WS(os, 2)] = T1r - T1s;
		    ro[WS(os, 8)] = TX + TY;
		    io[WS(os, 8)] = T1r + T1s;
	       }
	  }
     }
}

static const kdft_desc desc = { 12, "n1_12", {88, 8, 8, 0}, &GENUS, 0, 0, 0, 0 };
void X(codelet_n1_12) (planner *p) {
     X(kdft_register) (p, n1_12, &desc);
}
