/*
 * jcdctmgr.c
 *
 * Copyright (C) 1994-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains the forward-DCT management logic.
 * This code selects a particular DCT implementation to be used,
 * and it performs related housekeeping chores including coefficient
 * quantization.
 */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jdct.h"		/* Private declarations for DCT subsystem */
#include "cpu_accel.h"
#include "mmx.h"

/* Private subobject for this module */

typedef enum 
{ QUANT_INT,					/* Integer quantiser requiring scaling */
  QUANT_INT16,					/* Fast unscaled integer quantiser */
  QUANT_FLOAT32					/* Fast unscaler float quantiser  */
} quant_kind;

typedef struct {
  struct jpeg_forward_dct pub;	/* public fields */

	/* Pointer to the DCT routine actually in use */
	forward_DCT_method_ptr do_dct;
	float32_quant_method_ptr do_float32_quant;
	quant_kind fast_quantiser;
	/* The actual post-DCT divisors --- not identical to the quant table
	 * entries, because of scaling (especially for an unnormalized DCT).
	 * Each table is given in normal array order.
	 */
	DCTELEM * divisors[NUM_QUANT_TBLS];
	UINT16 *int16_divisors[NUM_QUANT_TBLS];
	FLOAT32 *float32_divisors[NUM_QUANT_TBLS];
	unsigned int shift;
#ifdef DCT_FLOAT_SUPPORTED
	/* Same as above for the floating-point case. */
	float_DCT_method_ptr do_float_dct;
	FAST_FLOAT * float_divisors[NUM_QUANT_TBLS];
#endif
} my_fdct_controller;

typedef my_fdct_controller * my_fdct_ptr;


METHODDEF(UINT8*)
	simd_aligned_smallbuf( j_compress_ptr cinfo,
						   size_t bufsize )
{
	UINT8 *rawbuf = (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, 
												JPOOL_IMAGE,
												bufsize+SIMD_ALIGN );
	unsigned int odd_bytes = ((size_t)rawbuf)%SIMD_ALIGN;
	return odd_bytes == 0 ? rawbuf : rawbuf + SIMD_ALIGN-odd_bytes;
	
}
/*
 * Initialize for a processing pass.
 * Verify that all referenced Q-tables are present, and set up
 * the divisor table for each one.
 * In the current implementation, DCT of all components is done during
 * the first pass, even if only some components will be output in the
 * first scan.  Hence all components should be examined here.
 */

