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
/* Generated on Sat Jul  5 21:56:41 EDT 2003 */

#include "codelet-rdft.h"

/* Generated by: /homee/stevenj/cvs/fftw3.0.1/genfft/gen_r2hc -compact -variables 4 -n 11 -name r2hc_11 -include r2hc.h */

/*
 * This function contains 60 FP additions, 50 FP multiplications,
 * (or, 20 additions, 10 multiplications, 40 fused multiply/add),
 * 28 stack variables, and 22 memory accesses
 */
/*
 * Generator Id's : 
 * $Id: algsimp.ml,v 1.7 2003/03/15 20:29:42 stevenj Exp $
 * $Id: fft.ml,v 1.2 2003/03/15 20:29:42 stevenj Exp $
 * $Id: gen_r2hc.ml,v 1.13 2003/04/17 19:25:50 athena Exp $
 */

#include "r2hc.h"

static void r2hc_11(const R *I, R *ro, R *io, stride is, stride ros, stride ios, int v, int ivs, int ovs)
{
     DK(KP654860733, +0.654860733945285064056925072466293553183791199);
     DK(KP142314838, +0.142314838273285140443792668616369668791051361);
     DK(KP959492973, +0.959492973614497389890368057066327699062454848);
     DK(KP415415013, +0.415415013001886425529274149229623203524004910);
     DK(KP841253532, +0.841253532831181168861811648919367717513292498);
     DK(KP989821441, +0.989821441880932732376092037776718787376519372);
     DK(KP909631995, +0.909631995354518371411715383079028460060241051);
     DK(KP281732556, +0.281732556841429697711417915346616899035777899);
     DK(KP540640817, +0.540640817455597582107635954318691695431770608);
     DK(KP755749574, +0.755749574354258283774035843972344420179717445);
     int i;
     for (i = v; i > 0; i = i - 1, I = I + ivs, ro = ro + ovs, io = io + ovs) {
	  E T1, T4, Tl, Tg, Th, Td, Ti, Ta, Tk, T7, Tj, Tb, Tc;
	  T1 = I[0];
	  {
	       E T2, T3, Te, Tf;
	       T2 = I[WS(is, 2)];
	       T3 = I[WS(is, 9)];
	       T4 = T2 + T3;
	       Tl = T3 - T2;
	       Te = I[WS(is, 1)];
	       Tf = I[WS(is, 10)];
	       Tg = Te + Tf;
	       Th = Tf - Te;
	  }
	  Tb = I[WS(is, 3)];
	  Tc = I[WS(is, 8)];
	  Td = Tb + Tc;
	  Ti = Tc - Tb;
	  {
	       E T8, T9, T5, T6;
	       T8 = I[WS(is, 5)];
	       T9 = I[WS(is, 6)];
	       Ta = T8 + T9;
	       Tk = T9 - T8;
	       T5 = I[WS(is, 4)];
	       T6 = I[WS(is, 7)];
	       T7 = T5 + T6;
	       Tj = T6 - T5;
	  }
	  io[WS(ios, 4)] = FMA(KP755749574, Th, KP540640817 * Ti) + FNMS(KP909631995, Tk, KP281732556 * Tj) - (KP989821441 * Tl);
	  ro[WS(ros, 4)] = FMA(KP841253532, Td, T1) + FNMS(KP959492973, T7, KP415415013 * Ta) + FNMA(KP142314838, T4, KP654860733 * Tg);
	  io[WS(ios, 2)] = FMA(KP909631995, Th, KP755749574 * Tl) + FNMA(KP540640817, Tk, KP989821441 * Tj) - (KP281732556 * Ti);
	  io[WS(ios, 5)] = FMA(KP281732556, Th, KP755749574 * Ti) + FNMS(KP909631995, Tj, KP989821441 * Tk) - (KP540640817 * Tl);
	  io[WS(ios, 1)] = FMA(KP540640817, Th, KP909631995 * Tl) + FMA(KP989821441, Ti, KP755749574 * Tj) + (KP281732556 * Tk);
	  io[WS(ios, 3)] = FMA(KP989821441, Th, KP540640817 * Tj) + FNMS(KP909631995, Ti, KP755749574 * Tk) - (KP281732556 * Tl);
	  ro[WS(ros, 3)] = FMA(KP415415013, Td, T1) + FNMS(KP654860733, Ta, KP841253532 * T7) + FNMA(KP959492973, T4, KP142314838 * Tg);
	  ro[WS(ros, 1)] = FMA(KP841253532, Tg, T1) + FNMS(KP959492973, Ta, KP415415013 * T4) + FNMA(KP654860733, T7, KP142314838 * Td);
	  ro[0] = T1 + Tg + T4 + Td + T7 + Ta;
	  ro[WS(ros, 2)] = FMA(KP415415013, Tg, T1) + FNMS(KP142314838, T7, KP841253532 * Ta) + FNMA(KP959492973, Td, KP654860733 * T4);
	  ro[WS(ros, 5)] = FMA(KP841253532, T4, T1) + FNMS(KP142314838, Ta, KP415415013 * T7) + FNMA(KP654860733, Td, KP959492973 * Tg);
     }
}

static const kr2hc_desc desc = { 11, "r2hc_11", {20, 10, 40, 0}, &GENUS, 0, 0, 0, 0, 0 };

void X(codelet_r2hc_11) (planner *p) {
     X(kr2hc_register) (p, r2hc_11, &desc);
}
