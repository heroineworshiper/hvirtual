 /*
 *  encode.c
 *
 *     Copyright (C) James Bowman  - July 2000
 *                   Peter Schlaile - Jan 2001
 *
 *  This file is part of libdv, a free DV (IEC 61834/SMPTE 314M)
 *  codec.
 *
 *  libdv is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser Public License as published by
 *  the Free Software Foundation; either version 2.1, or (at your
 *  option) any later version.
 *   
 *  libdv is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser Public License for more details.
 *   
 *  You should have received a copy of the GNU Lesser Public License
 *  along with libdv; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 *  The libdv homepage is http://libdv.sourceforge.net/.  
 */

/** @file
 *  @ingroup encoder
 *  @brief Implementation for @link encoder DV Encoder @endlink
 */

/** @weakgroup encoder DV Encoder
 *
 *  This is the toplevel module of the \c liddv encoder.  
 *  
 *  @{
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <time.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <pthread.h>

#include "dv.h"
#include "encode.h"
#include "idct_248.h"
#include "quant.h"
#include "weighting.h"
#include "vlc.h"
#include "parse.h"
#include "place.h"
#include "headers.h"
#if ARCH_X86 || ARCH_X86_64
#include "mmx.h"
#endif

#include "enc_input.h"
#include "enc_output.h"

#define BITS_PER_LONG  32

/* FIXME: Just guessed! */
#define VLC_BITS_ON_FULL_MBLOCK_CYCLE_QUANT_2 750
#define VLC_MAX_RUNS_PER_CYCLE_QUANT_2        3
#define VLC_BITS_ON_FULL_MBLOCK_CYCLE_QUANT_3 500
#define VLC_MAX_RUNS_PER_CYCLE_QUANT_3        3

/* typedef unsigned long dv_vlc_entry_t; */
typedef uint32_t dv_vlc_entry_t;

static inline unsigned long get_dv_vlc_val(dv_vlc_entry_t v) 
{
	return (v >> 8);
}

static inline unsigned long get_dv_vlc_len(dv_vlc_entry_t v)
{
	return v & 0xff;
}

static inline dv_vlc_entry_t set_dv_vlc(unsigned long val, unsigned long len)
{
	return len | (val << 8);
}

typedef struct dv_vlc_block_s {
	dv_vlc_entry_t coeffs[128] ALIGN8;
	dv_vlc_entry_t * coeffs_end;
	dv_vlc_entry_t * coeffs_start;
	unsigned int coeffs_bits;
	long bit_offset;
	long bit_budget;
	int can_supply;
} dv_vlc_block_t;

extern int     dv_super_map_vertical[5];
extern int     dv_super_map_horizontal[5];

extern int    dv_parse_bit_start[6];
extern int    dv_parse_bit_end[6];

static long runs_used[15];
static long cycles_used[15*5*6];
static long classes_used[4];
static long qnos_used[16];
static long dct_used[2];
static long vlc_overflows;

static inline void
dv_place_411_macroblock(dv_macroblock_t *mb) 
{
	int mb_num; /* mb number withing the 6 x 5 zig-zag pattern  */
	int mb_num_mod_6, mb_num_div_6; /* temporaries */
	int mb_row;    /* mb row within sb (de-zigzag) */
	int mb_col;    /* mb col within sb (de-zigzag) */
	/* Column offset of superblocks in macroblocks. */
	static const unsigned int column_offset[] = {0, 4, 9, 13, 18};  
	
	/* Consider the area spanned super block as 30 element macroblock
	   grid (6 rows x 5 columns).  The macroblocks are laid out in a in
	   a zig-zag down and up the columns of the grid.  Of course,
	   superblocks are not perfect rectangles, since there are only 27
	   blocks.  The missing three macroblocks are either at the start
	   or end depending on the superblock column.
	
	   Within a superblock, the macroblocks start at the topleft corner
	   for even-column superblocks, and 3 down for odd-column
	   superblocks. */

	mb_num = ((mb->j % 2) == 1) ? mb->k + 3: mb->k;  
	mb_num_mod_6 = mb_num % 6;
	mb_num_div_6 = mb_num / 6;

	/* Compute superblock-relative row position (de-zigzag) */
	mb_row = ((mb_num_div_6 % 2) == 0) ? 
		mb_num_mod_6 : (5 - mb_num_mod_6); 
	/* Compute macroblock's frame-relative column position (in blocks) */
	mb_col = (mb_num_div_6 + column_offset[mb->j]) * 4;
	/* Compute frame-relative byte offset of macroblock's top-left corner
	   with special case for right-edge macroblocks */
	if(mb_col < (22 * 4)) {
		/* Convert from superblock-relative row position 
		   to frame relative (in blocks). */
		mb_row += (mb->i * 6); /* each superblock is 6 blocks high */
		/* Normal case */
	} else { 
		/* Convert from superblock-relative row position to 
		   frame relative (in blocks). */
		mb_row = mb_row * 2 + mb->i * 6; 
                /* each right-edge macroblock is 2 blocks high, 
		   and each superblock is 6 blocks high */
	}
	mb->x = mb_col * 8;
	mb->y = mb_row * 8;
} /* dv_place_411_macroblock */

static inline void 
dv_place_420_macroblock(dv_macroblock_t *mb) 
{
	int mb_num; /* mb number withing the 6 x 5 zig-zag pattern */
	int mb_num_mod_3, mb_num_div_3; /* temporaries */
	int mb_row;    /* mb row within sb (de-zigzag) */
	int mb_col;    /* mb col within sb (de-zigzag) */
	/* Column offset of superblocks in macroblocks. */
	static const unsigned int column_offset[] = {0, 9, 18, 27, 36};  
	
	/* Consider the area spanned super block as 30 element macroblock
	   grid (6 rows x 5 columns).  The macroblocks are laid out in a in
	   a zig-zag down and up the columns of the grid.  Of course,
	   superblocks are not perfect rectangles, since there are only 27
	   blocks.  The missing three macroblocks are either at the start
	   or end depending on the superblock column. */
	
	/* Within a superblock, the macroblocks start at the topleft corner
	   for even-column superblocks, and 3 down for odd-column
	   superblocks. */
	mb_num = mb->k;  
	mb_num_mod_3 = mb_num % 3;
	mb_num_div_3 = mb_num / 3;
	/* Compute superblock-relative row position (de-zigzag) */
	mb_row = ((mb_num_div_3 % 2) == 0) ? mb_num_mod_3 : (2- mb_num_mod_3); 
	/* Compute macroblock's frame-relative column position (in blocks) */
	mb_col = mb_num_div_3 + column_offset[mb->j];
	/* Compute frame-relative byte offset of macroblock's top-left corner
	   Convert from superblock-relative row position to frame relative 
	   (in blocks). */
	mb_row += (mb->i * 3); /* each right-edge macroblock is 
				  2 blocks high, and each superblock is 
				  6 blocks high */
	mb->x = mb_col * 16;
	mb->y = mb_row * 16;
} /* dv_place_420_macroblock */

/* FIXME: Could still be faster... */
static inline unsigned int put_bits(unsigned char *s, unsigned int offset, 
                             unsigned int len, unsigned int value)
{
#if (!ARCH_X86) && (!ARCH_X86_64)
        s += (offset >> 3);
        
        value <<= (24 - len);
        value &= 0xffffff;
        value >>= (offset & 7);
        s[0] |= (value >> 16) & 0xff;
        s[1] |= (value >> 8) & 0xff;
        s[2] |= value & 0xff;
        return offset + len;
#else
        s += (offset >> 3);
        value <<= BITS_PER_LONG - len - (offset & 7);
        __asm__("bswap %0" : "=r" (value) : "0" (value));

        *((unsigned long*) s) |= value;
        return offset + len;
#endif
}

typedef struct {
	int8_t run;
	int8_t amp;
	uint16_t val;
	uint8_t len;
} dv_vlc_encode_t;

