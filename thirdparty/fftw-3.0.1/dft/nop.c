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

/* $Id: nop.c,v 1.15 2003/03/15 20:29:42 stevenj Exp $ */

/* plans for vrank -infty DFTs (nothing to do) */

#include "dft.h"

static void apply(const plan *ego_, R *ri, R *ii, R *ro, R *io)
{
     UNUSED(ego_);
     UNUSED(ri);
     UNUSED(ii);
     UNUSED(ro);
     UNUSED(io);
}

static int applicable(const solver *ego_, const problem *p_)
{
     UNUSED(ego_);
     if (DFTP(p_)) {
          const problem_dft *p = (const problem_dft *) p_;
          return 0
	       /* case 1 : -infty vector rank */
	       || (!FINITE_RNK(p->vecsz->rnk))

	       /* case 2 : rank-0 in-place dft */
	       || (1
		   && p->sz->rnk == 0
		   && FINITE_RNK(p->vecsz->rnk)
		   && p->ro == p->ri
		   && X(tensor_inplace_strides)(p->vecsz)
                    );
     }
     return 0;
}

static void print(const plan *ego, printer *p)
{
     UNUSED(ego);
     p->print(p, "(dft-nop)");
}

static plan *mkplan(const solver *ego, const problem *p, planner *plnr)
{
     static const plan_adt padt = {
	  X(dft_solve), X(null_awake), print, X(plan_null_destroy)
     };
     plan_dft *pln;

     UNUSED(plnr);

     if (!applicable(ego, p))
          return (plan *) 0;
     pln = MKPLAN_DFT(plan_dft, &padt, apply);
     X(ops_zero)(&pln->super.ops);

     return &(pln->super);
}

static solver *mksolver(void)
{
     static const solver_adt sadt = { mkplan };
     return MKSOLVER(solver, &sadt);
}

void X(dft_nop_register)(planner *p)
{
     REGISTER_SOLVER(p, mksolver());
}
