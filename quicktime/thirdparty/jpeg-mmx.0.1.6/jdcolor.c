/*
 * jdcolor.c
 *
 * Copyright (C) 1991-1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains output colorspace conversion routines.
 */

//#include <asm/msr.h>
#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"


/* Private subobject */

typedef struct {
  struct jpeg_color_deconverter pub; /* public fields */

  /* Private state for YCC->RGB conversion */
  int * Cr_r_tab;		/* => table for Cr to R conversion */
  int * Cb_b_tab;		/* => table for Cb to B conversion */
  INT32 * Cr_g_tab;		/* => table for Cr to G conversion */
  INT32 * Cb_g_tab;		/* => table for Cb to G conversion */
} my_color_deconverter;

typedef my_color_deconverter * my_cconvert_ptr;


/**************** YCbCr -> RGB conversion: most common case **************/

/*
 * YCbCr is defined per CCIR 601-1, except that Cb and Cr are
 * normalized to the range 0..MAXJSAMPLE rather than -0.5 .. 0.5.
 * The conversion equations to be implemented are therefore
 *	R = Y                + 1.40200 * Cr
 *	G = Y - 0.34414 * Cb - 0.71414 * Cr
 *	B = Y + 1.77200 * Cb
 * where Cb and Cr represent the incoming values less CENTERJSAMPLE.
 * (These numbers are derived from TIFF 6.0 section 21, dated 3-June-92.)
 *
 * To avoid floating-point arithmetic, we represent the fractional constants
 * as integers scaled up by 2^16 (about 4 digits precision); we have to divide
 * the products by 2^16, with appropriate rounding, to get the correct answer.
 * Notice that Y, being an integral input, does not contribute any fraction
 * so it need not participate in the rounding.
 *
 * For even more speed, we avoid doing any multiplications in the inner loop
 * by precalculating the constants times Cb and Cr for all possible values.
 * For 8-bit JSAMPLEs this is very reasonable (only 256 entries per table);
 * for 12-bit samples it is still acceptable.  It's not very reasonable for
 * 16-bit samples, but if you want lossless storage you shouldn't be changing
 * colorspace anyway.
 * The Cr=>R and Cb=>B values can be rounded to integers in advance; the
 * values for the G calculation are left scaled up, since we must add them
 * together before rounding.
 */

#define SCALEBITS	16	/* speediest right-shift on some machines */
#define ONE_HALF	((INT32) 1 << (SCALEBITS-1))
#define FIX(x)		((INT32) ((x) * (1L<<SCALEBITS) + 0.5))


/*
 * Initialize tables for YCC->RGB colorspace conversion.
 */

LOCAL(void)
build_ycc_rgb_table (j_decompress_ptr cinfo)
{
  my_cconvert_ptr cconvert = (my_cconvert_ptr) cinfo->cconvert;
  int i;
  INT32 x;
  SHIFT_TEMPS;

  cconvert->Cr_r_tab = (int *)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				(MAXJSAMPLE+1) * SIZEOF(int));
  cconvert->Cb_b_tab = (int *)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				(MAXJSAMPLE+1) * SIZEOF(int));
  cconvert->Cr_g_tab = (INT32 *)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				(MAXJSAMPLE+1) * SIZEOF(INT32));
  cconvert->Cb_g_tab = (INT32 *)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				(MAXJSAMPLE+1) * SIZEOF(INT32));


  /* not needed in MMX YUV->RGB */
  for (i = 0, x = -CENTERJSAMPLE; i <= MAXJSAMPLE; i++, x++) {
    /* i is the actual input pixel value, in the range 0..MAXJSAMPLE */
    /* The Cb or Cr value we are thinking of is x = i - CENTERJSAMPLE */
    /* Cr=>R value is nearest int to 1.40200 * x */
    cconvert->Cr_r_tab[i] = (int)
      RIGHT_SHIFT(FIX(1.40200) * x + ONE_HALF, SCALEBITS);
    /* Cb=>B value is nearest int to 1.77200 * x */
    cconvert->Cb_b_tab[i] = (int) 
      RIGHT_SHIFT(FIX(1.77200) * x + ONE_HALF, SCALEBITS);
    /* Cr=>G value is scaled-up -0.71414 * x */ 
   cconvert->Cr_g_tab[i] = (- FIX(0.71414)) * x; 
   /* Cb=>G value is scaled-up -0.34414 * x */
   /* We also add in ONE_HALF so that need not do it in inner loop */
   cconvert->Cb_g_tab[i] = (- FIX(0.34414)) * x + ONE_HALF; 
  }
} 