METHODDEF(void)
start_pass_fdctmgr (j_compress_ptr cinfo)
{
	my_fdct_ptr fdct = (my_fdct_ptr) cinfo->fdct;
	int ci, qtblno, i;
	jpeg_component_info *compptr;
	JQUANT_TBL * qtbl;
	DCTELEM * dtbl;
	UINT16 *idtbl;
	FAST_FLOAT * fdtbl;
  

	for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
		 ci++, compptr++) {
		qtblno = compptr->quant_tbl_no;
		/* Make sure specified quantization table is present */
		if (qtblno < 0 || qtblno >= NUM_QUANT_TBLS ||
			cinfo->quant_tbl_ptrs[qtblno] == NULL)
			ERREXIT1(cinfo, JERR_NO_QUANT_TABLE, qtblno);
		qtbl = cinfo->quant_tbl_ptrs[qtblno];
		/* Compute divisors for this quant table */
		/* We may do this more than once for same table, but it's not a big deal */
		switch (cinfo->dct_method) {
#ifdef DCT_ISLOW_SUPPORTED
		case JDCT_ISLOW:
			/* For LL&M IDCT method, divisors are equal to raw quantization
			 * coefficients multiplied by 8 (to counteract scaling).
			 */
			if (fdct->divisors[qtblno] == NULL) {
				fdct->divisors[qtblno] = (DCTELEM *)
					(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
												DCTSIZE2 * SIZEOF(DCTELEM));
			}

			dtbl = fdct->divisors[qtblno];
			for (i = 0; i < DCTSIZE2; i++) {
				dtbl[i] = ((DCTELEM) qtbl->quantval[i]) << 3;
			}
			break;
#endif
#ifdef DCT_IFAST_SUPPORTED
		case JDCT_IFAST:
		{
			/* For AA&N IDCT method, divisors are equal to quantization
			 * coefficients scaled by scalefactor[row]*scalefactor[col], where
			 *   scalefactor[0] = 1
			 *   scalefactor[k] = cos(k*PI/16) * sqrt(2)    for k=1..7
			 * We apply a further scale factor of 8.
			 */
#define CONST_BITS 14
			static const INT16 aanscales[DCTSIZE2] = {
				/* precomputed values scaled up by 14 bits */
				16384, 22725, 21407, 19266, 16384, 12873,  8867,  4520,
				22725, 31521, 29692, 26722, 22725, 17855, 12299,  6270,
				21407, 29692, 27969, 25172, 21407, 16819, 11585,  5906,
				19266, 26722, 25172, 22654, 19266, 15137, 10426,  5315,
				16384, 22725, 21407, 19266, 16384, 12873,  8867,  4520,
				12873, 17855, 16819, 15137, 12873, 10114,  6967,  3552,
				8867, 12299, 11585, 10426,  8867,  6967,  4799,  2446,
				4520,  6270,  5906,  5315,  4520,  3552,  2446,  1247
			};
			SHIFT_TEMPS;

			/* Allocate arrays of quantisation factors for the various
			 * kinds of fast quantiser
			 */


			switch( fdct->fast_quantiser)
			{
			case QUANT_INT16 :
				if (fdct->int16_divisors[qtblno] == NULL) {
					fdct->int16_divisors[qtblno] = (INT16 *)
						simd_aligned_smallbuf( cinfo, DCTSIZE2 * SIZEOF(INT16));
				}
			case QUANT_INT :
				if (fdct->divisors[qtblno] == NULL) {
					fdct->divisors[qtblno] = (DCTELEM *)
						simd_aligned_smallbuf( cinfo, DCTSIZE2 * SIZEOF(DCTELEM));
				}
				break;
			case QUANT_FLOAT32 :
				if (fdct->float32_divisors[qtblno] == NULL) {
					fdct->float32_divisors[qtblno] = (FLOAT32 *)
						simd_aligned_smallbuf( cinfo, DCTSIZE2 * SIZEOF(FLOAT32) );
				}
				break;
			}

			dtbl = fdct->divisors[qtblno];
			idtbl = fdct->int16_divisors[qtblno];
			fdtbl = fdct->float32_divisors[qtblno];
			switch( fdct->fast_quantiser )
			{
			case QUANT_INT :
				for (i = 0; i < DCTSIZE2; i++) 
				{
					dtbl[i] = (DCTELEM)
						DESCALE(MULTIPLY16V16((INT32) qtbl->quantval[i],
											  (INT32) aanscales[i]),
								CONST_BITS-3);
				}
				break;
			case QUANT_INT16 :
			{
				int overflow = 0;
				unsigned int scalebase = 1<<17;
				unsigned int shift = 4;
				unsigned int divprod;
				for (i = 0; i < DCTSIZE2; i++) 
				{
				restart:
					divprod = scalebase/qtbl->quantval[i];
					/*
					  Rescale product coefficients if quantisation is too
					  low
					*/
					
					if( divprod >= (1U<<15) )
					{
						i = 0;
						--shift;
						scalebase = (scalebase >> 1);
						goto restart;
					}
					idtbl[i] = (UINT16)divprod;
					dtbl[i] = qtbl->quantval[i];
				}
				fdct->shift = shift;
				break;
			}
			case QUANT_FLOAT32 :
				for (i = 0; i < DCTSIZE2; i++) 
				{
					fdtbl[i] = 1.0f/((FLOAT32)(qtbl->quantval[i]<<3));
				}
				break;
			}
		}
		break;
#endif
#ifdef DCT_FLOAT_SUPPORTED
		case JDCT_FLOAT:
		{
			/* For float AA&N IDCT method, divisors are equal to quantization
			 * coefficients scaled by scalefactor[row]*scalefactor[col], where
			 *   scalefactor[0] = 1
			 *   scalefactor[k] = cos(k*PI/16) * sqrt(2)    for k=1..7
			 * We apply a further scale factor of 8.
			 * What's actually stored is 1/divisor so that the inner loop can
			 * use a multiplication rather than a division.
			 */
			FAST_FLOAT * fdtbl;
			int row, col;
			static const double aanscalefactor[DCTSIZE] = {
				1.0, 1.387039845, 1.306562965, 1.175875602,
				1.0, 0.785694958, 0.541196100, 0.275899379
			};

			if (fdct->float_divisors[qtblno] == NULL) {
				fdct->float_divisors[qtblno] = (FAST_FLOAT *)
					(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
												DCTSIZE2 * SIZEOF(FAST_FLOAT));
			}
			fdtbl = fdct->float_divisors[qtblno];
			i = 0;
			for (row = 0; row < DCTSIZE; row++) {
				for (col = 0; col < DCTSIZE; col++) {
					fdtbl[i] = (FAST_FLOAT)
						(1.0 / (((double) qtbl->quantval[i] *
								 aanscalefactor[row] * aanscalefactor[col] * 8.0)));
					i++;
				}
			}
		}
		break;
#endif
		default:
			ERREXIT(cinfo, JERR_NOT_COMPILED);
			break;
		}
	}
}

