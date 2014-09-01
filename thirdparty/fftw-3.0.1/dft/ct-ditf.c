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

/* $Id: ct-ditf.c,v 1.20 2003/03/15 20:29:42 stevenj Exp $ */

/* decimation in time Cooley-Tukey */
#include "dft.h"
#include "ct.h"

static void apply(const plan *ego_, R *ri, R *ii, R *ro, R *io)
{
     const plan_ct *ego = (const plan_ct *) ego_;
     plan *cld0 = ego->cld;
     plan_dft *cld = (plan_dft *) cld0;

     UNUSED(ro);  /* == ri */
     UNUSED(io);  /* == ii */
     ego->k.difsq(ri, ii, ego->td->W, ego->ios, ego->vs, ego->m, ego->is);

     /* two-dimensional r x vl sub-transform: */
     cld->apply(cld0, ri, ii, ri, ii);
}

static int applicable(const solver_ct *ego, const problem *p_,
		      const planner *plnr)
{
     UNUSED(plnr);
     if (X(dft_ct_applicable)(ego, p_)) {
          const ct_desc *e = ego->desc;
          const problem_dft *p = (const problem_dft *) p_;
          iodim *d = p->sz->dims, *vd = p->vecsz->dims;
	  int m = d[0].n / e->radix;

          return (1
                  && p->ri == p->ro  /* inplace only */
                  && p->vecsz->rnk == 1
                  && vd[0].n == e->radix
                  && d[0].os == vd[0].is
                  && d[0].is == (int)e->radix * vd[0].is
                  && vd[0].os == (int)d[0].n * vd[0].is

		  && (e->genus->okp(e, p->ri, p->ii, 
				    vd[0].os, vd[0].is, m, d[0].is, plnr))
	       );
     }
     return 0;
}

static void finish(plan_ct *ego)
{
     const ct_desc *d = ego->slv->desc;
     ego->ios = X(mkstride)(ego->r, ego->ovs);
     ego->vs = X(mkstride)(ego->r, ego->ivs);
     X(ops_madd)(ego->m / d->genus->vl, &ego->slv->desc->ops,
		 &ego->cld->ops, &ego->super.super.ops);
}

static problem *mkcld(const solver_ct *ego, const problem_dft *p)
{
     iodim *d = p->sz->dims;
     iodim *vd = p->vecsz->dims;
     const ct_desc *e = ego->desc;

     return X(mkproblem_dft_d)(
	  X(mktensor_1d)(d[0].n / e->radix, d[0].is, d[0].is),
	  X(mktensor_2d)(vd[0].n, vd[0].os, vd[0].os,
			 e->radix, vd[0].is,vd[0].is),
	  p->ro, p->io, p->ro, p->io);
}

static plan *mkplan(const solver *ego, const problem *p, planner *plnr)
{
     static const ctadt adt = {
	  sizeof(plan_ct), mkcld, finish, applicable, apply
     };
     return X(mkplan_dft_ct)((const solver_ct *) ego, p, plnr, &adt);
}


solver *X(mksolver_dft_ct_ditf)(kdft_difsq codelet, const ct_desc *desc)
{
     static const solver_adt sadt = { mkplan };
     static const char name[] = "dft-ditf";
     union kct k;
     k.difsq = codelet;

     return X(mksolver_dft_ct)(k, desc, name, &sadt);
}