/*
 * Convert some rows of samples to the output colorspace.
 *
 * Note that we change from noninterleaved, one-plane-per-component format
 * to interleaved-pixel format.  The output buffer is therefore three times
 * as wide as the input buffer.
 * A starting row offset is provided only for the input buffer.  The caller
 * can easily adjust the passed output_buf value to accommodate any row
 * offset required on that side.
 */

#if defined(HAVE_MMX_INTEL_MNEMONICS) || defined(HAVE_MMX_ATT_MNEMONICS)
#if defined(__GNUC__)
#define int64 unsigned long long
#endif

#if defined(HAVE_MMX_INTEL_MNEMONICS)
static const int64 bpte0 = 0x0080008000800080; // 128
static const int64 bpte1 = 0x7168e9f97168e9f9; // for cb (Cb/b, Cb/g, Cb/b, Cb/g)
static const int64 bpte2 = 0xd21a59bad21a59ba; // for cr (Cr/g, Cr/r, Cr/g, Cr/r)
#else
static const int64 te0 = 0x0200020002000200LL; // -128 << 2
static const int64 te1 = 0xe9fa7168e9fa7168LL; // for cb
static const int64 te2 = 0x59bad24d59bad24dLL; // for cr
#endif
//static const int64 te2 = 0x59ba524b59ba524b; // for cr
/* How to calculate the constants (see constants from above for YCbCr->RGB):
   trunc(-0.34414*16384) << 16 + trunc(1.772 * 16348) || mind that negative numbers are in 2-complement form (2^32+x+1) */

/* *	R = Y                + 1.40200 * Cr
 *	G = Y - 0.34414 * Cb - 0.71414 * Cr
 *	B = Y + 1.77200 * Cb
 */
