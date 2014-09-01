#include "copyblock.h"

void EncCopyBlock(uint8_t *pSrc, int16_t *pDest, int stride)
{
    int x, y;

    for (y = 0; y < 8; y++)
	for (x = 0; x < 8; x++)
	    pDest[x + y * 8] = (int16_t) pSrc[x + y * stride];
}
