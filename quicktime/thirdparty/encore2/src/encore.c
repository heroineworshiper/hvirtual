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
 *  encore.c, front-end to encoder
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
 *  22.08.2001 added support for ENC_OPT_ENCODE_EXT encoding mode
 *
 *  Michael Militzer <isibaar@videocoding.de>
 *
 **************************************************************************/


#include "encore2.h"

#include "encoder.h"

int encore(void *handle, int enc_opt, void *param1, void *param2)

{

    Encoder *pEnc = (Encoder *) handle;



    switch (enc_opt)

    {

    case ENC_OPT_INIT:

	return CreateEncoder((ENC_PARAM *) param1);



    case ENC_OPT_RELEASE:

	return FreeEncoder(pEnc);



    case ENC_OPT_ENCODE:

	return EncodeFrame(pEnc, 
		(ENC_FRAME *) param1, 
		(ENC_RESULT *) param2, 
		RC_MODE); 


	
   	case ENC_OPT_ENCODE_VBR:

	return EncodeFrame(pEnc, (ENC_FRAME *) param1, 
		(ENC_RESULT *) param2, 
		VBR_MODE);



	case ENC_OPT_ENCODE_EXT:

	return EncodeFrame(pEnc, (ENC_FRAME *) param1, (ENC_RESULT *) param2, EXT_MODE);


    default:

	return ENC_FAIL;

    }

}