METHODDEF(void)
ycc_rgb_convert_mmx (j_decompress_ptr cinfo,
		     JSAMPIMAGE input_buf, JDIMENSION input_row,
		     JSAMPARRAY output_buf, int num_rows)
{
  INT32 y, cb, cr;
  int temp;
  JSAMPROW outptr;
  JSAMPROW inptr0, inptr1, inptr2;
  JDIMENSION col;
  JDIMENSION num_cols = cinfo->output_width;

  while (--num_rows >= 0) 
  {
    inptr0 = input_buf[0][input_row];
    inptr1 = input_buf[1][input_row];
    inptr2 = input_buf[2][input_row];
    input_row++;
    outptr = *output_buf++;
    num_cols/=4;    
	for (col = 0; col < num_cols; col++) 
	{

#if defined(HAVE_MMX_INTEL_MNEMONICS)
// implemented by Brian Potetz <bpotetz@cs.cmu.edu> - thanks for that ! /gz :-) 
// #error "jdcolor's MMX routines haven't been converted to INTEL assembler yet - contact JPEGlib/MMX"
    _asm {
      // :"=m"(outptr[0])
      // :"m"(inptr0),"m"(inptr1),"m"(inptr2) //y cb cr
      // :"eax", "ebx", "ecx", "st"
      mov eax, inptr0     // mov %1, %%eax
      mov ebx, inptr1     // mov %2, %%ebx
      mov ecx, inptr2     // mov %3, %%ecx
      mov edx, outptr     // -------------              keep output pointer in register.
      movd mm0, [eax]     // movd (%%eax),%%mm0         mm0: 0 0 0 0 y3 y2 y1 y0 - 8 bit
      movd mm1, [ebx]     // movd (%%ebx),%%mm1         mm1: 0 0 0 0 cb3 cb2 cb1 cb0
      movd mm2, [ecx]     // movd (%%ecx),%%mm2         mm2: 0 0 0 0 cr3 cr2 cr1 cr0

      pxor mm7, mm7       // pxor %%mm7,%%mm7           mm7 = 0
      punpcklbw mm0, mm7  // punpcklbw %%mm7,%%mm0      mm0: y3 y2 y1 y0 - expand to 16 bit
      punpcklbw mm1, mm7  // punpcklbw %%mm7,%%mm1      mm1: cb3 cb2 cb1 cb0
      punpcklbw mm2, mm7  // punpcklbw %%mm7,%%mm2      mm2: cr3 cr2 cr1 cr0
      psubw mm1, bpte0    // psubw te0,%%mm1            minus 128 for cb and cr
      psubw mm2, bpte0    // psubw te0,%%mm2
      psllw mm1, 2        // psllw $2,%%mm1             shift left 2 bits for Cr and Cb to fit the mult constants
      psllw mm2, 2        // psllw $2,%%mm2

      //-------------------------------------
      // prepare for RGB 1 & 0
      movq mm3, mm1       // movq %%mm1,%%mm3           mm3_16: cb3 cb2 cb1 cb0
      movq mm4, mm2       // movq %%mm2,%%mm4           mm4_16: cr3 cr2 cr1 cr0
      punpcklwd mm3, mm3  // punpcklwd %%mm3,%%mm3      expand to 32 bit: mm3: cb1 cb1 cb0 cb0
      punpcklwd mm4, mm4  // punpcklwd %%mm4,%%mm4      mm4: cr1 cr1 cr0 cr0

      // Y    Y     Y    Y
      // CD*b CB*g  0    0
      // 0    CR*g  CR*r 0
      //------------------
      // B    G     R

      // Multiply in the constants:
      pmulhw mm3, bpte1   // pmulhw te1,%%mm3           mm3: cb1/b cb1/g cb0/b cb0/g
      pmulhw mm4, bpte2   // pmulhw te2,%%mm4           mm4: cr1/g cb1/r cr0/g cr0/r

      movq mm5, mm0       // movq %%mm0,%%mm5           mm5: y3 y2 y1 y0
      punpcklwd mm5, mm5  // punpcklwd %%mm5,%%mm5      expand to 32 bit: y1 y1 y0 y0
      movq mm6, mm5       // movq %%mm5,%%mm6           mm6: y1 y1 y0 y0
      punpcklwd mm5, mm5  // punpcklwd %%mm5,%%mm5      mm5: y0 y0 y0 y0
      punpckhwd mm6, mm6  // punpckhwd %%mm6,%%mm6      mm6: y1 y1 y1 y1

      // RGB 0
      movq mm7, mm3       // movq %%mm3,%%mm7           mm7: cb1/g cb1/b cb0/g cb0/b
      psllq mm7, 32       // psllq $32,%%mm7            shift left 32 bits: mm7: cb0/b cb0/g 0 0
      paddw mm5, mm7      // paddw %%mm7,%%mm5          add: mm7: y+cb
      movq mm7, mm4       // movq %%mm4,%%mm7           mm7 = cr1 cr1 cr0 cr0
      psllq mm7, 32       // psllq $32,%%mm7            shift left 32 bits: mm7: cr0/g cr0/r 0 0
      psrlq mm7, 16       // psrlq $16,%%mm7            mm7 = 0 cr0/g cr0/r 0
      paddw mm5, mm7      // paddw %%mm7,%%mm5          y+cb+cr->mm5= r g b ?

      // RGB 1
      psrlq mm4, 32       // psrlq $32,%%mm4            mm4: 0 0 cr1/g cr1/r
      paddw mm6, mm4      // paddw %%mm4,%%mm6          y+cr
      psrlq mm3, 32       // psrlq $32,%%mm3            mm3: 0 0 cb1/b cb1/g
      psllq mm3, 16       // psllq $16,%%mm4            mm4: 0 cr1/b cr1/g 0
      paddw mm6, mm3      // paddw %%mm3,%%mm6          y+cr+cb->mm6 = ? r g b

      packuswb mm5, mm6   // packuswb %%mm6,%%mm5       mm5 = ? r1 g1 b1 r0 g0 b0 ?
      psrlq mm5, 8        // psrlq $8,%%mm5             mm5: 0 ? r1 g1 b1 r0 g0 b0
      movq [edx], mm5     // movq %%mm5,%0              store mm5

      // prepare for RGB 2 & 3
      punpckhwd mm0, mm0  // punpckhwd %%mm0,%%mm0      mm0 = y3 y3 y2 y2
      punpckhwd mm1, mm1  // punpckhwd %%mm1,%%mm1      mm1 = cb3 cb3 cb2 cb2
      punpckhwd mm2, mm2  // punpckhwd %%mm2,%%mm2      mm2 = cr3 cr3 cr2 cr2
      pmulhw mm1, bpte1   // pmulhw te1,%%mm1           mm1 = cb * ?
      pmulhw mm2, bpte2   // pmulhw te2,%%mm2           mm2 = cr * ?
      movq mm3, mm0       // movq %%mm0,%%mm3           mm3 = y3 y3 y2 y2
      punpcklwd mm3, mm3  // punpcklwd %%mm3,%%mm3      mm3 = y2 y2 y2 y2
      punpckhwd mm0, mm0  // punpckhwd %%mm0,%%mm0      mm0 = y3 y3 y3 y3

      // RGB 2
      movq mm4, mm1       // movq %%mm1,%%mm4           mm4 = cb3 cb3 cb2 cb2
      movq mm5, mm2       // movq %%mm2,%%mm5           mm5 = cr3 cr3 cr2 cr2
      psllq mm4, 32       // psllq $32,%%mm4            mm4 = cb2/b cb2/g 0 0
      psllq mm5, 32       // psllq $32,%%mm5            mm5 = cr2/g cr2/r 0 0
      psrlq mm5, 16       // psrlq $16,%%mm4            mm5 = 0 cr2/g cr2/g 0
      paddw mm3, mm4      // paddw %%mm4,%%mm3          y+cb
      paddw mm3, mm5      // paddw %%mm5,%%mm3          mm3 = y+cb+cr

      // RGB 3
      psrlq mm2, 32       // psrlq $32,%%mm2            mm2 = 0 0 cr3/g cr3/r
      psrlq mm1, 32       // psrlq $32,%%mm1            mm1 = 0 0 cb3/b cb3/g
      psllq mm1, 16       // psllq $16,%%mm2            mm1 = 0 cb3/b cb3/g 0
      paddw mm0, mm2      // paddw %%mm2,%%mm0          y+cr
      paddw mm0, mm1      // paddw %%mm1,%%mm0          y+cb+cr

      packuswb mm3, mm0   // packuswb %%mm0,%%mm3       pack in a quadword
      psrlq mm3, 8        // psrlq $8,%%mm3             shift to the right corner
      movq [edx+6], mm3   // movq %%mm3,6%0             save two more RGB pixels
    }
#endif






#if defined(HAVE_MMX_ATT_MNEMONICS)
      __asm__(
	  "pushl %%ebx\n"
	      "mov %1, %%eax\n"  
	      "mov %2, %%ebx\n"  
	      "mov %3, %%ecx\n"  
	      "movd (%%eax),%%mm0\n"    // mm0: 0 0 0 0 y3 y2 y1 y0 - 8 bit
	  "movd (%%ebx),%%mm1\n"    // mm1: 0 0 0 0 cb3 cb2 cb1 cb0
	  "movd (%%ecx),%%mm2\n"    // mm2: 0 0 0 0 cr3 cr2 cr1 cr0
	  "pxor %%mm7,%%mm7\n"      // mm7 = 0
	  "punpcklbw %%mm7,%%mm0\n" // mm0: y3 y2 y1 y0 - expand to 16 bit
	  "punpcklbw %%mm7,%%mm1\n" // mm1: cb3 cb2 cb1 cb0
	  "punpcklbw %%mm7,%%mm2\n" // mm2: cr3 cr2 cr1 cr0
	  "psubw %4,%%mm1\n"       // minus 128 for cb and cr
	  "psubw %4,%%mm2\n"
	  "psllw $2,%%mm1\n"        // shift left 2 bits for Cr and Cb to fit the mult constants
	  "psllw $2,%%mm2\n"

	  // prepare for RGB 1 & 0
	  "movq %%mm1,%%mm3\n"     // mm3_16: cb3 cb2 cb1 cb0
	  "movq %%mm2,%%mm4\n"     // mm4_16: cr3 cr2 cr1 cr0
	  "punpcklwd %%mm3,%%mm3\n"// expand to 32 bit: mm3: cb1 cb1 cb0 cb0
	  "punpcklwd %%mm4,%%mm4\n"// mm4: cr1 cr1 cr0 cr0
	  
	  // Y    Y     Y    Y 
	  // 0    CB*g  CB*b 0
	  // CR*r CR*g  0    0
	  //------------------
	  // R    G     B  

	  "pmulhw %5,%%mm3\n"// multiplicate in the constants: mm3: cb1/green cb1/blue cb0/green cb0/blue
	  "pmulhw %6,%%mm4\n"// mm4: cr1/red cb1/green cr0/red cr0/green

	  "movq %%mm0,%%mm5\n"      // mm5: y3 y2 y1 y0
	  "punpcklwd %%mm5,%%mm5\n" // expand to 32 bit: y1 y1 y0 y0
	  "movq %%mm5,%%mm6\n"      // mm6: y1 y1 y0 y0
	  "punpcklwd %%mm5,%%mm5\n" // mm5: y0 y0 y0 y0
	  "punpckhwd %%mm6,%%mm6\n" // mm6: y1 y1 y1 y1

	  // RGB 0
	  "movq %%mm3,%%mm7\n"      // mm7: cb1/g cb1/b cb0/g cb0/b
	  "psllq $32,%%mm7\n"       // shift left 32 bits: mm7: cb0/g cb0/b 0 0
	  "psrlq $16,%%mm7\n"       // mm7 = 0 cb0/g cb0/b 0
	  "paddw %%mm7,%%mm5\n"     // add: mm7: y+cb
	  "movq %%mm4,%%mm7\n"      // mm7 = cr1 cr1 cr0 cr0
	  "psllq $32,%%mm7\n"       // shift left 32 bits: mm7: cr0/r cr0/g 0 0
	  "paddw %%mm7,%%mm5\n"     // y+cb+cr r g b ?
	  
	  // RGB 1
	  "psrlq $32,%%mm4\n"       // mm4: 0 0 cr1 cr1 
	  "psllq $16,%%mm4\n"       // mm4: 0 cr1 cr1 0 
	  "paddw %%mm4,%%mm6\n"     //y+cr
	  "psrlq $32,%%mm3\n"       // mm3: 0 0 cb1 cb1
	  "paddw %%mm3,%%mm6\n"     //y+cr+cb: mm6 = r g b

	  "packuswb %%mm6,%%mm5\n"   //mm5 = ? r1 g1 b1 r0 g0 b0 ?
	  "psrlq $8,%%mm5\n"        // mm5: 0 ? r1 g1 b1 r0 g0 b0
	  "movq %%mm5,%0\n"         // store mm5

	  // prepare for RGB 2 & 3
	  "punpckhwd %%mm0,%%mm0\n" //mm0 = y3 y3 y2 y2
	  "punpckhwd %%mm1,%%mm1\n" //mm1 = cb3 cb3 cb2 cb2
	  "punpckhwd %%mm2,%%mm2\n" //mm2 = cr3 cr3 cr2 cr2
	  "pmulhw %5,%%mm1\n"      //mm1 = cb * ?
	  "pmulhw %6,%%mm2\n"      //mm2 = cr * ?
	  "movq %%mm0,%%mm3\n"      //mm3 = y3 y3 y2 y2
	  "punpcklwd %%mm3,%%mm3\n" //mm3 = y2 y2 y2 y2
	  "punpckhwd %%mm0,%%mm0\n" //mm0 = y3 y3 y3 y3

	  // RGB 2
	  "movq %%mm1,%%mm4\n"      //mm4 = cb3 cb3 cb2 cb2
	  "movq %%mm2,%%mm5\n"      //mm5 = cr3 cr3 cr2 cr2
	  "psllq $32,%%mm4\n"       //mm4 = cb2 cb2 0 0
 	  "psllq $32,%%mm5\n"       //mm5 = cr2 cr2 0 0
	  "psrlq $16,%%mm4\n"       //mm4 = 0 cb2 cb2 0
	  "paddw %%mm4,%%mm3\n"     // y+cb
	  "paddw %%mm5,%%mm3\n"     //mm3 = y+cb+cr

	  // RGB 3
	  "psrlq $32,%%mm2\n"       //mm2 = 0 0 cr3 cr3
	  "psrlq $32,%%mm1\n"       //mm1 = 0 0 cb3 cb3
	  "psllq $16,%%mm2\n"       //mm1 = 0 cr3 cr3 0
	  "paddw %%mm2,%%mm0\n"     //y+cr
	  "paddw %%mm1,%%mm0\n"     //y+cb+cr

	  "packuswb %%mm0,%%mm3\n"  // pack in a quadword
	  "psrlq $8,%%mm3\n"        // shift to the right corner
	  "movq %%mm3,6%0\n"       //  save two more RGB pixels

	  :"=m"(outptr[0])
	  :"m"(inptr0),"m"(inptr1),"m"(inptr2), //y cb cr
	   "m"(te0),"m"(te1),"m"(te2)
	  :"eax", "ebx", "ecx", "st");
#endif

      outptr += 12;
      inptr0+=4;
      inptr1+=4;
      inptr2+=4;
    }
  }
}
#endif

