/**************************************************************************
 *
 *  Modifications:
 *
 *  23.11.2001  added sBackup to Encoder_s struct
 *	16.11.2001  new bitstream internals
 *	02.11.2001	included MBPRED_SIZE
 *  24.08.2001  removed MODE_INTER4V_Q support (not MPEG4 compliant)
 *  20.08.2001  added support for MODE_INTER4V_Q
 *
 *  Michael Militzer <isibaar@videocoding.de>
 *
 **************************************************************************/

#ifndef _ENCORE_ENCTYPES_H
#define _ENCORE_ENCTYPES_H

#include "enc_portab.h"

typedef int8_t bool;


typedef enum
{
/* Inter-coded macroblock, 1 motion vector */
    MODE_INTER = 0,
/* Inter-coded macroblock + dquant */
    MODE_INTER_Q = 1,
/* Inter-coded macroblock, 4 motion vectors */
    MODE_INTER4V = 2,
/* Intra-coded macroblock */
    MODE_INTRA = 3,
/* Intra-coded macroblock + dquant */
    MODE_INTRA_Q = 4,
}
MBMODE;

typedef enum
{
    I_VOP = 0,
    P_VOP = 1
}
VOP_TYPE;


typedef struct MotionVector_s
{
    int16_t x;
    int16_t y;
}
MotionVector;


typedef struct Bitstream_s
{
	uint64_t buf;
	uint32_t pos;
	unsigned char *start;
	unsigned char *tail;
}
Bitstream;


/* for each block we store the DC and first AC column & row coeffs
*/
#define MBPRED_SIZE  15

typedef struct
{
    /*
       Motion vectors for this macroblock. Initialized in MBMotionEstComp().
       When mode is MODE_INTER or MODE_INTER_Q, all four vectors are equal. 
       When mode is MODE_INTRA or MODE_INTRA_Q, all four vectors are zero. 
     */

    MotionVector mvs[4];

    /*
       Difference between found and predicted MVs ( will be sent into bitstream ).
       Initialized in MBPrediction(). 
     */

    MotionVector pmvs[4];

    /*
       Values used for AC/DC prediction. Initialized in MBPrediction() 
     */

    int16_t pred_values[6][MBPRED_SIZE];
    uint8_t acpred_directions[6];

    /*
       Macroblock mode. Initialized for P-VOP's in MBMotionEstComp(),
       for I-VOP's in FrameCodeI() ( encoder.c ) 
     */

    MBMODE mode;

    /*
       only meaningful when mode=MODE_INTRA_Q or mode=MODE_INTER_Q ( i.e. never ) 
     */

    uint8_t dquant;

	uint8_t aquant; // absolute quantizer for given MB

    /*
       Coded block pattern. For inter MBs calculated in MBTransQuant(), for 
       intra MBs in MBPrediction(). 
     */

    uint8_t cbp;
}
Macroblock;


typedef struct Image_s
{
/* Various kinds of dimensions */

    uint16_t iWidth;
    uint16_t iHeight;
    uint16_t iEdgedWidth;
    uint16_t iEdgedHeight;
    uint16_t iMbWcount;
    uint16_t iMbHcount;

/* Pointers to (0,0) pixels */

    uint8_t *pY;
    uint8_t *pU;
    uint8_t *pV;
    VOP_TYPE ePredictionType;
    Macroblock *pMBs;
}
Image;


/***********************************

       Encoding Parameters

************************************/ 

typedef struct
{
    uint16_t width;
    uint16_t height;
    VOP_TYPE coding_type;

    /*
	   Rounding type for image interpolation
	   Switched 0->1 and back after each interframe
	   Used in motion compensation, methods Interpolate*, vop.c 
    */

    uint8_t rounding_type;


	/*
	   Motion estimation parameter 
	   1<=iFcode<=4;  motion vector search range is +/- 1<<(iFcode+3) pixels
	   Automatically adjusted using motion vector statistics inside
	   EncodeDeltaFrame() ( encoder.c ) 
	 */

    uint8_t fixed_code;

	/*
	   Motion estimation quality/performance balance 
	   Supplied by user
	   1<=iQuality<=9
	   5: highest quality, slowest encoding ( everything turned on )
	   4: faster 16x16 vector search method
	   disabled half-pel search
	   ( + ~10% bitrate, +~40% fps )
	   3: disabled search for 8x8 vectors
	   ( + ~20% bitrate, +80-90% fps )
	   2: lower quality of SAD calculation for 16x16 vectors
	   ( + 30-50% bitrate, +120% fps )
	   1: fastest encoding 
	   ( + 60-70% bitrate, +150% fps )
	   all bitrates & fps are relative to quality 9, performance is for
	   non-MMX version.
	 */ 

    uint8_t quality;

    uint8_t quant;

/*    
    uint8_t enable_8x8_MV; 
    double frame_rate; 
    uint16_t time_base;  
    uint8_t intra_acdc_pred_disable; 
    uint8_t intra_dc_vlc_thr; 
    uint8_t intra_acdc_pred_disable; 
    uint8_t RVLC_enabled; 
*/
}
MBParam;



/***********************************

Rate Control Parameters

************************************/ 


typedef struct
{
    double quant;
    uint32_t rc_period;
    double target_rate;
    double average_rate;
    double reaction_rate;
    double average_delta;
    double reaction_delta;
    double reaction_ratio;
    uint32_t max_key_interval;
    uint8_t max_quant;
    uint8_t min_quant;
}
RateCtlParam;


typedef struct
{
    int iTextBits;
    int iMvBits;
    float fMvPrevSigma;
    int iMvSum;
    int iMvCount;
}
Statistics;


typedef struct Encoder_s
{
    MBParam mbParam;
    RateCtlParam rateCtlParam;

    int iFrameNum;
    int iMaxKeyInterval;

    Image sCurrent;
	Image sBackup;
    Image sReference;
    Image vInterH;
    Image vInterV;
    Image vInterHV;

    Statistics sStat;
}
Encoder;

#endif