static dv_vlc_encode_t dv_vlc_test_table[89] = {
	{ 0, 1, 0x0, 2 },
	{ 0, 2, 0x2, 3 },
	{-1, 0, 0x6, 4 },
	{ 1, 1, 0x7, 4 },
	{ 0, 3, 0x8, 4 },
	{ 0, 4, 0x9, 4 },
	{ 2, 1, 0x14, 5 },
	{ 1, 2, 0x15, 5 },
	{ 0, 5, 0x16, 5 },
	{ 0, 6, 0x17, 5 },
	{ 3, 1, 0x30, 6 },
	{ 4, 1, 0x31, 6 },
	{ 0, 7, 0x32, 6 },
	{ 0, 8, 0x33, 6 },
	{ 5, 1,  0x68, 7 },
	{ 6, 1,  0x69, 7 },
	{ 2, 2,  0x6a, 7 },
	{ 1, 3,  0x6b, 7 },
	{ 1, 4,  0x6c, 7 },
	{ 0, 9,  0x6d, 7 },
	{ 0, 10, 0x6e, 7 },
	{ 0, 11, 0x6f, 7 },
	{ 7,  1,  0xe0, 8 },
	{ 8,  1,  0xe1, 8 },
	{ 9,  1,  0xe2, 8 },
	{ 10, 1,  0xe3, 8 },
	{ 3,  2,  0xe4, 8 },
	{ 4,  2,  0xe5, 8 },
	{ 2,  3,  0xe6, 8 },
	{ 1,  5,  0xe7, 8 },
	{ 1,  6,  0xe8, 8 },
	{ 1,  7,  0xe9, 8 },
	{ 0,  12, 0xea, 8 },
	{ 0,  13, 0xeb, 8 },
	{ 0,  14, 0xec, 8 },
	{ 0,  15, 0xed, 8 },
	{ 0,  16, 0xee, 8 },
	{ 0,  17, 0xef, 8 },
	{ 11, 1,  0x1e0, 9 },
	{ 12, 1,  0x1e1, 9 },
	{ 13, 1,  0x1e2, 9 },
	{ 14, 1,  0x1e3, 9 },
	{ 5,  2,  0x1e4, 9 },
	{ 6,  2,  0x1e5, 9 },
	{ 3,  3,  0x1e6, 9 },
	{ 4,  3,  0x1e7, 9 },
	{ 2,  4,  0x1e8, 9 },
	{ 2,  5,  0x1e9, 9 },
	{ 1,  8,  0x1ea, 9 },
	{ 0,  18, 0x1eb, 9 },
	{ 0,  19, 0x1ec, 9 },
	{ 0,  20, 0x1ed, 9 },
	{ 0,  21, 0x1ee, 9 },
	{ 0,  22, 0x1ef, 9 },
	{ 5, 3,  0x3e0, 10 },
	{ 3, 4,  0x3e1, 10 },
	{ 3, 5,  0x3e2, 10 },
	{ 2, 6,  0x3e3, 10 },
	{ 1, 9,  0x3e4, 10 },
	{ 1, 10, 0x3e5, 10 },
	{ 1, 11, 0x3e6, 10 },
	{ 0, 0,  0x7ce, 11 },
	{ 1, 0,  0x7cf, 11 },
	{ 6, 3,  0x7d0, 11 },
	{ 4, 4,  0x7d1, 11 },
	{ 3, 6,  0x7d2, 11 },
	{ 1, 12, 0x7d3, 11 },
	{ 1, 13, 0x7d4, 11 },
	{ 1, 14, 0x7d5, 11 },
	{ 2, 0, 0xfac, 12 },
	{ 3, 0, 0xfad, 12 },
	{ 4, 0, 0xfae, 12 },
	{ 5, 0, 0xfaf, 12 },
	{ 7,  2,  0xfb0, 12 },
	{ 8,  2,  0xfb1, 12 },
	{ 9,  2,  0xfb2, 12 },
	{ 10, 2,  0xfb3, 12 },
	{ 7,  3,  0xfb4, 12 },
	{ 8,  3,  0xfb5, 12 },
	{ 4,  5,  0xfb6, 12 },
	{ 3,  7,  0xfb7, 12 },
	{ 2,  7,  0xfb8, 12 },
	{ 2,  8,  0xfb9, 12 },
	{ 2,  9,  0xfba, 12 },
	{ 2,  10, 0xfbb, 12 },
	{ 2,  11, 0xfbc, 12 },
	{ 1,  15, 0xfbd, 12 },
	{ 1,  16, 0xfbe, 12 },
	{ 1,  17, 0xfbf, 12 },
}; /* dv_vlc_test_table */

static dv_vlc_encode_t * vlc_test_lookup[512];

void _dv_init_vlc_test_lookup()
{
	int i;
	memset(vlc_test_lookup, 0, 512 * sizeof(dv_vlc_encode_t*));
	for (i = 0; i < 89; i++) {
		dv_vlc_encode_t *pvc = &dv_vlc_test_table[i];
		vlc_test_lookup[((pvc->run + 1) << 5) + pvc->amp] = pvc;
	}
}

static inline dv_vlc_encode_t * find_vlc_entry(int run, int amp)
{
	if (run > 14 || amp > 22) { /* run < -1 || amp < 0 never happens! */
		return NULL;
	} else {
		return vlc_test_lookup[((run + 1) << 5) + amp];
	}
}

static inline void vlc_encode_r(int run, int amp, int sign, dv_vlc_entry_t * o)
{
	dv_vlc_encode_t * hit = find_vlc_entry(run, amp);

	if (hit != NULL) {
		/* 1111110 */
		int val, len;
		val = hit->val;
		len = hit->len;
		if (amp != 0) {
			val <<= 1;
			val |= sign;
			len++;
		}
		*o = set_dv_vlc(val, len);
	} else {
		if (amp == 0) {
			/* 1111110 */
			*o = set_dv_vlc((0x7e << 6) | run, 7+6);
		} else {
			/* 1111111 */
			*o = set_dv_vlc((0x7f << 9) | (amp << 1) | sign,
					7 + 8 + 1);
		}
	}
}

static inline dv_vlc_entry_t * vlc_encode_orig(int run, int amp, int sign, 
					       dv_vlc_entry_t * o)
{
	dv_vlc_encode_t * hit = find_vlc_entry(run, amp);

	if (hit != NULL) {
		/* 1111110 */
		int val, len;
		val = hit->val;
		len = hit->len;
		if (amp != 0) {
			val <<= 1;
			val |= sign;
			len++;
		}
		*o++ = 0;
		*o = set_dv_vlc(val, len);
	} else {
		if (amp == 0) {
			*o++ = 0;
			if (run < 62) {
				/* 1111110 */
				*o = set_dv_vlc((0x7e << 6) | run, 7+6);
			} else {
				*o = set_dv_vlc((0x7cf << 13) 
						| (0x7e << 6) | (run - 2),
						11 + 7 + 6);
			}
		} else if (run == 0) {
			/* 1111111 */
			*o++ = 0;
			*o = set_dv_vlc((0x7f << 9) | (amp << 1) | sign,
					7 + 8 + 1);
		} else {
			vlc_encode_r(run - 1, 0, 0, o);
			++o;
			vlc_encode_r(0, amp, sign, o);
			return ++o;
		}
	}
	return ++o;
}

dv_vlc_entry_t * vlc_encode_lookup = NULL;
unsigned char  * vlc_num_bits_lookup = NULL;

void _dv_init_vlc_encode_lookup(void)
{
	int run,amp;
	
	if (vlc_encode_lookup == NULL)
		vlc_encode_lookup = (dv_vlc_entry_t *) malloc(
			32768 * 2 * sizeof(dv_vlc_entry_t));
	if (vlc_num_bits_lookup == NULL)
		vlc_num_bits_lookup = (unsigned char*) malloc(32768);
		
	for (run = 0; run <= 63; run++) {
		for (amp = 0; amp <= 255; amp++) {
			int index1 = (255 + amp) | (run << 9);
			int index2 = (255 - amp) | (run << 9);
			vlc_encode_orig(run,amp,0,vlc_encode_lookup+2* index1);
			vlc_encode_orig(run,amp,1,vlc_encode_lookup+2* index2);
			vlc_num_bits_lookup[index1] =
				vlc_num_bits_lookup[index2] =
				get_dv_vlc_len(vlc_encode_lookup[2*index1])
				+get_dv_vlc_len(vlc_encode_lookup[2*index1+1]);
		}
	}
}

static inline void vlc_encode(int run, int amp, int sign, dv_vlc_entry_t * o)
{
	dv_vlc_entry_t * s= vlc_encode_lookup + 2 * ((amp + 255) | (run << 9));
	*o++ = *s++;
	*o = *s++ | (sign << 8);
}

static inline unsigned long vlc_num_bits(int run, int amp)
{
	return vlc_num_bits_lookup[(amp + 255) | (run << 9)];
}

static unsigned short reorder_88[64] = {
	1, 2, 6, 7,15,16,28,29,
	3, 5, 8,14,17,27,30,43,
	4, 9,13,18,26,31,42,44,
	10,12,19,25,32,41,45,54,
	11,20,24,33,40,46,53,55,
	21,23,34,39,47,52,56,61,
	22,35,38,48,51,57,60,62,
	36,37,49,50,58,59,63,64
};
static unsigned short reorder_248[64] = {
	1, 3, 7,19,21,35,37,51,
	5, 9,17,23,33,39,49,53,
	11,15,25,31,41,47,55,61,
	13,27,29,43,45,57,59,63,

	2, 4, 8,20,22,36,38,52,
	6,10,18,24,34,40,50,54,
	12,16,26,32,42,48,56,62,
	14,28,30,44,46,58,60,64
};


