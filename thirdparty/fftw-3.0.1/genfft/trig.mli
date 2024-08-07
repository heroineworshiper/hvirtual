(*
 * Copyright (c) 1997-1999 Massachusetts Institute of Technology
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
 *)
(* $Id: trig.mli,v 1.3 2003/03/15 20:29:42 stevenj Exp $ *)

val rdft : int -> int -> Complex.signal -> Complex.signal
val hdft : int -> int -> Complex.signal -> Complex.signal
val dft_via_rdft : int -> int -> Complex.signal -> Complex.signal
val dht : int -> int -> Complex.signal -> Complex.signal

val dctI : int -> Complex.signal -> Complex.signal
val dctII : int -> Complex.signal -> Complex.signal
val dctIII : int -> Complex.signal -> Complex.signal
val dctIV : int -> Complex.signal -> Complex.signal

val dstI : int -> Complex.signal -> Complex.signal
val dstII : int -> Complex.signal -> Complex.signal
val dstIII : int -> Complex.signal -> Complex.signal
val dstIV : int -> Complex.signal -> Complex.signal
