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

/* $Id: rank-geq2-rdft2.c,v 1.20 2003/03/15 20:29:43 stevenj Exp $ */

/* plans for RDFT2 of rank >= 2 (multidimensional) */

#include "rdft.h"
#include "dft.h"

typedef struct {
     solver super;
     int spltrnk;
     const int *buddies;
     int nbuddies;
} S;

typedef struct {
     plan_dft super;
     plan *cldr, *cldc;
     const S *solver;
} P;

static void apply_r2hc(const plan *ego_, R *r, R *rio, R *iio)
{
     const P *ego = (const P *) ego_;

     {
	  plan_rdft2 *cldr = (plan_rdft2 *) ego->cldr;
	  cldr->apply((plan *) cldr, r, rio, iio);
     }
     
     {
	  plan_dft *cldc = (plan_dft *) ego->cldc;
	  cldc->apply((plan *) cldc, rio, iio, rio, iio);
     }
}

static void apply_hc2r(const plan *ego_, R *r, R *rio, R *iio)
{
     const P *ego = (const P *) ego_;

     {
	  plan_dft *cldc = (plan_dft *) ego->cldc;
	  cldc->apply((plan *) cldc, iio, rio, iio, rio);
     }

     {
	  plan_rdft2 *cldr = (plan_rdft2 *) ego->cldr;
	  cldr->apply((plan *) cldr, r, rio, iio);
     }
     
}

static void awake(plan *ego_, int flg)
{
     P *ego = (P *) ego_;
     AWAKE(ego->cldr, flg);
     AWAKE(ego->cldc, flg);
}

static void destroy(plan *ego_)
{
     P *ego = (P *) ego_;
     X(plan_destroy_internal)(ego->cldr);
     X(plan_destroy_internal)(ego->cldc);
}

static void print(const plan *ego_, printer *p)
{
     const P *ego = (const P *) ego_;
     const S *s = ego->solver;
     p->print(p, "(rdft2-rank>=2/%d%(%p%)%(%p%))", 
	      s->spltrnk, ego->cldr, ego->cldc);
}
 
static int picksplit(const S *ego, const tensor *sz, int *rp)
{
     A(sz->rnk > 1); /* cannot split rnk <= 1 */
     if (!X(pickdim)(ego->spltrnk, ego->buddies, ego->nbuddies, sz, 1, rp))
          return 0;
     *rp += 1; /* convert from dim. index to rank */
     if (*rp >= sz->rnk) /* split must reduce rank */
          return 0;
     return 1;
}

static int applicable0(const solver *ego_, const problem *p_, int *rp,
		       const planner *plnr)
{
     if (RDFT2P(p_)) {
          const problem_rdft2 *p = (const problem_rdft2 *) p_;
	  const S *ego = (const S *)ego_;
          return (1
                  && p->sz->rnk >= 2
		  && picksplit(ego, p->sz, rp)
                  && (0

		      /* can work out-of-place, but HC2R destroys input */
                      || (p->r != p->rio && p->r != p->iio && 
			  (p->kind == R2HC || DESTROY_INPUTP(plnr)))

		      /* FIXME: what are sufficient conditions for inplace? */
                      || (!(p->r != p->rio && p->r != p->iio))
		       )
	       );
     }

     return 0;
}

/* TODO: revise this. */
static int applicable(const solver *ego_, const problem *p_, 
		      const planner *plnr, int *rp)
{
     const S *ego = (const S *)ego_;

     if (!applicable0(ego_, p_, rp, plnr)) return 0;

     /* fixed spltrnk (unlike fftw2's spltrnk=1, default buddies[0] is
        spltrnk=0, which is an asymptotic "theoretical optimum" for
        an ideal cache; it's equivalent to spltrnk=1 for rnk < 4). */
     if (NO_RANK_SPLITSP(plnr) && (ego->spltrnk != ego->buddies[0]))
          return 0;

     if (NO_UGLYP(plnr)) {
	  const problem_rdft2 *p = (const problem_rdft2 *) p_;

	  /* Heuristic: if the vector stride is greater than the transform
	     sz, don't use (prefer to do the vector loop first with a
	     vrank-geq1 plan). */
	  if (p->vecsz->rnk > 0 &&
	      X(tensor_min_stride)(p->vecsz) 
	      > X(rdft2_tensor_max_index)(p->sz, p->kind))
	       return 0;
     }

     return 1;
}