void _dv_prepare_reorder_tables(void)
{
	int i;
	for (i = 0; i < 64; i++) {
		reorder_88[i]--;
		reorder_88[i] *= 2;
		reorder_248[i]--;
		reorder_248[i] *= 2;
	}
}

extern int _dv_reorder_block_mmx(dv_coeff_t * a, 
			     const unsigned short* reorder_table);

extern int _dv_reorder_block_mmx_x86_64(dv_coeff_t * a, 
			     const unsigned short* reorder_table);

static void reorder_block(dv_block_t *bl)
{
#if (!ARCH_X86) && (!ARCH_X86_64)
	dv_coeff_t zigzag[64];
	int i;
#endif
	const unsigned short *reorder;

	if (bl->dct_mode == DV_DCT_88)
		reorder = reorder_88;
	else
		reorder = reorder_248;

#if ARCH_X86
	_dv_reorder_block_mmx(bl->coeffs, reorder);
	emms();
#elif ARCH_X86_64
	_dv_reorder_block_mmx_x86_64(bl->coeffs, reorder);
	emms();
#else	
	for (i = 0; i < 64; i++) {
	  //		*(unsigned short*) ((char*) zigzag + reorder[i])=bl->coeffs[i];
	  zigzag[reorder[i] - 1] = bl->coeffs[i];
	}
	memcpy(bl->coeffs, zigzag, 64 * sizeof(dv_coeff_t));
#endif
}

extern unsigned long _dv_vlc_encode_block_mmx(dv_coeff_t* coeffs,
					  dv_vlc_entry_t ** out);

extern unsigned long _dv_vlc_encode_block_mmx_x86_64(dv_coeff_t* coeffs,
					  dv_vlc_entry_t ** out);

static unsigned long vlc_encode_block(dv_coeff_t* coeffs, dv_vlc_block_t* out)
{
	dv_vlc_entry_t * o = out->coeffs;
#if (!ARCH_X86) && (!ARCH_X86_64)
	dv_coeff_t * z = coeffs + 1; /* First AC coeff */
	dv_coeff_t * z_end = coeffs + 64;
	int run, amp, sign;
	int num_bits = 0;

	do {
		run = 0;
		while (*z == 0) {
			z++; run++;
			if (z == z_end) { 
				goto z_out;
			}
		}

		amp = *z++;
		sign = 0;
		if (amp < 0) {
			amp = -amp;
			sign = 1;
		}
		vlc_encode(run, amp, sign, o);
		num_bits += get_dv_vlc_len(*o++);
		num_bits += get_dv_vlc_len(*o++);
	} while (z != z_end);
 z_out:
#elif ARCH_X86
	int num_bits;

	num_bits = _dv_vlc_encode_block_mmx(coeffs, &o);
	emms();
#else
	int num_bits;

	num_bits = _dv_vlc_encode_block_mmx_x86_64(coeffs, &o);
	emms();
#endif
	*o++ = set_dv_vlc(0x6, 4); /* EOB */

	out->coeffs_start = out->coeffs;
	out->coeffs_end = o;
	out->coeffs_bits = num_bits + 4;
	return num_bits;
}

extern unsigned long _dv_vlc_num_bits_block_x86(dv_coeff_t* coeffs);
extern unsigned long _dv_vlc_num_bits_block_x86_64(dv_coeff_t* coeffs);

extern unsigned long _dv_vlc_num_bits_block(dv_coeff_t* coeffs)
{
#if (!ARCH_X86) && (!ARCH_X86_64)
	dv_coeff_t * z = coeffs + 1; /* First AC coeff */
	dv_coeff_t * z_end = coeffs + 64;
	int run;
	unsigned long num_bits = 0;

	do {
		run = 0;
		while (*z == 0) {
			z++; run++;
			if (z == z_end) { 
				return num_bits;
			}
		}
		num_bits += vlc_num_bits(run, *z++);
	} while (z != z_end);

	return num_bits;
#elif ARCH_X86_64
	return _dv_vlc_num_bits_block_x86_64(coeffs);
#else
	return _dv_vlc_num_bits_block_x86(coeffs);
#endif
}

static void vlc_make_fit(dv_vlc_block_t * bl, int num_blocks, long bit_budget)
{
	dv_vlc_block_t* b = bl + num_blocks;
	long bits_used = 0;
	for (b = bl; b != bl + num_blocks; b++) {
		bits_used += b->coeffs_bits;
	}
	if (bits_used <= bit_budget) {
		return;
	}
	vlc_overflows++;
	while (bits_used > bit_budget) {
		b--;
		if (b->coeffs_end != b->coeffs + 1) {
			b->coeffs_end--;
			bits_used -= get_dv_vlc_len(*b->coeffs_end);
			b->coeffs_bits -= get_dv_vlc_len(*b->coeffs_end);
		}
		if (b == bl) {
			b = bl + num_blocks;
		}
	}
	for (b = bl; b != bl + num_blocks; b++) {
		b->coeffs_end[-1] = set_dv_vlc(0x6, 4); /* EOB */
	}
}

static inline void vlc_split_code(dv_vlc_block_t * src, 
				  dv_vlc_block_t * dst,
				  unsigned char *vsbuffer)
{
	dv_vlc_entry_t* e = src->coeffs_start;	
	long val = get_dv_vlc_val(*e);
	long len = get_dv_vlc_len(*e);
			
	len -= dst->bit_budget; 
	val >>= len;
	dst->bit_offset = put_bits(vsbuffer,
				   dst->bit_offset, dst->bit_budget, val);
	dst->bit_budget = 0;
	*e = set_dv_vlc(get_dv_vlc_val(*e) & ((1 << len)-1), len);
}	   

extern void _dv_vlc_encode_block_pass_1_x86(dv_vlc_entry_t** start,
					dv_vlc_entry_t*  end,
					long* bit_budget,
					long* bit_offset,
					unsigned char* vsbuffer);

extern void _dv_vlc_encode_block_pass_1_x86_64(dv_vlc_entry_t** start,
					dv_vlc_entry_t*  end,
					long* bit_budget,
					long* bit_offset,
					unsigned char* vsbuffer);

static void vlc_encode_block_pass_1(dv_vlc_block_t * bl, 
				    unsigned char *vsbuffer,
				    int vlc_encode_passes)
{
#if (!ARCH_X86) && (!ARCH_X86_64)
	dv_vlc_entry_t * start = bl->coeffs_start;
	dv_vlc_entry_t * end = bl->coeffs_end;
	unsigned long bit_budget = bl->bit_budget;
	unsigned long bit_offset = bl->bit_offset;

	while (start != end && bit_budget >= get_dv_vlc_len(*start)) {
		dv_vlc_entry_t code = *start;
		unsigned long len = get_dv_vlc_len(code);
		unsigned long val = get_dv_vlc_val(code);
		bit_offset = put_bits(vsbuffer, bit_offset, len, val);
		bit_budget -= len;
		start++;
	}

	bl->coeffs_start = start;
	bl->bit_budget = bit_budget;
	bl->bit_offset = bit_offset;
#elif ARCH_X86_64
	_dv_vlc_encode_block_pass_1_x86_64(&bl->coeffs_start, bl->coeffs_end,
				    &bl->bit_budget, &bl->bit_offset,
				    vsbuffer);
#else
	_dv_vlc_encode_block_pass_1_x86(&bl->coeffs_start, bl->coeffs_end,
				    &bl->bit_budget, &bl->bit_offset,
				    vsbuffer);
#endif
	if (vlc_encode_passes > 1) {
		if (bl->coeffs_start == bl->coeffs_end) {
			bl->can_supply = 1;
		} else {
			vlc_split_code(bl, bl, vsbuffer);
			bl->can_supply = 0;
		}
	}
}

static void vlc_encode_block_pass_n(dv_vlc_block_t * blocks, 
				    unsigned char *vsbuffer,
				    int vlc_encode_passes, int current_pass)
{
	dv_vlc_block_t * supplier[30];
	dv_vlc_block_t * receiver[30];
	int b,max_b;
	dv_vlc_block_t ** s_;
	dv_vlc_block_t ** r_;
	dv_vlc_block_t ** s_last = supplier;
	dv_vlc_block_t ** r_last = receiver;

	if (current_pass > vlc_encode_passes) {
		return;
	}

	max_b = (current_pass == 2) ? 6 : 30;

	for (b = 0; b < max_b; b++) {
		dv_vlc_block_t * bl = blocks + b;
		if (!bl->can_supply) {
			if (bl->coeffs_start != bl->coeffs_end) {
				*r_last++ = bl;
			}
		} else if (bl->bit_budget) {
			*s_last++ = bl;
		}
	}

	s_ = supplier;
	r_ = receiver;

	for (; r_ != r_last && s_ != s_last; r_++) {
		dv_vlc_block_t* r = *r_;
		for (; s_ != s_last; s_++) {
			dv_vlc_block_t * s = *s_;
			while (r->coeffs_start != r->coeffs_end && 
			       s->bit_budget >= 
			       get_dv_vlc_len(*r->coeffs_start)) {
				unsigned long val;
				long len;
				val = get_dv_vlc_val(*r->coeffs_start);
				len = get_dv_vlc_len(*r->coeffs_start);
				s->bit_offset = put_bits(vsbuffer, 
							 s->bit_offset,
							 len, val);
				r->coeffs_start++;
				s->bit_budget -= len;
			}
			
			if (r->coeffs_start == r->coeffs_end) {
				break;
			}
			if (s->bit_budget) {
				vlc_split_code(r, s, vsbuffer);
			}
		}
	}

	return;
}

