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

/* $Id: plan.c,v 1.2 2003/03/15 20:29:43 stevenj Exp $ */

#include "rdft.h"

plan *X(mkplan_rdft)(size_t size, const plan_adt *adt, rdftapply apply)
{
     plan_rdft *ego;

     ego = (plan_rdft *) X(mkplan)(size, adt);
     ego->apply = apply;

     return &(ego->super);
}