static plan *mkplan(const solver *ego_, const problem *p_, planner *plnr)
{
     const S *ego = (const S *) ego_;
     const problem_rdft2 *p;
     P *pln;
     plan *cldr = 0, *cldc = 0;
     tensor *sz1, *sz2, *vecszi, *sz2i;
     int spltrnk;
     inplace_kind k;
     problem *cldp;

     static const plan_adt padt = {
	  X(rdft2_solve), awake, print, destroy
     };

     if (!applicable(ego_, p_, plnr, &spltrnk))
          return (plan *) 0;

     p = (const problem_rdft2 *) p_;
     X(tensor_split)(p->sz, &sz1, spltrnk, &sz2);

     k = p->kind == R2HC ? INPLACE_OS : INPLACE_IS;
     vecszi = X(tensor_copy_inplace)(p->vecsz, k);
     sz2i = X(tensor_copy_inplace)(sz2, k);

     /* complex data is ~half of real */
     sz2i->dims[sz2i->rnk - 1].n = sz2i->dims[sz2i->rnk - 1].n/2 + 1;

     cldr = X(mkplan_d)(plnr, 
		       X(mkproblem_rdft2_d)(X(tensor_copy)(sz2),
					    X(tensor_append)(p->vecsz, sz1),
					    p->r, p->rio, p->iio, p->kind));
     if (!cldr) goto nada;

     if (p->kind == R2HC)
	  cldp = X(mkproblem_dft_d)(X(tensor_copy_inplace)(sz1, k),
				    X(tensor_append)(vecszi, sz2i),
				    p->rio, p->iio, p->rio, p->iio);
     else /* HC2R must swap re/im parts to get IDFT */
	  cldp = X(mkproblem_dft_d)(X(tensor_copy_inplace)(sz1, k),
				    X(tensor_append)(vecszi, sz2i),
				    p->iio, p->rio, p->iio, p->rio);
     cldc = X(mkplan_d)(plnr, cldp);
     if (!cldc) goto nada;

     pln = MKPLAN_RDFT2(P, &padt, p->kind == R2HC ? apply_r2hc : apply_hc2r);

     pln->cldr = cldr;
     pln->cldc = cldc;

     pln->solver = ego;
     X(ops_add)(&cldr->ops, &cldc->ops, &pln->super.super.ops);

     X(tensor_destroy4)(sz2i, vecszi, sz2, sz1);

     return &(pln->super.super);

 nada:
     X(plan_destroy_internal)(cldr);
     X(plan_destroy_internal)(cldc);
     X(tensor_destroy4)(sz2i, vecszi, sz2, sz1);
     return (plan *) 0;
}

static solver *mksolver(int spltrnk, const int *buddies, int nbuddies)
{
     static const solver_adt sadt = { mkplan };
     S *slv = MKSOLVER(S, &sadt);
     slv->spltrnk = spltrnk;
     slv->buddies = buddies;
     slv->nbuddies = nbuddies;
     return &(slv->super);
}

void X(rdft2_rank_geq2_register)(planner *p)
{
     int i;
     static const int buddies[] = { 0, 1, -2 };

     const int nbuddies = sizeof(buddies) / sizeof(buddies[0]);

     for (i = 0; i < nbuddies; ++i)
          REGISTER_SOLVER(p, mksolver(buddies[i], buddies, nbuddies));

     /* FIXME: Should we try more buddies?  See also dft/rank-geq2. */
}