extern int _dv_classify_mmx(dv_coeff_t * a, unsigned short* amp_ofs,
			unsigned short* amp_cmp);

extern int _dv_classify_mmx_x86_64(dv_coeff_t * a, unsigned short* amp_ofs,
			unsigned short* amp_cmp);

static inline int classify(dv_coeff_t * bl)
{
#if ARCH_X86
	static unsigned short amp_ofs[3][4] = { 
		{ 32768+35,32768+35,32768+35,32768+35 },
		{ 32768+23,32768+23,32768+23,32768+23 },
		{ 32768+11,32768+11,32768+11,32768+11 }
	};
	static unsigned short amp_cmp[3][4] = { 
		{ 32768+(35+35),32768+(35+35),32768+(35+35),32768+(35+35) },
		{ 32768+(23+23),32768+(23+23),32768+(23+23),32768+(23+23) },
		{ 32768+(11+11),32768+(11+11),32768+(11+11),32768+(11+11) }
	};
	int i,dc;

	dc = bl[0];
	bl[0] = 0;
	for (i = 0; i < 3; i++) {
		if (_dv_classify_mmx(bl, amp_ofs[i], amp_cmp[i])) {
			bl[0] = dc;
			emms();
			return 3-i;
		}
	}
	bl[0] = dc;
	emms();
	return 0;
#elif ARCH_X86_64
	static unsigned short amp_ofs[3][4] = { 
		{ 32768+35,32768+35,32768+35,32768+35 },
		{ 32768+23,32768+23,32768+23,32768+23 },
		{ 32768+11,32768+11,32768+11,32768+11 }
	};
	static unsigned short amp_cmp[3][4] = { 
		{ 32768+(35+35),32768+(35+35),32768+(35+35),32768+(35+35) },
		{ 32768+(23+23),32768+(23+23),32768+(23+23),32768+(23+23) },
		{ 32768+(11+11),32768+(11+11),32768+(11+11),32768+(11+11) }
	};
	int i,dc;

	dc = bl[0];
	bl[0] = 0;
	for (i = 0; i < 3; i++) {
		if (_dv_classify_mmx_x86_64(bl, amp_ofs[i], amp_cmp[i])) {
			bl[0] = dc;
			emms();
			return 3-i;
		}
	}
	bl[0] = dc;
	emms();
	return 0;
#else
	int rval = 0;

	dv_coeff_t* p = bl + 1;
	dv_coeff_t* p_end = p + 64;

	while (p != p_end) {
		int a = *p++;
		int b = (a >> 15);
		a ^= b;
		a -= b;
		if (rval < a) {
			rval = a;
		}
	}

	if (rval > 35) {
		rval = 3;
	} else if (rval > 23) {
		rval = 2;
	} else if (rval > 11) {
		rval = 1;
	} else {
		rval = 0;
	}

	return rval;
#endif
}

static void do_dct(dv_macroblock_t *mb)
{
	unsigned int b;

	for (b = 0; b < 6; b++) {
		dv_block_t *bl = &mb->b[b];
		
		if (bl->dct_mode == DV_DCT_88) {
			_dv_dct_88(bl->coeffs);
#if (!ARCH_X86) && (!ARCH_X86_64)
			reorder_block(bl);
#endif
#if BRUTE_FORCE_DCT_88
			_dv_weight_88(bl->coeffs);
#endif
		} else {
			_dv_dct_248(bl->coeffs);
			reorder_block(bl);
#if BRUTE_FORCE_DCT_248
			_dv_weight_248(bl->coeffs);
#endif
		}
		dct_used[bl->dct_mode]++;
	}
}

static int classes[3][4] = {
	{ 0, 1, 2, 3},
	{ 1, 2, 3, 3},
	{ 2, 3, 3, 3}
};

static int qnos[4][16] = {
	{ 15,                  8,    6,    4,    2, 0},
	{ 15,         11, 10,  8,    6,    4,    2, 0},
	{ 15, 14, 13, 11,      8,    6,    4,    2, 0},
	{ 15,     13, 12, 10,  8,    6,    4,    2, 0}
};

static int quant_2_static_table[2][20] = {
	{1700, 0, 1500, 2, 1000, 4, 900, 6, 750, 8, 650, 10, 550, 12, 512, 13, 0, 15},
	{1700, 0, 1400, 2, 1200, 4, 1000,6, 800, 8, 650, 10, 550, 12, 512, 13, 0, 15}
};

static int qnos_class_combi[16][16];
static int qno_next_hit[4][16];

void _dv_init_qno_start(void)
{
	int qno;
	int klass;
	int qno_p[4];
	int combi_p[16];

	memset(qno_p, 0, sizeof(qno_p));
	memset(combi_p, 0, sizeof(combi_p));

	for (qno = 15; qno >= 0; qno--) {
		int i;

		for (klass = 0; klass < 4; klass++) {
			if (qnos[klass][qno_p[klass]] > qno) {
				qno_p[klass]++;
			}
                        i = 0;
                        while (qnos[klass][i] > qno) {
                                i++;
                        }
                        qno_next_hit[klass][qno] = i;

		}
		for (i = 1; i < 16; i++) {
			int q = 0;
			for (klass = 0; klass < 4; klass++) {
				if ((i & (1 << klass))
				    && qnos[klass][qno_p[klass]] > q) {
					q = qnos[klass][qno_p[klass]];
				}
			}
			if (combi_p[i] == 0 || 
			    qnos_class_combi[i][combi_p[i] - 1] != q) {
				qnos_class_combi[i][combi_p[i]++] = q;
			}
		}
	}
}

static void do_classify(dv_macroblock_t * mb, int static_qno)
{
	int b;
	dv_block_t *bl;

	if (static_qno) { /* We want to be fast, so don't waste time! */
		for (b=0; b < 6; b++) {
			mb->b[b].class_no = 3;
		}
		return;
	} 

	for (b = 0; b < 4; b++) {
		bl = &mb->b[b];
		bl->class_no = classes[0][classify(bl->coeffs)];
		classes_used[bl->class_no]++;
	}
	bl = &mb->b[4];
	bl->class_no = classes[1][classify(bl->coeffs)];
	classes_used[bl->class_no]++;
	bl = &mb->b[5];
	bl->class_no = classes[2][classify(bl->coeffs)];
	classes_used[bl->class_no]++;
	
}

static void quant_1_pass(dv_videosegment_t* videoseg, 
			 dv_vlc_block_t * vblocks, int static_qno)
{
	dv_macroblock_t *mb;
	int m;
	int b;

	dv_coeff_t bb[6][64];

	for (m = 0, mb = videoseg->mb; m < 5; m++, mb++) {
		int smallest_qno = 15;
		int qno_index;
		int cycles = 0;

		for (b = 0; b < 6; b++) {
			dv_block_t *bl = &mb->b[b];
			unsigned int ac_coeff_budget = (((b < 4) ? 100 : 68) - 4); 
			qno_index = qno_next_hit[bl->class_no][smallest_qno];
			while (smallest_qno > 0) {
				memcpy(bb[b], bl->coeffs, 
				       64 *sizeof(dv_coeff_t));
				_dv_quant(bb[b], smallest_qno, bl->class_no);
				if (_dv_vlc_num_bits_block(bb[b]) 
				    <= ac_coeff_budget)
					break;
				qno_index++;
				cycles++;
				smallest_qno = qnos[bl->class_no][qno_index];
			}
			if (smallest_qno == 0) {
				break;
			}
		}

		mb->qno = smallest_qno;
		cycles_used[cycles]++;
		qnos_used[smallest_qno]++;
		if (smallest_qno != 15) { 
			for (b = 0; b < 6; b++) {
				dv_block_t *bl = &mb->b[b];
				_dv_quant(bl->coeffs, smallest_qno, bl->class_no);
				vlc_encode_block(bl->coeffs, vblocks + b);
			}
			
			if (smallest_qno == 0) {
				for (b = 0; b < 6; b++) {
					vlc_make_fit(vblocks + b, 1,
						     ((b < 4) ? 100 : 68));
				}
			}
		} else {
			for (b = 0; b < 6; b++) {
				vlc_encode_block(bb[b], vblocks + b);
			}
		}
		vblocks += 6;
	}
}

