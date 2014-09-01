/**************************************************************************
 *                                                                        *
 * This code is developed by Eugene Kuznetsov.  This software is an       *
 * implementation of a part of one or more MPEG-4 Video tools as          *
 * specified in ISO/IEC 14496-2 standard.  Those intending to use this    *
 * software module in hardware or software products are advised that its  *
 * use may infringe existing patents or copyrights, and any such use      *
 * would be at such party's own risk.  The original developer of this     *
 * software module and his/her company, and subsequent editors and their  *
 * companies (including Project Mayo), will have no liability for use of  *
 * this software or modifications or derivatives thereof.                 *
 *                                                                        *
 * Project Mayo gives users of the Codec a license to this software       *
 * module or modifications thereof for use in hardware or software        *
 * products claiming conformance to the MPEG-4 Video Standard as          *
 * described in the Open DivX license.                                    *
 *                                                                        *
 * The complete Open DivX license can be found at                         *
 * http://www.projectmayo.com/opendivx/license.php .                      *
 *                                                                        *
 **************************************************************************/

 
/**************************************************************************
 *
 *  encoder.c, video encoder kernel
 *
 *  Copyright (C) 2001  Project Mayo
 *
 *  Eugene Kuznetsov
 *
 *  DivX Advance Research Center <darc@projectmayo.com>
 *
 **************************************************************************/ 


/**************************************************************************
 *
 *  Modifications:
 *
 *	24.11.2001 mmx rgb24/32 support
 *  23.11.2001 'dirty keyframes' bugfix
 *  18.11.2001 introduced new encoding mode quality=0 (keyframes only)
 *  17.11.2001 aquant bug fix 
 *	16.11.2001 support for new bitstream headers; relocated dquant malloc
 *	10.11.2001 removed init_dct_codes(); now done in mbtransquant.c
 *			   removed old idct/fdct init, added new idct init
 *	03.11.2001 case ENC_CSP_IYUV break was missing, thanks anon 
 *	28.10.2001 added new colorspace switch, reset minQ/maxQ limited to 1/31
 *  01.10.2001 added experimental luminance masking support
 *  26.08.2001 reactivated INTER4V encoding mode for EXT_MODE
 *  26.08.2001 dquants are filled with absolute quant value after MB coding
 *  24.08.2001 fixed bug in EncodeFrameP which lead to ugly keyframes
 *  23.08.2001 fixed bug when EXT_MODE is called without options
 *  22.08.2001 support for EXT_MODE encoding
 *             support for setting quantizer on a per macro block level
 *  10.08.2001 fixed some compiling errors, get rid of compiler warnings
 *
 *  Michael Militzer <isibaar@videocoding.de>
 *
 **************************************************************************/

#include "encoder.h"
#include "enc_mbfunctions.h"
#include "enc_bitstream.h"
#include "colorspace/enc_colorspace.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "mbtransquant/enc_dct.h"
#include "enc_timer.h"
#include "ratecontrol.h"

static int FrameCodeI(Encoder * pEnc, Bitstream * bs, uint32_t *pBits, int enc_mode);
static int FrameCodeP(Encoder * pEnc, Bitstream * bs, uint32_t *pBits, bool force_inter, int enc_mode);

#define MAX(a,b)      (((a) > (b)) ? (a) : (b))

// gruel's normalize code
// should be moved to a seperate file
// ***********
// ***********
int MBXDIM;
int MBYDIM;

int normalize_quantizer_field(float *in, int *out, int num, int min_quant, int max_quant);
int adaptive_quantization(unsigned char* buf, int stride, int* intquant, 
	int framequant, int min_quant, int max_quant);  // no qstride because normalization

