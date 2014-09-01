
#ifndef _ENCORE_MAD_H
#define _ENCORE_MAD_H
#include "enc_image.h"
float MAD_Image(const Image * pIm, const Image * pImage);

// x & y in blocks ( 8 pixel units )
// dx & dy in pixels
int32_t SAD_Block(const Image * pIm, const Image * pImage, int x, int y, int dx,
		  int dy, int32_t sad_opt, int component);

// x & y in macroblocks
// dx & dy in half-pixels

int32_t SAD_Macroblock(const Image * pIm,
		       const Image * pImageN, const Image * pImageH,
		       const Image * pImageV, const Image * pImageHV, int x, int y,
		       int dx, int dy, int sad_opt, int quality);

// x & y in macroblocks
int32_t SAD_Deviation_MB(const Image * pIm, int x, int y);

#endif
