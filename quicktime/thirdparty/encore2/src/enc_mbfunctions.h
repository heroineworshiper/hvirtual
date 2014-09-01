/**************************************************************************
 *
 *  Modifications:
 *
 *  16.11.2001 const/uint32_t changes to MBMotionEstComp()
 *  26.08.2001 added inter4v_mode parameter to MBMotionEstComp()
 *
 *  Michael Militzer <isibaar@videocoding.de>
 *
 **************************************************************************/

#ifndef _ENCORE_BLOCK_H
#define _ENCORE_BLOCK_H

#include "enctypes.h"
#include "bitstream.h"



/** MBMotionEstComp.c **/

bool MBMotionEstComp(const MBParam * const pParam,	/* <-- Motion estimation parameters */
		     const uint32_t xpos,		 /* <-- X position of the MB to be searched */
		     const uint32_t ypos,		 /* <-- Y position of the MB to be searched */
		     const Image * const pvRef,		 /* <-- Image, reconstructed during last call to encore() */
		     const Image * const pvInterH,	 /* <-- Same as above, interpolated horizontally */
		     const Image * const pvInterV,	 /* <-- Same as above, interpolated vertically */
		     const Image * const pvInterHV,	 /* <-- Same as above, interpolated both horizontally and vertically */
		     Image * const pCurrent,		 /* --> Compensated reconstructed image */
		     int16_t dct_codes[][64],	 /* --> Motion compensation error */
			 const int inter4v_mode			 /* <-- enc_mode: RC_MODE / VBR_MODE / EXT_MODE */
    );


/** MBPrediction.c **/

void MBPrediction(const MBParam *pParam,	 /* <-- the parameter for ACDC and MV prediction */
		  uint16_t x_pos,		 /* <-- The x position of the MB to be searched */
		  uint16_t y_pos, 		 /* <-- The y position of the MB to be searched */ 
		  uint16_t x_dim,		 /* <-- Number of macroblocks in a row */
		  int16_t qcoeff[][64], 	 /* <-> The quantized DCT coefficients */ 
		  Macroblock *MB_array		 /* <-> the array of all the MB Infomations */
    );


/** MBCoding.c **/

void MBCoding(const MBParam *pParam,	 	 /* <-- the parameter for coding of the bitstream */
	      const Macroblock *pMB, 		 /* <-- Info of the MB to be coded */ 
	      int16_t qcoeff[][64], 		 /* <-- the quantized DCT coefficients */ 
	      Bitstream * bs, 			 /* <-> the bitstream */ 
	      Statistics * pStat		 /* <-> statistical data collected for current frame */
    );


/** MBTransQuant.c **/



void MBTransQuantIntra(const MBParam *pParam,	 
		       const uint16_t x_pos, 		 /* <-- The x position of the MB to be searched */ 
		       const uint16_t y_pos, 		 /* <-- The y position of the MB to be searched */ 
		       int16_t data[][64],	 /* <-> the data of the MB to be coded */ 
		       int16_t qcoeff[][64], 	 /* <-> the quantized DCT coefficients */ 
		       Image * const pCurrent         /* <-> the reconstructed image ( function will update one
   	         							    MB in it with data from data[] ) */
);

uint8_t MBTransQuantInter(const MBParam *pParam, /* <-- the parameter for DCT transformation 
						 						   and Quantization */ 
			   const uint16_t x_pos, 	 /* <-- The x position of the MB to be searched */ 
			   const uint16_t y_pos,	 /* <-- The y position of the MB to be searched */
			   int16_t data[][64], 	 /* <-> the data of the MB to be coded */ 
			   int16_t qcoeff[][64], /* <-> the quantized DCT coefficients */ 
			   Image * const pCurrent		 /* <-> the reconstructed image ( function will
								    update one MB in it with data from data[] ) */
    );

#endif