#define RDIFF(a,b)    ((int)(a+0.5)-(int)(b+0.5))
int normalize_quantizer_field(float *in, int *out, int num, int min_quant, int max_quant)
{
	int i;
	int finished;
	
	do
	{    
		finished = 1;
		for(i = 1; i < num; i++)
		{
			if(RDIFF(in[i], in[i-1]) > 2)
            {
				in[i] -= (float) 0.5;
				finished = 0;
			}
			else if(RDIFF(in[i], in[i-1]) < -2)
			{
				in[i-1] -= (float) 0.5;
				finished = 0;
			}
        
          if(in[i] > max_quant)
		  {
			  in[i] = (float) max_quant;
			  finished = 0;
		  }
          if(in[i] < min_quant)
		  { 
			  in[i] = (float) min_quant;
			  finished = 0;
		  }
          if(in[i-1] > max_quant)
		  { 
			  in[i-1] = (float) max_quant;
			  finished = 0;
		  }
          if(in[i-1] < min_quant) 
		  { 
			  in[i-1] = (float) min_quant;
			  finished = 0;
		  }
		}
	} while(!finished);
	
	out[0] = 0;
	for (i = 1; i < num; i++)
		out[i] = RDIFF(in[i], in[i-1]);
	
	return (int) (in[0] + 0.5);
}

int adaptive_quantization(unsigned char* buf, int stride, int* intquant, 
        int framequant, int min_quant, int max_quant)  // no qstride because normalization
{
	int i,j,k,l;
	
	static float *quant;
	unsigned char *ptr;
	float val;
	
	const float DarkAmpl    = 14 / 2;
	const float BrightAmpl  = 10 / 2;
	const float DarkThres   = 70;
	const float BrightThres = 200;
	
	if(!quant)
		if(!(quant = (float *) malloc(MBXDIM*MBYDIM * sizeof(float))))
			return -1;


    for(k = 0; k < MBYDIM; k++)
	{
		for(l = 0;l < MBXDIM; l++)        // do this for all macroblocks individually 
		{
			quant[k*MBXDIM+l] = (float) framequant;
			
			// calculate luminance-masking
			ptr = &buf[16*k*stride+16*l];			// address of MB
			
			val = 0.;
			
			for(i = 0; i < 16; i++)
				for(j = 0; j < 16; j++)
					val += ptr[i*stride+j];
				val /= 256.;
			   
			   if(val < DarkThres)
				   quant[k*MBXDIM+l] += DarkAmpl*(DarkThres-val)/DarkThres;
			   else if (val>BrightThres)
				   quant[k*MBXDIM+l] += BrightAmpl*(val-BrightThres)/(255-BrightThres);
		}
	}
	
	return normalize_quantizer_field(quant, intquant, MBXDIM*MBYDIM, min_quant, max_quant);
}
// ***********
// ***********

static __inline uint8_t get_fcode(uint16_t sr)
{
    if (sr <= 16)
		return 1;

    else if (sr <= 32) 
		return 2;

    else if (sr <= 64)
		return 3;

    else if (sr <= 128)
		return 4;

    else if (sr <= 256)
		return 5;

    else if (sr <= 512)
		return 6;

    else if (sr <= 1024)
		return 7;

    else
		return 0;
}

#define ENC_CHECK(X) if(!(X)) return ENC_BAD_FORMAT

