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
(* $Id: c.mli,v 1.8 2003/03/15 20:29:42 stevenj Exp $ *)

type stride = 
  | SVar of string
  | SConst of string
  | SInteger of int
  | SNeg of stride
val array_subscript : string -> stride -> int -> string
val varray_subscript : string -> stride -> stride -> int -> int -> string

val real_of : string -> string
val imag_of : string -> string

val realtype : string
val realtypep : string
val constrealtype : string
val constrealtypep : string
val stridetype : string

type c_decl = 
  | Decl of string * string
  | Idecl of string * string * string (* decl with initializer *)
  | Adecl of string * string * int (* array declaration *)
  | Tdecl of string                (* arbitrary text declaration *)

and c_ast =
  | Asch of Annotate.annotated_schedule
  | Return of c_ast
  | For of c_ast * c_ast * c_ast * c_ast
  | If of c_ast * c_ast
  | Block of (c_decl list) * (c_ast list)
  | Binop of string * c_ast * c_ast
  | Expr_assign of c_ast * c_ast
  | Stmt_assign of c_ast * c_ast
  | Comma of c_ast * c_ast
  | Integer of int
  | CVar of string
  | CPlus of c_ast list
  | CTimes of c_ast * c_ast
  | CUminus of c_ast
and c_fcn = | Fcn of string * string * c_decl list * c_ast

val unparse_expr : Expr.expr -> string
val unparse_assignment : Expr.assignment -> string
val unparse_annotated : bool -> Annotate.annotated_schedule -> string
val unparse_decl : c_decl -> string
val unparse_ast : c_ast -> string
val unparse_function : c_fcn -> string

val flops_of : c_fcn -> string
val print_cost : c_fcn -> string

val ast_to_expr_list : c_ast -> Expr.expr list
val extract_constants : c_ast -> c_decl list
val ctimes : (c_ast * c_ast) -> c_ast
