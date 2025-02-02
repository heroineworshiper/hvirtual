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
/* Generated on Sat Jul  5 21:45:22 EDT 2003 */

#include "codelet-dft.h"

/* Generated by: /homee/stevenj/cvs/fftw3.0.1/genfft/gen_twidsq_c -simd -compact -variables 4 -n 8 -dif -name q1bv_8 -include q1b.h -sign 1 */

/*
 * This function contains 264 FP additions, 128 FP multiplications,
 * (or, 264 additions, 128 multiplications, 0 fused multiply/add),
 * 77 stack variables, and 128 memory accesses
 */
/*
 * Generator Id's : 
 * $Id: algsimp.ml,v 1.7 2003/03/15 20:29:42 stevenj Exp $
 * $Id: fft.ml,v 1.2 2003/03/15 20:29:42 stevenj Exp $
 * $Id: gen_twidsq_c.ml,v 1.1 2003/03/26 12:45:03 athena Exp $
 */

#include "q1b.h"

static const R *q1bv_8(R *ri, R *ii, const R *W, stride is, stride vs, int m, int dist)
{
     DVK(KP707106781, +0.707106781186547524400844362104849039284835938);
     int i;
     R *x;
     x = ii;
     BEGIN_SIMD();
     for (i = 0; i < m; i = i + VL, x = x + (VL * dist), W = W + (TWVL * 14)) {
	  V Ta, Tv, Te, Tp, T1L, T26, T1P, T20, T2i, T2D, T2m, T2x, T3T, T4e, T3X;
	  V T48, TH, T12, TL, TW, T1e, T1z, T1i, T1t, T2P, T3a, T2T, T34, T3m, T3H;
	  V T3q, T3B, T7, Tw, Tf, Ts, T1I, T27, T1Q, T23, T2f, T2E, T2n, T2A, T3Q;
	  V T4f, T3Y, T4b, TE, T13, TM, TZ, T1b, T1A, T1j, T1w, T2M, T3b, T2U, T37;
	  V T3j, T3I, T3r, T3E, T28, T14;
	  {
	       V T8, T9, To, Tc, Td, Tn;
	       T8 = LD(&(x[WS(is, 2)]), dist, &(x[0]));
	       T9 = LD(&(x[WS(is, 6)]), dist, &(x[0]));
	       To = VADD(T8, T9);
	       Tc = LD(&(x[0]), dist, &(x[0]));
	       Td = LD(&(x[WS(is, 4)]), dist, &(x[0]));
	       Tn = VADD(Tc, Td);
	       Ta = VSUB(T8, T9);
	       Tv = VADD(Tn, To);
	       Te = VSUB(Tc, Td);
	       Tp = VSUB(Tn, To);
	  }
	  {
	       V T1J, T1K, T1Z, T1N, T1O, T1Y;
	       T1J = LD(&(x[WS(vs, 3) + WS(is, 2)]), dist, &(x[WS(vs, 3)]));
	       T1K = LD(&(x[WS(vs, 3) + WS(is, 6)]), dist, &(x[WS(vs, 3)]));
	       T1Z = VADD(T1J, T1K);
	       T1N = LD(&(x[WS(vs, 3)]), dist, &(x[WS(vs, 3)]));
	       T1O = LD(&(x[WS(vs, 3) + WS(is, 4)]), dist, &(x[WS(vs, 3)]));
	       T1Y = VADD(T1N, T1O);
	       T1L = VSUB(T1J, T1K);
	       T26 = VADD(T1Y, T1Z);
	       T1P = VSUB(T1N, T1O);
	       T20 = VSUB(T1Y, T1Z);
	  }
	  {
	       V T2g, T2h, T2w, T2k, T2l, T2v;
	       T2g = LD(&(x[WS(vs, 4) + WS(is, 2)]), dist, &(x[WS(vs, 4)]));
	       T2h = LD(&(x[WS(vs, 4) + WS(is, 6)]), dist, &(x[WS(vs, 4)]));
	       T2w = VADD(T2g, T2h);
	       T2k = LD(&(x[WS(vs, 4)]), dist, &(x[WS(vs, 4)]));
	       T2l = LD(&(x[WS(vs, 4) + WS(is, 4)]), dist, &(x[WS(vs, 4)]));
	       T2v = VADD(T2k, T2l);
	       T2i = VSUB(T2g, T2h);
	       T2D = VADD(T2v, T2w);
	       T2m = VSUB(T2k, T2l);
	       T2x = VSUB(T2v, T2w);
	  }
	  {
	       V T3R, T3S, T47, T3V, T3W, T46;
	       T3R = LD(&(x[WS(vs, 7) + WS(is, 2)]), dist, &(x[WS(vs, 7)]));
	       T3S = LD(&(x[WS(vs, 7) + WS(is, 6)]), dist, &(x[WS(vs, 7)]));
	       T47 = VADD(T3R, T3S);
	       T3V = LD(&(x[WS(vs, 7)]), dist, &(x[WS(vs, 7)]));
	       T3W = LD(&(x[WS(vs, 7) + WS(is, 4)]), dist, &(x[WS(vs, 7)]));
	       T46 = VADD(T3V, T3W);
	       T3T = VSUB(T3R, T3S);
	       T4e = VADD(T46, T47);
	       T3X = VSUB(T3V, T3W);
	       T48 = VSUB(T46, T47);
	  }
	  {
	       V TF, TG, TV, TJ, TK, TU;
	       TF = LD(&(x[WS(vs, 1) + WS(is, 2)]), dist, &(x[WS(vs, 1)]));
	       TG = LD(&(x[WS(vs, 1) + WS(is, 6)]), dist, &(x[WS(vs, 1)]));
	       TV = VADD(TF, TG);
	       TJ = LD(&(x[WS(vs, 1)]), dist, &(x[WS(vs, 1)]));
	       TK = LD(&(x[WS(vs, 1) + WS(is, 4)]), dist, &(x[WS(vs, 1)]));
	       TU = VADD(TJ, TK);
	       TH = VSUB(TF, TG);
	       T12 = VADD(TU, TV);
	       TL = VSUB(TJ, TK);
	       TW = VSUB(TU, TV);
	  }
	  {
	       V T1c, T1d, T1s, T1g, T1h, T1r;
	       T1c = LD(&(x[WS(vs, 2) + WS(is, 2)]), dist, &(x[WS(vs, 2)]));
	       T1d = LD(&(x[WS(vs, 2) + WS(is, 6)]), dist, &(x[WS(vs, 2)]));
	       T1s = VADD(T1c, T1d);
	       T1g = LD(&(x[WS(vs, 2)]), dist, &(x[WS(vs, 2)]));
	       T1h = LD(&(x[WS(vs, 2) + WS(is, 4)]), dist, &(x[WS(vs, 2)]));
	       T1r = VADD(T1g, T1h);
	       T1e = VSUB(T1c, T1d);
	       T1z = VADD(T1r, T1s);
	       T1i = VSUB(T1g, T1h);
	       T1t = VSUB(T1r, T1s);
	  }
	  {
	       V T2N, T2O, T33, T2R, T2S, T32;
	       T2N = LD(&(x[WS(vs, 5) + WS(is, 2)]), dist, &(x[WS(vs, 5)]));
	       T2O = LD(&(x[WS(vs, 5) + WS(is, 6)]), dist, &(x[WS(vs, 5)]));
	       T33 = VADD(T2N, T2O);
	       T2R = LD(&(x[WS(vs, 5)]), dist, &(x[WS(vs, 5)]));
	       T2S = LD(&(x[WS(vs, 5) + WS(is, 4)]), dist, &(x[WS(vs, 5)]));
	       T32 = VADD(T2R, T2S);
	       T2P = VSUB(T2N, T2O);
	       T3a = VADD(T32, T33);
	       T2T = VSUB(T2R, T2S);
	       T34 = VSUB(T32, T33);
	  }
	  {
	       V T3k, T3l, T3A, T3o, T3p, T3z;
	       T3k = LD(&(x[WS(vs, 6) + WS(is, 2)]), dist, &(x[WS(vs, 6)]));
	       T3l = LD(&(x[WS(vs, 6) + WS(is, 6)]), dist, &(x[WS(vs, 6)]));
	       T3A = VADD(T3k, T3l);
	       T3o = LD(&(x[WS(vs, 6)]), dist, &(x[WS(vs, 6)]));
	       T3p = LD(&(x[WS(vs, 6) + WS(is, 4)]), dist, &(x[WS(vs, 6)]));
	       T3z = VADD(T3o, T3p);
	       T3m = VSUB(T3k, T3l);
	       T3H = VADD(T3z, T3A);
	       T3q = VSUB(T3o, T3p);
	       T3B = VSUB(T3z, T3A);
	  }
	  {
	       V T3, Tq, T6, Tr;
	       {
		    V T1, T2, T4, T5;
		    T1 = LD(&(x[WS(is, 1)]), dist, &(x[WS(is, 1)]));
		    T2 = LD(&(x[WS(is, 5)]), dist, &(x[WS(is, 1)]));
		    T3 = VSUB(T1, T2);
		    Tq = VADD(T1, T2);
		    T4 = LD(&(x[WS(is, 7)]), dist, &(x[WS(is, 1)]));
		    T5 = LD(&(x[WS(is, 3)]), dist, &(x[WS(is, 1)]));
		    T6 = VSUB(T4, T5);
		    Tr = VADD(T4, T5);
	       }
	       T7 = VMUL(LDK(KP707106781), VSUB(T3, T6));
	       Tw = VADD(Tq, Tr);
	       Tf = VMUL(LDK(KP707106781), VADD(T3, T6));
	       Ts = VBYI(VSUB(Tq, Tr));
	  }
	  {
	       V T1E, T21, T1H, T22;
	       {
		    V T1C, T1D, T1F, T1G;
		    T1C = LD(&(x[WS(vs, 3) + WS(is, 1)]), dist, &(x[WS(vs, 3) + WS(is, 1)]));
		    T1D = LD(&(x[WS(vs, 3) + WS(is, 5)]), dist, &(x[WS(vs, 3) + WS(is, 1)]));
		    T1E = VSUB(T1C, T1D);
		    T21 = VADD(T1C, T1D);
		    T1F = LD(&(x[WS(vs, 3) + WS(is, 7)]), dist, &(x[WS(vs, 3) + WS(is, 1)]));
		    T1G = LD(&(x[WS(vs, 3) + WS(is, 3)]), dist, &(x[WS(vs, 3) + WS(is, 1)]));
		    T1H = VSUB(T1F, T1G);
		    T22 = VADD(T1F, T1G);
	       }
	       T1I = VMUL(LDK(KP707106781), VSUB(T1E, T1H));
	       T27 = VADD(T21, T22);
	       T1Q = VMUL(LDK(KP707106781), VADD(T1E, T1H));
	       T23 = VBYI(VSUB(T21, T22));
	  }
	  {
	       V T2b, T2y, T2e, T2z;
	       {
		    V T29, T2a, T2c, T2d;
		    T29 = LD(&(x[WS(vs, 4) + WS(is, 1)]), dist, &(x[WS(vs, 4) + WS(is, 1)]));
		    T2a = LD(&(x[WS(vs, 4) + WS(is, 5)]), dist, &(x[WS(vs, 4) + WS(is, 1)]));
		    T2b = VSUB(T29, T2a);
		    T2y = VADD(T29, T2a);
		    T2c = LD(&(x[WS(vs, 4) + WS(is, 7)]), dist, &(x[WS(vs, 4) + WS(is, 1)]));
		    T2d = LD(&(x[WS(vs, 4) + WS(is, 3)]), dist, &(x[WS(vs, 4) + WS(is, 1)]));
		    T2e = VSUB(T2c, T2d);
		    T2z = VADD(T2c, T2d);
	       }
	       T2f = VMUL(LDK(KP707106781), VSUB(T2b, T2e));
	       T2E = VADD(T2y, T2z);
	       T2n = VMUL(LDK(KP707106781), VADD(T2b, T2e));
	       T2A = VBYI(VSUB(T2y, T2z));
	  }
	  {
	       V T3M, T49, T3P, T4a;
	       {
		    V T3K, T3L, T3N, T3O;
		    T3K = LD(&(x[WS(vs, 7) + WS(is, 1)]), dist, &(x[WS(vs, 7) + WS(is, 1)]));
		    T3L = LD(&(x[WS(vs, 7) + WS(is, 5)]), dist, &(x[WS(vs, 7) + WS(is, 1)]));
		    T3M = VSUB(T3K, T3L);
		    T49 = VADD(T3K, T3L);
		    T3N = LD(&(x[WS(vs, 7) + WS(is, 7)]), dist, &(x[WS(vs, 7) + WS(is, 1)]));
		    T3O = LD(&(x[WS(vs, 7) + WS(is, 3)]), dist, &(x[WS(vs, 7) + WS(is, 1)]));
		    T3P = VSUB(T3N, T3O);
		    T4a = VADD(T3N, T3O);
	       }
	       T3Q = VMUL(LDK(KP707106781), VSUB(T3M, T3P));
	       T4f = VADD(T49, T4a);
	       T3Y = VMUL(LDK(KP707106781), VADD(T3M, T3P));
	       T4b = VBYI(VSUB(T49, T4a));
	  }
	  {
	       V TA, TX, TD, TY;
	       {
		    V Ty, Tz, TB, TC;
		    Ty = LD(&(x[WS(vs, 1) + WS(is, 1)]), dist, &(x[WS(vs, 1) + WS(is, 1)]));
		    Tz = LD(&(x[WS(vs, 1) + WS(is, 5)]), dist, &(x[WS(vs, 1) + WS(is, 1)]));
		    TA = VSUB(Ty, Tz);
		    TX = VADD(Ty, Tz);
		    TB = LD(&(x[WS(vs, 1) + WS(is, 7)]), dist, &(x[WS(vs, 1) + WS(is, 1)]));
		    TC = LD(&(x[WS(vs, 1) + WS(is, 3)]), dist, &(x[WS(vs, 1) + WS(is, 1)]));
		    TD = VSUB(TB, TC);
		    TY = VADD(TB, TC);
	       }
	       TE = VMUL(LDK(KP707106781), VSUB(TA, TD));
	       T13 = VADD(TX, TY);
	       TM = VMUL(LDK(KP707106781), VADD(TA, TD));
	       TZ = VBYI(VSUB(TX, TY));
	  }
	  {
	       V T17, T1u, T1a, T1v;
	       {
		    V T15, T16, T18, T19;
		    T15 = LD(&(x[WS(vs, 2) + WS(is, 1)]), dist, &(x[WS(vs, 2) + WS(is, 1)]));
		    T16 = LD(&(x[WS(vs, 2) + WS(is, 5)]), dist, &(x[WS(vs, 2) + WS(is, 1)]));
		    T17 = VSUB(T15, T16);
		    T1u = VADD(T15, T16);
		    T18 = LD(&(x[WS(vs, 2) + WS(is, 7)]), dist, &(x[WS(vs, 2) + WS(is, 1)]));
		    T19 = LD(&(x[WS(vs, 2) + WS(is, 3)]), dist, &(x[WS(vs, 2) + WS(is, 1)]));
		    T1a = VSUB(T18, T19);
		    T1v = VADD(T18, T19);
	       }
	       T1b = VMUL(LDK(KP707106781), VSUB(T17, T1a));
	       T1A = VADD(T1u, T1v);
	       T1j = VMUL(LDK(KP707106781), VADD(T17, T1a));
	       T1w = VBYI(VSUB(T1u, T1v));
	  }
	  {
	       V T2I, T35, T2L, T36;
	       {
		    V T2G, T2H, T2J, T2K;
		    T2G = LD(&(x[WS(vs, 5) + WS(is, 1)]), dist, &(x[WS(vs, 5) + WS(is, 1)]));
		    T2H = LD(&(x[WS(vs, 5) + WS(is, 5)]), dist, &(x[WS(vs, 5) + WS(is, 1)]));
		    T2I = VSUB(T2G, T2H);
		    T35 = VADD(T2G, T2H);
		    T2J = LD(&(x[WS(vs, 5) + WS(is, 7)]), dist, &(x[WS(vs, 5) + WS(is, 1)]));
		    T2K = LD(&(x[WS(vs, 5) + WS(is, 3)]), dist, &(x[WS(vs, 5) + WS(is, 1)]));
		    T2L = VSUB(T2J, T2K);
		    T36 = VADD(T2J, T2K);
	       }
	       T2M = VMUL(LDK(KP707106781), VSUB(T2I, T2L));
	       T3b = VADD(T35, T36);
	       T2U = VMUL(LDK(KP707106781), VADD(T2I, T2L));
	       T37 = VBYI(VSUB(T35, T36));
	  }
	  {
	       V T3f, T3C, T3i, T3D;
	       {
		    V T3d, T3e, T3g, T3h;
		    T3d = LD(&(x[WS(vs, 6) + WS(is, 1)]), dist, &(x[WS(vs, 6) + WS(is, 1)]));
		    T3e = LD(&(x[WS(vs, 6) + WS(is, 5)]), dist, &(x[WS(vs, 6) + WS(is, 1)]));
		    T3f = VSUB(T3d, T3e);
		    T3C = VADD(T3d, T3e);
		    T3g = LD(&(x[WS(vs, 6) + WS(is, 7)]), dist, &(x[WS(vs, 6) + WS(is, 1)]));
		    T3h = LD(&(x[WS(vs, 6) + WS(is, 3)]), dist, &(x[WS(vs, 6) + WS(is, 1)]));
		    T3i = VSUB(T3g, T3h);
		    T3D = VADD(T3g, T3h);
	       }
	       T3j = VMUL(LDK(KP707106781), VSUB(T3f, T3i));
	       T3I = VADD(T3C, T3D);
	       T3r = VMUL(LDK(KP707106781), VADD(T3f, T3i));
	       T3E = VBYI(VSUB(T3C, T3D));
	  }
	  ST(&(x[0]), VADD(Tv, Tw), dist, &(x[0]));
	  ST(&(x[WS(is, 2)]), VADD(T1z, T1A), dist, &(x[0]));
	  ST(&(x[WS(is, 5)]), VADD(T3a, T3b), dist, &(x[WS(is, 1)]));
	  ST(&(x[WS(is, 7)]), VADD(T4e, T4f), dist, &(x[WS(is, 1)]));
	  ST(&(x[WS(is, 6)]), VADD(T3H, T3I), dist, &(x[0]));
	  ST(&(x[WS(is, 4)]), VADD(T2D, T2E), dist, &(x[0]));
	  {
	       V Tt, T4c, T2B, T24;
	       ST(&(x[WS(is, 3)]), VADD(T26, T27), dist, &(x[WS(is, 1)]));
	       ST(&(x[WS(is, 1)]), VADD(T12, T13), dist, &(x[WS(is, 1)]));
	       Tt = BYTW(&(W[TWVL * 10]), VSUB(Tp, Ts));
	       ST(&(x[WS(vs, 6)]), Tt, dist, &(x[WS(vs, 6)]));
	       T4c = BYTW(&(W[TWVL * 10]), VSUB(T48, T4b));
	       ST(&(x[WS(vs, 6) + WS(is, 7)]), T4c, dist, &(x[WS(vs, 6) + WS(is, 1)]));
	       T2B = BYTW(&(W[TWVL * 10]), VSUB(T2x, T2A));
	       ST(&(x[WS(vs, 6) + WS(is, 4)]), T2B, dist, &(x[WS(vs, 6)]));
	       T24 = BYTW(&(W[TWVL * 10]), VSUB(T20, T23));
	       ST(&(x[WS(vs, 6) + WS(is, 3)]), T24, dist, &(x[WS(vs, 6) + WS(is, 1)]));
	  }
	  {
	       V T10, T1x, T3F, T38, T1y, Tu;
	       T10 = BYTW(&(W[TWVL * 10]), VSUB(TW, TZ));
	       ST(&(x[WS(vs, 6) + WS(is, 1)]), T10, dist, &(x[WS(vs, 6) + WS(is, 1)]));
	       T1x = BYTW(&(W[TWVL * 10]), VSUB(T1t, T1w));
	       ST(&(x[WS(vs, 6) + WS(is, 2)]), T1x, dist, &(x[WS(vs, 6)]));
	       T3F = BYTW(&(W[TWVL * 10]), VSUB(T3B, T3E));
	       ST(&(x[WS(vs, 6) + WS(is, 6)]), T3F, dist, &(x[WS(vs, 6)]));
	       T38 = BYTW(&(W[TWVL * 10]), VSUB(T34, T37));
	       ST(&(x[WS(vs, 6) + WS(is, 5)]), T38, dist, &(x[WS(vs, 6) + WS(is, 1)]));
	       T1y = BYTW(&(W[TWVL * 2]), VADD(T1t, T1w));
	       ST(&(x[WS(vs, 2) + WS(is, 2)]), T1y, dist, &(x[WS(vs, 2)]));
	       Tu = BYTW(&(W[TWVL * 2]), VADD(Tp, Ts));
	       ST(&(x[WS(vs, 2)]), Tu, dist, &(x[WS(vs, 2)]));
	  }
	  {
	       V T2C, T3G, T11, T25, T39, T4d;
	       T2C = BYTW(&(W[TWVL * 2]), VADD(T2x, T2A));
	       ST(&(x[WS(vs, 2) + WS(is, 4)]), T2C, dist, &(x[WS(vs, 2)]));
	       T3G = BYTW(&(W[TWVL * 2]), VADD(T3B, T3E));
	       ST(&(x[WS(vs, 2) + WS(is, 6)]), T3G, dist, &(x[WS(vs, 2)]));
	       T11 = BYTW(&(W[TWVL * 2]), VADD(TW, TZ));
	       ST(&(x[WS(vs, 2) + WS(is, 1)]), T11, dist, &(x[WS(vs, 2) + WS(is, 1)]));
	       T25 = BYTW(&(W[TWVL * 2]), VADD(T20, T23));
	       ST(&(x[WS(vs, 2) + WS(is, 3)]), T25, dist, &(x[WS(vs, 2) + WS(is, 1)]));
	       T39 = BYTW(&(W[TWVL * 2]), VADD(T34, T37));
	       ST(&(x[WS(vs, 2) + WS(is, 5)]), T39, dist, &(x[WS(vs, 2) + WS(is, 1)]));
	       T4d = BYTW(&(W[TWVL * 2]), VADD(T48, T4b));
	       ST(&(x[WS(vs, 2) + WS(is, 7)]), T4d, dist, &(x[WS(vs, 2) + WS(is, 1)]));
	  }
	  {
	       V Tx, T1B, T3c, T4g, T3J, T2F;
	       Tx = BYTW(&(W[TWVL * 6]), VSUB(Tv, Tw));
	       ST(&(x[WS(vs, 4)]), Tx, dist, &(x[WS(vs, 4)]));
	       T1B = BYTW(&(W[TWVL * 6]), VSUB(T1z, T1A));
	       ST(&(x[WS(vs, 4) + WS(is, 2)]), T1B, dist, &(x[WS(vs, 4)]));
	       T3c = BYTW(&(W[TWVL * 6]), VSUB(T3a, T3b));
	       ST(&(x[WS(vs, 4) + WS(is, 5)]), T3c, dist, &(x[WS(vs, 4) + WS(is, 1)]));
	       T4g = BYTW(&(W[TWVL * 6]), VSUB(T4e, T4f));
	       ST(&(x[WS(vs, 4) + WS(is, 7)]), T4g, dist, &(x[WS(vs, 4) + WS(is, 1)]));
	       T3J = BYTW(&(W[TWVL * 6]), VSUB(T3H, T3I));
	       ST(&(x[WS(vs, 4) + WS(is, 6)]), T3J, dist, &(x[WS(vs, 4)]));
	       T2F = BYTW(&(W[TWVL * 6]), VSUB(T2D, T2E));
	       ST(&(x[WS(vs, 4) + WS(is, 4)]), T2F, dist, &(x[WS(vs, 4)]));
	  }
	  T28 = BYTW(&(W[TWVL * 6]), VSUB(T26, T27));
	  ST(&(x[WS(vs, 4) + WS(is, 3)]), T28, dist, &(x[WS(vs, 4) + WS(is, 1)]));
	  T14 = BYTW(&(W[TWVL * 6]), VSUB(T12, T13));
	  ST(&(x[WS(vs, 4) + WS(is, 1)]), T14, dist, &(x[WS(vs, 4) + WS(is, 1)]));
	  {
	       V Th, Ti, Tb, Tg;
	       Tb = VBYI(VSUB(T7, Ta));
	       Tg = VSUB(Te, Tf);
	       Th = BYTW(&(W[TWVL * 4]), VADD(Tb, Tg));
	       Ti = BYTW(&(W[TWVL * 8]), VSUB(Tg, Tb));
	       ST(&(x[WS(vs, 3)]), Th, dist, &(x[WS(vs, 3)]));
	       ST(&(x[WS(vs, 5)]), Ti, dist, &(x[WS(vs, 5)]));
	  }
	  {
	       V T40, T41, T3U, T3Z;
	       T3U = VBYI(VSUB(T3Q, T3T));
	       T3Z = VSUB(T3X, T3Y);
	       T40 = BYTW(&(W[TWVL * 4]), VADD(T3U, T3Z));
	       T41 = BYTW(&(W[TWVL * 8]), VSUB(T3Z, T3U));
	       ST(&(x[WS(vs, 3) + WS(is, 7)]), T40, dist, &(x[WS(vs, 3) + WS(is, 1)]));
	       ST(&(x[WS(vs, 5) + WS(is, 7)]), T41, dist, &(x[WS(vs, 5) + WS(is, 1)]));
	  }
	  {
	       V T2p, T2q, T2j, T2o;
	       T2j = VBYI(VSUB(T2f, T2i));
	       T2o = VSUB(T2m, T2n);
	       T2p = BYTW(&(W[TWVL * 4]), VADD(T2j, T2o));
	       T2q = BYTW(&(W[TWVL * 8]), VSUB(T2o, T2j));
	       ST(&(x[WS(vs, 3) + WS(is, 4)]), T2p, dist, &(x[WS(vs, 3)]));
	       ST(&(x[WS(vs, 5) + WS(is, 4)]), T2q, dist, &(x[WS(vs, 5)]));
	  }
	  {
	       V T1S, T1T, T1M, T1R;
	       T1M = VBYI(VSUB(T1I, T1L));
	       T1R = VSUB(T1P, T1Q);
	       T1S = BYTW(&(W[TWVL * 4]), VADD(T1M, T1R));
	       T1T = BYTW(&(W[TWVL * 8]), VSUB(T1R, T1M));
	       ST(&(x[WS(vs, 3) + WS(is, 3)]), T1S, dist, &(x[WS(vs, 3) + WS(is, 1)]));
	       ST(&(x[WS(vs, 5) + WS(is, 3)]), T1T, dist, &(x[WS(vs, 5) + WS(is, 1)]));
	  }
	  {
	       V TO, TP, TI, TN;
	       TI = VBYI(VSUB(TE, TH));
	       TN = VSUB(TL, TM);
	       TO = BYTW(&(W[TWVL * 4]), VADD(TI, TN));
	       TP = BYTW(&(W[TWVL * 8]), VSUB(TN, TI));
	       ST(&(x[WS(vs, 3) + WS(is, 1)]), TO, dist, &(x[WS(vs, 3) + WS(is, 1)]));
	       ST(&(x[WS(vs, 5) + WS(is, 1)]), TP, dist, &(x[WS(vs, 5) + WS(is, 1)]));
	  }
	  {
	       V T1l, T1m, T1f, T1k;
	       T1f = VBYI(VSUB(T1b, T1e));
	       T1k = VSUB(T1i, T1j);
	       T1l = BYTW(&(W[TWVL * 4]), VADD(T1f, T1k));
	       T1m = BYTW(&(W[TWVL * 8]), VSUB(T1k, T1f));
	       ST(&(x[WS(vs, 3) + WS(is, 2)]), T1l, dist, &(x[WS(vs, 3)]));
	       ST(&(x[WS(vs, 5) + WS(is, 2)]), T1m, dist, &(x[WS(vs, 5)]));
	  }
	  {
	       V T3t, T3u, T3n, T3s;
	       T3n = VBYI(VSUB(T3j, T3m));
	       T3s = VSUB(T3q, T3r);
	       T3t = BYTW(&(W[TWVL * 4]), VADD(T3n, T3s));
	       T3u = BYTW(&(W[TWVL * 8]), VSUB(T3s, T3n));
	       ST(&(x[WS(vs, 3) + WS(is, 6)]), T3t, dist, &(x[WS(vs, 3)]));
	       ST(&(x[WS(vs, 5) + WS(is, 6)]), T3u, dist, &(x[WS(vs, 5)]));
	  }
	  {
	       V T2W, T2X, T2Q, T2V;
	       T2Q = VBYI(VSUB(T2M, T2P));
	       T2V = VSUB(T2T, T2U);
	       T2W = BYTW(&(W[TWVL * 4]), VADD(T2Q, T2V));
	       T2X = BYTW(&(W[TWVL * 8]), VSUB(T2V, T2Q));
	       ST(&(x[WS(vs, 3) + WS(is, 5)]), T2W, dist, &(x[WS(vs, 3) + WS(is, 1)]));
	       ST(&(x[WS(vs, 5) + WS(is, 5)]), T2X, dist, &(x[WS(vs, 5) + WS(is, 1)]));
	  }
	  {
	       V T1p, T1q, T1n, T1o;
	       T1n = VBYI(VADD(T1e, T1b));
	       T1o = VADD(T1i, T1j);
	       T1p = BYTW(&(W[0]), VADD(T1n, T1o));
	       T1q = BYTW(&(W[TWVL * 12]), VSUB(T1o, T1n));
	       ST(&(x[WS(vs, 1) + WS(is, 2)]), T1p, dist, &(x[WS(vs, 1)]));
	       ST(&(x[WS(vs, 7) + WS(is, 2)]), T1q, dist, &(x[WS(vs, 7)]));
	  }
	  {
	       V Tl, Tm, Tj, Tk;
	       Tj = VBYI(VADD(Ta, T7));
	       Tk = VADD(Te, Tf);
	       Tl = BYTW(&(W[0]), VADD(Tj, Tk));
	       Tm = BYTW(&(W[TWVL * 12]), VSUB(Tk, Tj));
	       ST(&(x[WS(vs, 1)]), Tl, dist, &(x[WS(vs, 1)]));
	       ST(&(x[WS(vs, 7)]), Tm, dist, &(x[WS(vs, 7)]));
	  }
	  {
	       V T2t, T2u, T2r, T2s;
	       T2r = VBYI(VADD(T2i, T2f));
	       T2s = VADD(T2m, T2n);
	       T2t = BYTW(&(W[0]), VADD(T2r, T2s));
	       T2u = BYTW(&(W[TWVL * 12]), VSUB(T2s, T2r));
	       ST(&(x[WS(vs, 1) + WS(is, 4)]), T2t, dist, &(x[WS(vs, 1)]));
	       ST(&(x[WS(vs, 7) + WS(is, 4)]), T2u, dist, &(x[WS(vs, 7)]));
	  }
	  {
	       V T3x, T3y, T3v, T3w;
	       T3v = VBYI(VADD(T3m, T3j));
	       T3w = VADD(T3q, T3r);
	       T3x = BYTW(&(W[0]), VADD(T3v, T3w));
	       T3y = BYTW(&(W[TWVL * 12]), VSUB(T3w, T3v));
	       ST(&(x[WS(vs, 1) + WS(is, 6)]), T3x, dist, &(x[WS(vs, 1)]));
	       ST(&(x[WS(vs, 7) + WS(is, 6)]), T3y, dist, &(x[WS(vs, 7)]));
	  }
	  {
	       V TS, TT, TQ, TR;
	       TQ = VBYI(VADD(TH, TE));
	       TR = VADD(TL, TM);
	       TS = BYTW(&(W[0]), VADD(TQ, TR));
	       TT = BYTW(&(W[TWVL * 12]), VSUB(TR, TQ));
	       ST(&(x[WS(vs, 1) + WS(is, 1)]), TS, dist, &(x[WS(vs, 1) + WS(is, 1)]));
	       ST(&(x[WS(vs, 7) + WS(is, 1)]), TT, dist, &(x[WS(vs, 7) + WS(is, 1)]));
	  }
	  {
	       V T1W, T1X, T1U, T1V;
	       T1U = VBYI(VADD(T1L, T1I));
	       T1V = VADD(T1P, T1Q);
	       T1W = BYTW(&(W[0]), VADD(T1U, T1V));
	       T1X = BYTW(&(W[TWVL * 12]), VSUB(T1V, T1U));
	       ST(&(x[WS(vs, 1) + WS(is, 3)]), T1W, dist, &(x[WS(vs, 1) + WS(is, 1)]));
	       ST(&(x[WS(vs, 7) + WS(is, 3)]), T1X, dist, &(x[WS(vs, 7) + WS(is, 1)]));
	  }
	  {
	       V T30, T31, T2Y, T2Z;
	       T2Y = VBYI(VADD(T2P, T2M));
	       T2Z = VADD(T2T, T2U);
	       T30 = BYTW(&(W[0]), VADD(T2Y, T2Z));
	       T31 = BYTW(&(W[TWVL * 12]), VSUB(T2Z, T2Y));
	       ST(&(x[WS(vs, 1) + WS(is, 5)]), T30, dist, &(x[WS(vs, 1) + WS(is, 1)]));
	       ST(&(x[WS(vs, 7) + WS(is, 5)]), T31, dist, &(x[WS(vs, 7) + WS(is, 1)]));
	  }
	  {
	       V T44, T45, T42, T43;
	       T42 = VBYI(VADD(T3T, T3Q));
	       T43 = VADD(T3X, T3Y);
	       T44 = BYTW(&(W[0]), VADD(T42, T43));
	       T45 = BYTW(&(W[TWVL * 12]), VSUB(T43, T42));
	       ST(&(x[WS(vs, 1) + WS(is, 7)]), T44, dist, &(x[WS(vs, 1) + WS(is, 1)]));
	       ST(&(x[WS(vs, 7) + WS(is, 7)]), T45, dist, &(x[WS(vs, 7) + WS(is, 1)]));
	  }
     }
     END_SIMD();
     return W;
}

static const tw_instr twinstr[] = {
     VTW(1),
     VTW(2),
     VTW(3),
     VTW(4),
     VTW(5),
     VTW(6),
     VTW(7),
     {TW_NEXT, VL, 0}
};

static const ct_desc desc = { 8, "q1bv_8", twinstr, {264, 128, 0, 0}, &GENUS, 0, 0, 0 };

void X(codelet_q1bv_8) (planner *p) {
     X(kdft_difsq_register) (p, q1bv_8, &desc);
}