int CreateEncoder(ENC_PARAM * pParam)
{
    Encoder *pEnc;

    pParam->handle = NULL;

    ENC_CHECK(pParam);


/* Validate input parameters */

    ENC_CHECK(pParam->x_dim > 0);
    ENC_CHECK(pParam->y_dim > 0);
    ENC_CHECK(!(pParam->x_dim % 2));
    ENC_CHECK(!(pParam->y_dim % 2));

    /* 
       these two limits are introduced by limitations of decore */

//    ENC_CHECK(pParam->x_dim <= 720);
//    ENC_CHECK(pParam->y_dim <= 576);


/* Set default values for all other parameters if they are outside
    acceptable range */

    if (pParam->framerate <= 0)
		pParam->framerate = 25.;

    if (pParam->bitrate <= 0)
		pParam->bitrate = 910000;

    if (pParam->rc_period <= 0)
		pParam->rc_period = 50;

    if (pParam->rc_reaction_period <= 0)
		pParam->rc_reaction_period = 10;

    if (pParam->rc_reaction_ratio <= 0)
		pParam->rc_reaction_ratio = 10;

    if ((pParam->min_quantizer <= 0) || (pParam->min_quantizer > 31))
		pParam->min_quantizer = 1;

    if ((pParam->max_quantizer <= 0) || (pParam->max_quantizer > 31))
		pParam->max_quantizer = 31;

    if (pParam->max_key_interval == 0)
		pParam->max_key_interval = 250;		 /* 1 keyframe each 10 seconds */

    if (pParam->max_quantizer < pParam->min_quantizer)
		pParam->max_quantizer = pParam->min_quantizer;

    if ((pParam->quality < 0) || (pParam->quality > 5))
		pParam->quality = 5;

    pEnc = (Encoder *) malloc(sizeof(Encoder));

    if (pEnc == 0)
		return ENC_MEMORY;

/* Fill members of Encoder structure */

    pEnc->mbParam.width = pParam->x_dim;
    pEnc->mbParam.height = pParam->y_dim;
    pEnc->mbParam.quality = pParam->quality;
    pEnc->sStat.fMvPrevSigma = -1;

/* Fill rate control parameters */

    pEnc->rateCtlParam.max_quant = pParam->max_quantizer;
    pEnc->rateCtlParam.min_quant = pParam->min_quantizer;

    pEnc->mbParam.quant = 4;

    if (pEnc->rateCtlParam.max_quant < pEnc->mbParam.quant)
		pEnc->mbParam.quant = pEnc->rateCtlParam.max_quant;

    if (pEnc->rateCtlParam.min_quant > pEnc->mbParam.quant)
		pEnc->mbParam.quant = pEnc->rateCtlParam.min_quant;

    pEnc->iFrameNum = 0;
    pEnc->iMaxKeyInterval = pParam->max_key_interval;

    if (CreateImage(&(pEnc->sCurrent), pEnc->mbParam.width, pEnc->mbParam.height) < 0)
    {
		free(pEnc);
		return ENC_MEMORY;
    }

    if (CreateImage(&(pEnc->sBackup), pEnc->mbParam.width, pEnc->mbParam.height) < 0)
    {
		FreeImage(&(pEnc->sCurrent));
		free(pEnc);
		return ENC_MEMORY;
    }
    
	if (CreateImage(&(pEnc->sReference), pEnc->mbParam.width, pEnc->mbParam.height) < 0)
    {
		FreeImage(&(pEnc->sCurrent));
		FreeImage(&(pEnc->sBackup));
		free(pEnc);
		return ENC_MEMORY;
    }

    if (CreateImage(&pEnc->vInterH, pEnc->mbParam.width, pEnc->mbParam.height) < 0)
    {
		FreeImage(&(pEnc->sCurrent));
		FreeImage(&(pEnc->sBackup));
		FreeImage(&(pEnc->sReference));
		free(pEnc);
		return ENC_MEMORY;
    }

    if (CreateImage(&pEnc->vInterV, pEnc->mbParam.width, pEnc->mbParam.height) < 0)
    {
		FreeImage(&(pEnc->sCurrent));
		FreeImage(&(pEnc->sBackup));
		FreeImage(&(pEnc->sReference));
		FreeImage(&(pEnc->vInterH));
		free(pEnc);
		return ENC_MEMORY;
    }

    if (CreateImage(&pEnc->vInterHV, pEnc->mbParam.width, pEnc->mbParam.height) < 0)
    {
		FreeImage(&(pEnc->sCurrent));
		FreeImage(&(pEnc->sBackup));
		FreeImage(&(pEnc->sReference));
		FreeImage(&(pEnc->vInterH));
		FreeImage(&(pEnc->vInterV));
		free(pEnc);
		return ENC_MEMORY;
    }

    pParam->handle = (void *) pEnc;

    RateCtlInit(&(pEnc->rateCtlParam), pEnc->mbParam.quant,
		pParam->bitrate / pParam->framerate,
		pParam->rc_period,
		pParam->rc_reaction_period, pParam->rc_reaction_ratio);

#if !(defined(WIN32) && defined(_MMX_))
	enc_idct_int32_init();
#endif

	init_timer();

return ENC_OK;
}