#if defined(HAVE_MMX_INTEL_MNEMONICS) || defined(HAVE_MMX_ATT_MNEMONICS)
METHODDEF(void)
ycc_rgb_convert_orig (j_decompress_ptr cinfo,
		 JSAMPIMAGE input_buf, JDIMENSION input_row,
		 JSAMPARRAY output_buf, int num_rows)
#else
METHODDEF(void)
ycc_rgb_convert (j_decompress_ptr cinfo,
		 JSAMPIMAGE input_buf, JDIMENSION input_row,
		 JSAMPARRAY output_buf, int num_rows)
#endif
{
  my_cconvert_ptr cconvert = (my_cconvert_ptr) cinfo->cconvert;
  register int y, cb, cr;
  register JSAMPROW outptr;
  register JSAMPROW inptr0, inptr1, inptr2;
  register JDIMENSION col;
  JDIMENSION num_cols = cinfo->output_width;
  /* copy these pointers into registers if possible */
  register JSAMPLE * range_limit = cinfo->sample_range_limit;
  register int * Crrtab = cconvert->Cr_r_tab;
  register int * Cbbtab = cconvert->Cb_b_tab;
  register INT32 * Crgtab = cconvert->Cr_g_tab;
  register INT32 * Cbgtab = cconvert->Cb_g_tab;
  SHIFT_TEMPS

  while (--num_rows >= 0) {
    inptr0 = input_buf[0][input_row];
    inptr1 = input_buf[1][input_row];
    inptr2 = input_buf[2][input_row];
    input_row++;
    outptr = *output_buf++;
    for (col = 0; col < num_cols; col++) {
      y  = GETJSAMPLE(inptr0[col]);
      cb = GETJSAMPLE(inptr1[col]);
      cr = GETJSAMPLE(inptr2[col]);
      /* Range-limiting is essential due to noise introduced by DCT losses. */
      outptr[RGB_RED] =   range_limit[y + Crrtab[cr]];
      outptr[RGB_GREEN] = range_limit[y +
			      ((int) RIGHT_SHIFT(Cbgtab[cb] + Crgtab[cr],
						 SCALEBITS))];
      outptr[RGB_BLUE] =  range_limit[y + Cbbtab[cb]];
      outptr += RGB_PIXELSIZE;
    }
  }
}