static INT16 int16_centrejsample[8] ATTR_ALIGN(8) = 
{ CENTERJSAMPLE, CENTERJSAMPLE,CENTERJSAMPLE, CENTERJSAMPLE,
  CENTERJSAMPLE, CENTERJSAMPLE,CENTERJSAMPLE, CENTERJSAMPLE
};


METHODDEF(void)
jcquant_int16( DCTELEM *workspace, INT16 *output_ptr,  
			   DCTELEM *divisors, INT16 *idivisors)
{ 
	register int temp;
	register int i;

	for (i = 0; i < DCTSIZE2; i++) {
		temp = workspace[i];
		/* Divide the coefficient value by qval, ensuring proper rounding.
		 * Since C does not specify the direction of rounding for negative
		 * quotients, we have to force the dividend positive for portability.
		 */

		if (temp < 0) {
			temp = -temp;
			temp += divisors[i]>>1;	/* for rounding */
			temp *= idivisors[i];
			temp = -(temp+(1<<(16+2))>>(16+3));
		} else {
			temp += divisors[i]>>1;	/* for rounding */
			temp *= idivisors[i];
			temp = (temp+(1<<(16+2)))>>(16+3);
			
		}
		output_ptr[i] = (JCOEF) temp;
	}
}


METHODDEF(void)
jcquant_int( DCTELEM *workspace, INT16 *output_ptr,  DCTELEM *divisors )
{ 
	register DCTELEM temp, qval;
	register int i;
	for (i = 0; i < DCTSIZE2; i++) {
		qval = divisors[i];
		temp = workspace[i];
		/* Divide the coefficient value by qval, ensuring proper rounding.
		 * Since C does not specify the direction of rounding for negative
		 * quotients, we have to force the dividend positive for portability.
		 *
		 * In most files, at least half of the output values will be zero
		 * (at default quantization settings, more like three-quarters...)
		 * so we should ensure that this case is fast.  On many machines,
		 * a comparison is enough cheaper than a divide to make a special test
		 * a win.  Since both inputs will be nonnegative, we need only test
		 * for a < b to discover whether a/b is 0.
		 * If your machine's division is fast enough, define FAST_DIVIDE.
		 */

#ifdef FAST_DIVIDE
#define DIVIDE_BY(a,b)	a /= b
#else
#define DIVIDE_BY(a,b)	if (a >= b) a /= b; else a = 0
#endif
		if (temp < 0) {
			temp = -temp;
			temp += qval>>1;	/* for rounding */
			DIVIDE_BY(temp, qval);
			temp = -temp;
		} else {
			temp += qval>>1;	/* for rounding */
			DIVIDE_BY(temp, qval);
		}
		output_ptr[i] = (JCOEF) temp;
	}
}