int FreeEncoder(Encoder * pEnc)
{
    ENC_CHECK(pEnc);
    ENC_CHECK(pEnc->sCurrent.pY);
    ENC_CHECK(pEnc->sReference.pY);


    FreeImage(&(pEnc->sCurrent));
	FreeImage(&(pEnc->sBackup));
    FreeImage(&(pEnc->sReference));
    FreeImage(&(pEnc->vInterH));
    FreeImage(&(pEnc->vInterV));
    FreeImage(&(pEnc->vInterHV));
    free(pEnc);

    return ENC_OK;
}


#if defined(_MMX_)






#define RGB24_TO_YUV	rgb24_to_yuv_mmx
#define RGB32_TO_YUV	rgb32_to_yuv_mmx




#if defined(_XMM_)

#define YUV_TO_YUV		yuv_to_yuv_xmm




#else

#define YUV_TO_YUV      yuv_to_yuv_mmx



#endif
#else




#define RGB24_TO_YUV	rgb24_to_yuv
#define RGB32_TO_YUV	rgb32_to_yuv
#define YUV_TO_YUV		yuv_to_yuv





#endif

int EncodeFrame(Encoder * pEnc, 
	ENC_FRAME * pFrame, 
	ENC_RESULT * pResult, 
	int enc_mode)
{
    int result;
    uint16_t x, y;
    Bitstream bs;
    uint32_t bits;
	ENC_FRAME_EXT *pFrame_ext;
    Image *pCurrent = &(pEnc->sCurrent);
	int *temp_dquants;

//printf("EncodeFrame 1\n");
	start_global_timer();

	temp_dquants = NULL;
	
    ENC_CHECK(pEnc);
    ENC_CHECK(pFrame);
    ENC_CHECK(pFrame->bitstream);
    ENC_CHECK(pFrame->image);


	/* convert input colorspace into yuv planar 
	*
	* testing results
	*	rgb24	yes
	*	rgb32	yes
	*	iyuv
	*	yv12
	*	yuy2	yes
	*	yvyu
	*	uyvy	yes
	*/
//printf("EncodeFrame 1\n");

	start_timer();
//printf("EncodeFrame 1\n");

	switch(pFrame->colorspace) {

	case ENC_CSP_RGB24 :
		RGB24_TO_YUV(pEnc->sCurrent.pY, pEnc->sCurrent.pU, pEnc->sCurrent.pV, 
						pFrame->image, 
						pEnc->mbParam.width, pEnc->mbParam.height, 
						pEnc->sCurrent.iEdgedWidth);
		break;

	case ENC_CSP_RGB32 :
		RGB32_TO_YUV(pEnc->sCurrent.pY, pEnc->sCurrent.pU, pEnc->sCurrent.pV,
						pFrame->image,
						pEnc->mbParam.width, pEnc->mbParam.height,
						pEnc->sCurrent.iEdgedWidth);
		break;

	case ENC_CSP_IYUV : /* ENC_CSP_I420 : */
		YUV_TO_YUV(pEnc->sCurrent.pY, pEnc->sCurrent.pU, pEnc->sCurrent.pV,
					pFrame->image, 
					pEnc->mbParam.width, pEnc->mbParam.height,
					pEnc->sCurrent.iEdgedWidth);
		break;

	case ENC_CSP_YV12 :	/* u/v simply swapped */
//printf("EncodeFrame 1.1 %p\n", pEnc);
		YUV_TO_YUV(pEnc->sCurrent.pY, 
					pEnc->sCurrent.pV, 
					pEnc->sCurrent.pU,
					pFrame->image, 
					pEnc->mbParam.width, 
					pEnc->mbParam.height, 
					pEnc->sCurrent.iEdgedWidth);
//printf("EncodeFrame 1.2\n");
		break;

	case ENC_CSP_YUY2 :
		yuyv_to_yuv(pEnc->sCurrent.pY, pEnc->sCurrent.pU, pEnc->sCurrent.pV,
			pFrame->image,
			pEnc->mbParam.width, pEnc->mbParam.height,
			pEnc->sCurrent.iEdgedWidth);
		break;

	case ENC_CSP_YVYU :	/* u/v simply swapped */
		yuyv_to_yuv(pEnc->sCurrent.pY, pEnc->sCurrent.pV, pEnc->sCurrent.pU,
			pFrame->image,
			pEnc->mbParam.width, pEnc->mbParam.height, 
			pEnc->sCurrent.iEdgedWidth);
		break;


	case ENC_CSP_UYVY :
		uyvy_to_yuv(pEnc->sCurrent.pY, pEnc->sCurrent.pU, pEnc->sCurrent.pV,
			pFrame->image,
			pEnc->mbParam.width, pEnc->mbParam.height,
			pEnc->sCurrent.iEdgedWidth);
		break;
			
	default :
		return ENC_BAD_FORMAT;
    }
//printf("EncodeFrame 1\n");

	stop_conv_timer();
//printf("EncodeFrame 1\n");

    BitstreamInit(&bs, pFrame->bitstream);
//printf("EncodeFrame 1\n");

    switch (enc_mode)
	{
	case RC_MODE: 
		pEnc->mbParam.quant = RateCtlGetQ(&(pEnc->rateCtlParam), 0);
		break;

    case VBR_MODE: 
		pEnc->mbParam.quant = pFrame->quant;
		break;

	case EXT_MODE:

		temp_dquants = (int *) malloc(pCurrent->iMbHcount * pCurrent->iMbWcount * sizeof(int));
		
		MBXDIM = pCurrent->iMbWcount;
		MBYDIM = pCurrent->iMbHcount;
		
		pFrame_ext = (ENC_FRAME_EXT *) pFrame->mvs;

		if((pFrame_ext->ext_opt & EXT_OPT_LUMINANCE_MASKING) > 0) {
			pEnc->mbParam.quant = adaptive_quantization(pEnc->sCurrent.pY, pEnc->mbParam.width,
				temp_dquants, pFrame->quant, pFrame->quant,
				MAX(pEnc->rateCtlParam.max_quant, pFrame->quant));
			
			for (y = 0; y < pCurrent->iMbHcount; y++)
			for (x = 0; x < pCurrent->iMbWcount; x++)
			{
				Macroblock *pMB = &pCurrent->pMBs[x + y * pCurrent->iMbWcount];
				pMB->dquant = iDQtab[(temp_dquants[y * pCurrent->iMbWcount + x] + 2)];
			}
			break;
		}

		if((pFrame_ext->ext_opt & EXT_OPT_QUANT_ARRAY) > 0) {
			pEnc->mbParam.quant = normalize_quantizer_field((float *) pFrame_ext->quant_array, 
				temp_dquants, pCurrent->iMbHcount * pCurrent->iMbWcount, 1, 31);
			
			for (y = 0; y < pCurrent->iMbHcount; y++)
			for (x = 0; x < pCurrent->iMbWcount; x++)
			{
				Macroblock *pMB = &pCurrent->pMBs[x + y * pCurrent->iMbWcount];
				pMB->dquant = iDQtab[(temp_dquants[y * pCurrent->iMbWcount + x] + 2)];
			}
			break;
		}
		
		if(pFrame_ext->ext_opt == 0) {
			for (y = 0; y < pCurrent->iMbHcount; y++)
			for (x = 0; x < pCurrent->iMbWcount; x++)
			{
				Macroblock *pMB = &pCurrent->pMBs[x + y * pCurrent->iMbWcount];
				pMB->dquant = NO_CHANGE;
			}
			pEnc->mbParam.quant = pFrame->quant;
		}

		break;

	default:
		return ENC_FAIL; // should never happen
	}
//printf("EncodeFrame 1\n");

    if ((enc_mode == RC_MODE) || (pFrame->intra < 0))
    {
//printf("EncodeFrame 1\n");
		if ((pEnc->iFrameNum == 0) || ((pEnc->iMaxKeyInterval > 0) 
			&& (pEnc->iFrameNum >= pEnc->iMaxKeyInterval)) 
			|| (pEnc->mbParam.quality==0) )

			result = FrameCodeI(pEnc, &bs, &bits, enc_mode);
		else
			result = FrameCodeP(pEnc, &bs, &bits, 0, enc_mode);
    }
    else
    {
//printf("EncodeFrame 2\n");
		if (pFrame->intra == 1)
		    result = FrameCodeI(pEnc, &bs, &bits, enc_mode);
		else
			result = FrameCodeP(pEnc, &bs, &bits, 1, enc_mode);
    }

//printf("EncodeFrame 1 %d\n", pFrame->intra);
	if (pResult)
    {
	pResult->is_key_frame = result;
	pResult->texture_bits = pEnc->sStat.iTextBits;
	pResult->motion_bits = pEnc->sStat.iMvBits;
	pResult->total_bits = bits;
	pResult->quantizer = pEnc->mbParam.quant;    
    }
//printf("EncodeFrame 2\n");


    BitstreamPutBits(&bs, 0x0000, 16);
    BitstreamPutBits(&bs, 0x0000, 16);
    
    BitstreamPad(&bs);
    pFrame->length = BitstreamLength(&bs);
    pEnc->iFrameNum++;

    if (enc_mode == RC_MODE)
        RateCtlUpdate(&(pEnc->rateCtlParam), bits);

    SwapImages(&(pEnc->sCurrent), &(pEnc->sReference));
	
	if (temp_dquants)
	{
		free(temp_dquants);
	}
    
	stop_global_timer();
	write_timer();

	return ENC_OK;
}