static void quant_2_passes(dv_videosegment_t* videoseg, 
			   dv_vlc_block_t * vblocks, int static_qno)
{
	dv_macroblock_t *mb;
	int m;
	dv_coeff_t bb[6][64];
	const int ac_coeff_budget = 4*100+2*68-6*4;

	for (m = 0, mb = videoseg->mb; m < 5; m++, mb++) {
		int b;
		int cycles = 0;
		int bits_used = 0;
		int qno = 15;
		int run = 0;
#if 0
		int bits_used_ = 0;
#endif

		for (b = 0; b < 6; b++) {
			int bits;
			dv_block_t *bl = &mb->b[b];
			memcpy(bb[b], bl->coeffs, 64 *sizeof(dv_coeff_t));
			_dv_quant(bb[b], qno, bl->class_no);
			bits = _dv_vlc_num_bits_block(bb[b]);
			bits_used += bits;
#if 0
			bits_used_ += bits;
#endif
		}

		if (static_qno && bits_used > ac_coeff_budget) {
			int i = 0;
			while (bits_used <= 
			       quant_2_static_table[static_qno-1][i])
				i += 2;
			qno = quant_2_static_table[static_qno-1][i+1];
		} else if (bits_used > ac_coeff_budget) {
			int qno_ok = 0;
			int runs = (bits_used - ac_coeff_budget) / 
				VLC_BITS_ON_FULL_MBLOCK_CYCLE_QUANT_2 + 1;
			int qno_incr = 8;
			int i;
			qno++;

			for (; run < runs 
				     && run < VLC_MAX_RUNS_PER_CYCLE_QUANT_2;
			     run++) {
				qno -= qno_incr;
				qno_incr >>= 1;
			}
			for (i = run; i < 5; i++) {
				bits_used = 0;
				for (b = 0; b < 6; b++) {
					dv_block_t *bl = &mb->b[b];
					memcpy(bb[b], bl->coeffs, 
					       64 *sizeof(dv_coeff_t));
					_dv_quant(bb[b], qno, bl->class_no);
					bits_used += _dv_vlc_num_bits_block(bb[b]);
				}

				if (bits_used > ac_coeff_budget) {
					qno -= qno_incr;
				} else {
					qno_ok = qno;
					qno += qno_incr;
				}
				cycles++;
				if (qno_incr == 1 && qno < 10) {
					break;
				}
				qno_incr >>= 1;
			} 
			qno = qno_ok;
#if 0
			fprintf(stderr, "%d %d\n", bits_used_, qno);
#endif
		}

		mb->qno = qno;
		runs_used[run]++;
		cycles_used[cycles]++;
		qnos_used[qno]++;
		if (qno != 15) { 
			for (b = 0; b < 6; b++) {
				dv_block_t *bl = &mb->b[b];
				_dv_quant(bl->coeffs, qno, bl->class_no);
				vlc_encode_block(bl->coeffs, vblocks + b);
			}
			if (qno == 0 || static_qno) {
				vlc_make_fit(vblocks, 6, 4*100+2*68);
			}
		} else {
			for (b = 0; b < 6; b++) {
				vlc_encode_block(bb[b], vblocks + b);
			}
		}
		vblocks += 6;
	}
}

static void quant_3_passes(dv_videosegment_t* videoseg, 
			   dv_vlc_block_t * vblocks, int static_qno)
{
	dv_macroblock_t *mb;
	int m;
	int b;
	int smallest_qno[5];
	int qno_index[5];
	int class_combi[5];
	int cycles = 0;
	dv_coeff_t bb[5][6][64];
	const int ac_coeff_budget = 5*(4*100+2*68-6*4);
	int bits_used[5];
	int bits_used_total;
	for (m = 0; m < 5; m++) {
		smallest_qno[m] = 15;
		qno_index[m] = 0;
		class_combi[m] = 0;
	}

	bits_used_total = 0;
	for (m = 0, mb = videoseg->mb; m < 5; m++, mb++) {
		bits_used[m] = 0;
		for (b = 0; b < 6; b++) {
			dv_block_t *bl = &mb->b[b];
			memcpy(bb[m][b], bl->coeffs, 64 * sizeof(dv_coeff_t));
			_dv_quant(bb[m][b], smallest_qno[m], bl->class_no);
			bits_used[m] += _dv_vlc_num_bits_block(bb[m][b]);
			class_combi[m] |= (1 << bl->class_no);
		}
		while (qnos_class_combi[class_combi[m]][qno_index[m]] > 15) {
			qno_index[m]++;
		}
		bits_used_total += bits_used[m];
	}
#if 0
	if (bits_used_total > ac_coeff_budget) {
		fprintf(stderr, "\nWant: %d ", 
			bits_used_total-ac_coeff_budget);
	}
#endif

	if (static_qno && bits_used_total > ac_coeff_budget) {
		for (m = 0; m < 5; m++) {
			int i = 0;
			while (bits_used[m] <= 
			       quant_2_static_table[static_qno-1][i])
				i += 2;
			smallest_qno[m] =
				quant_2_static_table[static_qno-1][i+1];
			if (smallest_qno[m] < 14) {
				smallest_qno[m] ++; /* just guessed... */
			}
		}
	} else while (bits_used_total > ac_coeff_budget) {
		int m_max = 0;
		int bits_used_ = 0;
		int runs = (bits_used_total - ac_coeff_budget) / 
			VLC_BITS_ON_FULL_MBLOCK_CYCLE_QUANT_3 + 1;
		int run;

		for (m = 1; m < 5; m++) {
			if (bits_used[m] > bits_used[m_max]) {
				m_max = m;
			}
		}
		m = m_max;
		mb = videoseg->mb + m;

		cycles++;
		
		for (run = 0; run < runs 
			     && run < VLC_MAX_RUNS_PER_CYCLE_QUANT_3; 
		     run++) {
			qno_index[m]++;
			smallest_qno[m] = 
				qnos_class_combi[class_combi[m]][qno_index[m]];
			if (smallest_qno[m] == 0) {
				break;
			}
		}
		runs_used[run]++;
		if (smallest_qno[m] == 0) {
			break;
		}
		
		for (b = 0; b < 6; b++) {
			dv_block_t *bl = &mb->b[b];
			memcpy(bb[m][b], bl->coeffs, 64 *sizeof(dv_coeff_t));
			_dv_quant(bb[m][b], smallest_qno[m], bl->class_no);
			bits_used_ += _dv_vlc_num_bits_block(bb[m][b]);
		}
#if 0
		fprintf(stderr, "(qno: %d, gain: %d, run: %d) ", 
			smallest_qno[m], bits_used[m] - bits_used_, run);
#endif
		bits_used_total -= bits_used[m];
		bits_used_total += bits_used_;
		bits_used[m] = bits_used_;
	}

	cycles_used[cycles]++;
	for (m = 0, mb = videoseg->mb; m < 5; m++, mb++) {
		mb->qno = smallest_qno[m];
		qnos_used[smallest_qno[m]]++;
		if (smallest_qno[m] != 15) { 
			for (b = 0; b < 6; b++) {
				dv_block_t *bl = &mb->b[b];
				_dv_quant(bl->coeffs,smallest_qno[m],bl->class_no);
				vlc_encode_block(bl->coeffs,vblocks+6 * m + b);
			}
		} else {
			for (b = 0; b < 6; b++) {
				vlc_encode_block(bb[m][b], vblocks+6 * m + b);
			}
		}
	}
	if (bits_used_total > ac_coeff_budget) {
		vlc_make_fit(vblocks, 30, 5*(4*100+2*68));
	}
}

