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
/* Generated on Sat Jul  5 21:56:49 EDT 2003 */

#include "codelet-rdft.h"

/* Generated by: /homee/stevenj/cvs/fftw3.0.1/genfft/gen_r2hc_noinline -compact -variables 4 -n 64 -name mr2hc_64 -include r2hc.h */

/*
 * This function contains 394 FP additions, 124 FP multiplications,
 * (or, 342 additions, 72 multiplications, 52 fused multiply/add),
 * 105 stack variables, and 128 memory accesses
 */
/*
 * Generator Id's : 
 * $Id: algsimp.ml,v 1.7 2003/03/15 20:29:42 stevenj Exp $
 * $Id: fft.ml,v 1.2 2003/03/15 20:29:42 stevenj Exp $
 * $Id: gen_r2hc_noinline.ml,v 1.1 2003/04/17 19:25:50 athena Exp $
 */

#include "r2hc.h"

static void mr2hc_64_0(const R *I, R *ro, R *io, stride is, stride ros, stride ios)
{
     DK(KP773010453, +0.773010453362736960810906609758469800971041293);
     DK(KP634393284, +0.634393284163645498215171613225493370675687095);
     DK(KP098017140, +0.098017140329560601994195563888641845861136673);
     DK(KP995184726, +0.995184726672196886244836953109479921575474869);
     DK(KP290284677, +0.290284677254462367636192375817395274691476278);
     DK(KP956940335, +0.956940335732208864935797886980269969482849206);
     DK(KP471396736, +0.471396736825997648556387625905254377657460319);
     DK(KP881921264, +0.881921264348355029712756863660388349508442621);
     DK(KP195090322, +0.195090322016128267848284868477022240927691618);
     DK(KP980785280, +0.980785280403230449126182236134239036973933731);
     DK(KP555570233, +0.555570233019602224742830813948532874374937191);
     DK(KP831469612, +0.831469612302545237078788377617905756738560812);
     DK(KP382683432, +0.382683432365089771728459984030398866761344562);
     DK(KP923879532, +0.923879532511286756128183189396788286822416626);
     DK(KP707106781, +0.707106781186547524400844362104849039284835938);
     {
	  E T4l, T5a, T15, T3n, T2T, T3Q, T7, Te, Tf, T4A, T4L, T1X, T3B, T23, T3y;
	  E T5I, T66, T4R, T52, T2j, T3F, T2H, T3I, T5P, T69, T1i, T3t, T1l, T3u, TZ;
	  E T63, T4v, T58, T1r, T3r, T1u, T3q, TK, T62, T4s, T57, Tm, Tt, Tu, T4o;
	  E T5b, T1c, T3R, T2Q, T3o, T1M, T3z, T5L, T67, T26, T3C, T4H, T4M, T2y, T3J;
	  E T5S, T6a, T2C, T3G, T4Y, T53;
	  {
	       E T3, T11, Td, T13, T6, T2S, Ta, T12, T14, T2R;
	       {
		    E T1, T2, Tb, Tc;
		    T1 = I[0];
		    T2 = I[WS(is, 32)];
		    T3 = T1 + T2;
		    T11 = T1 - T2;
		    Tb = I[WS(is, 56)];
		    Tc = I[WS(is, 24)];
		    Td = Tb + Tc;
		    T13 = Tb - Tc;
	       }
	       {
		    E T4, T5, T8, T9;
		    T4 = I[WS(is, 16)];
		    T5 = I[WS(is, 48)];
		    T6 = T4 + T5;
		    T2S = T4 - T5;
		    T8 = I[WS(is, 8)];
		    T9 = I[WS(is, 40)];
		    Ta = T8 + T9;
		    T12 = T8 - T9;
	       }
	       T4l = T3 - T6;
	       T5a = Td - Ta;
	       T14 = KP707106781 * (T12 + T13);
	       T15 = T11 + T14;
	       T3n = T11 - T14;
	       T2R = KP707106781 * (T13 - T12);
	       T2T = T2R - T2S;
	       T3Q = T2S + T2R;
	       T7 = T3 + T6;
	       Te = Ta + Td;
	       Tf = T7 + Te;
	  }
	  {
	       E T1P, T4J, T21, T4y, T1S, T4K, T1W, T4z;
	       {
		    E T1N, T1O, T1Z, T20;
		    T1N = I[WS(is, 57)];
		    T1O = I[WS(is, 25)];
		    T1P = T1N - T1O;
		    T4J = T1N + T1O;
		    T1Z = I[WS(is, 1)];
		    T20 = I[WS(is, 33)];
		    T21 = T1Z - T20;
		    T4y = T1Z + T20;
	       }
	       {
		    E T1Q, T1R, T1U, T1V;
		    T1Q = I[WS(is, 9)];
		    T1R = I[WS(is, 41)];
		    T1S = T1Q - T1R;
		    T4K = T1Q + T1R;
		    T1U = I[WS(is, 17)];
		    T1V = I[WS(is, 49)];
		    T1W = T1U - T1V;
		    T4z = T1U + T1V;
	       }
	       T4A = T4y - T4z;
	       T4L = T4J - T4K;
	       {
		    E T1T, T22, T5G, T5H;
		    T1T = KP707106781 * (T1P - T1S);
		    T1X = T1T - T1W;
		    T3B = T1W + T1T;
		    T22 = KP707106781 * (T1S + T1P);
		    T23 = T21 + T22;
		    T3y = T21 - T22;
		    T5G = T4y + T4z;
		    T5H = T4K + T4J;
		    T5I = T5G + T5H;
		    T66 = T5G - T5H;
	       }
	  }
	  {
	       E T2b, T4P, T2G, T4Q, T2e, T51, T2h, T50;
	       {
		    E T29, T2a, T2E, T2F;
		    T29 = I[WS(is, 63)];
		    T2a = I[WS(is, 31)];
		    T2b = T29 - T2a;
		    T4P = T29 + T2a;
		    T2E = I[WS(is, 15)];
		    T2F = I[WS(is, 47)];
		    T2G = T2E - T2F;
		    T4Q = T2E + T2F;
	       }
	       {
		    E T2c, T2d, T2f, T2g;
		    T2c = I[WS(is, 7)];
		    T2d = I[WS(is, 39)];
		    T2e = T2c - T2d;
		    T51 = T2c + T2d;
		    T2f = I[WS(is, 55)];
		    T2g = I[WS(is, 23)];
		    T2h = T2f - T2g;
		    T50 = T2f + T2g;
	       }
	       T4R = T4P - T4Q;
	       T52 = T50 - T51;
	       {
		    E T2i, T2D, T5N, T5O;
		    T2i = KP707106781 * (T2e + T2h);
		    T2j = T2b + T2i;
		    T3F = T2b - T2i;
		    T2D = KP707106781 * (T2h - T2e);
		    T2H = T2D - T2G;
		    T3I = T2G + T2D;
		    T5N = T4P + T4Q;
		    T5O = T51 + T50;
		    T5P = T5N + T5O;
		    T69 = T5N - T5O;
	       }
	  }
	  {
	       E TN, T1e, TX, T1g, TQ, T1k, TU, T1f, T1h, T1j;
	       {
		    E TL, TM, TV, TW;
		    TL = I[WS(is, 62)];
		    TM = I[WS(is, 30)];
		    TN = TL + TM;
		    T1e = TL - TM;
		    TV = I[WS(is, 54)];
		    TW = I[WS(is, 22)];
		    TX = TV + TW;
		    T1g = TV - TW;
	       }
	       {
		    E TO, TP, TS, TT;
		    TO = I[WS(is, 14)];
		    TP = I[WS(is, 46)];
		    TQ = TO + TP;
		    T1k = TO - TP;
		    TS = I[WS(is, 6)];
		    TT = I[WS(is, 38)];
		    TU = TS + TT;
		    T1f = TS - TT;
	       }
	       T1h = KP707106781 * (T1f + T1g);
	       T1i = T1e + T1h;
	       T3t = T1e - T1h;
	       T1j = KP707106781 * (T1g - T1f);
	       T1l = T1j - T1k;
	       T3u = T1k + T1j;
	       {
		    E TR, TY, T4t, T4u;
		    TR = TN + TQ;
		    TY = TU + TX;
		    TZ = TR + TY;
		    T63 = TR - TY;
		    T4t = TN - TQ;
		    T4u = TX - TU;
		    T4v = FNMS(KP382683432, T4u, KP923879532 * T4t);
		    T58 = FMA(KP382683432, T4t, KP923879532 * T4u);
	       }
	  }
	  {
	       E Ty, T1s, TI, T1n, TB, T1q, TF, T1o, T1p, T1t;
	       {
		    E Tw, Tx, TG, TH;
		    Tw = I[WS(is, 2)];
		    Tx = I[WS(is, 34)];
		    Ty = Tw + Tx;
		    T1s = Tw - Tx;
		    TG = I[WS(is, 58)];
		    TH = I[WS(is, 26)];
		    TI = TG + TH;
		    T1n = TG - TH;
	       }
	       {
		    E Tz, TA, TD, TE;
		    Tz = I[WS(is, 18)];
		    TA = I[WS(is, 50)];
		    TB = Tz + TA;
		    T1q = Tz - TA;
		    TD = I[WS(is, 10)];
		    TE = I[WS(is, 42)];
		    TF = TD + TE;
		    T1o = TD - TE;
	       }
	       T1p = KP707106781 * (T1n - T1o);
	       T1r = T1p - T1q;
	       T3r = T1q + T1p;
	       T1t = KP707106781 * (T1o + T1n);
	       T1u = T1s + T1t;
	       T3q = T1s - T1t;
	       {
		    E TC, TJ, T4q, T4r;
		    TC = Ty + TB;
		    TJ = TF + TI;
		    TK = TC + TJ;
		    T62 = TC - TJ;
		    T4q = Ty - TB;
		    T4r = TI - TF;
		    T4s = FMA(KP923879532, T4q, KP382683432 * T4r);
		    T57 = FNMS(KP382683432, T4q, KP923879532 * T4r);
	       }
	  }
	  {
	       E Ti, T16, Ts, T1a, Tl, T17, Tp, T19, T4m, T4n;
	       {
		    E Tg, Th, Tq, Tr;
		    Tg = I[WS(is, 4)];
		    Th = I[WS(is, 36)];
		    Ti = Tg + Th;
		    T16 = Tg - Th;
		    Tq = I[WS(is, 12)];
		    Tr = I[WS(is, 44)];
		    Ts = Tq + Tr;
		    T1a = Tq - Tr;
	       }
	       {
		    E Tj, Tk, Tn, To;
		    Tj = I[WS(is, 20)];
		    Tk = I[WS(is, 52)];
		    Tl = Tj + Tk;
		    T17 = Tj - Tk;
		    Tn = I[WS(is, 60)];
		    To = I[WS(is, 28)];
		    Tp = Tn + To;
		    T19 = Tn - To;
	       }
	       Tm = Ti + Tl;
	       Tt = Tp + Ts;
	       Tu = Tm + Tt;
	       T4m = Ti - Tl;
	       T4n = Tp - Ts;
	       T4o = KP707106781 * (T4m + T4n);
	       T5b = KP707106781 * (T4n - T4m);
	       {
		    E T18, T1b, T2O, T2P;
		    T18 = FNMS(KP382683432, T17, KP923879532 * T16);
		    T1b = FMA(KP923879532, T19, KP382683432 * T1a);
		    T1c = T18 + T1b;
		    T3R = T1b - T18;
		    T2O = FNMS(KP923879532, T1a, KP382683432 * T19);
		    T2P = FMA(KP382683432, T16, KP923879532 * T17);
		    T2Q = T2O - T2P;
		    T3o = T2P + T2O;
	       }
	  }
	  {
	       E T1A, T4E, T1K, T4C, T1D, T4F, T1H, T4B;
	       {
		    E T1y, T1z, T1I, T1J;
		    T1y = I[WS(is, 61)];
		    T1z = I[WS(is, 29)];
		    T1A = T1y - T1z;
		    T4E = T1y + T1z;
		    T1I = I[WS(is, 21)];
		    T1J = I[WS(is, 53)];
		    T1K = T1I - T1J;
		    T4C = T1I + T1J;
	       }
	       {
		    E T1B, T1C, T1F, T1G;
		    T1B = I[WS(is, 13)];
		    T1C = I[WS(is, 45)];
		    T1D = T1B - T1C;
		    T4F = T1B + T1C;
		    T1F = I[WS(is, 5)];
		    T1G = I[WS(is, 37)];
		    T1H = T1F - T1G;
		    T4B = T1F + T1G;
	       }
	       {
		    E T1E, T1L, T5J, T5K;
		    T1E = FNMS(KP923879532, T1D, KP382683432 * T1A);
		    T1L = FMA(KP382683432, T1H, KP923879532 * T1K);
		    T1M = T1E - T1L;
		    T3z = T1L + T1E;
		    T5J = T4B + T4C;
		    T5K = T4E + T4F;
		    T5L = T5J + T5K;
		    T67 = T5K - T5J;
	       }
	       {
		    E T24, T25, T4D, T4G;
		    T24 = FNMS(KP382683432, T1K, KP923879532 * T1H);
		    T25 = FMA(KP923879532, T1A, KP382683432 * T1D);
		    T26 = T24 + T25;
		    T3C = T25 - T24;
		    T4D = T4B - T4C;
		    T4G = T4E - T4F;
		    T4H = KP707106781 * (T4D + T4G);
		    T4M = KP707106781 * (T4G - T4D);
	       }
	  }
	  {
	       E T2m, T4S, T2w, T4W, T2p, T4T, T2t, T4V;
	       {
		    E T2k, T2l, T2u, T2v;
		    T2k = I[WS(is, 3)];
		    T2l = I[WS(is, 35)];
		    T2m = T2k - T2l;
		    T4S = T2k + T2l;
		    T2u = I[WS(is, 11)];
		    T2v = I[WS(is, 43)];
		    T2w = T2u - T2v;
		    T4W = T2u + T2v;
	       }
	       {
		    E T2n, T2o, T2r, T2s;
		    T2n = I[WS(is, 19)];
		    T2o = I[WS(is, 51)];
		    T2p = T2n - T2o;
		    T4T = T2n + T2o;
		    T2r = I[WS(is, 59)];
		    T2s = I[WS(is, 27)];
		    T2t = T2r - T2s;
		    T4V = T2r + T2s;
	       }
	       {
		    E T2q, T2x, T5Q, T5R;
		    T2q = FNMS(KP382683432, T2p, KP923879532 * T2m);
		    T2x = FMA(KP923879532, T2t, KP382683432 * T2w);
		    T2y = T2q + T2x;
		    T3J = T2x - T2q;
		    T5Q = T4S + T4T;
		    T5R = T4V + T4W;
		    T5S = T5Q + T5R;
		    T6a = T5R - T5Q;
	       }
	       {
		    E T2A, T2B, T4U, T4X;
		    T2A = FNMS(KP923879532, T2w, KP382683432 * T2t);
		    T2B = FMA(KP382683432, T2m, KP923879532 * T2p);
		    T2C = T2A - T2B;
		    T3G = T2B + T2A;
		    T4U = T4S - T4T;
		    T4X = T4V - T4W;
		    T4Y = KP707106781 * (T4U + T4X);
		    T53 = KP707106781 * (T4X - T4U);
	       }
	  }
	  {
	       E Tv, T10, T5X, T5Y, T5Z, T60;
	       Tv = Tf + Tu;
	       T10 = TK + TZ;
	       T5X = Tv + T10;
	       T5Y = T5I + T5L;
	       T5Z = T5P + T5S;
	       T60 = T5Y + T5Z;
	       ro[WS(ros, 16)] = Tv - T10;
	       io[WS(ios, 16)] = T5Z - T5Y;
	       ro[WS(ros, 32)] = T5X - T60;
	       ro[0] = T5X + T60;
	  }
	  {
	       E T5F, T5V, T5U, T5W, T5M, T5T;
	       T5F = Tf - Tu;
	       T5V = TZ - TK;
	       T5M = T5I - T5L;
	       T5T = T5P - T5S;
	       T5U = KP707106781 * (T5M + T5T);
	       T5W = KP707106781 * (T5T - T5M);
	       ro[WS(ros, 24)] = T5F - T5U;
	       io[WS(ios, 24)] = T5W - T5V;
	       ro[WS(ros, 8)] = T5F + T5U;
	       io[WS(ios, 8)] = T5V + T5W;
	  }
	  {
	       E T65, T6l, T6k, T6m, T6c, T6g, T6f, T6h;
	       {
		    E T61, T64, T6i, T6j;
		    T61 = T7 - Te;
		    T64 = KP707106781 * (T62 + T63);
		    T65 = T61 + T64;
		    T6l = T61 - T64;
		    T6i = FNMS(KP382683432, T66, KP923879532 * T67);
		    T6j = FMA(KP382683432, T69, KP923879532 * T6a);
		    T6k = T6i + T6j;
		    T6m = T6j - T6i;
	       }
	       {
		    E T68, T6b, T6d, T6e;
		    T68 = FMA(KP923879532, T66, KP382683432 * T67);
		    T6b = FNMS(KP382683432, T6a, KP923879532 * T69);
		    T6c = T68 + T6b;
		    T6g = T6b - T68;
		    T6d = KP707106781 * (T63 - T62);
		    T6e = Tt - Tm;
		    T6f = T6d - T6e;
		    T6h = T6e + T6d;
	       }
	       ro[WS(ros, 28)] = T65 - T6c;
	       io[WS(ios, 28)] = T6k - T6h;
	       ro[WS(ros, 4)] = T65 + T6c;
	       io[WS(ios, 4)] = T6h + T6k;
	       io[WS(ios, 12)] = T6f + T6g;
	       ro[WS(ros, 12)] = T6l + T6m;
	       io[WS(ios, 20)] = T6g - T6f;
	       ro[WS(ros, 20)] = T6l - T6m;
	  }
	  {
	       E T5n, T5D, T5x, T5z, T5q, T5A, T5t, T5B;
	       {
		    E T5l, T5m, T5v, T5w;
		    T5l = T4l - T4o;
		    T5m = T58 - T57;
		    T5n = T5l + T5m;
		    T5D = T5l - T5m;
		    T5v = T4v - T4s;
		    T5w = T5b - T5a;
		    T5x = T5v - T5w;
		    T5z = T5w + T5v;
	       }
	       {
		    E T5o, T5p, T5r, T5s;
		    T5o = T4A - T4H;
		    T5p = T4M - T4L;
		    T5q = FMA(KP831469612, T5o, KP555570233 * T5p);
		    T5A = FNMS(KP555570233, T5o, KP831469612 * T5p);
		    T5r = T4R - T4Y;
		    T5s = T53 - T52;
		    T5t = FNMS(KP555570233, T5s, KP831469612 * T5r);
		    T5B = FMA(KP555570233, T5r, KP831469612 * T5s);
	       }
	       {
		    E T5u, T5C, T5y, T5E;
		    T5u = T5q + T5t;
		    ro[WS(ros, 26)] = T5n - T5u;
		    ro[WS(ros, 6)] = T5n + T5u;
		    T5C = T5A + T5B;
		    io[WS(ios, 6)] = T5z + T5C;
		    io[WS(ios, 26)] = T5C - T5z;
		    T5y = T5t - T5q;
		    io[WS(ios, 10)] = T5x + T5y;
		    io[WS(ios, 22)] = T5y - T5x;
		    T5E = T5B - T5A;
		    ro[WS(ros, 22)] = T5D - T5E;
		    ro[WS(ros, 10)] = T5D + T5E;
	       }
	  }
	  {
	       E T4x, T5j, T5d, T5f, T4O, T5g, T55, T5h;
	       {
		    E T4p, T4w, T59, T5c;
		    T4p = T4l + T4o;
		    T4w = T4s + T4v;
		    T4x = T4p + T4w;
		    T5j = T4p - T4w;
		    T59 = T57 + T58;
		    T5c = T5a + T5b;
		    T5d = T59 - T5c;
		    T5f = T5c + T59;
	       }
	       {
		    E T4I, T4N, T4Z, T54;
		    T4I = T4A + T4H;
		    T4N = T4L + T4M;
		    T4O = FMA(KP980785280, T4I, KP195090322 * T4N);
		    T5g = FNMS(KP195090322, T4I, KP980785280 * T4N);
		    T4Z = T4R + T4Y;
		    T54 = T52 + T53;
		    T55 = FNMS(KP195090322, T54, KP980785280 * T4Z);
		    T5h = FMA(KP195090322, T4Z, KP980785280 * T54);
	       }
	       {
		    E T56, T5i, T5e, T5k;
		    T56 = T4O + T55;
		    ro[WS(ros, 30)] = T4x - T56;
		    ro[WS(ros, 2)] = T4x + T56;
		    T5i = T5g + T5h;
		    io[WS(ios, 2)] = T5f + T5i;
		    io[WS(ios, 30)] = T5i - T5f;
		    T5e = T55 - T4O;
		    io[WS(ios, 14)] = T5d + T5e;
		    io[WS(ios, 18)] = T5e - T5d;
		    T5k = T5h - T5g;
		    ro[WS(ros, 18)] = T5j - T5k;
		    ro[WS(ros, 14)] = T5j + T5k;
	       }
	  }
	  {
	       E T3p, T41, T4c, T3S, T3w, T4b, T49, T4h, T3P, T42, T3E, T3W, T46, T4g, T3L;
	       E T3X;
	       {
		    E T3s, T3v, T3A, T3D;
		    T3p = T3n + T3o;
		    T41 = T3n - T3o;
		    T4c = T3R - T3Q;
		    T3S = T3Q + T3R;
		    T3s = FMA(KP831469612, T3q, KP555570233 * T3r);
		    T3v = FNMS(KP555570233, T3u, KP831469612 * T3t);
		    T3w = T3s + T3v;
		    T4b = T3v - T3s;
		    {
			 E T47, T48, T3N, T3O;
			 T47 = T3F - T3G;
			 T48 = T3J - T3I;
			 T49 = FNMS(KP471396736, T48, KP881921264 * T47);
			 T4h = FMA(KP471396736, T47, KP881921264 * T48);
			 T3N = FNMS(KP555570233, T3q, KP831469612 * T3r);
			 T3O = FMA(KP555570233, T3t, KP831469612 * T3u);
			 T3P = T3N + T3O;
			 T42 = T3O - T3N;
		    }
		    T3A = T3y + T3z;
		    T3D = T3B + T3C;
		    T3E = FMA(KP956940335, T3A, KP290284677 * T3D);
		    T3W = FNMS(KP290284677, T3A, KP956940335 * T3D);
		    {
			 E T44, T45, T3H, T3K;
			 T44 = T3y - T3z;
			 T45 = T3C - T3B;
			 T46 = FMA(KP881921264, T44, KP471396736 * T45);
			 T4g = FNMS(KP471396736, T44, KP881921264 * T45);
			 T3H = T3F + T3G;
			 T3K = T3I + T3J;
			 T3L = FNMS(KP290284677, T3K, KP956940335 * T3H);
			 T3X = FMA(KP290284677, T3H, KP956940335 * T3K);
		    }
	       }
	       {
		    E T3x, T3M, T3V, T3Y;
		    T3x = T3p + T3w;
		    T3M = T3E + T3L;
		    ro[WS(ros, 29)] = T3x - T3M;
		    ro[WS(ros, 3)] = T3x + T3M;
		    T3V = T3S + T3P;
		    T3Y = T3W + T3X;
		    io[WS(ios, 3)] = T3V + T3Y;
		    io[WS(ios, 29)] = T3Y - T3V;
	       }
	       {
		    E T3T, T3U, T3Z, T40;
		    T3T = T3P - T3S;
		    T3U = T3L - T3E;
		    io[WS(ios, 13)] = T3T + T3U;
		    io[WS(ios, 19)] = T3U - T3T;
		    T3Z = T3p - T3w;
		    T40 = T3X - T3W;
		    ro[WS(ros, 19)] = T3Z - T40;
		    ro[WS(ros, 13)] = T3Z + T40;
	       }
	       {
		    E T43, T4a, T4f, T4i;
		    T43 = T41 + T42;
		    T4a = T46 + T49;
		    ro[WS(ros, 27)] = T43 - T4a;
		    ro[WS(ros, 5)] = T43 + T4a;
		    T4f = T4c + T4b;
		    T4i = T4g + T4h;
		    io[WS(ios, 5)] = T4f + T4i;
		    io[WS(ios, 27)] = T4i - T4f;
	       }
	       {
		    E T4d, T4e, T4j, T4k;
		    T4d = T4b - T4c;
		    T4e = T49 - T46;
		    io[WS(ios, 11)] = T4d + T4e;
		    io[WS(ios, 21)] = T4e - T4d;
		    T4j = T41 - T42;
		    T4k = T4h - T4g;
		    ro[WS(ros, 21)] = T4j - T4k;
		    ro[WS(ros, 11)] = T4j + T4k;
	       }
	  }
	  {
	       E T1d, T33, T3e, T2U, T1w, T3d, T3b, T3j, T2N, T34, T28, T2Y, T38, T3i, T2J;
	       E T2Z;
	       {
		    E T1m, T1v, T1Y, T27;
		    T1d = T15 - T1c;
		    T33 = T15 + T1c;
		    T3e = T2T + T2Q;
		    T2U = T2Q - T2T;
		    T1m = FMA(KP195090322, T1i, KP980785280 * T1l);
		    T1v = FNMS(KP195090322, T1u, KP980785280 * T1r);
		    T1w = T1m - T1v;
		    T3d = T1v + T1m;
		    {
			 E T39, T3a, T2L, T2M;
			 T39 = T2j + T2y;
			 T3a = T2H + T2C;
			 T3b = FNMS(KP098017140, T3a, KP995184726 * T39);
			 T3j = FMA(KP995184726, T3a, KP098017140 * T39);
			 T2L = FNMS(KP195090322, T1l, KP980785280 * T1i);
			 T2M = FMA(KP980785280, T1u, KP195090322 * T1r);
			 T2N = T2L - T2M;
			 T34 = T2M + T2L;
		    }
		    T1Y = T1M - T1X;
		    T27 = T23 - T26;
		    T28 = FMA(KP634393284, T1Y, KP773010453 * T27);
		    T2Y = FNMS(KP634393284, T27, KP773010453 * T1Y);
		    {
			 E T36, T37, T2z, T2I;
			 T36 = T1X + T1M;
			 T37 = T23 + T26;
			 T38 = FMA(KP098017140, T36, KP995184726 * T37);
			 T3i = FNMS(KP098017140, T37, KP995184726 * T36);
			 T2z = T2j - T2y;
			 T2I = T2C - T2H;
			 T2J = FNMS(KP634393284, T2I, KP773010453 * T2z);
			 T2Z = FMA(KP773010453, T2I, KP634393284 * T2z);
		    }
	       }
	       {
		    E T1x, T2K, T2X, T30;
		    T1x = T1d + T1w;
		    T2K = T28 + T2J;
		    ro[WS(ros, 25)] = T1x - T2K;
		    ro[WS(ros, 7)] = T1x + T2K;
		    T2X = T2U + T2N;
		    T30 = T2Y + T2Z;
		    io[WS(ios, 7)] = T2X + T30;
		    io[WS(ios, 25)] = T30 - T2X;
	       }
	       {
		    E T2V, T2W, T31, T32;
		    T2V = T2N - T2U;
		    T2W = T2J - T28;
		    io[WS(ios, 9)] = T2V + T2W;
		    io[WS(ios, 23)] = T2W - T2V;
		    T31 = T1d - T1w;
		    T32 = T2Z - T2Y;
		    ro[WS(ros, 23)] = T31 - T32;
		    ro[WS(ros, 9)] = T31 + T32;
	       }
	       {
		    E T35, T3c, T3h, T3k;
		    T35 = T33 + T34;
		    T3c = T38 + T3b;
		    ro[WS(ros, 31)] = T35 - T3c;
		    ro[WS(ros, 1)] = T35 + T3c;
		    T3h = T3e + T3d;
		    T3k = T3i + T3j;
		    io[WS(ios, 1)] = T3h + T3k;
		    io[WS(ios, 31)] = T3k - T3h;
	       }
	       {
		    E T3f, T3g, T3l, T3m;
		    T3f = T3d - T3e;
		    T3g = T3b - T38;
		    io[WS(ios, 15)] = T3f + T3g;
		    io[WS(ios, 17)] = T3g - T3f;
		    T3l = T33 - T34;
		    T3m = T3j - T3i;
		    ro[WS(ros, 17)] = T3l - T3m;
		    ro[WS(ros, 15)] = T3l + T3m;
	       }
	  }
     }
}

static void mr2hc_64(const R *I, R *ro, R *io, stride is, stride ros, stride ios, int v, int ivs, int ovs)
{
     int i;
     for (i = v; i > 0; --i) {
	  mr2hc_64_0(I, ro, io, is, ros, ios);
	  I += ivs;
	  ro += ovs;
	  io += ovs;
     }
}

static const kr2hc_desc desc = { 64, "mr2hc_64", {342, 72, 52, 0}, &GENUS, 0, 0, 0, 0, 0 };

void X(codelet_mr2hc_64) (planner *p) {
     X(kr2hc_register) (p, mr2hc_64, &desc);
}