static int FrameCodeI(Encoder * pEnc, Bitstream * bs, uint32_t *pBits, int enc_mode)
{
    int16_t dct_codes[6][64];
    int16_t qcoeff[6][64];
    uint16_t x, y;
    Image *pCurrent = &(pEnc->sCurrent);

    pEnc->iFrameNum = 0;
    pEnc->mbParam.rounding_type = 1;
    pEnc->mbParam.coding_type = I_VOP;

//printf("FrameCodeI 1\n");

	BitstreamVolHeader(bs, pEnc->mbParam.width, pEnc->mbParam.height);
	BitstreamVopHeader(bs, I_VOP, pEnc->mbParam.rounding_type,
			pEnc->mbParam.quant,
			pEnc->mbParam.fixed_code); 

//printf("FrameCodeI 2\n");
    *pBits = BsPos(bs);

    for (y = 0; y < pCurrent->iMbHcount; y++)
		for (x = 0; x < pCurrent->iMbWcount; x++)
		{
		    Macroblock *pMB = &pCurrent->pMBs[x + y * pCurrent->iMbWcount];

//printf("FrameCodeI 1 %d %d %p %p\n", x, y, bs->tail, bs->start);
			if(enc_mode == EXT_MODE) {
//printf("FrameCodeI 3.1\n");
				if(pMB->dquant == NO_CHANGE)
					pMB->mode = MODE_INTRA;
				else {
					pMB->mode = MODE_INTRA_Q;
//printf("FrameCodeI 3.2\n");
					pEnc->mbParam.quant += DQtab[pMB->dquant];
//printf("FrameCodeI 3.3\n");
					if(pEnc->mbParam.quant > 31) pEnc->mbParam.quant = 31;
//printf("FrameCodeI 3.4\n");
					if(pEnc->mbParam.quant < 1) pEnc->mbParam.quant = 1;
//printf("FrameCodeI 3.5\n");
				}
			}
			else
				pMB->mode = MODE_INTRA;
	

			pMB->aquant = pEnc->mbParam.quant;

//printf("FrameCodeI 3.6\n");
			MBTransQuantIntra(&pEnc->mbParam, x, y, dct_codes, qcoeff, pCurrent);
//printf("FrameCodeI 3.7\n");

			start_timer();
//printf("FrameCodeI 3\n");

			MBPrediction(&pEnc->mbParam, x, y, pCurrent->iMbWcount, qcoeff, pCurrent->pMBs);
//printf("FrameCodeI 3\n");

			stop_prediction_timer();
//printf("FrameCodeI 3\n");

			start_timer();
//printf("FrameCodeI 3\n");

//printf("FrameCodeI 3\n");
			MBCoding(&pEnc->mbParam,
		   		pCurrent->pMBs + x + y * pCurrent->iMbWcount, 
				qcoeff, 
				bs, 
				&pEnc->sStat);

//printf("FrameCodeI 4\n");
			stop_coding_timer();
//printf("FrameCodeI 5\n");
		}

	EMMS;

    *pBits = BsPos(bs) - *pBits;
    pEnc->sStat.fMvPrevSigma = -1;
    pEnc->sStat.iTextBits = *pBits;
    pEnc->sStat.iMvBits = 0;
    pEnc->sStat.iMvSum = 0;
    pEnc->sStat.iMvCount = 0;
    pEnc->mbParam.fixed_code = 2;
//printf("FrameCodeI 5\n");

    return 1;					 // intra
}


