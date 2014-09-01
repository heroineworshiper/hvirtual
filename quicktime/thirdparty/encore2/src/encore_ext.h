/**************************************************************************
 *
 *  encore_ext.h
 *
 *  Extended API description
 *  
 *  (C) 2001 Michael Militzer <isibaar@videocoding.de>
 *
 **************************************************************************/

#ifndef _ENCORE_ENCORE_EXT_H
#define _ENCORE_ENCORE_EXT_H

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _ENC_FRAME_EXT_
{
	int  ext_opt;		// specific options for ENC_OPT_ENCODE_EXT encoding mode
	void *quant_array;	// pointer to an array of quantizer values
						// for now: float quant_array[width/16 * height/16]
						// or int quant_aray[width/16 * height/16] in DQUANT-mode
	void *mvs;		    // Pointer to mv_hint_frame_t structure (see mv_hint.h)
}
ENC_FRAME_EXT;

// extensions to encore options (the enc_opt parameter of encore())
#define ENC_OPT_ENCODE_EXT	5

// valid ext_opt flags
// if ext_opt = 0, encoder runs in standard mode with all additional options switched off

#define EXT_OPT_LUMINANCE_MASKING	32		// switch on Lumi Masking
#define EXT_OPT_QUANT_ARRAY			64		// this option indicates the use of quant_array
											// out of ENC_FRAME_EXT
#define EXT_OPT_DQUANT_ARRAY		128		// same with integer-[-2,2] dquant
#define EXT_OPT_MVS_ARRAY			256		// mv_hint_frame_t structure for MVs,
											// like in mv_hint.h

/* additional colorspaces */

#define ENC_CSP_RGB32 	1000
#define ENC_CSP_YVYU	1002


#ifdef __cplusplus
}
#endif

#endif
