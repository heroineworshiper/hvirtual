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
(* $Id: variable.mli,v 1.5 2003/03/15 20:29:42 stevenj Exp $ *)

type variable

val hash : variable -> int
val same : variable -> variable -> bool
val is_constant : variable -> bool
val is_temporary : variable -> bool
val is_locative : variable -> bool
val same_location : variable -> variable -> bool
val same_class : variable -> variable -> bool
val make_temporary : unit -> variable
val make_constant : Unique.unique -> string -> variable
val make_locative :
    Unique.unique -> Unique.unique -> (int -> string) -> int -> variable
val unparse : variable -> string
val unparse_for_alignment : int -> variable -> string