/*
 * Perform forward DCT on one or more blocks of a component.
 *
 * The input samples are taken from the sample_data[] array starting at
 * position start_row/start_col, and moving to the right for any additional
 * blocks. The quantized coefficients are returned in coef_blocks[].
 */


METHODDEF(void)
forward_DCT (j_compress_ptr cinfo, jpeg_component_info * compptr,
	     JSAMPARRAY sample_data, JBLOCKROW coef_blocks,
	     JDIMENSION start_row, JDIMENSION start_col,
	     JDIMENSION num_blocks)
/* This version is used for integer DCT implementations. */
{
  /* This routine is heavily used, so it's worth coding it tightly. */
  my_fdct_ptr fdct = (my_fdct_ptr) cinfo->fdct;
  forward_DCT_method_ptr do_dct = fdct->do_dct;
  DCTELEM * divisors = fdct->divisors[compptr->quant_tbl_no];
  DCTELEM workspace[DCTSIZE2];	/* work area for FDCT subroutine */
  JDIMENSION bi;

  sample_data += start_row;	/* fold in the vertical offset once */

  for (bi = 0; bi < num_blocks; bi++, start_col += DCTSIZE) {
    /* Load data into workspace, applying unsigned->signed conversion */
    { register DCTELEM *workspaceptr;
      register JSAMPROW elemptr;
      register int elemr;

      workspaceptr = workspace;
      for (elemr = 0; elemr < DCTSIZE; elemr++) 
	  {
		  elemptr = sample_data[elemr] + start_col;
#if DCTSIZE == 8		/* unroll the inner loop */
		  workspaceptr[0] = GETJSAMPLE(elemptr[0]) - CENTERJSAMPLE;
		  workspaceptr[1] = GETJSAMPLE(elemptr[1]) - CENTERJSAMPLE;
		  workspaceptr[2] = GETJSAMPLE(elemptr[2]) - CENTERJSAMPLE;
		  workspaceptr[3] = GETJSAMPLE(elemptr[3]) - CENTERJSAMPLE;
		  workspaceptr[4] = GETJSAMPLE(elemptr[4]) - CENTERJSAMPLE;
		  workspaceptr[5] = GETJSAMPLE(elemptr[5]) - CENTERJSAMPLE;
		  workspaceptr[6] = GETJSAMPLE(elemptr[6]) - CENTERJSAMPLE;
		  workspaceptr[7] = GETJSAMPLE(elemptr[7]) - CENTERJSAMPLE;
		  workspaceptr += 8; elemptr += 8;
#else
		  { 
			  register int elemc;
			  for (elemc = DCTSIZE; elemc > 0; elemc--) {
				  *workspaceptr++ = GETJSAMPLE(*elemptr++) - CENTERJSAMPLE;
			  }
		  }
#endif
      }
    }

    /* Perform the DCT */
    (*do_dct) (workspace);

	jcquant_int( workspace, coef_blocks[bi], 
				 fdct->divisors[compptr->quant_tbl_no] );
  }
}


#if DCTSIZE == 8 && defined(HAVE_MMX_ATT_MNEMONICS)	
METHODDEF(void)
	forward_DCT_x86float32 (j_compress_ptr cinfo, jpeg_component_info * compptr,
							JSAMPARRAY sample_data, JBLOCKROW coef_blocks,
							JDIMENSION start_row, JDIMENSION start_col,
							JDIMENSION num_blocks)
