#ifndef _DCT_H_
#define _DCT_H_

#include "enc_portab.h"


void enc_fdct_int32(short * const block);
void enc_fdct_mmx (short * const block);

void enc_idct_int32_init();
void enc_idct_int32 (short * const block);

void enc_idct_mmx (short * const src_result);
void enc_idct_sse (short * const src_result);

#endif /* _DCT_H_ */

