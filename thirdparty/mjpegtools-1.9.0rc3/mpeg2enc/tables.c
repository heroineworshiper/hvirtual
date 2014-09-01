/* tables.c, Tables for MPEG syntax                        */

/* Copyright (C) 1996, MPEG Software Simulation Group. All Rights Reserved. */

/*
 * Disclaimer of Warranty
 *
 * These software programs are available to the user without any license fee or
 * royalty on an "as is" basis.  The MPEG Software Simulation Group disclaims
 * any and all warranties, whether express, implied, or statuary, including any
 * implied warranties or merchantability or of fitness for a particular
 * purpose.  In no event shall the copyright-holder be liable for any
 * incidental, punitive, or consequential damages of any kind whatsoever
 * arising from the use of these programs.
 *
 * This disclaimer of warranty extends to the user of these programs and user's
 * customers, employees, agents, transferees, successors, and assigns.
 *
 * The MPEG Software Simulation Group does not represent or warrant that the
 * programs furnished hereunder are free of infringement of any third-party
 * patents.
 *
 * Commercial implementations of MPEG-1 and MPEG-2 video, including shareware,
 * are subject to royalty fees to patent holders.  Many of these patents are
 * general enough such that they are unavoidable regardless of implementation
 * design.
 *
 */

#include <config.h>
#include "tables.h"

const char version[]

="MSSG+ 1.3 2001/6/10 (development of mpeg2encode V1.2, 96/07/19)"

;

const char author[]

="(C) 1996, MPEG Software Simulation Group, (C) 2000, 2001,2002 Andrew Stevens, Rainer Johanni"

;

/* zig-zag scan */
const uint8_t zig_zag_scan[64]

=
{
	0,1,8,16,9,2,3,10,17,24,32,25,18,11,4,5,
	12,19,26,33,40,48,41,34,27,20,13,6,7,14,21,28,
	35,42,49,56,57,50,43,36,29,22,15,23,30,37,44,51,
	58,59,52,45,38,31,39,46,53,60,61,54,47,55,62,63
}

;

/* alternate scan */
const uint8_t alternate_scan[64]

=
{
	0,8,16,24,1,9,2,10,17,25,32,40,48,56,57,49,
	41,33,26,18,3,11,4,12,19,27,34,42,50,58,35,43,
	51,59,20,28,5,13,6,14,21,29,36,44,52,60,37,45,
	53,61,22,30,7,15,23,31,38,46,54,62,39,47,55,63
}

;

/* default intra quantization matrix */
const uint16_t default_intra_quantizer_matrix[64]

=
{
	8, 16, 19, 22, 26, 27, 29, 34,
	16, 16, 22, 24, 27, 29, 34, 37,
	19, 22, 26, 27, 29, 34, 34, 38,
	22, 22, 26, 27, 29, 34, 37, 40,
	22, 26, 27, 29, 32, 35, 40, 48,
	26, 27, 29, 32, 35, 40, 48, 58,
	26, 27, 29, 34, 38, 46, 56, 69,
	27, 29, 35, 38, 46, 56, 69, 83
}

;

const uint16_t hires_intra_quantizer_matrix[64]

=
{
	8, 16, 18, 20, 24, 25, 26, 30,
	16, 16, 20, 23, 25, 26, 30, 30,
	18, 20, 22, 24, 26, 28, 29, 31,
	20, 21, 23, 24, 26, 28, 31, 31,
	21, 23, 24, 25, 28, 30, 30, 33,
	23, 24, 25, 28, 30, 30, 33, 36,
	24, 25, 26, 29, 29, 31, 34, 38,
	25, 26, 28, 29, 31, 34, 38, 42
}

;

const uint16_t default_nonintra_quantizer_matrix[64]

=
{
	16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16
}

;

const uint16_t kvcd_intra_quantizer_matrix[64]

=
{
	8,  9, 12, 22, 26, 27, 29, 34,
	9, 10, 14, 26, 27, 29, 34, 37,
	12, 14, 18, 27, 29, 34, 37, 38,
	22, 26, 27, 31, 36, 37, 38, 40,
	26, 27, 29, 36, 39, 38, 40, 48,
	27, 29, 34, 37, 38, 40, 48, 58,
	29, 34, 37, 38, 40, 48, 58, 69,
	34, 37, 38, 40, 48, 58, 69, 79
}

;

const uint16_t kvcd_nonintra_quantizer_matrix[64]

=
{
	16, 18, 20, 22, 24, 26, 28, 30,
	18, 20, 22, 24, 26, 28, 30, 32,
	20, 22, 24, 26, 28, 30, 32, 34,
	22, 24, 26, 30, 32, 32, 34, 36,
	24, 26, 28, 32, 34, 34, 36, 38,
	26, 28, 30, 32, 34, 36, 38, 40,
	28, 30, 32, 34, 36, 38, 42, 42,
	30, 32, 34, 36, 38, 40, 42, 44
}

