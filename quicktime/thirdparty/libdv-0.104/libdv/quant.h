/* Copyright (C) 2000 Oregon Graduate Institute of Science & Technology
 *
 * See the file COPYRIGHT, which should have been supplied with this
 * package, for licensing details.  We may be contacted through email
 * at <quasar-help@cse.ogi.edu>.
 */

/** @file
 *  @ingroup quant
 *  @brief   Interface for @link quant Coefficient Quantization @endlink
 */

/** @addtogroup quant Coefficient Quantization
 *  @{
 */

#ifndef DV_QUANT_H
#define DV_QUANT_H

#include "dv_types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void _dv_quant(dv_coeff_t *block,int qno,int klass);
extern void _dv_quant_88_inverse(dv_coeff_t *block,int qno,int klass);
extern void (*_dv_quant_248_inverse) (dv_coeff_t *block,int qno,int klass,
                                  dv_248_coeff_t *co);
extern void _dv_quant_88_inverse_x86(dv_coeff_t *block,int qno,int klass);
extern void _dv_quant_88_inverse_x86_64(dv_coeff_t *block,int qno,int klass);
extern void dv_quant_init (void);
#ifdef __cplusplus
}
#endif

#endif /* DV_QUANT_H */

/*@}*/