static void process_videosegment(dv_enc_input_filter_t * input,
				 dv_videosegment_t* videoseg,
				 uint8_t * vsbuffer, int vlc_encode_passes,
				 int static_qno)
{
	dv_macroblock_t *mb;
	int m;
	unsigned int b;
	dv_vlc_block_t vlc_block[5*6];

	for (m = 0, mb = videoseg->mb; m < 5; m++, mb++) {
		mb->vlc_error = 0;
		mb->eob_count = 0;
		mb->i = (videoseg->i+ dv_super_map_vertical[m]) 
			% (videoseg->isPAL ? 12 : 10);
		mb->j = dv_super_map_horizontal[m];
		mb->k = videoseg->k;
		
		if (videoseg->isPAL) {
			dv_place_420_macroblock(mb);
		} else {
			dv_place_411_macroblock(mb);
		}
		input->fill_macroblock(mb, videoseg->isPAL);
		do_dct(mb);
		do_classify(mb, static_qno);
	}

#if 0
	for (m = 0; m < 5; m++) {
		dv_vlc_block_t *v_bl = vlc_block;
		for (b = 0; b < 6; b++) {
			v_bl->bit_offset = (8* (80 * m))+dv_parse_bit_start[b];
			v_bl->bit_budget = (b < 4) ? 100 : 68;
			v_bl++;
		}
	}
#endif

	switch (vlc_encode_passes) {
	case 1:
		quant_1_pass(videoseg, vlc_block, static_qno);
		break;
	case 2:
		quant_2_passes(videoseg, vlc_block, static_qno);
		break;
	case 3:
		quant_3_passes(videoseg, vlc_block, static_qno);
		break;
	default:
		fprintf(stderr, "Invalid value for vlc_encode_passes "
			"specified: %d!\n", vlc_encode_passes);
		exit(-1);
	}
		
	for (m = 0, mb = videoseg->mb; m < 5; m++, mb++) {
		put_bits(vsbuffer, (8 * (80 * m)) + 28, 4, mb->qno);
		
		for (b = 0; b < 6; b++) {
			dv_block_t *bl = &mb->b[b];
			dv_vlc_block_t *v_bl = vlc_block + m*6 + b;

			v_bl->bit_offset = (8* (80 * m))+dv_parse_bit_start[b];
			v_bl->bit_budget = (b < 4) ? 100 : 68;

			put_bits(vsbuffer, v_bl->bit_offset - 12,
				 12, (bl->coeffs[0] << 3) | (bl->dct_mode << 2)
				 | bl->class_no);
			vlc_encode_block_pass_1(v_bl, vsbuffer, 
						vlc_encode_passes);
		}
		vlc_encode_block_pass_n(vlc_block + m * 6, vsbuffer, 
					vlc_encode_passes, 2);
	}
	vlc_encode_block_pass_n(vlc_block, vsbuffer, vlc_encode_passes, 3);
}

static void encode(dv_enc_input_filter_t * input,
		   int isPAL, unsigned char* target, int vlc_encode_passes,
		   int static_qno)
{
	static dv_videosegment_t videoseg ALIGN64;

	int numDIFseq;
	int ds;
	int v;
	unsigned int dif;
	unsigned int offset;

	memset(target, 0, 144000);

	dif = 0;
	offset = dif * 80;
	if (isPAL) {
		target[offset + 3] |= 0x80;
	}

	numDIFseq = isPAL ? 12 : 10;

	for (ds = 0; ds < numDIFseq; ds++) { 
		/* Each DIF segment conists of 150 dif blocks, 
		   135 of which are video blocks */
		dif += 6; /* skip the first 6 dif blocks in a dif sequence */
		/* A video segment consists of 5 video blocks, where each video
		   block contains one compressed macroblock.  DV bit allocation
		   for the VLC stage can spill bits between blocks in the same
		   video segment.  So parsing needs the whole segment to decode
		   the VLC data */
		/* Loop through video segments */
		for (v = 0; v < 27; v++) {
			/* skip audio block - 
			   interleaved before every 3rd video segment
			*/

			if(!(v % 3)) dif++; 

			offset = dif * 80;
			
			videoseg.i = ds;
			videoseg.k = v;
			videoseg.isPAL = isPAL;

			process_videosegment(input, 
					     &videoseg, target + offset,
					     vlc_encode_passes,
					     static_qno);
			
			dif += 5;
		} 
	} 
}

int dv_encoder_loop(dv_enc_input_filter_t * input,
		 dv_enc_audio_input_filter_t * audio_input,
		 dv_enc_output_filter_t * output,
		 int start, int end, const char* filename,
		 const char* audio_filename,
		 int vlc_encode_passes, int static_qno, int verbose_mode,
		 int fps, int is16x9)
{
	time_t now;
	int i;
	unsigned char target[144000];
	unsigned char fbuf[1024];
	dv_enc_audio_info_t audio_info_;
	dv_enc_audio_info_t* audio_info;
	long skip_frames_pal = fps ? fps * 65536 / 25 : 65536;
	long skip_frames_ntsc = fps ? fps * 65536 / 30 : 65536;
	long skip_frame_count = 0;
	int isPAL = -1;

	audio_info = (audio_input != NULL) ? &audio_info_ : NULL;

	now = time(NULL);

	if (audio_input) {
		if (audio_input->init(audio_filename, audio_info) < 0) {
			return(-1);
		}
		if (verbose_mode) {
			fprintf(stderr, "Opening audio source with:\n"
				"Channels: %d\n"
				"Frequency: %d\n"
				"Bytes per second: %d\n"
				"Byte alignment: %d\n"
				"Bits per sample: %d\n",
				audio_info->channels, audio_info->frequency, 
				audio_info->bytespersecond,
				audio_info->bytealignment, 
				audio_info->bitspersample);
		}
	}

	if (verbose_mode && start > 0) {
		fprintf(stderr, "Skipping %d frames (video and audio!!!)\n",
			start);
	}
	for (i = 0; i < start; i++) {
		snprintf(fbuf, 1024, filename, i);
		if (audio_input) {
			if (audio_input->load(audio_info, isPAL) < 0) {
				return -1;
			}
		}
		if (input->skip(fbuf, &isPAL) < 0) {
			return -1;
		}
	}

	for (i = start; i <= end; i++) {
		long skip_frame_step;
		int skipped = 0;

		snprintf(fbuf, 1024, filename, i);

		skip_frame_step = isPAL ? skip_frames_pal : skip_frames_ntsc;
		skip_frame_count += 65536 - skip_frame_step;

		if (audio_input) {
			if (audio_input->load(audio_info, isPAL) < 0) {
				return -1;
			}
		}
		if (skip_frame_count < 65536 || isPAL == -1) {
			if (input->load(fbuf, &isPAL) < 0) {
				return -1;
			}
		} else {
			if (input->skip(fbuf, &isPAL) < 0) {
				return -1;
			}
		}
		if (skip_frame_count < 65536) {
			encode(input, isPAL, target, vlc_encode_passes, 
			       static_qno);
		} else {
			skip_frame_count -= 65536;
			skipped = 1;
		}
		if (output->store(target, audio_info, FALSE,
				  isPAL, is16x9,now) < 0) {
			return -1;
		}
		if (verbose_mode) {
			if (skipped) {
				fprintf(stderr, "_%d_ ", i);
			} else {
				fprintf(stderr, "[%d] ", i);
			}
		}
	}
	return 0;
}

void dv_show_statistics()
{
	int i = 0;
	fprintf(stderr, "\n\nFinal statistics:\n"
		"========================================================\n"
		"\n  |CYCLES    |RUNS/CYCLE|QNOS     |CLASS    "
		"|VLC OVERF|DCT\n"
		"========================================================\n");
	fprintf(stderr, "%2d: %8ld |%8ld  |%8ld |%8ld |%8ld "
		"|%8ld (DCT88)\n", 
		i, cycles_used[i], runs_used[i], qnos_used[i],
		classes_used[i], vlc_overflows, dct_used[DV_DCT_88]);
	i++;
	fprintf(stderr, "%2d: %8ld |%8ld  |%8ld |%8ld |         "
		"|%8ld (DCT248)\n", 
		i, cycles_used[i], runs_used[i], qnos_used[i],
		classes_used[i], 
		dct_used[DV_DCT_248]);
	i++;
	for (;i < 4; i++) {
		fprintf(stderr, "%2d: %8ld |%8ld  |%8ld |%8ld |         |\n", 
			i, cycles_used[i], runs_used[i], qnos_used[i],
			classes_used[i]);
	}
	for (;i < 16; i++) {
		fprintf(stderr, "%2d: %8ld |%8ld  |%8ld |         "
			"|         |\n", 
			i, cycles_used[i], runs_used[i], qnos_used[i]);
	}
}


/****** public encoder implementation ***********************************/
/* By Dan Dennedy <dan@dennedy.org> */

/** @brief Construct a dv_encoder_t with settings.
 *
 *  This function allocates memory for you that must be set free using
 *  dv_encoder_free()
 * 
 * @param ignored A retired option.
 * @param clamp_luma A boolean to indicate that luma values should be 
 *          trimmed to their ITU-R 601 legal limits. When this is false, it
 *          preserves superblack and superwhite.
 *          Typically, this should be set true.
 * @param clamp_chroma A boolean to indicate that color values should be
 *          trimmed to their ITU-R 601 legal limits.
 *          Typically this should be set true.
 */