/* This version is used for integer DCT implementations. */
{
	/* This routine is heavily used, so it's worth coding it tightly. */
	my_fdct_ptr fdct = (my_fdct_ptr) cinfo->fdct;
	forward_DCT_method_ptr do_dct = fdct->do_dct;
	float32_quant_method_ptr do_quant = fdct->do_float32_quant;
	DCTELEM * divisors = fdct->divisors[compptr->quant_tbl_no];
	DCTELEM workspace[DCTSIZE2] ATTR_ALIGN(SIMD_ALIGN);	/* work area for FDCT subroutine */
	JDIMENSION bi;

	sample_data += start_row;	/* fold in the vertical offset once */

	for (bi = 0; bi < num_blocks; bi++, start_col += DCTSIZE) 
	{
		register DCTELEM *workspaceptr;
		register JSAMPROW elemptr;
		register int elemr;

		workspaceptr = workspace;
		elemr = 0;
		elemptr = sample_data[elemr] + start_col;
	  
		pxor_r2r(mm7,mm7);
		movq_m2r( *(mmx_t*)&int16_centrejsample, mm6 );
		while(elemr < DCTSIZE) 
		{
			movq_m2r( *(mmx_t*)elemptr, mm0 );
			elemptr = sample_data[elemr+1] + start_col;
			movq_m2r( *(mmx_t*)elemptr, mm1 );

			movq_r2r( mm0, mm2 );
			punpcklbw_r2r( mm7, mm0 );
			movq_r2r( mm1, mm3 );
			punpcklbw_r2r( mm7, mm1 );
			psubw_r2r( mm6, mm0 );
			psubw_r2r( mm6, mm1 );
			elemr += 2;
			punpckhbw_r2r( mm7, mm2 );
			punpckhbw_r2r( mm7, mm3 );
			elemptr = sample_data[elemr] + start_col;
			psubw_r2r( mm6, mm2 );
			psubw_r2r( mm6, mm3 );

			movq_r2m( mm0, *(mmx_t*)(&workspaceptr[0]) );
			movq_r2m( mm2, *(mmx_t*)(&workspaceptr[4]) );
			movq_r2m( mm1, *(mmx_t*)(&workspaceptr[8]) );
			movq_r2m( mm3, *(mmx_t*)(&workspaceptr[12]) );
			workspaceptr += 16;

		}
		emms();

		/* Perform the DCT */
		(*do_dct)(workspace);
		
		(*do_quant)( workspace, coef_blocks[bi], 
					 fdct->float32_divisors[compptr->quant_tbl_no] );
	}
}


METHODDEF(void)
	forward_DCT_mmx (j_compress_ptr cinfo, jpeg_component_info * compptr,
						  JSAMPARRAY sample_data, JBLOCKROW coef_blocks,
						  JDIMENSION start_row, JDIMENSION start_col,
						  JDIMENSION num_blocks)
