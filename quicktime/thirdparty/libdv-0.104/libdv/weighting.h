/* 
 *  weighting.h
 *
 *     Copyright (C) Charles 'Buck' Krasic - April 2000
 *     Copyright (C) Erik Walthinsen - April 2000
 *
 *  This file is part of libdv, a free DV (IEC 61834/SMPTE 314M)
 *  codec.
 *
 *  libdv is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser Public License as published by
 *  the Free Software Foundation; either version 2.1, or (at your
 *  option) any later version.
 *   
 *  libdv is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser Public License for more details.
 *   
 *  You should have received a copy of the GNU Lesser Public License
 *  along with libdv; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 *  The libdv homepage is http://libdv.sourceforge.net/.  
 */

/** @file
 *  @ingroup weighting
 *  @brief   Interface for @link weighting Coefficient Weighting @endlink
 */

/** @addtogroup weighting Coefficient Weighting
 *  @{
 */

#ifndef DV_WEIGHTING_H
#define DV_WEIGHTING_H

#include "dv_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void _dv_weight_init(void);
void _dv_weight_88(dv_coeff_t *block);
void _dv_weight_248(dv_coeff_t *block);
void _dv_weight_88_inverse(dv_coeff_t *block);
void _dv_weight_248_inverse(dv_coeff_t *block);

#ifdef __cplusplus
}
#endif

#endif // DV_WEIGHTING_H

/*@}*/
