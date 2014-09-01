#ifndef _ENCORE_TRANSFERIDCT_H
#define _ENCORE_TRANSFERIDCT_H

#include "../portab.h"
void EncTransferIDCT_add(uint8_t *pDest, int16_t *pSrc, int stride);
void EncTransferIDCT_copy(uint8_t *pDest, int16_t *pSrc, int stride);

#endif