#define INTRA_THRESHOLD 0.5

static int FrameCodeP(Encoder * pEnc, Bitstream * bs, uint32_t *pBits,
		      bool force_inter, int enc_mode)
{
    float fSigma;
    int16_t dct_codes[6][64];
    int16_t qcoeff[6][64];
    int x, y;
    int iIntra, iLimit;
    int iSearchRange;
	int backup_quant;

    Image *pCurrent = &(pEnc->sCurrent);
    Image *pRef = &(pEnc->sReference);

//printf("FrameCodeP 1\n");
    SetEdges(pRef);
//printf("FrameCodeP 1\n");

//    DumpImage(pRef, "ref.bmp");

    pEnc->mbParam.rounding_type = 1 - pEnc->mbParam.rounding_type;

//printf("FrameCodeP 1\n");
	backup_quant = pEnc->mbParam.quant;
	CopyImages(&(pEnc->sBackup), &(pEnc->sCurrent));
//printf("FrameCodeP 1\n");

    iIntra = 0;
    
    if (!force_inter)
		iLimit = (int)(pCurrent->iMbWcount * pCurrent->iMbHcount * INTRA_THRESHOLD);
    else
		iLimit = pCurrent->iMbWcount * pCurrent->iMbHcount + 1;

    start_timer();

    Interpolate(pRef, &pEnc->vInterH, &pEnc->vInterV, &pEnc->vInterHV,
		pEnc->mbParam.rounding_type,
		(pEnc->mbParam.quality < 4));
//printf("FrameCodeP 1\n");

    stop_inter_timer();

    pEnc->mbParam.coding_type = P_VOP;
//printf("FrameCodeP 1\n");

    BitstreamVopHeader(bs, P_VOP, pEnc->mbParam.rounding_type,
			 pEnc->mbParam.quant,
			 pEnc->mbParam.fixed_code); 
//printf("FrameCodeP 1\n");

    *pBits = BsPos(bs);
    pEnc->sStat.iTextBits = 0;
    pEnc->sStat.iMvBits = 0;
    pEnc->sStat.iMvSum = 0;
    pEnc->sStat.iMvCount = 0;

//printf("FrameCodeP 1\n");

    for (y = 0; y < pCurrent->iMbHcount; y++)
		for (x = 0; x < pCurrent->iMbWcount; x++)
		{

	    // 

	    // No floating calculations inside this loop please!

	    // 

//printf("FrameCodeP 1\n");
		    bool bIntra;
	
		    Macroblock *pMB = &pCurrent->pMBs[x + y * pCurrent->iMbWcount];
//printf("FrameCodeP 1\n");

		    start_timer();
//printf("FrameCodeP 1\n");

				bIntra = MBMotionEstComp(&pEnc->mbParam,
					     x, 
						 y, 
						 &pEnc->sReference,
					     &pEnc->vInterH, 
						 &pEnc->vInterV,
					     &pEnc->vInterHV, 
						 &pEnc->sCurrent,
						 dct_codes, 
						 (pMB->dquant == NO_CHANGE || 
						 	enc_mode != EXT_MODE));
//printf("FrameCodeP 1\n");

			stop_motion_timer();
//printf("FrameCodeP 1\n");

			if (bIntra)
		    {
				iIntra++;
				if (iIntra >= iLimit)
				{
				    BitstreamReset(bs);
					pEnc->mbParam.quant = backup_quant;
					CopyImages(&(pEnc->sCurrent), &(pEnc->sBackup));
					return FrameCodeI(pEnc, bs, pBits, enc_mode);
				}
		    }
	
		    bIntra = (pMB->mode == MODE_INTRA) || (pMB->mode == MODE_INTRA_Q);

			if (bIntra)
		    {
				if(enc_mode == EXT_MODE) {
					if(pMB->dquant != NO_CHANGE) {
						pMB->mode = MODE_INTRA_Q;
						pEnc->mbParam.quant += DQtab[pMB->dquant];
						if(pEnc->mbParam.quant > 31) pEnc->mbParam.quant = 31;
						if(pEnc->mbParam.quant < 1) pEnc->mbParam.quant = 1;
					}
				}
				pMB->aquant = pEnc->mbParam.quant;

				MBTransQuantIntra(&pEnc->mbParam, x, y,
				  dct_codes, qcoeff, pCurrent);
		    }

			else {
				if(enc_mode == EXT_MODE) {
					if(pMB->dquant != NO_CHANGE) {
						pMB->mode = MODE_INTER_Q;
						pEnc->mbParam.quant += DQtab[pMB->dquant];
						if(pEnc->mbParam.quant > 31) pEnc->mbParam.quant = 31;
						if(pEnc->mbParam.quant < 1) pEnc->mbParam.quant = 1;
					}
				}
				pMB->aquant = pEnc->mbParam.quant;

				pEnc->sCurrent.pMBs[x + y * pCurrent->iMbWcount].cbp
			    = MBTransQuantInter(&pEnc->mbParam, x, y,
					dct_codes, qcoeff, pCurrent);
			}

//printf("FrameCodeP 1\n");
		    start_timer();

			MBPrediction(&pEnc->mbParam, x, y, pCurrent->iMbWcount, qcoeff, pCurrent->pMBs);
//printf("FrameCodeP 1\n");

			stop_prediction_timer();

//printf("FrameCodeP 1\n");
			start_timer();

//printf("FrameCodeP 1\n");
			MBCoding(&pEnc->mbParam,
		     pCurrent->pMBs + x + y * pCurrent->iMbWcount, qcoeff, bs,
		     &pEnc->sStat);
//printf("FrameCodeP 1\n");
	
			stop_coding_timer();
//printf("FrameCodeP 2\n");
	}
//printf("FrameCodeP 1\n");

	EMMS;

//    DumpImage(pCurrent, "cur.bmp");

	if (pEnc->sStat.iMvCount == 0)

	pEnc->sStat.iMvCount = 1;

    fSigma = (float)sqrt((float) pEnc->sStat.iMvSum / pEnc->sStat.iMvCount);
//printf("FrameCodeP 1\n");

	//printf("Texture: %d bits, motion: %d bits, mv sigma: %f\n",
	//   pEnc->sStat.iTextBits, pEnc->sStat.iMvBits, fSigma);

    iSearchRange = 1 << (3 + pEnc->mbParam.fixed_code);

//printf("FrameCodeP 1\n");
    if ((fSigma > iSearchRange / 3) && (pEnc->mbParam.fixed_code <= 3))	// maximum search

	// range 128

    {
		pEnc->mbParam.fixed_code++;
		iSearchRange *= 2;
		// printf("New search range: %d\n", iSearchRange);
    }

    else 
		if ((fSigma < iSearchRange / 6)
	    && (pEnc->sStat.fMvPrevSigma >= 0)
	    && (pEnc->sStat.fMvPrevSigma < iSearchRange / 6)
	    && (pEnc->mbParam.fixed_code >= 2))	// minimum search range 16
    {

		pEnc->mbParam.fixed_code--;
		iSearchRange /= 2;
//		printf("New search range: %d\n", iSearchRange);
    }

//printf("FrameCodeP 1\n");

    pEnc->sStat.fMvPrevSigma = fSigma;
    *pBits = BsPos(bs) - *pBits;

//printf("FrameCodeP 2\n");
    return 0;					 // inter
}
