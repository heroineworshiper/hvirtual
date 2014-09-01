/**************************************************************************
 *
 *  Modifications:
 *
 *  22.08.2001 added support for EXT_MODE encoding mode
 *             support for EXTENDED API
 *  22.08.2001 fixed bug in iDQtab
 *
 *  Michael Militzer <isibaar@videocoding.de>
 *
 **************************************************************************/

#ifndef _ENCORE_ENCODER_H

#define _ENCORE_ENCODER_H

#include "enc_image.h"
#include "encore2.h"
#include "encore_ext.h"

// internal encoder modes:

#define RC_MODE 0
#define VBR_MODE 1
#define EXT_MODE 2

// indicates no quantizer changes in INTRA_Q/INTER_Q modes
#define NO_CHANGE 64

static int DQtab[4] = {
	-1, -2, 1, 2
};

static int iDQtab[5] = {
	1, 0, NO_CHANGE, 2, 3
};

int CreateEncoder(ENC_PARAM * pParam);

int FreeEncoder(Encoder * pEnc);

int EncodeFrame(Encoder * pEnc, ENC_FRAME * pFrame, ENC_RESULT * pResult,

		int enc_mode);



#endif

