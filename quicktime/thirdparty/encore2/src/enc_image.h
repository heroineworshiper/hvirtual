#ifndef _ENCORE_VOP_H

#define _ENCORE_VOP_H



#include "enctypes.h"



int CreateImage(Image * pImage, int width, int height);

void FreeImage(Image * pImage);

void SwapImages(Image * pIm1, Image * pIm2);

void CopyImages(Image * pIm1, Image * pIm2);

void SetEdges(Image * pImage);

void Interpolate(const Image * pRef, Image * pInterH, Image * pInterV,

		 Image * pInterHV, const int iRounding, int iChromOnly);



#endif