dv_encoder_t * 
dv_encoder_new(int ignored, int clamp_luma, int clamp_chroma) {
  dv_encoder_t *result;
  
  result = (dv_encoder_t *)calloc(1,sizeof(dv_encoder_t));
  if(!result) return(NULL);
  
  dv_init( clamp_luma, clamp_chroma);

  result->img_y = (short*) calloc(DV_PAL_HEIGHT * DV_WIDTH, sizeof(short));
  if(!result->img_y) goto no_y;
  result->img_cr = (short*) calloc(DV_PAL_HEIGHT * DV_WIDTH / 2, sizeof(short));
  if(!result->img_cr) goto no_cr;
  result->img_cb = (short*) calloc(DV_PAL_HEIGHT * DV_WIDTH / 2, sizeof(short));
  if(!result->img_cb) goto no_cb;

  result->rem_ntsc_setup = FALSE;
  result->clamp_luma = clamp_luma;
  result->clamp_chroma = clamp_chroma;
  result->force_dct = DV_DCT_AUTO;

  result->frame_count = 0;
  return(result);
  
no_cb:
  free(result->img_cb);
no_cr:
  free(result->img_y);
no_y:
  free(result);
  
  return(NULL);

} /* dv_encoder_new */


/** @brief Destroy a dv_encoder_t.
 *
 * This function deallocates the memory allocated by dv_encoder_new().
 * 
 * @param encoder A pointer to a dv_encoder_t structure
 *
 */
void
dv_encoder_free( dv_encoder_t *encoder)
{
  if (encoder != NULL) {
    if (encoder->img_y != NULL) free(encoder->img_y);
    if (encoder->img_cr != NULL) free(encoder->img_cr);
    if (encoder->img_cb != NULL) free(encoder->img_cb);
    free(encoder);
  }
} /* dv_encoder_free */

/** @brief call dv_cleanup() automatically
 *
 * This function calls dv_cleanup() automatically before a dynamic
 * libdv is unloaded.
 * This fixes potential memory leaks when repeatedly loading 
 * and unloading libdv.
 */

static void do_free() __attribute__ ((destructor));

static void do_free() {
  dv_cleanup();
} /* do_free */

/** @brief Free the dynamically allocated global memory.
 *
 * This function deallocates the memory allocated by dv_init().
 *
 */
void dv_cleanup(void) {
  /* the encoder allocates mem on the heap and must be free-ed */
  if (vlc_encode_lookup != NULL) {
    free(vlc_encode_lookup);
    vlc_encode_lookup = NULL;
  }
  if (vlc_num_bits_lookup != NULL) {
    free(vlc_num_bits_lookup);
    vlc_num_bits_lookup = NULL;
  }
} /* dv_cleanup */


int dv_encode_videosegment( dv_encoder_t *dv_enc,
				dv_videosegment_t *videoseg, uint8_t *vsbuffer)
{
	dv_macroblock_t *mb;
	int m;
	unsigned int b;
	dv_vlc_block_t vlc_block[5*6];

	for (m = 0, mb = videoseg->mb; m < 5; m++, mb++) {
		mb->vlc_error = 0;
		mb->eob_count = 0;
		mb->i = (videoseg->i+ dv_super_map_vertical[m]) 
			% (videoseg->isPAL ? 12 : 10);
		mb->j = dv_super_map_horizontal[m];
		mb->k = videoseg->k;
		
		if (videoseg->isPAL) {
			dv_place_420_macroblock(mb);
		} else {
			dv_place_411_macroblock(mb);
		}
		_dv_ycb_fill_macroblock(dv_enc, mb);
		do_dct(mb);
		do_classify(mb, dv_enc->static_qno);
	}

#if 0
	for (m = 0; m < 5; m++) {
		dv_vlc_block_t *v_bl = vlc_block;
		for (b = 0; b < 6; b++) {
			v_bl->bit_offset = (8* (80 * m))+dv_parse_bit_start[b];
			v_bl->bit_budget = (b < 4) ? 100 : 68;
			v_bl++;
		}
	}
#endif

	switch (dv_enc->vlc_encode_passes) {
	case 1:
		quant_1_pass(videoseg, vlc_block, dv_enc->static_qno);
		break;
	case 2:
		quant_2_passes(videoseg, vlc_block, dv_enc->static_qno);
		break;
	case 3:
		quant_3_passes(videoseg, vlc_block, dv_enc->static_qno);
		break;
	default:
		fprintf(stderr, "Invalid value for vlc_encode_passes "
			"specified: %d!\n", dv_enc->vlc_encode_passes);
		exit(-1);
	}

	for (m = 0, mb = videoseg->mb; m < 5; m++, mb++) {
		put_bits(vsbuffer, (8 * (80 * m)) + 28, 4, mb->qno);
		
		for (b = 0; b < 6; b++) {
			dv_block_t *bl = &mb->b[b];
			dv_vlc_block_t *v_bl = vlc_block + m*6 + b;

			v_bl->bit_offset = (8* (80 * m))+dv_parse_bit_start[b];
			v_bl->bit_budget = (b < 4) ? 100 : 68;

			put_bits(vsbuffer, v_bl->bit_offset - 12,
				 12, (bl->coeffs[0] << 3) | (bl->dct_mode << 2)
				 | bl->class_no);
			vlc_encode_block_pass_1(v_bl, vsbuffer, 
						dv_enc->vlc_encode_passes);
		}
		vlc_encode_block_pass_n(vlc_block + m * 6, vsbuffer, 
					dv_enc->vlc_encode_passes, 2);
	}
	vlc_encode_block_pass_n(vlc_block, vsbuffer, dv_enc->vlc_encode_passes, 3);

	return 0;
}

/* ---------------------------------------------------------------------------
 */
static void
yuy2_to_ycb( uint8_t *data, int isPAL, short *img_y, short *img_cr, short *img_cb)
{
	register int total = (DV_WIDTH * (isPAL ? DV_PAL_HEIGHT : DV_NTSC_HEIGHT)) >> 1;
	register int i;
	register uint8_t *p = data;

	for (i = 0; i < total; i++) {
		img_y[i*2]   = (((short) *p++) - 128) << DCT_YUV_PRECISION;
		img_cb[i]    = (((short) *p++) - 128) << DCT_YUV_PRECISION;
		img_y[i*2+1] = (((short) *p++) - 128) << DCT_YUV_PRECISION;
		img_cr[i]    = (((short) *p++) - 128) << DCT_YUV_PRECISION;
	}
}


#ifdef YUV_420_USE_YV12
/* ---------------------------------------------------------------------------
 */
static void
yv12_to_ycb( uint8_t **in, int isPAL, short *img_y, short *img_cr, short *img_cb)
{
	register int total = DV_WIDTH * (isPAL ? DV_PAL_HEIGHT : DV_NTSC_HEIGHT);
	register int i, j, k;
	
	
	for (i = 0; i < total; i++) img_y[i]   = (((short) in[0][i]) - 128) << DCT_YUV_PRECISION;
	
	for (i = 0; i < (isPAL ? DV_PAL_HEIGHT : DV_NTSC_HEIGHT)/2; ++i) {
	  for (j = 0; j < DV_WIDTH/2 ; j++) {
	    
	    k = i * DV_WIDTH/2 + j;
	    
	    img_cb[2*i*DV_WIDTH/2+j]    = (((short) in[1][k]) - 128) << DCT_YUV_PRECISION;
	    img_cr[2*i*DV_WIDTH/2+j]    = (((short) in[2][k]) - 128) << DCT_YUV_PRECISION;
	    img_cb[(2*i+1)*DV_WIDTH/2+j]    = (((short) in[1][k]) - 128) << DCT_YUV_PRECISION;
	    img_cr[(2*i+1)*DV_WIDTH/2+j]    = (((short) in[2][k]) - 128) << DCT_YUV_PRECISION;
	  }//col-j
	}//row-i

}
#endif


/** @brief DV encode a buffer containing a frame of video
 * 
 * DV interlaced video is always lower field first.
 * 
 * @param dv_enc A pointer to a dv_encoder_t struct containing relevant options:
 *        -isPAL  Set true (non-zero) to encode the data in PAL format.
 *        -is16x9 Set true (non-zero) to set the flag bits in the DV
 *          header to indicate widescreen video.
 *        -vlc_encode_passes variable length coding distribution passes
 *          (1-3) greater values = better quality but not necessarily slower
 *          encoding! If zero or >3. this defaults to 3 for best quality.
 *        -static_qno Default is 0 for dynamic quantisation.
 *          Static qno tables can be used for quantisation on 2+ VLC passes. 
 *          There are only two static qno tables registered right now:\n
 *            1 : for sharp DV pictures\n
 *            2 : for somewhat noisy satelite television signal
 *        -force_dct Normally, motion estimation is used to determine
 *          the level of interfield motion in interlaced video in order to
 *          the best configuration of the DCT algorithm. This option forces
 *          a particular one. Use DV_DCT_AUTO(-1), DV_DCT_88(0), or DV_DCT_248(1).
 * @param in An array of buffers. YUV/YUY2 and RGB only require
 *          one entry. If you configured YUV for YV12. Then 3 array entries
 *          correspond to pointers to the Y (luma) buffer, Cb and Cr (chroma)
 *          buffers.
 * @param color_space Indicates which color space and sample pattern
 *          of the data in the \c in parameter.
 * @param out A pointer to the output buffer, which should alreayd be
 *          allocated width*height*2 bytes. The function will clear the buffer
 *          before filling it.
 * @return -1 for failure, 0 for success
 */