/**************** Cases other than YCbCr -> RGB **************/


/*
 * Color conversion for no colorspace change: just copy the data,
 * converting from separate-planes to interleaved representation.
 */

METHODDEF(void)
null_convert (j_decompress_ptr cinfo,
	      JSAMPIMAGE input_buf, JDIMENSION input_row,
	      JSAMPARRAY output_buf, int num_rows)
{
  register JSAMPROW inptr, outptr;
  register JDIMENSION count;
  register int num_components = cinfo->num_components;
  JDIMENSION num_cols = cinfo->output_width;
  int ci;

  while (--num_rows >= 0) {
    for (ci = 0; ci < num_components; ci++) {
      inptr = input_buf[ci][input_row];
      outptr = output_buf[0] + ci;
      for (count = num_cols; count > 0; count--) {
	*outptr = *inptr++;	/* needn't bother with GETJSAMPLE() here */
	outptr += num_components;
      }
    }
    input_row++;
    output_buf++;
  }
}

/*
 * Color conversion for grayscale: just copy the data.
 * This also works for YCbCr -> grayscale conversion, in which
 * we just copy the Y (luminance) component and ignore chrominance.
 */

METHODDEF(void)
grayscale_convert (j_decompress_ptr cinfo,
		   JSAMPIMAGE input_buf, JDIMENSION input_row,
		   JSAMPARRAY output_buf, int num_rows)
{
  jcopy_sample_rows(input_buf[0], (int) input_row, output_buf, 0,
		    num_rows, cinfo->output_width);
}


