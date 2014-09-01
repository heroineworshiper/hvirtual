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

/* $Id: ct-dif.c,v 1.30 2003/03/15 20:29:42 stevenj Exp $ */

/* decimation in time Cooley-Tukey */
#include "dft.h"
#include "ct.h"

static void apply(const plan *ego_, R *ri, R *ii, R *ro, R *io)
{
     const plan_ct *ego = (const plan_ct *) ego_;

     {
          int i, m = ego->m, vl = ego->vl;
          int is = ego->is, ivs = ego->ivs;

          for (i = 0; i < vl; ++i)
               ego->k.dif(ri + i * ivs, ii + i * ivs, ego->td->W,
			  ego->ios, m, is);
     }

     /* two-dimensional r x vl sub-transform: */
     {
	  plan *cld0 = ego->cld;
	  plan_dft *cld = (plan_dft *) cld0;
	  cld->apply(cld0, ri, ii, ro, io);
     }
}

static int applicable0(const solver_ct *ego, const problem *p_,
		       const planner *plnr)
{
     if (X(dft_ct_applicable)(ego, p_)) {
	  int ivs, ovs;
	  int vl;
          const ct_desc *e = ego->desc;
          const problem_dft *p = (const problem_dft *) p_;
          iodim *d = p->sz->dims;
	  int m = d[0].n / e->radix;
	  X(tensor_tornk1)(p->vecsz, &vl, &ivs, &ovs);
          return (1
                  /* DIF destroys the input and we don't like it */
                  && (p->ri == p->ro || DESTROY_INPUTP(plnr))

		  && (e->genus->okp(e, p->ri, p->ii,
				    (int)m * d[0].is, 0, m, d[0].is, plnr))
		  && (e->genus->okp(e, p->ri + ivs, p->ii + ivs,
				    (int)m * d[0].is, 0, m, d[0].is, plnr))
	       );
     }
     return 0;
}

static int applicable(const solver_ct *ego, const problem *p_,
		 const planner *plnr)
{
     const problem_dft *p;
     
     if (!applicable0(ego, p_, plnr))  return 0;

     p = (const problem_dft *) p_;

     /* emulate fftw2 behavior */
     if (NO_VRECURSEP(plnr) && (p->vecsz->rnk > 0)) return 0;

     if (NO_UGLYP(plnr) && X(ct_uglyp)(16, p->sz->dims[0].n, ego->desc->radix))
	  return 0;

     return 1;
}

static void finish(plan_ct *ego)
{
     const ct_desc *d = ego->slv->desc;
     ego->ios = X(mkstride)(ego->r, ego->m * ego->is);
     X(ops_madd)(ego->vl * ego->m / d->genus->vl, &d->ops, &ego->cld->ops,
		 &ego->super.super.ops);
}

static plan *mkplan(const solver *ego, const problem *p, planner *plnr)
{
     static const ctadt adt = {
	  sizeof(plan_ct), X(dft_mkcld_dif), finish, applicable, apply
     };
     return X(mkplan_dft_ct)((const solver_ct *) ego, p, plnr, &adt);
}


solver *X(mksolver_dft_ct_dif)(kdft_dif codelet, const ct_desc *desc)
{
     static const solver_adt sadt = { mkplan };
     static const char name[] = "dft-dif";
     union kct k;
     k.dif = codelet;

     return X(mksolver_dft_ct)(k, desc, name, &sadt);
}