int dv_encode_full_frame(dv_encoder_t *dv_enc, uint8_t **in,
			dv_color_space_t color_space, uint8_t *out)
{
	dv_videosegment_t videoseg ALIGN64;
	int numDIFseq;
	int ds, v, i;
	unsigned int dif = 0;
	unsigned int offset = 0;
	uint8_t *target = out;
	time_t now;
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	
	now = time(NULL);
	if (dv_enc->vlc_encode_passes < 1 || dv_enc->vlc_encode_passes > 3)
		dv_enc->vlc_encode_passes = 3;
	if (dv_enc->static_qno < 1 || dv_enc->static_qno > 2)
		dv_enc->static_qno = 0;
	if (dv_enc->force_dct < DV_DCT_AUTO || dv_enc->force_dct > DV_DCT_248)
		dv_enc->force_dct = DV_DCT_AUTO;

	memset(out, 0, 480 * (dv_enc->isPAL ? 300 : 250));

	pthread_mutex_lock(&mutex);
	switch (color_space) {
	case e_dv_color_rgb:
		dv_enc_rgb_to_ycb(in[0], (dv_enc->isPAL ? DV_PAL_HEIGHT : DV_NTSC_HEIGHT),
			dv_enc->img_y, dv_enc->img_cr, dv_enc->img_cb);
		break;
	case e_dv_color_yuv:
#ifndef YUV_420_USE_YV12
		yuy2_to_ycb( in[0], dv_enc->isPAL, dv_enc->img_y, dv_enc->img_cr, dv_enc->img_cb);
#else
		yv12_to_ycb( in, dv_enc->isPAL, dv_enc->img_y, dv_enc->img_cr, dv_enc->img_cb);
#endif
		break;
	default:
		fprintf(stderr, "Invalid value for color_space "
			"specified: %d!\n", (int) color_space);
		pthread_mutex_unlock(&mutex);
		return -1;
	}
	
	if (dv_enc->isPAL == FALSE && dv_enc->rem_ntsc_setup == TRUE) {
		for (i = 0;
			i < (DV_WIDTH * (dv_enc->isPAL ? DV_PAL_HEIGHT : DV_NTSC_HEIGHT));
			dv_enc->img_y[i++] -= (32) );
	}
			
	/* -224 = (16-128)*2
	    214 = (235-128)*2
	*/
	if (dv_enc->clamp_luma == TRUE) {
		for (i = 0;
			i < (DV_WIDTH * (dv_enc->isPAL ? DV_PAL_HEIGHT : DV_NTSC_HEIGHT));
			dv_enc->img_y[i] = CLAMP(dv_enc->img_y[i++], -224, 214) );
	}
		
	/* -224 = (16-128)*2
	    224 = (240-128)*2
	*/
	if (dv_enc->clamp_chroma == TRUE) {
		for (i = 0;
			i < (DV_WIDTH * (dv_enc->isPAL ? DV_PAL_HEIGHT : DV_NTSC_HEIGHT) >> 2);
			i++ ) {
			dv_enc->img_cb[i] = CLAMP(dv_enc->img_cb[i], -224, 224);
			dv_enc->img_cr[i] = CLAMP(dv_enc->img_cr[i], -224, 224);
		}
	}

	if (dv_enc->isPAL) 
		target[offset + 3] |= 0x80;

	numDIFseq = dv_enc->isPAL ? 12 : 10;
	
	for (ds = 0; ds < numDIFseq; ds++) { 
		/* Each DIF segment conists of 150 dif blocks, 
		   135 of which are video blocks */
		/* skip the first 6 dif blocks in a dif sequence */
		dif += 6; 

		/* A video segment consists of 5 video blocks, where each video
		   block contains one compressed macroblock.  DV bit allocation
		   for the VLC stage can spill bits between blocks in the same
		   video segment.  So parsing needs the whole segment to decode
		   the VLC data */
		/* Loop through video segments */
		for (v = 0; v < 27; v++) {
			/* skip audio block -
			   interleaved before every 3rd video segment
			*/

			if(!(v % 3)) dif++; 

			offset = dif * 80;
			
			videoseg.i = ds;
			videoseg.k = v;
			videoseg.isPAL = dv_enc->isPAL;

			if (dv_encode_videosegment(dv_enc, &videoseg, target + offset) < 0) {
				fprintf(stderr, "Enocder failed to process video segment.");
				pthread_mutex_unlock(&mutex);
				return -1;
			}
			
			dif += 5;
		} 
	} 
	
	_dv_write_meta_data(target, dv_enc->frame_count++, dv_enc->isPAL, dv_enc->is16x9, &now);

	pthread_mutex_unlock(&mutex);

	return 0;
}

#ifdef __linux__
void swab(const void*, void*, ssize_t);
#endif

/** @brief Encode signed 16-bit integer PCM audio data into a frame of DV video.
 *
 * @param dv_enc A pointer to a dv_encoder_t struct containing relevant options:
 *        -isPAL  Set true (non-zero) to encode the data in PAL format.
 * @param pcm An array of buffers containing PCM audio data where each
 *          array entry corresponds to a single channel of audio.
 * @param channels The number of channels being used (<=4). Currently,
 *          only two channels are supported.
 * @param frequency The sampling rate of the input must be one of 32000,
 *          44100, or 48000.
 * @param frame_buf A pointer to a DV frame
 * @return -1 for failure, 0 for success
 * 
 * @todo handle 4 channels
 */
int dv_encode_full_audio(dv_encoder_t *dv_enc, int16_t **pcm,
			int channels, int frequency, uint8_t *frame_buf)
{
	/* this assumes signed 16 bit pcm audio */
	int i,j;
	dv_enc_audio_info_t audio;
	audio.channels = channels;
	audio.frequency = frequency;
	audio.bitspersample = 16;
	audio.bytealignment = 4;
	audio.bytespersecond = frequency * audio.bytealignment;

	/* estimate the number of samples per frame if not specified. */
	dv_enc->isPAL = frame_buf[ 3 ] & 0x80;
	if ( dv_enc->samples_this_frame == 0 )
		audio.bytesperframe = audio.bytespersecond/(dv_enc->isPAL ? 25 : 30);
	else
		audio.bytesperframe = dv_enc->samples_this_frame;


	/* interleave channels */
	if (channels > 1) {
		for (i=0; i < DV_AUDIO_MAX_SAMPLES; i++)
			for (j=0; j < channels; j++)
				swab( pcm[j]+i, audio.data + (i*2+j)*channels, 2);
	}

	return _dv_raw_insert_audio(frame_buf, &audio, dv_enc->isPAL);
}

/** @brief Calculate number of samples to be applied to a new frame.
 *
 * @param dv_enc A pointer to a dv_encoder_t struct containing relevant options:
 *        -isPAL  Set true (non-zero) to encode the data in PAL format.
 * @param frequency The sampling rate of the input must be one of 32000,
 *          44100, or 48000.
 * @param frame_count a sequential number representing the frame number.
 * @return number of samples associated to the frame or 0 if invalid frequency
 * 			received.
 *
 * @todo confirm samples returned for all frequencies
 */

int dv_calculate_samples( dv_encoder_t *encoder, int frequency, int frame_count )
{
	int samples = 0;

	if ( encoder->isPAL )
	{
		samples = frequency / 25;

		switch( frequency )
		{
			case 48000:
				if ( frame_count % 25 == 0 )
					samples --;
				break;
			case 44100:
			case 32000:
				break;
			default:
				samples = 0;
				break;
		}
	}
	else
	{
		samples = frequency / 30;

		switch ( frequency )
		{
			case 48000:
				if ( frame_count % 5 != 0 )
					samples += 2;
				break;
			case 44100:
				if ( frame_count % 300 == 0 )
					samples = 1471;
				else if ( frame_count % 30 == 0 )
					samples = 1470;
				else if ( frame_count % 2 == 0 )
					samples = 1472;
				else
					samples = 1471;
				break;
			case 32000:
				if ( frame_count % 30 == 0 )
					samples = 1068;
				else if ( frame_count % 29 == 0 )
					samples = 1067;
				else if ( frame_count % 4 == 2 )
					samples = 1067;
				else
					samples = 1068;
				break;
			default:
				samples = 0;
		}
	}

	encoder->samples_this_frame = samples;

	return samples;
}