/*
 * Convert grayscale to RGB: just duplicate the graylevel three times.
 * This is provided to support applications that don't want to cope
 * with grayscale as a separate case.
 */

METHODDEF(void)
gray_rgb_convert (j_decompress_ptr cinfo,
		  JSAMPIMAGE input_buf, JDIMENSION input_row,
		  JSAMPARRAY output_buf, int num_rows)
{
  register JSAMPROW inptr, outptr;
  register JDIMENSION col;
  JDIMENSION num_cols = cinfo->output_width;

  while (--num_rows >= 0) {
    inptr = input_buf[0][input_row++];
    outptr = *output_buf++;
    for (col = 0; col < num_cols; col++) {
      /* We can dispense with GETJSAMPLE() here */
      outptr[RGB_RED] = outptr[RGB_GREEN] = outptr[RGB_BLUE] = inptr[col];
      outptr += RGB_PIXELSIZE;
    }
  } 
} 


/*
 * Adobe-style YCCK->CMYK conversion.
 * We convert YCbCr to R=1-C, G=1-M, and B=1-Y using the same
 * conversion as above, while passing K (black) unchanged.
 * We assume build_ycc_rgb_table has been called.
 */

METHODDEF(void)
ycck_cmyk_convert (j_decompress_ptr cinfo,
		   JSAMPIMAGE input_buf, JDIMENSION input_row,
		   JSAMPARRAY output_buf, int num_rows)
{
  my_cconvert_ptr cconvert = (my_cconvert_ptr) cinfo->cconvert;
  register int y, cb, cr;
  register JSAMPROW outptr;
  register JSAMPROW inptr0, inptr1, inptr2, inptr3;
  register JDIMENSION col;
  JDIMENSION num_cols = cinfo->output_width;
  /* copy these pointers into registers if possible */
  register JSAMPLE * range_limit = cinfo->sample_range_limit;
  register int * Crrtab = cconvert->Cr_r_tab;
  register int * Cbbtab = cconvert->Cb_b_tab;
  register INT32 * Crgtab = cconvert->Cr_g_tab;
  register INT32 * Cbgtab = cconvert->Cb_g_tab;
  SHIFT_TEMPS

  while (--num_rows >= 0) {
    inptr0 = input_buf[0][input_row];
    inptr1 = input_buf[1][input_row];
    inptr2 = input_buf[2][input_row];
    inptr3 = input_buf[3][input_row];
    input_row++;
    outptr = *output_buf++;
    for (col = 0; col < num_cols; col++) {
      y  = GETJSAMPLE(inptr0[col]);
      cb = GETJSAMPLE(inptr1[col]);
      cr = GETJSAMPLE(inptr2[col]);
      /* Range-limiting is essential due to noise introduced by DCT losses.*/
      outptr[0] = range_limit[MAXJSAMPLE - (y + Crrtab[cr])];	/* red*/
      outptr[1] = range_limit[MAXJSAMPLE - (y +			/* green*/
					    ((int) RIGHT_SHIFT(Cbgtab[cb] + Crgtab[cr],
							       SCALEBITS)))];
      outptr[2] = range_limit[MAXJSAMPLE - (y + Cbbtab[cb])];	/* blue*/
      /* K passes through unchanged*/
      outptr[3] = inptr3[col];	/* don't need GETJSAMPLE here*/
      outptr += 4;
    }
  }
}