/* This version is used for integer DCT implementations. */
{
	/* This routine is heavily used, so it's worth coding it tightly. */
	my_fdct_ptr fdct = (my_fdct_ptr) cinfo->fdct;
	forward_DCT_method_ptr do_dct = fdct->do_dct;
	float32_quant_method_ptr do_quant = fdct->do_float32_quant;
	DCTELEM workspace[DCTSIZE2] ATTR_ALIGN(SIMD_ALIGN);	/* work area for FDCT subroutine */
	JDIMENSION bi;
	int i;
	sample_data += start_row;	/* fold in the vertical offset once */

	for (bi = 0; bi < num_blocks; bi++, start_col += DCTSIZE) 
	{
		register DCTELEM *workspaceptr;
		register JSAMPROW elemptr;
		register int elemr;

		workspaceptr = workspace;
		elemr = 0;
		elemptr = sample_data[elemr] + start_col;
	  
		pxor_r2r(mm7,mm7);
		movq_m2r( *(mmx_t*)&int16_centrejsample, mm6 );
		while(elemr < DCTSIZE) 
		{
			movq_m2r( *(mmx_t*)elemptr, mm0 );
			elemptr = sample_data[elemr+1] + start_col;
			movq_m2r( *(mmx_t*)elemptr, mm1 );

			movq_r2r( mm0, mm2 );
			punpcklbw_r2r( mm7, mm0 );
			movq_r2r( mm1, mm3 );
			punpcklbw_r2r( mm7, mm1 );
			psubw_r2r( mm6, mm0 );
			psubw_r2r( mm6, mm1 );
			elemr += 2;
			punpckhbw_r2r( mm7, mm2 );
			punpckhbw_r2r( mm7, mm3 );
			elemptr = sample_data[elemr] + start_col;
			psubw_r2r( mm6, mm2 );
			psubw_r2r( mm6, mm3 );

			movq_r2m( mm0, *(mmx_t*)(&workspaceptr[0]) );
			movq_r2m( mm2, *(mmx_t*)(&workspaceptr[4]) );
			movq_r2m( mm1, *(mmx_t*)(&workspaceptr[8]) );
			movq_r2m( mm3, *(mmx_t*)(&workspaceptr[12]) );
			workspaceptr += 16;

		}
		emms();

		/* Perform the DCT */
		(*do_dct)(workspace);
		
		jcquant_mmx( workspace,
					 coef_blocks[bi],  
					 fdct->divisors[compptr->quant_tbl_no],
					 fdct->int16_divisors[compptr->quant_tbl_no],
					 fdct->shift
			);

	}
}

#endif
#ifdef DCT_FLOAT_SUPPORTED

METHODDEF(void)
forward_DCT_float (j_compress_ptr cinfo, jpeg_component_info * compptr,
		   JSAMPARRAY sample_data, JBLOCKROW coef_blocks,
		   JDIMENSION start_row, JDIMENSION start_col,
		   JDIMENSION num_blocks)
/* This version is used for floating-point DCT implementations. */
{
  /* This routine is heavily used, so it's worth coding it tightly. */
  my_fdct_ptr fdct = (my_fdct_ptr) cinfo->fdct;
  float_DCT_method_ptr do_dct = fdct->do_float_dct;
  FAST_FLOAT * divisors = fdct->float_divisors[compptr->quant_tbl_no];
  FAST_FLOAT workspace[DCTSIZE2]; /* work area for FDCT subroutine */
  JDIMENSION bi;

  sample_data += start_row;	/* fold in the vertical offset once */

  for (bi = 0; bi < num_blocks; bi++, start_col += DCTSIZE) {
    /* Load data into workspace, applying unsigned->signed conversion */
    { register FAST_FLOAT *workspaceptr;
      register JSAMPROW elemptr;
      register int elemr;

      workspaceptr = workspace;
      for (elemr = 0; elemr < DCTSIZE; elemr++) {
	elemptr = sample_data[elemr] + start_col;
#if DCTSIZE == 8		/* unroll the inner loop */
	*workspaceptr++ = (FAST_FLOAT)(GETJSAMPLE(*elemptr++) - CENTERJSAMPLE);
	*workspaceptr++ = (FAST_FLOAT)(GETJSAMPLE(*elemptr++) - CENTERJSAMPLE);
	*workspaceptr++ = (FAST_FLOAT)(GETJSAMPLE(*elemptr++) - CENTERJSAMPLE);
	*workspaceptr++ = (FAST_FLOAT)(GETJSAMPLE(*elemptr++) - CENTERJSAMPLE);
	*workspaceptr++ = (FAST_FLOAT)(GETJSAMPLE(*elemptr++) - CENTERJSAMPLE);
	*workspaceptr++ = (FAST_FLOAT)(GETJSAMPLE(*elemptr++) - CENTERJSAMPLE);
	*workspaceptr++ = (FAST_FLOAT)(GETJSAMPLE(*elemptr++) - CENTERJSAMPLE);
	*workspaceptr++ = (FAST_FLOAT)(GETJSAMPLE(*elemptr++) - CENTERJSAMPLE);
#else
	{ register int elemc;
	  for (elemc = DCTSIZE; elemc > 0; elemc--) {
	    *workspaceptr++ = (FAST_FLOAT)
	      (GETJSAMPLE(*elemptr++) - CENTERJSAMPLE);
	  }
	}
#endif
      }
    }

    /* Perform the DCT */
    (*do_dct) (workspace);

    /* Quantize/descale the coefficients, and store into coef_blocks[] */
    { register FAST_FLOAT temp;
      register int i;
      register JCOEFPTR output_ptr = coef_blocks[bi];

      for (i = 0; i < DCTSIZE2; i++) {
	/* Apply the quantization and scaling factor */
	temp = workspace[i] * divisors[i];
	/* Round to nearest integer.
	 * Since C does not specify the direction of rounding for negative
	 * quotients, we have to force the dividend positive for portability.
	 * The maximum coefficient size is +-16K (for 12-bit data), so this
	 * code should work for either 16-bit or 32-bit ints.
	 */
	output_ptr[i] = (JCOEF) ((int) (temp + (FAST_FLOAT) 16384.5) - 16384);
      }
    }
  }
}

