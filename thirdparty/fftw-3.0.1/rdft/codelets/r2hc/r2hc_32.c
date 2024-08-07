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
/* Generated on Sat Jul  5 21:56:44 EDT 2003 */

#include "codelet-rdft.h"

/* Generated by: /homee/stevenj/cvs/fftw3.0.1/genfft/gen_r2hc -compact -variables 4 -n 32 -name r2hc_32 -include r2hc.h */

/*
 * This function contains 156 FP additions, 42 FP multiplications,
 * (or, 140 additions, 26 multiplications, 16 fused multiply/add),
 * 54 stack variables, and 64 memory accesses
 */
/*
 * Generator Id's : 
 * $Id: algsimp.ml,v 1.7 2003/03/15 20:29:42 stevenj Exp $
 * $Id: fft.ml,v 1.2 2003/03/15 20:29:42 stevenj Exp $
 * $Id: gen_r2hc.ml,v 1.13 2003/04/17 19:25:50 athena Exp $
 */

#include "r2hc.h"

static void r2hc_32(const R *I, R *ro, R *io, stride is, stride ros, stride ios, int v, int ivs, int ovs)
{
     DK(KP555570233, +0.555570233019602224742830813948532874374937191);
     DK(KP831469612, +0.831469612302545237078788377617905756738560812);
     DK(KP195090322, +0.195090322016128267848284868477022240927691618);
     DK(KP980785280, +0.980785280403230449126182236134239036973933731);
     DK(KP382683432, +0.382683432365089771728459984030398866761344562);
     DK(KP923879532, +0.923879532511286756128183189396788286822416626);
     DK(KP707106781, +0.707106781186547524400844362104849039284835938);
     int i;
     for (i = v; i > 0; i = i - 1, I = I + ivs, ro = ro + ovs, io = io + ovs) {
	  E T7, T2b, Tv, T1l, Te, T2o, Ty, T1k, Tt, T2d, TF, T1h, Tm, T2c, TC;
	  E T1i, T1Z, T22, T2k, T2j, T1e, T1C, T19, T1B, T1S, T1V, T2h, T2g, TX, T1z;
	  E TS, T1y;
	  {
	       E T1, T2, T3, T4, T5, T6;
	       T1 = I[0];
	       T2 = I[WS(is, 16)];
	       T3 = T1 + T2;
	       T4 = I[WS(is, 8)];
	       T5 = I[WS(is, 24)];
	       T6 = T4 + T5;
	       T7 = T3 + T6;
	       T2b = T3 - T6;
	       Tv = T1 - T2;
	       T1l = T4 - T5;
	  }
	  {
	       E Ta, Tw, Td, Tx;
	       {
		    E T8, T9, Tb, Tc;
		    T8 = I[WS(is, 4)];
		    T9 = I[WS(is, 20)];
		    Ta = T8 + T9;
		    Tw = T8 - T9;
		    Tb = I[WS(is, 28)];
		    Tc = I[WS(is, 12)];
		    Td = Tb + Tc;
		    Tx = Tb - Tc;
	       }
	       Te = Ta + Td;
	       T2o = Td - Ta;
	       Ty = KP707106781 * (Tw + Tx);
	       T1k = KP707106781 * (Tx - Tw);
	  }
	  {
	       E Tp, TD, Ts, TE;
	       {
		    E Tn, To, Tq, Tr;
		    Tn = I[WS(is, 30)];
		    To = I[WS(is, 14)];
		    Tp = Tn + To;
		    TD = Tn - To;
		    Tq = I[WS(is, 6)];
		    Tr = I[WS(is, 22)];
		    Ts = Tq + Tr;
		    TE = Tq - Tr;
	       }
	       Tt = Tp + Ts;
	       T2d = Tp - Ts;
	       TF = FMA(KP923879532, TD, KP382683432 * TE);
	       T1h = FNMS(KP923879532, TE, KP382683432 * TD);
	  }
	  {
	       E Ti, TA, Tl, TB;
	       {
		    E Tg, Th, Tj, Tk;
		    Tg = I[WS(is, 2)];
		    Th = I[WS(is, 18)];
		    Ti = Tg + Th;
		    TA = Tg - Th;
		    Tj = I[WS(is, 10)];
		    Tk = I[WS(is, 26)];
		    Tl = Tj + Tk;
		    TB = Tj - Tk;
	       }
	       Tm = Ti + Tl;
	       T2c = Ti - Tl;
	       TC = FNMS(KP382683432, TB, KP923879532 * TA);
	       T1i = FMA(KP382683432, TA, KP923879532 * TB);
	  }
	  {
	       E T11, T1X, T1d, T1Y, T14, T20, T17, T21, T1a, T18;
	       {
		    E TZ, T10, T1b, T1c;
		    TZ = I[WS(is, 31)];
		    T10 = I[WS(is, 15)];
		    T11 = TZ - T10;
		    T1X = TZ + T10;
		    T1b = I[WS(is, 7)];
		    T1c = I[WS(is, 23)];
		    T1d = T1b - T1c;
		    T1Y = T1b + T1c;
	       }
	       {
		    E T12, T13, T15, T16;
		    T12 = I[WS(is, 3)];
		    T13 = I[WS(is, 19)];
		    T14 = T12 - T13;
		    T20 = T12 + T13;
		    T15 = I[WS(is, 27)];
		    T16 = I[WS(is, 11)];
		    T17 = T15 - T16;
		    T21 = T15 + T16;
	       }
	       T1Z = T1X + T1Y;
	       T22 = T20 + T21;
	       T2k = T21 - T20;
	       T2j = T1X - T1Y;
	       T1a = KP707106781 * (T17 - T14);
	       T1e = T1a - T1d;
	       T1C = T1d + T1a;
	       T18 = KP707106781 * (T14 + T17);
	       T19 = T11 + T18;
	       T1B = T11 - T18;
	  }
	  {
	       E TK, T1Q, TW, T1R, TN, T1T, TQ, T1U, TT, TR;
	       {
		    E TI, TJ, TU, TV;
		    TI = I[WS(is, 1)];
		    TJ = I[WS(is, 17)];
		    TK = TI - TJ;
		    T1Q = TI + TJ;
		    TU = I[WS(is, 9)];
		    TV = I[WS(is, 25)];
		    TW = TU - TV;
		    T1R = TU + TV;
	       }
	       {
		    E TL, TM, TO, TP;
		    TL = I[WS(is, 5)];
		    TM = I[WS(is, 21)];
		    TN = TL - TM;
		    T1T = TL + TM;
		    TO = I[WS(is, 29)];
		    TP = I[WS(is, 13)];
		    TQ = TO - TP;
		    T1U = TO + TP;
	       }
	       T1S = T1Q + T1R;
	       T1V = T1T + T1U;
	       T2h = T1U - T1T;
	       T2g = T1Q - T1R;
	       TT = KP707106781 * (TQ - TN);
	       TX = TT - TW;
	       T1z = TW + TT;
	       TR = KP707106781 * (TN + TQ);
	       TS = TK + TR;
	       T1y = TK - TR;
	  }
	  {
	       E Tf, Tu, T27, T28, T29, T2a;
	       Tf = T7 + Te;
	       Tu = Tm + Tt;
	       T27 = Tf + Tu;
	       T28 = T1S + T1V;
	       T29 = T1Z + T22;
	       T2a = T28 + T29;
	       ro[WS(ros, 8)] = Tf - Tu;
	       io[WS(ios, 8)] = T29 - T28;
	       ro[WS(ros, 16)] = T27 - T2a;
	       ro[0] = T27 + T2a;
	  }
	  {
	       E T1P, T25, T24, T26, T1W, T23;
	       T1P = T7 - Te;
	       T25 = Tt - Tm;
	       T1W = T1S - T1V;
	       T23 = T1Z - T22;
	       T24 = KP707106781 * (T1W + T23);
	       T26 = KP707106781 * (T23 - T1W);
	       ro[WS(ros, 12)] = T1P - T24;
	       io[WS(ios, 12)] = T26 - T25;
	       ro[WS(ros, 4)] = T1P + T24;
	       io[WS(ios, 4)] = T25 + T26;
	  }
	  {
	       E T2f, T2v, T2p, T2r, T2m, T2q, T2u, T2w, T2e, T2n;
	       T2e = KP707106781 * (T2c + T2d);
	       T2f = T2b + T2e;
	       T2v = T2b - T2e;
	       T2n = KP707106781 * (T2d - T2c);
	       T2p = T2n - T2o;
	       T2r = T2o + T2n;
	       {
		    E T2i, T2l, T2s, T2t;
		    T2i = FMA(KP923879532, T2g, KP382683432 * T2h);
		    T2l = FNMS(KP382683432, T2k, KP923879532 * T2j);
		    T2m = T2i + T2l;
		    T2q = T2l - T2i;
		    T2s = FNMS(KP382683432, T2g, KP923879532 * T2h);
		    T2t = FMA(KP382683432, T2j, KP923879532 * T2k);
		    T2u = T2s + T2t;
		    T2w = T2t - T2s;
	       }
	       ro[WS(ros, 14)] = T2f - T2m;
	       io[WS(ios, 14)] = T2u - T2r;
	       ro[WS(ros, 2)] = T2f + T2m;
	       io[WS(ios, 2)] = T2r + T2u;
	       io[WS(ios, 6)] = T2p + T2q;
	       ro[WS(ros, 6)] = T2v + T2w;
	       io[WS(ios, 10)] = T2q - T2p;
	       ro[WS(ros, 10)] = T2v - T2w;
	  }
	  {
	       E TH, T1t, T1s, T1u, T1g, T1o, T1n, T1p;
	       {
		    E Tz, TG, T1q, T1r;
		    Tz = Tv + Ty;
		    TG = TC + TF;
		    TH = Tz + TG;
		    T1t = Tz - TG;
		    T1q = FNMS(KP195090322, TS, KP980785280 * TX);
		    T1r = FMA(KP195090322, T19, KP980785280 * T1e);
		    T1s = T1q + T1r;
		    T1u = T1r - T1q;
	       }
	       {
		    E TY, T1f, T1j, T1m;
		    TY = FMA(KP980785280, TS, KP195090322 * TX);
		    T1f = FNMS(KP195090322, T1e, KP980785280 * T19);
		    T1g = TY + T1f;
		    T1o = T1f - TY;
		    T1j = T1h - T1i;
		    T1m = T1k - T1l;
		    T1n = T1j - T1m;
		    T1p = T1m + T1j;
	       }
	       ro[WS(ros, 15)] = TH - T1g;
	       io[WS(ios, 15)] = T1s - T1p;
	       ro[WS(ros, 1)] = TH + T1g;
	       io[WS(ios, 1)] = T1p + T1s;
	       io[WS(ios, 7)] = T1n + T1o;
	       ro[WS(ros, 7)] = T1t + T1u;
	       io[WS(ios, 9)] = T1o - T1n;
	       ro[WS(ros, 9)] = T1t - T1u;
	  }
	  {
	       E T1x, T1N, T1M, T1O, T1E, T1I, T1H, T1J;
	       {
		    E T1v, T1w, T1K, T1L;
		    T1v = Tv - Ty;
		    T1w = T1i + T1h;
		    T1x = T1v + T1w;
		    T1N = T1v - T1w;
		    T1K = FNMS(KP555570233, T1y, KP831469612 * T1z);
		    T1L = FMA(KP555570233, T1B, KP831469612 * T1C);
		    T1M = T1K + T1L;
		    T1O = T1L - T1K;
	       }
	       {
		    E T1A, T1D, T1F, T1G;
		    T1A = FMA(KP831469612, T1y, KP555570233 * T1z);
		    T1D = FNMS(KP555570233, T1C, KP831469612 * T1B);
		    T1E = T1A + T1D;
		    T1I = T1D - T1A;
		    T1F = TF - TC;
		    T1G = T1l + T1k;
		    T1H = T1F - T1G;
		    T1J = T1G + T1F;
	       }
	       ro[WS(ros, 13)] = T1x - T1E;
	       io[WS(ios, 13)] = T1M - T1J;
	       ro[WS(ros, 3)] = T1x + T1E;
	       io[WS(ios, 3)] = T1J + T1M;
	       io[WS(ios, 5)] = T1H + T1I;
	       ro[WS(ros, 5)] = T1N + T1O;
	       io[WS(ios, 11)] = T1I - T1H;
	       ro[WS(ros, 11)] = T1N - T1O;
	  }
     }
}

static const kr2hc_desc desc = { 32, "r2hc_32", {140, 26, 16, 0}, &GENUS, 0, 0, 0, 0, 0 };

void X(codelet_r2hc_32) (planner *p) {
     X(kr2hc_register) (p, r2hc_32, &desc);
}
