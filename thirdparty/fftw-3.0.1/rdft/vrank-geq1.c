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

/* $Id: vrank-geq1.c,v 1.30 2003/04/15 19:03:20 stevenj Exp $ */


/* Plans for handling vector transform loops.  These are *just* the
   loops, and rely on child plans for the actual RDFTs.
 
   They form a wrapper around solvers that don't have apply functions
   for non-null vectors.
 
   vrank-geq1 plans also recursively handle the case of multi-dimensional
   vectors, obviating the need for most solvers to deal with this.  We
   can also play games here, such as reordering the vector loops.
 
   Each vrank-geq1 plan reduces the vector rank by 1, picking out a
   dimension determined by the vecloop_dim field of the solver. */

#include "rdft.h"

typedef struct {
     solver super;
     int vecloop_dim;
     const int *buddies;
     int nbuddies;
} S;

typedef struct {
     plan_rdft super;

     plan *cld;
     int vl;
     int ivs, ovs;
     const S *solver;
} P;

static void apply(const plan *ego_, R *I, R *O)
{
     const P *ego = (const P *) ego_;
     int i, vl = ego->vl;
     int ivs = ego->ivs, ovs = ego->ovs;
     rdftapply cldapply = ((plan_rdft *) ego->cld)->apply;

     for (i = 0; i < vl; ++i) {
          cldapply(ego->cld, I + i * ivs, O + i * ovs);
     }
}

static void awake(plan *ego_, int flg)
{
     P *ego = (P *) ego_;
     AWAKE(ego->cld, flg);
}

static void destroy(plan *ego_)
{
     P *ego = (P *) ego_;
     X(plan_destroy_internal)(ego->cld);
}

static void print(const plan *ego_, printer *p)
{
     const P *ego = (const P *) ego_;
     const S *s = ego->solver;
     p->print(p, "(rdft-vrank>=1-x%d/%d%(%p%))",
	      ego->vl, s->vecloop_dim, ego->cld);
}

static int pickdim(const S *ego, const tensor *vecsz, int oop, int *dp)
{
     return X(pickdim)(ego->vecloop_dim, ego->buddies, ego->nbuddies,
		       vecsz, oop, dp);
}

static int applicable0(const solver *ego_, const problem *p_, int *dp)
{
     if (RDFTP(p_)) {
          const S *ego = (const S *) ego_;
          const problem_rdft *p = (const problem_rdft *) p_;

          return (1
                  && FINITE_RNK(p->vecsz->rnk)
                  && p->vecsz->rnk > 0
                  && pickdim(ego, p->vecsz, p->I != p->O, dp)
	       );
     }

     return 0;
}

static int applicable(const solver *ego_, const problem *p_, 
		      const planner *plnr, int *dp)
{
     const S *ego = (const S *)ego_;
     const problem_rdft *p;

     if (!applicable0(ego_, p_, dp)) return 0;

     /* fftw2 behavior */
     if (NO_VRANK_SPLITSP(plnr) && (ego->vecloop_dim != ego->buddies[0]))
	  return 0;

     if (NO_UGLYP(plnr)) {
	  p = (const problem_rdft *) p_;

	  /* Heuristic: if the transform is multi-dimensional, and the
	     vector stride is less than the transform size, then we
	     probably want to use a rank>=2 plan first in order to combine
	     this vector with the transform-dimension vectors. */
	  {
	       iodim *d = p->vecsz->dims + *dp;
	       if (1
		   && p->sz->rnk > 1 
		   && X(imin)(X(iabs)(d->is), X(iabs)(d->os))
		   < X(tensor_max_index)(p->sz)
		    )
		    return 0;
	  }

	  /* Heuristic: don't use a vrank-geq1 for rank-0 vrank-1
	     transforms, since this case is better handled by rank-0
	     solvers. */
	  if (p->sz->rnk == 0 && p->vecsz->rnk == 1) return 0;

	  /* prefer threaded version */
	  if (NONTHREADED_ICKYP(plnr)) return 0;

	  /* exploit built-in vecloops of (ugly) r{e,o}dft solvers */
	  if (p->vecsz->rnk == 1 && p->sz->rnk == 1 
	      && REODFT_KINDP(p->kind[0]))
	       return 0;
     }

     return 1;
}

static plan *mkplan(const solver *ego_, const problem *p_, planner *plnr)
{
     const S *ego = (const S *) ego_;
     const problem_rdft *p;
     P *pln;
     plan *cld;
     int vdim;
     iodim *d;

     static const plan_adt padt = {
	  X(rdft_solve), awake, print, destroy
     };

     if (!applicable(ego_, p_, plnr, &vdim))
          return (plan *) 0;
     p = (const problem_rdft *) p_;

     d = p->vecsz->dims + vdim;

     A(d->n > 1); 

     cld = X(mkplan_d)(plnr, 
		       X(mkproblem_rdft_d)(
			    X(tensor_copy)(p->sz),
			    X(tensor_copy_except)(p->vecsz, vdim),
			    TAINT(p->I, d->is), TAINT(p->O, d->os),
			    p->kind));
     if (!cld) return (plan *) 0;

     pln = MKPLAN_RDFT(P, &padt, apply);

     pln->cld = cld;
     pln->vl = d->n;
     pln->ivs = d->is;
     pln->ovs = d->os;

     pln->solver = ego;
     X(ops_zero)(&pln->super.super.ops);
     pln->super.super.ops.other = 3.14159; /* magic to prefer codelet loops */
     X(ops_madd2)(pln->vl, &cld->ops, &pln->super.super.ops);
     pln->super.super.pcost = pln->vl * cld->pcost;

     return &(pln->super.super);
}

static solver *mksolver(int vecloop_dim, const int *buddies, int nbuddies)
{
     static const solver_adt sadt = { mkplan };
     S *slv = MKSOLVER(S, &sadt);
     slv->vecloop_dim = vecloop_dim;
     slv->buddies = buddies;
     slv->nbuddies = nbuddies;
     return &(slv->super);
}

void X(rdft_vrank_geq1_register)(planner *p)
{
     int i;

     /* FIXME: Should we try other vecloop_dim values? */
     static const int buddies[] = { 1, -1 };

     const int nbuddies = sizeof(buddies) / sizeof(buddies[0]);

     for (i = 0; i < nbuddies; ++i)
          REGISTER_SOLVER(p, mksolver(buddies[i], buddies, nbuddies));
}
