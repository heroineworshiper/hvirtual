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

#include "api.h"
#include "dft.h"

X(plan) X(plan_guru_split_dft)(int rank, const X(iodim) *dims,
			       int howmany_rank, const X(iodim) *howmany_dims,
			       R *ri, R *ii, R *ro, R *io, unsigned flags)
{
     if (!X(guru_kosherp)(rank, dims, howmany_rank, howmany_dims)) return 0;

     return X(mkapiplan)(
	  ii - ri == 1 && io - ro == 1 ? FFT_SIGN : -FFT_SIGN, flags,
	  X(mkproblem_dft_d)(X(mktensor_iodims)(rank, dims, 1, 1),
			     X(mktensor_iodims)(howmany_rank, howmany_dims,
						1, 1),
			     TAINT_UNALIGNED(ri, flags),
			     TAINT_UNALIGNED(ii, flags), 
			     TAINT_UNALIGNED(ro, flags),
			     TAINT_UNALIGNED(io, flags)));
}
