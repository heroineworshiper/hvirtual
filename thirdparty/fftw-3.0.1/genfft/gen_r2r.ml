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
(* $Id: gen_r2r.ml,v 1.3 2003/04/17 19:25:50 athena Exp $ *)

(* generation of trigonometric transforms *)

open Util
open Genutil
open C

let cvsid = "$Id: gen_r2r.ml,v 1.3 2003/04/17 19:25:50 athena Exp $"

let usage = "Usage: " ^ Sys.argv.(0) ^ " -n <number>"

let uistride = ref Stride_variable
let uostride = ref Stride_variable
let uivstride = ref Stride_variable
let uovstride = ref Stride_variable

type mode =
  | RDFT
  | HDFT
  | DHT
  | REDFT00
  | REDFT10
  | REDFT01
  | REDFT11
  | RODFT00
  | RODFT10
  | RODFT01
  | RODFT11
  | NONE

let mode = ref NONE

let speclist = [
  "-with-istride",
  Arg.String(fun x -> uistride := arg_to_stride x),
  " specialize for given input stride";

  "-with-ostride",
  Arg.String(fun x -> uostride := arg_to_stride x),
  " specialize for given output stride";

  "-with-ivstride",
  Arg.String(fun x -> uivstride := arg_to_stride x),
  " specialize for given input vector stride";

  "-with-ovstride",
  Arg.String(fun x -> uovstride := arg_to_stride x),
  " specialize for given output vector stride";

  "-rdft",
  Arg.Unit(fun () -> mode := RDFT),
  " generate a real DFT codelet";

  "-hdft",
  Arg.Unit(fun () -> mode := HDFT),
  " generate a Hermitian DFT codelet";

  "-dht",
  Arg.Unit(fun () -> mode := DHT),
  " generate a DHT codelet";

  "-redft00",
  Arg.Unit(fun () -> mode := REDFT00),
  " generate a DCT-I codelet";

  "-redft10",
  Arg.Unit(fun () -> mode := REDFT10),
  " generate a DCT-II codelet";

  "-redft01",
  Arg.Unit(fun () -> mode := REDFT01),
  " generate a DCT-III codelet";

  "-redft11",
  Arg.Unit(fun () -> mode := REDFT11),
  " generate a DCT-IV codelet";

  "-rodft00",
  Arg.Unit(fun () -> mode := RODFT00),
  " generate a DST-I codelet";

  "-rodft10",
  Arg.Unit(fun () -> mode := RODFT10),
  " generate a DST-II codelet";

  "-rodft01",
  Arg.Unit(fun () -> mode := RODFT01),
  " generate a DST-III codelet";

  "-rodft11",
  Arg.Unit(fun () -> mode := RODFT11),
  " generate a DST-IV codelet";
]

let generate n mode =
  let iarray = "I"
  and oarray = "O"
  and istride = "istride"
  and ostride = "ostride" in

  let ns = string_of_int n
  and sign = !Genutil.sign 
  and name = !Magic.codelet_name in
  let name0 = name ^ "_0" in

  let vistride = either_stride (!uistride) (C.SVar istride)
  and vostride = either_stride (!uostride) (C.SVar ostride)
  in

  let _ = Simd.ovs := stride_to_string "ovs" !uovstride in
  let _ = Simd.ivs := stride_to_string "ivs" !uivstride in

  let (transform, load_input, store_output) = match mode with
  | RDFT -> Trig.rdft sign, load_array_r, store_array_hc
  | HDFT -> Trig.hdft sign, load_array_c, store_array_r  (* TODO *)
  | DHT -> Trig.dht 1, load_array_r, store_array_r
  | REDFT00 -> Trig.dctI, load_array_r, store_array_r
  | REDFT10 -> Trig.dctII, load_array_r, store_array_r
  | REDFT01 -> Trig.dctIII, load_array_r, store_array_r
  | REDFT11 -> Trig.dctIV, load_array_r, store_array_r
  | RODFT00 -> Trig.dstI, load_array_r, store_array_r
  | RODFT10 -> Trig.dstII, load_array_r, store_array_r
  | RODFT01 -> Trig.dstIII, load_array_r, store_array_r
  | RODFT11 -> Trig.dstIV, load_array_r, store_array_r
  | _ -> failwith "must specify transform kind"
  in
    
  let locations = unique_array_c n in
  let input = 
    locative_array_c n 
      (C.array_subscript iarray vistride)
      (C.array_subscript "BUG" vistride)
      locations in
  let output = transform n (load_array_c n input) in
  let oloc = 
    locative_array_c n 
      (C.array_subscript oarray vostride)
      (C.array_subscript "BUG" vostride)
      locations in
  let odag = store_output n oloc output in
  let annot = standard_optimizer odag in

  let tree0 =
    Fcn ("static void", name0,
	 ([Decl (C.constrealtypep, iarray);
	   Decl (C.realtypep, oarray)]
	  @ (if stride_fixed !uistride then [] 
               else [Decl (C.stridetype, istride)])
	  @ (if stride_fixed !uostride then [] 
	       else [Decl (C.stridetype, ostride)])
	  @ (choose_simd []
	       (if stride_fixed !uivstride then [] else 
	       [Decl ("int", !Simd.ivs)]))
	  @ (choose_simd []
	       (if stride_fixed !uovstride then [] else 
	       [Decl ("int", !Simd.ovs)]))
	 ),
	 add_constants (Asch annot))

  in let loop =
    "static void " ^ name ^
    "(const " ^ C.realtype ^ " *I, " ^ 
    C.realtype ^ " *O, " ^
    C.stridetype ^ " is, " ^ 
    C.stridetype ^ " os, " ^ 
      " int v, int ivs, int ovs)\n" ^
    "{\n" ^
    "int i;\n" ^
    "for (i = v; i > 0; --i) {\n" ^
      name0 ^ "(I, O" ^
       (if stride_fixed !uistride then "" else ", is") ^ 
       (if stride_fixed !uostride then "" else ", os") ^ 
       (choose_simd ""
	  (if stride_fixed !uivstride then "" else ", ivs")) ^ 
       (choose_simd ""
	  (if stride_fixed !uovstride then "" else ", ovs")) ^ 
    ");\n" ^
    "I += ivs; O += ovs;\n" ^
    "}\n}\n\n"

  and desc = 
    Printf.sprintf 
      "static const kr2r_desc desc = { %d, \"%s\", %s, &GENUS, %s, %s, %s, %s, %s };\n\n"
      n name (flops_of tree0) 
      (match mode with
      | RDFT -> "RDFT00"
      | HDFT -> "HDFT00"
      | DHT  -> "DHT"
      | REDFT00 -> "REDFT00"
      | REDFT10 -> "REDFT10"
      | REDFT01 -> "REDFT01"
      | REDFT11 -> "REDFT11"
      | RODFT00 -> "RODFT00"
      | RODFT10 -> "RODFT10"
      | RODFT01 -> "RODFT01"
      | RODFT11 -> "RODFT11"
      | _ -> failwith "must specify a transform kind")
      (stride_to_solverparm !uistride) 
      (stride_to_solverparm !uostride)
      (choose_simd "0" (stride_to_solverparm !uivstride))
      (choose_simd "0" (stride_to_solverparm !uovstride))

  and init =
    (declare_register_fcn name) ^
    "{" ^
    "  X(kr2r_register)(p, " ^ name ^ ", &desc);\n" ^
    "}\n"

  in
  (unparse cvsid tree0) ^ "\n" ^ loop ^ desc ^ init


let main () =
  begin
    parse speclist usage;
    print_string (generate (check_size ()) !mode);
  end

let _ = main()
