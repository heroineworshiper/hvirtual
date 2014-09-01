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

#include "codelet-dft.h"
#include "t1f.h"

#if HAVE_SIMD
static int okp(const ct_desc *d,
	       const R *rio, const R *iio, 
	       int ios, int vs, int m, int dist,
	       const planner *plnr)
{
     return (RIGHT_CPU()
             && ALIGNEDA(rio)
	     && !NO_SIMDP(plnr)
	     && SIMD_STRIDE_OKA(ios)
	     && SIMD_VSTRIDE_OKA(dist)
	     && iio == rio + 1
             && (m % VL) == 0
	     && (!d->s1 || (d->s1 == ios))
	     && (!d->s2 || (d->s2 == vs))
	     && (!d->dist || (d->dist == dist))
	  );
}
const ct_genus GENUS = { okp, VL };
#endif
