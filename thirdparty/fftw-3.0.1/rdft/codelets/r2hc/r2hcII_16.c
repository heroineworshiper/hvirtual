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
/* Generated on Sat Jul  5 21:58:21 EDT 2003 */

#include "codelet-rdft.h"

/* Generated by: /homee/stevenj/cvs/fftw3.0.1/genfft/gen_r2hc -compact -variables 4 -n 16 -name r2hcII_16 -dft-II -include r2hcII.h */

/*
 * This function contains 66 FP additions, 30 FP multiplications,
 * (or, 54 additions, 18 multiplications, 12 fused multiply/add),
 * 32 stack variables, and 32 memory accesses
 */
/*
 * Generator Id's : 
 * $Id: algsimp.ml,v 1.7 2003/03/15 20:29:42 stevenj Exp $
 * $Id: fft.ml,v 1.2 2003/03/15 20:29:42 stevenj Exp $
 * $Id: gen_r2hc.ml,v 1.13 2003/04/17 19:25:50 athena Exp $
 */

#include "r2hcII.h"

static void r2hcII_16(const R *I, R *ro, R *io, stride is, stride ros, stride ios, int v, int ivs, int ovs)
{
     DK(KP555570233, +0.555570233019602224742830813948532874374937191);
     DK(KP831469612, +0.831469612302545237078788377617905756738560812);
     DK(KP980785280, +0.980785280403230449126182236134239036973933731);
     DK(KP195090322, +0.195090322016128267848284868477022240927691618);
     DK(KP382683432, +0.382683432365089771728459984030398866761344562);
     DK(KP923879532, +0.923879532511286756128183189396788286822416626);
     DK(KP707106781, +0.707106781186547524400844362104849039284835938);
     int i;
     for (i = v; i > 0; i = i - 1, I = I + ivs, ro = ro + ovs, io = io + ovs) {
	  E T5, T11, TB, TV, Tr, TK, Tu, TJ, Ti, TH, Tl, TG, Tc, T10, TE;
	  E TS;
	  {
	       E T1, TU, T4, TT, T2, T3;
	       T1 = I[0];
	       TU = I[WS(is, 8)];
	       T2 = I[WS(is, 4)];
	       T3 = I[WS(is, 12)];
	       T4 = KP707106781 * (T2 - T3);
	       TT = KP707106781 * (T2 + T3);
	       T5 = T1 + T4;
	       T11 = TU - TT;
	       TB = T1 - T4;
	       TV = TT + TU;
	  }
	  {
	       E Tq, Tt, Tp, Ts, Tn, To;
	       Tq = I[WS(is, 15)];
	       Tt = I[WS(is, 7)];
	       Tn = I[WS(is, 3)];
	       To = I[WS(is, 11)];
	       Tp = KP707106781 * (Tn - To);
	       Ts = KP707106781 * (Tn + To);
	       Tr = Tp - Tq;
	       TK = Tt - Ts;
	       Tu = Ts + Tt;
	       TJ = Tp + Tq;
	  }
	  {
	       E Te, Tk, Th, Tj, Tf, Tg;
	       Te = I[WS(is, 1)];
	       Tk = I[WS(is, 9)];
	       Tf = I[WS(is, 5)];
	       Tg = I[WS(is, 13)];
	       Th = KP707106781 * (Tf - Tg);
	       Tj = KP707106781 * (Tf + Tg);
	       Ti = Te + Th;
	       TH = Tk - Tj;
	       Tl = Tj + Tk;
	       TG = Te - Th;
	  }
	  {
	       E T8, TC, Tb, TD;
	       {
		    E T6, T7, T9, Ta;
		    T6 = I[WS(is, 2)];
		    T7 = I[WS(is, 10)];
		    T8 = FNMS(KP382683432, T7, KP923879532 * T6);
		    TC = FMA(KP382683432, T6, KP923879532 * T7);
		    T9 = I[WS(is, 6)];
		    Ta = I[WS(is, 14)];
		    Tb = FNMS(KP923879532, Ta, KP382683432 * T9);
		    TD = FMA(KP923879532, T9, KP382683432 * Ta);
	       }
	       Tc = T8 + Tb;
	       T10 = Tb - T8;
	       TE = TC - TD;
	       TS = TC + TD;
	  }
	  {
	       E Td, TW, Tw, TR, Tm, Tv;
	       Td = T5 - Tc;
	       TW = TS + TV;
	       Tm = FMA(KP195090322, Ti, KP980785280 * Tl);
	       Tv = FNMS(KP980785280, Tu, KP195090322 * Tr);
	       Tw = Tm + Tv;
	       TR = Tv - Tm;
	       ro[WS(ros, 4)] = Td - Tw;
	       io[WS(ios, 7)] = TR + TW;
	       ro[WS(ros, 3)] = Td + Tw;
	       io[0] = TR - TW;
	  }
	  {
	       E Tx, TY, TA, TX, Ty, Tz;
	       Tx = T5 + Tc;
	       TY = TV - TS;
	       Ty = FNMS(KP195090322, Tl, KP980785280 * Ti);
	       Tz = FMA(KP980785280, Tr, KP195090322 * Tu);
	       TA = Ty + Tz;
	       TX = Tz - Ty;
	       ro[WS(ros, 7)] = Tx - TA;
	       io[WS(ios, 3)] = TX + TY;
	       ro[0] = Tx + TA;
	       io[WS(ios, 4)] = TX - TY;
	  }
	  {
	       E TF, T12, TM, TZ, TI, TL;
	       TF = TB + TE;
	       T12 = T10 - T11;
	       TI = FMA(KP831469612, TG, KP555570233 * TH);
	       TL = FMA(KP831469612, TJ, KP555570233 * TK);
	       TM = TI - TL;
	       TZ = TI + TL;
	       ro[WS(ros, 6)] = TF - TM;
	       io[WS(ios, 2)] = T12 - TZ;
	       ro[WS(ros, 1)] = TF + TM;
	       io[WS(ios, 5)] = -(TZ + T12);
	  }
	  {
	       E TN, T14, TQ, T13, TO, TP;
	       TN = TB - TE;
	       T14 = T10 + T11;
	       TO = FNMS(KP555570233, TJ, KP831469612 * TK);
	       TP = FNMS(KP555570233, TG, KP831469612 * TH);
	       TQ = TO - TP;
	       T13 = TP + TO;
	       ro[WS(ros, 5)] = TN - TQ;
	       io[WS(ios, 1)] = T13 + T14;
	       ro[WS(ros, 2)] = TN + TQ;
	       io[WS(ios, 6)] = T13 - T14;
	  }
     }
}

static const kr2hc_desc desc = { 16, "r2hcII_16", {54, 18, 12, 0}, &GENUS, 0, 0, 0, 0, 0 };

void X(codelet_r2hcII_16) (planner *p) {
     X(kr2hcII_register) (p, r2hcII_16, &desc);
}