/*
 * Empty method for start_pass.
 */

METHODDEF(void)
start_pass_dcolor (j_decompress_ptr cinfo)
{
  /* no work needed */
}


/*
 * Module initialization routine for output colorspace conversion.
 */

GLOBAL(void)
jinit_color_deconverter (j_decompress_ptr cinfo)
{
  my_cconvert_ptr cconvert;
  int ci;

  cconvert = (my_cconvert_ptr)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				SIZEOF(my_color_deconverter));
  cinfo->cconvert = (struct jpeg_color_deconverter *) cconvert;
  cconvert->pub.start_pass = start_pass_dcolor;

  /* Make sure num_components agrees with jpeg_color_space */
  switch (cinfo->jpeg_color_space) { 
  case JCS_GRAYSCALE:
    if (cinfo->num_components != 1)
      ERREXIT(cinfo, JERR_BAD_J_COLORSPACE);
    break;
    
  case JCS_RGB:
  case JCS_YCbCr:
    if (cinfo->num_components != 3)
      ERREXIT(cinfo, JERR_BAD_J_COLORSPACE);
      break; 

  case JCS_CMYK:
  case JCS_YCCK:
    if (cinfo->num_components != 4)
      ERREXIT(cinfo, JERR_BAD_J_COLORSPACE);
    break;

  default:			/* JCS_UNKNOWN can be anything */
    if (cinfo->num_components < 1)
      ERREXIT(cinfo, JERR_BAD_J_COLORSPACE);
    break;
  } 

  /* Set out_color_components and conversion method based on requested space.
   * Also clear the component_needed flags for any unused components,
   * so that earlier pipeline stages can avoid useless computation.
   */

  switch (cinfo->out_color_space) {
  case JCS_GRAYSCALE:
    cinfo->out_color_components = 1;
    if (cinfo->jpeg_color_space == JCS_GRAYSCALE ||
	cinfo->jpeg_color_space == JCS_YCbCr) {
      cconvert->pub.color_convert = grayscale_convert;
      /* For color->grayscale conversion, only the Y (0) component is needed */
       for (ci = 1; ci < cinfo->num_components; ci++)
	cinfo->comp_info[ci].component_needed = FALSE;
    } else
      ERREXIT(cinfo, JERR_CONVERSION_NOTIMPL);
    break;

  case JCS_RGB: 
    cinfo->out_color_components = RGB_PIXELSIZE;
    if (cinfo->jpeg_color_space == JCS_YCbCr) {

#if defined(HAVE_MMX_INTEL_MNEMONICS) || defined(HAVE_MMX_ATT_MNEMONICS)
      if (MMXAvailable)
	{
	  cconvert->pub.color_convert = ycc_rgb_convert_mmx;
	  /* A.Stevens 2001 - sometimes we use non-MMX colorspace conversion
		 hacks  with MMX jpeglib.  TODO: A clean solution! */
	  build_ycc_rgb_table(cinfo);
	}
      else
	{
	  cconvert->pub.color_convert = ycc_rgb_convert_orig;
	  build_ycc_rgb_table(cinfo);
	}
#else
      cconvert->pub.color_convert = ycc_rgb_convert;
#endif

    } else if (cinfo->jpeg_color_space == JCS_GRAYSCALE) {
      cconvert->pub.color_convert = gray_rgb_convert;
    } else if (cinfo->jpeg_color_space == JCS_RGB && RGB_PIXELSIZE == 3) {
      cconvert->pub.color_convert = null_convert;
    } else
      ERREXIT(cinfo, JERR_CONVERSION_NOTIMPL);
    break; 

  case JCS_CMYK:
    cinfo->out_color_components = 4;
    if (cinfo->jpeg_color_space == JCS_YCCK) {
      cconvert->pub.color_convert = ycck_cmyk_convert;
      build_ycc_rgb_table(cinfo);
    } else if (cinfo->jpeg_color_space == JCS_CMYK) {
      cconvert->pub.color_convert = null_convert;
    } else
      ERREXIT(cinfo, JERR_CONVERSION_NOTIMPL);
    break;

  default:
    /* Permit null conversion to same output space */
    if (cinfo->out_color_space == cinfo->jpeg_color_space) {
      cinfo->out_color_components = cinfo->num_components;
      cconvert->pub.color_convert = null_convert;
    } else			/* unsupported non-null conversion */
      ERREXIT(cinfo, JERR_CONVERSION_NOTIMPL);
    break;
  }

  if (cinfo->quantize_colors)
    cinfo->output_components = 1; /* single colormapped output component */
  else
    cinfo->output_components = cinfo->out_color_components;
}