#endif /* DCT_FLOAT_SUPPORTED */


/*
 * Initialize FDCT manager.
 */

GLOBAL(void)
jinit_forward_dct (j_compress_ptr cinfo)
{
  my_fdct_ptr fdct;
  int i;

  fdct = (my_fdct_ptr)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				SIZEOF(my_fdct_controller));
  cinfo->fdct = (struct jpeg_forward_dct *) fdct;
  fdct->pub.start_pass = start_pass_fdctmgr;

  switch (cinfo->dct_method) {
#ifdef DCT_ISLOW_SUPPORTED
  case JDCT_ISLOW:
    fdct->pub.forward_DCT = forward_DCT;
    fdct->do_dct = jpeg_fdct_islow;
    break;
#endif
#ifdef DCT_IFAST_SUPPORTED
  case JDCT_IFAST:

  {
#if defined(HAVE_MMX_ATT_MNEMONICS)	  
	  int cpu_flags =  cpu_accel();
	  if( cpu_flags & ACCEL_X86_MMX )
	  {

		  fdct->do_dct = jpeg_fdct_ifast_mmx;
		  if( cpu_flags & ACCEL_X86_SSE )
		  {
			  fdct->fast_quantiser = QUANT_FLOAT32;
			  fdct->pub.forward_DCT = forward_DCT_x86float32;
			  fdct->do_float32_quant = jcquant_sse;
		  }
		  else if( cpu_flags & ACCEL_X86_3DNOW )
		  {
			  fdct->fast_quantiser = QUANT_FLOAT32;
			  fdct->pub.forward_DCT = forward_DCT_x86float32;
			  fdct->do_float32_quant = jcquant_3dnow;
		  }
		  else
		  {
			  fdct->fast_quantiser = QUANT_INT16;
			  fdct->pub.forward_DCT = forward_DCT_mmx;
			  fdct->do_float32_quant = NULL;
		  }
	  }
	  else
#endif
	  {
		  fdct->fast_quantiser = QUANT_INT;
		  fdct->pub.forward_DCT = forward_DCT;
		  fdct->do_dct = jpeg_fdct_ifast;
		  fdct->do_float32_quant = NULL;
	  }

	  break;
  }

#endif
#ifdef DCT_FLOAT_SUPPORTED
  case JDCT_FLOAT:
    fdct->pub.forward_DCT = forward_DCT_float;
    fdct->do_float_dct = jpeg_fdct_float;
    break;
#endif
  default:
    ERREXIT(cinfo, JERR_NOT_COMPILED);
    break;
  }

  /* Mark divisor tables unallocated */
  for (i = 0; i < NUM_QUANT_TBLS; i++) {
    fdct->divisors[i] = NULL;
    fdct->int16_divisors[i] = NULL;
    fdct->float32_divisors[i] = NULL;
#ifdef DCT_FLOAT_SUPPORTED
    fdct->float_divisors[i] = NULL;
#endif
  }
}