;

const uint16_t tmpgenc_intra_quantizer_matrix[64]

=
{
/*
  The original HEX table for reference
  08, 10, 13, 16, 1A, 1B, 1D, 22,
  10, 10, 16, 18, 1B, 1D, 22, 25,
  13, 16, 1A, 1B, 1D, 22, 22, 26,
  16, 16, 1A, 1B, 1D, 22, 25, 28,
  16, 1A, 1B, 1D, 20, 23, 28, 30,
  1A, 1B, 1D, 20, 23, 28, 30, 3A,
  1A, 1B, 1D, 22, 26, 2E, 38, 45,
  1B, 1D, 23, 26, 2E, 38, 45, 53
*/
/* Decimal table */
	8, 16, 19, 22, 26, 27, 29, 34,
	16, 16, 22, 24, 27, 29, 34, 37,
	19, 22, 26, 27, 29, 34, 34, 38,
	22, 22, 26, 27, 29, 34, 37, 40,
	22, 26, 27, 29, 32, 35, 40, 48,
	26, 27, 29, 32, 35, 40, 48, 58,
	26, 27, 29, 34, 38, 46, 56, 69,
	27, 29, 35, 38, 46, 56, 69, 83
}

;

const uint16_t tmpgenc_nonintra_quantizer_matrix[64]

=
{
/*
  The original HEX table for reference
  10, 11, 12, 13, 14, 15, 16, 17,
  11, 12, 13, 14, 15, 16, 17, 18,
  12, 13, 14, 15, 16, 17, 18, 19,
  13, 14, 15, 16, 17, 18, 1A, 1B,
  14, 15, 16, 17, 19, 1A, 1B, 1C,
  15, 16, 17, 18, 1A, 1B, 1C, 1E,
  16, 17, 18, 1A, 1B, 1C, 1E, 1F,
  17, 18, 19, 1B, 1C, 1E, 1F, 21
*/
/* DECIMAL table */
	16, 17, 18, 19, 20, 21, 22, 23,
	17, 18, 19, 20, 21, 22, 23, 24,
	18, 19, 20, 21, 22, 23, 24, 25,
	19, 20, 21, 22, 23, 24, 26, 27,
	20, 21, 22, 23, 25, 26, 27, 28,
	21, 22, 23, 24, 26, 27, 28, 30,
	22, 23, 24, 26, 27, 28, 30, 31,
	23, 24, 25, 27, 28, 30, 31, 33
}

;

/* Hires non intra quantization matrix.  THis *is*
   the MPEG default...	 */
const uint16_t *hires_nonintra_quantizer_matrix

= &default_nonintra_quantizer_matrix[0]

;


const char pict_type_char[6]

= {'X', 'I', 'P', 'B', 'D', 'X'}

;


/* Support for the picture layer(!) insertion of scan data fieldsas
   MPEG user-data section as part of I-frames.  */

const uint8_t dummy_svcd_scan_data[14]

= {
	0x10,                       /* Scan data tag */
	14,                         /* Length */
	0x00, 0x80, 0x81,            /* Dummy data - this will be filled */
	0x00, 0x80, 0x81,            /* By the multiplexor or cd image   */        
	0xff, 0xff, 0xff,            /* creation software                */        
	0xff, 0xff, 0xff

}

;



/* non-linear quantization coefficient table */
const uint8_t non_linear_mquant_table[32] =
{
	0, 1, 2, 3, 4, 5, 6, 7,
	8,10,12,14,16,18,20,22,
	24,28,32,36,40,44,48,52,
	56,64,72,80,88,96,104,112
};

/* non-linear mquant table for mapping from scale to code
 * since reconstruction levels are not bijective with the index map,
 * it is up to the designer to determine most of the quantization levels
 */

const uint8_t map_non_linear_mquant[113] =
{
	 0 ,1 , 2, 3, 4, 5, 6, 7, 8, 8, 
     9 ,9, 10,10,11,11,12,12,13,13,
     14,14,15,15,16,16,16,17,17,17,
     18,18,18,18,19,19,19,19,20,20,
     20,20,21,21,21,21,22,22,22,22,
     23,23,23,23,24,24,24,24,24,24,
     24,25,25,25,25,25,25,25,26,26,
	 26,26,26,26,26,26,27,27,27,27,
     27,27,27,27,28,28,28,28,28,28,
     28,29,29,29,29,29,29,29,29,29,
     29,30,30,30,30,30,30,30,31,31,
     31,31,31
};


