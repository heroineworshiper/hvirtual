#include "transferidct.h"



void EncTransferIDCT_add(uint8_t *pDest, int16_t *pSrc, int stride)

{

    int x, y;



    for (y = 0; y < 8; y++)

	for (x = 0; x < 8; x++)

	{

	    int16_t tmp = pDest[x + y * stride] + pSrc[x + y * 8];



	    if (tmp < 0)

		tmp = 0;

	    if (tmp > 255)

		tmp = 255;

	    pDest[x + y * stride] = (uint8_t) tmp;

	}

}



void EncTransferIDCT_copy(uint8_t *pDest, int16_t *pSrc, int stride)

{

    int x, y;



    for (y = 0; y < 8; y++)

	for (x = 0; x < 8; x++)

	{

	    int16_t tmp = pSrc[x + y * 8];



	    if (tmp < 0)

		tmp = 0;

	    if (tmp > 255)

		tmp = 255;

	    pDest[x + y * stride] = (uint8_t) tmp;

	}

}




