/* 
 *  dct_block_mmx_x86_64.S
 *
 *     Copyright (C) Peter Schlaile - February 2001
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

# rdi - input and output data pointer
# the input data each 16 bit element in the 8x8 matrix is left aligned:
# the output data is tranposed and each 16 bit element in the 8x8 matrix is left aligned:
# e.g. in 11...1110000 format
# israelh. 11/11/97 removed emms. moved to stub
# MMX implementation. Using MMX transpose 

#define YUV_PRECISION  1
	
#define NSHIFT     15
#define PRESHIFT   1
#define WA4_SHIFT  (NSHIFT-1)
#define WA5_SHIFT  (NSHIFT+1)


.data

.align 8	
			
WA1: .word 23171,23171,23171,23171  /* 0.70711 * 32768 */
WA2: .word 17734,17734,17734,17734  /* 0.54120 * 32768 */
WA3: .word 23171,23171,23171,23171  /* 0.70711 * 32768 */
WA4: .word 21407,21407,21407,21407  /* 1.30658 * 16384 */
WA5: .word 25079,25079,25079,25079  /* 0.38268 * 65536 */

scratch1:       .quad 0
scratch2:       .quad 0
scratch3:       .quad 0
scratch4:       .quad 0

.section .note.GNU-stack, "", @progbits

.text

.align 8	
.global _dv_dct_88_block_mmx_x86_64
_dv_dct_88_block_mmx_x86_64:

/* void _dv_dct_88_block_mmx_x86_64(int16_t* block); */
	
	/* argument is block=rdi */

	mov    %rdi, %r11              # block

# Column 0
	movq 16*0(%r11), %mm0          # v0
	movq 16*7(%r11), %mm1          # v7
	movq  %mm0, %mm2               # duplicate v0 
	paddw %mm1, %mm0               # v00: v0+v7  
	psubw %mm1, %mm2               # v07: v0-v7  

	movq 16*1(%r11), %mm1          # v1
	movq 16*6(%r11), %mm3          # v6
	movq  %mm1, %mm4               # duplicate v1 
	paddw %mm3, %mm1               # v01: v1+v6  
	psubw %mm3, %mm4               # v06: v1-v6  

	movq 16*2(%r11), %mm3          # v2
	movq 16*5(%r11), %mm5          # v5
	movq  %mm3, %mm6               # duplicate v2 
	paddw %mm5, %mm3               # v02: v2+v5  
	psubw %mm5, %mm6               # v05: v2-v5  

	movq 16*3(%r11), %mm5          # v3
	movq 16*4(%r11), %mm7          # v4
	movq  %mm7, scratch1(%rip)           # scratch1: v4   ; 
	movq  %mm5, %mm7               # duplicate v3 
	paddw scratch1(%rip), %mm5           # v03: v3+v4  
	psubw scratch1(%rip), %mm7           # v04: v3-v4  
	movq  %mm5, scratch2(%rip)           # scratch2: v03
	movq  %mm0, %mm5               # mm5: v00

	paddw scratch2(%rip), %mm0           # v10: v00+v03   
	psubw scratch2(%rip), %mm5           # v13: v00-v03   
	movq  %mm3, scratch3(%rip)     # scratch3: v02
	movq  %mm1, %mm3               # duplicate v01

	paddw scratch3(%rip), %mm1          # v11: v01+v02
	psubw scratch3(%rip), %mm3          # v12: v01-v02

	movq  %mm6, scratch4(%rip)           # scratch4: v05
	movq  %mm0, %mm6               # duplicate v10

	paddw %mm1, %mm0              # v10+v11
	psubw %mm1, %mm6              # v10-v11

	movq  %mm0, (%r11)             # out0: v10+v11 
	movq  %mm6, 16*4(%r11)         # out4: v10-v11 

	movq  %mm4, %mm0               # mm0: v06
	paddw scratch4(%rip), %mm4          # v15: v05+v06 
	paddw  %mm2, %mm0             # v16: v07+v06

	pmulhw WA3(%rip), %mm4               # v35~: WA3*v15
	psllw  $1, %mm4                # v35: compensate the coeefient scale

	movq   %mm4, %mm6              # duplicate v35
	paddw  %mm2, %mm4             # v45: v07+v35
	psubw  %mm6, %mm2             # v47: v07-v35

	paddw  %mm5, %mm3             # v22: v12+v13

	pmulhw WA1(%rip), %mm3               # v32~: WA1*v22
	psllw  $16-NSHIFT, %mm3        # v32: compensate the coeefient scale
	movq   %mm5, %mm6              # duplicate v13

	paddw  %mm3, %mm5             # v13+v32
	psubw  %mm3, %mm6             # v13-v32

	movq  %mm5, 16*2(%r11)         # out2: v13+v32 
	movq  %mm6, 16*6(%r11)         # out6: v13-v32 


	paddw  scratch4(%rip), %mm7         # v14n: v04+v05
	movq   %mm0, %mm5              # duplicate v16

	psubw  %mm7, %mm0             # va1: v16-v14n
	pmulhw WA5(%rip), %mm0               # va0~:  va1*WA5
	pmulhw WA4(%rip), %mm5               # v36~~: v16*WA4
	pmulhw WA2(%rip), %mm7               # v34~~: v14n*WA2
	psllw  $16-WA4_SHIFT, %mm5     # v36: compensate the coeefient scale 
	psllw  $16-NSHIFT, %mm7        # v34: compensate the coeefient scale

	psubw  %mm0, %mm5              # v36~: v36~~-va0~
	psubw  %mm0, %mm7              # v34~: v34~~-va0~

	movq   %mm4, %mm0              # duplicate v45
	paddw  %mm5, %mm4              # v45+v36
	psubw  %mm5, %mm0              # v45-v36

	movq  %mm4, 16*1(%r11)         # out1: v45+v36 
	movq  %mm0, 16*7(%r11)         # out7: v45-v36 

	movq   %mm2, %mm5              # duplicate v47
	paddw  %mm7, %mm2              # v47+v34
	psubw  %mm7, %mm5              # v47-v34

	movq  %mm2, 16*5(%r11)         # out5: v47+v34 
	movq  %mm5, 16*3(%r11)         # out3: v47-v34 


# column 1

	add $8, %r11                  # point to the next 4 columns. 
		                       # it can be done by adding 8 to 
		                       # immediates but this is nicer

	movq (%r11), %mm0                  # v0
	movq 16*7(%r11), %mm1              # v7
	movq  %mm0, %mm2               # duplicate v0 
	paddw %mm1, %mm0               # v00: v0+v7  
	psubw %mm1, %mm2               # v07: v0-v7  

	movq 16(%r11), %mm1                # v1
	movq 16*6(%r11), %mm3              # v6
	movq  %mm1, %mm4               # duplicate v1 
	paddw %mm3, %mm1               # v01: v1+v6  
	psubw %mm3, %mm4               # v06: v1-v6  

	movq 16*2(%r11), %mm3         # v2
	movq 16*5(%r11), %mm5              # v5
	movq  %mm3, %mm6               # duplicate v2 
	paddw %mm5, %mm3               # v02: v2+v5  
	psubw %mm5, %mm6               # v05: v2-v5  

	movq 16*3(%r11), %mm5              # v3
	movq 16*4(%r11), %mm7              # v4
	movq  %mm7, scratch1(%rip)                    # scratch1: v4   ; 
	movq  %mm5, %mm7               # duplicate v3 
	paddw scratch1(%rip), %mm5           # v03: v3+v4  
	psubw scratch1(%rip), %mm7           # v04: v3-v4  
	movq  %mm5, scratch2(%rip)        # scratch2: v03
	movq  %mm0, %mm5               # mm5: v00

	paddw scratch2(%rip), %mm0           # v10: v00+v03   
	psubw scratch2(%rip), %mm5           # v13: v00-v03   
	movq  %mm3, scratch3(%rip)         # scratc3: v02
	movq  %mm1, %mm3               # duplicate v01

	paddw scratch3(%rip), %mm1           # v11: v01+v02
	psubw scratch3(%rip), %mm3           # v12: v01-v02

	movq  %mm6, scratch4(%rip)         # scratc4: v05
	movq  %mm0, %mm6               # duplicate v10

	paddw %mm1, %mm0                            # v10+v11
	psubw %mm1, %mm6                        # v10-v11

	movq  %mm0, (%r11)         # out0: v10+v11 
	movq  %mm6, 16*4(%r11)          # out4: v10-v11 

	movq  %mm4, %mm0             # mm0: v06
	paddw scratch4(%rip), %mm4         # v15: v05+v06 
	paddw  %mm2, %mm0                       # v16: v07+v06

	pmulhw WA3(%rip), %mm4           # v35~: WA3*v15
	psllw  $16-NSHIFT, %mm4       # v35: compensate the coeefient scale

	movq   %mm4, %mm6            # duplicate v35
	paddw  %mm2, %mm4            # v45: v07+v35
	psubw  %mm6, %mm2            # v47: v07-v35

	paddw  %mm5, %mm3            # v22: v12+v13

	pmulhw WA1(%rip), %mm3           # v32~: WA3*v15
	psllw  $16-NSHIFT, %mm3       # v32: compensate the coeefient scale
	movq   %mm5, %mm6            # duplicate v13

	paddw  %mm3, %mm5            # v13+v32
	psubw  %mm3, %mm6            # v13-v32

	movq  %mm5, 16*2(%r11)          # out2: v13+v32 
	movq  %mm6, 16*6(%r11)          # out6: v13-v32 

	paddw  scratch4(%rip), %mm7                           # v14n: v04+v05
	movq   %mm0, %mm5                               # duplicate v16

	psubw  %mm7, %mm0                               # va1: v16-v14n
	pmulhw WA2(%rip), %mm7                # v34~~: v14n*WA2
	pmulhw WA5(%rip), %mm0                # va0~:  va1*WA5
	pmulhw WA4(%rip), %mm5                        # v36~~: v16*WA4
	psllw  $16-NSHIFT, %mm7
	psllw  $16-WA4_SHIFT, %mm5      # v36: compensate the coeffient 
		# scale note that WA4 is shifted 1 bit less than the others
	
	psubw  %mm0, %mm5   # v36~: v36~~-va0~
	psubw  %mm0, %mm7   # v34~: v34~~-va0~

	movq   %mm4, %mm0    # duplicate v45
	paddw  %mm5, %mm4    # v45+v36
	psubw  %mm5, %mm0    # v45-v36

	movq  %mm4, 16*1(%r11)          # out1: v45+v36 
	movq  %mm0, 16*7(%r11)          # out7: v45-v36 

	movq   %mm2, %mm5    # duplicate v47
	paddw  %mm7, %mm2    # v47+v34
	psubw  %mm7, %mm5    # v47-v34

	movq  %mm2, 16*5(%r11)          # out5: v47+v34 
	movq  %mm5, 16*3(%r11)          # out3: v47-v34 

	ret     $0

/* do mmx postscaling and reordering... */
		
.align 8	
.global _dv_dct_block_mmx_x86_64_postscale_88
_dv_dct_block_mmx_x86_64_postscale_88:

/* void _dv_dct_block_mmx_x86_64_postscale_88(int16_t* block, int16_t* postscale_matrix); */
	
	/* arguments are block=rdi, postscale=rsi */

	push	 %rbx
	push     %r12

	mov	 %rdi,%r11	# block matrix 
	mov	 %rsi,%r12      # postscale matrix 

	sub	$128+2, %rsp    # reserve some stack space in the redzone, plus 2 bytes
	
	movq	0*8(%r11), %mm0
	movq	1*8(%r11), %mm1
	movq	2*8(%r11), %mm2
	movq	3*8(%r11), %mm3

	movq	%mm0, %mm4
	movq	%mm1, %mm5
	movq	%mm2, %mm6
	movq	%mm3, %mm7
	psraw	$0xf, %mm4
	psraw	$0xf, %mm5
	psraw	$0xf, %mm6
	psraw	$0xf, %mm7

	pxor	%mm4, %mm0
	pxor	%mm5, %mm1
	pxor	%mm6, %mm2
	pxor	%mm7, %mm3

	psubw	%mm4, %mm0
	psubw	%mm5, %mm1
	psubw	%mm6, %mm2
	psubw	%mm7, %mm3
		
	pmulhw	0*8(%r12), %mm0
	pmulhw	1*8(%r12), %mm1
	pmulhw	2*8(%r12), %mm2
	pmulhw	3*8(%r12), %mm3
	
	psraw   $YUV_PRECISION, %mm0
	psraw   $YUV_PRECISION, %mm1
	psraw   $YUV_PRECISION, %mm2
	psraw   $YUV_PRECISION, %mm3

	pxor	%mm4, %mm0
	pxor	%mm5, %mm1
	pxor	%mm6, %mm2
	pxor	%mm7, %mm3

	psubw	%mm4, %mm0
	psubw	%mm5, %mm1
	psubw	%mm6, %mm2
	psubw	%mm7, %mm3
	
	movq	%mm0, 0*8(%r11)
	movq	%mm1, 1*8(%r11)
	movq	%mm2, 2*8(%r11)
	movq	%mm3, 3*8(%r11)

	movq	4*8(%r11), %mm0
	movq	5*8(%r11), %mm1
	movq	6*8(%r11), %mm2
	movq	7*8(%r11), %mm3

	movq	%mm0, %mm4
	movq	%mm1, %mm5

	movl	0*2(%r11), %eax
	movw	2*2(%r11), %bx

	movq	%mm2, %mm6
	movq	%mm3, %mm7

	movl	%eax, 1*2(%rsp)
	movw	%bx, 6*2(%rsp)

	psraw	$0xf, %mm4
	psraw	$0xf, %mm5

	movw	3*2(%r11), %ax
	movw	4*2(%r11), %bx
	
	psraw	$0xf, %mm6
	psraw	$0xf, %mm7

	pxor	%mm4, %mm0
	pxor	%mm5, %mm1

	movw	%ax, 7*2(%rsp)
	movw	%bx, 15*2(%rsp)

	pxor	%mm6, %mm2
	pxor	%mm7, %mm3

	movw	5*2(%r11), %ax
	movw	6*2(%r11), %bx
	
	psubw	%mm4, %mm0
	psubw	%mm5, %mm1

	movw	%ax, 16*2(%rsp)
	movw	%bx, 28*2(%rsp)
	
	psubw	%mm6, %mm2
	psubw	%mm7, %mm3

	movw	7*2(%r11), %ax
	movw	8*2(%r11), %bx
	
	pmulhw	4*8(%r12), %mm0
	pmulhw	5*8(%r12), %mm1

	movw	%ax, 29*2(%rsp)
	movw	%bx, 3*2(%rsp)
	
	pmulhw	6*8(%r12), %mm2
	pmulhw	7*8(%r12), %mm3

	movw	9*2(%r11), %ax
	movw	10*2(%r11), %bx
	
	psraw   $YUV_PRECISION, %mm0
	psraw   $YUV_PRECISION, %mm1

	movw	%ax, 5*2(%rsp)
	movw	%bx, 8*2(%rsp)
	
	psraw   $YUV_PRECISION, %mm2
	psraw   $YUV_PRECISION, %mm3

	movw	11*2(%r11), %ax
	movw	12*2(%r11), %bx
	
	pxor	%mm4, %mm0
	pxor	%mm5, %mm1

	movw	%ax, 14*2(%rsp)
	movw	%bx, 17*2(%rsp)
		
	pxor	%mm6, %mm2
	pxor	%mm7, %mm3

	movw	13*2(%r11), %ax
	movw	14*2(%r11), %bx
		
	psubw	%mm4, %mm0
	psubw	%mm5, %mm1

	movw	%ax, 27*2(%rsp)
	movw	%bx, 30*2(%rsp)

	psubw	%mm6, %mm2
	psubw	%mm7, %mm3

	movq	%mm0, 4*8(%r11)
	movq	%mm1, 5*8(%r11)
	movq	%mm2, 6*8(%r11)
	movq	%mm3, 7*8(%r11)
	
	movq	8*8(%r11), %mm0
	movq	9*8(%r11), %mm1

	movw	15*2(%r11), %ax
	movw	16*2(%r11), %bx
	
	movq	10*8(%r11), %mm2
	movq	11*8(%r11), %mm3

	movw	%ax, 43*2(%rsp)
	movw	%bx, 4*2(%rsp)
	
	movq	%mm0, %mm4
	movq	%mm1, %mm5

	movw	17*2(%r11), %ax
	movw	18*2(%r11), %bx
	
	movq	%mm2, %mm6
	movq	%mm3, %mm7

	movw	%ax, 9*2(%rsp)
	movw	%bx, 13*2(%rsp)
	
	psraw	$0xf, %mm4
	psraw	$0xf, %mm5

	movw	19*2(%r11), %ax
	movw	20*2(%r11), %bx
		
	psraw	$0xf, %mm6
	psraw	$0xf, %mm7

	movw	%ax, 18*2(%rsp)
	movw	%bx, 26*2(%rsp)
		
	pxor	%mm4, %mm0
	pxor	%mm5, %mm1

	movw	21*2(%r11), %ax
	movw	22*2(%r11), %bx
	
	pxor	%mm6, %mm2
	pxor	%mm7, %mm3

	movw	%ax, 31*2(%rsp)
	movw	%bx, 42*2(%rsp)

	psubw	%mm4, %mm0
	psubw	%mm5, %mm1

	movw	23*2(%r11), %ax
	movw	24*2(%r11), %bx
	
	psubw	%mm6, %mm2
	psubw	%mm7, %mm3

	movw	%ax, 44*2(%rsp)
	movw	%bx, 10*2(%rsp)
	
	pmulhw	8*8(%r12), %mm0
	pmulhw	9*8(%r12), %mm1

	movw	25*2(%r11), %ax
	movw	26*2(%r11), %bx
	
	pmulhw	10*8(%r12), %mm2
	pmulhw	11*8(%r12), %mm3

	psraw   $YUV_PRECISION, %mm0
	psraw   $YUV_PRECISION, %mm1

	movw	%ax, 12*2(%rsp)
	movw	%bx, 19*2(%rsp)
	
	psraw   $YUV_PRECISION, %mm2
	psraw   $YUV_PRECISION, %mm3

	movw	27*2(%r11), %ax
	movw	28*2(%r11), %bx
	
	pxor	%mm4, %mm0
	pxor	%mm5, %mm1

	movw	%ax, 25*2(%rsp)
	movw	%bx, 32*2(%rsp)
	
	pxor	%mm6, %mm2
	pxor	%mm7, %mm3

	movw	29*2(%r11), %ax
	movw	30*2(%r11), %bx
	
	psubw	%mm4, %mm0
	psubw	%mm5, %mm1

	movw	%ax, 41*2(%rsp)
	movw	%bx, 45*2(%rsp)
	
	psubw	%mm6, %mm2
	psubw	%mm7, %mm3

	movq	%mm0, 8*8(%r11)
	movq	%mm1, 9*8(%r11)
	movq	%mm2, 10*8(%r11)
	movq	%mm3, 11*8(%r11)

	
	movq	12*8(%r11), %mm0
	movq	13*8(%r11), %mm1
	movq	14*8(%r11), %mm2
	movq	15*8(%r11), %mm3

	movq	%mm0, %mm4
	movq	%mm1, %mm5

	movw	31*2(%r11), %ax
	movw	32*2(%r11), %bx
	
	movq	%mm2, %mm6
	movq	%mm3, %mm7

	movw	%ax, 54*2(%rsp)
	movw	%bx, 11*2(%rsp)
	
	psraw	$0xf, %mm4
	psraw	$0xf, %mm5

	movw	33*2(%r11), %ax
	movw	34*2(%r11), %bx
	
	psraw	$0xf, %mm6
	psraw	$0xf, %mm7

	movw	%ax, 20*2(%rsp)
	movw	%bx, 24*2(%rsp)
		
	pxor	%mm4, %mm0
	pxor	%mm5, %mm1
	
	movw	35*2(%r11), %ax
	movw	36*2(%r11), %bx
		
	pxor	%mm6, %mm2
	pxor	%mm7, %mm3
	
	movw	%ax, 33*2(%rsp)
	movw	%bx, 40*2(%rsp)
	
	psubw	%mm4, %mm0
	psubw	%mm5, %mm1

	movw	37*2(%r11), %ax
	movw	38*2(%r11), %bx
	
	psubw	%mm6, %mm2
	psubw	%mm7, %mm3

	movw	%ax, 46*2(%rsp)
	movw	%bx, 53*2(%rsp)
			
	pmulhw	12*8(%r12), %mm0
	pmulhw	13*8(%r12), %mm1

	movw	39*2(%r11), %ax
	movw	40*2(%r11), %bx
	
	pmulhw	14*8(%r12), %mm2
	pmulhw	15*8(%r12), %mm3
		
	movw	%ax, 55*2(%rsp)
	movw	%bx, 21*2(%rsp)
		
	psraw   $YUV_PRECISION, %mm0
	psraw   $YUV_PRECISION, %mm1

	movw	41*2(%r11), %ax
	movw	42*2(%r11), %bx
	
	psraw   $YUV_PRECISION, %mm2
	psraw   $YUV_PRECISION, %mm3

	movw	%ax, 23*2(%rsp)
	movw	%bx, 34*2(%rsp)

	pxor	%mm4, %mm0
	pxor	%mm5, %mm1

	movw	43*2(%r11), %ax
	movw	44*2(%r11), %bx
	
	pxor	%mm6, %mm2
	pxor	%mm7, %mm3
	
	movw	%ax, 39*2(%rsp)
	movw	%bx, 47*2(%rsp)
	
	psubw	%mm4, %mm0
	psubw	%mm5, %mm1

	movw	45*2(%r11), %ax
	movw	46*2(%r11), %bx
	
	psubw	%mm6, %mm2
	psubw	%mm7, %mm3

	movw	%ax, 52*2(%rsp)
	movw	%bx, 56*2(%rsp)
	
	movq	%mm0, 12*8(%r11)
	movq	%mm1, 13*8(%r11)

	movw	47*2(%r11), %ax
	movw	48*2(%r11), %bx
	
	movq	%mm2, 14*8(%r11)
	movq	%mm3, 15*8(%r11)
	
	movw	%ax, 61*2(%rsp)
	movw	%bx, 22*2(%rsp)
	
	movw	49*2(%r11), %ax
	movw	50*2(%r11), %bx

	
	movw	%ax, 35*2(%rsp)
	movw	%bx, 38*2(%rsp)

	
	movw	51*2(%r11), %ax
	movw	52*2(%r11), %bx
	
	movw	%ax, 48*2(%rsp)
	movw	%bx, 51*2(%rsp)

	
	movw	53*2(%r11), %ax
	movw	54*2(%r11), %bx
		
	movw	%ax, 57*2(%rsp)
	movw	%bx, 60*2(%rsp)

	movw	55*2(%r11), %ax
	movw	56*2(%r11), %bx


	movw	%ax, 62*2(%rsp)
	movw	%bx, 36*2(%rsp)
	
	movw	57*2(%r11), %ax
	movw	58*2(%r11), %bx
	
	movw	%ax, 37*2(%rsp)
	movw	%bx, 49*2(%rsp)
	
	movw	59*2(%r11), %ax
	movw	60*2(%r11), %bx

	movw	%ax, 50*2(%rsp)
	movw	%bx, 58*2(%rsp)
	
	movw	61*2(%r11), %ax
	movl	62*2(%r11), %ebx
	
	movw	%ax, 59*2(%rsp)
	movl	%ebx, 63*2(%rsp)

	add	$2, %rsp                 /* give back 2 bytes of stack space */
	
	movq	(%rsp)  , %mm0
	movq	8(%rsp) , %mm1
	movq	16(%rsp), %mm2
	movq	24(%rsp), %mm3

	movq	%mm0, (%r11)
	movq	%mm1, 8(%r11)
	movq	%mm2, 16(%r11)
	movq	%mm3, 24(%r11)

	movq	32(%rsp)   , %mm0
	movq	32+8(%rsp) , %mm1
	movq	32+16(%rsp), %mm2
	movq	32+24(%rsp), %mm3

	movq	%mm0, 32(%r11)
	movq	%mm1, 32+8(%r11)
	movq	%mm2, 32+16(%r11)
	movq	%mm3, 32+24(%r11)

	movq	64(%rsp)   , %mm0
	movq	64+8(%rsp) , %mm1
	movq	64+16(%rsp), %mm2
	movq	64+24(%rsp), %mm3

	movq	%mm0, 64(%r11)
	movq	%mm1, 64+8(%r11)
	movq	%mm2, 64+16(%r11)
	movq	%mm3, 64+24(%r11)

	movq	96(%rsp)   , %mm0
	movq	96+8(%rsp) , %mm1
	movq	96+16(%rsp), %mm2
	movq	96+24(%rsp), %mm3
	
	add	$128, %rsp             /* restore the stack pointer */

	pop	%r12
	pop	%rbx
	ret

.align 8	
.global _dv_dct_248_block_mmx_x86_64
_dv_dct_248_block_mmx_x86_64:

/* void _dv_dct_248_block_mmx_x86_64(int16_t* block); */
	
	/* argument is block=rdi */

	mov    %rdi, %r11          # source

# column 0

	movq 16*0(%r11), %mm0          # v00
	movq 16*2(%r11), %mm1          # v01
	movq 16*4(%r11), %mm2          # v02
	movq 16*6(%r11), %mm3          # v03

	movq %mm0, %mm4                # v00
	movq %mm1, %mm5                # v01
	
	paddw %mm3, %mm0               # v10: v00+v03
	psubw %mm3, %mm4	       # v13: v00-v03
	
	paddw %mm2, %mm1	       # v11: v01+v02
	psubw %mm2, %mm5	       # v12: v01-v02
	
	movq %mm0, %mm3                # v10

	paddw %mm4, %mm5               # v22: v12+v13	
	paddw %mm1, %mm0	       # v20: v10+v11
	psubw %mm1, %mm3	       # v21: v10-v11

	pmulhw WA1(%rip), %mm5               # v32~: WA1*v22
	movq  %mm4, %mm2	
	psllw  $16-NSHIFT, %mm5        # v32: compensate the coeffient scale

	movq  %mm0, 16*0(%r11)
	
	psubw %mm5, %mm2	       # v43: v13-v32	
	paddw %mm4, %mm5	       # v42: v32+v13

	movq  %mm2, 16*6(%r11)
	movq  %mm3, 16*4(%r11)
	movq  %mm5, 16*2(%r11)

# column 1

	add $8, %r11 # point to the next 4 columns. 
	              # it can be done by adding 8 to immediates 
	              # but this is nicer

	movq 16*0(%r11), %mm0          # v00
	movq 16*2(%r11), %mm1          # v01
	movq 16*4(%r11), %mm2          # v02
	movq 16*6(%r11), %mm3          # v03

	movq %mm0, %mm4
	movq %mm1, %mm5
	
	paddw %mm3, %mm0               # v10: v00+v03
	psubw %mm3, %mm4	       # v13: v00-v03
	
	paddw %mm2, %mm1	       # v11: v01+v02
	psubw %mm2, %mm5	       # v12: v01-v02
	
	movq %mm0, %mm3

	paddw %mm4, %mm5               # v22: v12+v13	
	paddw %mm1, %mm0	       # v20: v10+v11
	psubw %mm1, %mm3	       # v21: v10-v11

	pmulhw WA1(%rip), %mm5               # v32~: WA1*v22
	movq  %mm4, %mm2	
	psllw  $16-NSHIFT, %mm5        # v32: compensate the coeffient scale

	movq  %mm0, 16*0(%r11)
	
	psubw %mm5, %mm2	       # v43: v13-v32	
	paddw %mm4, %mm5	       # v42: v32+v13

	movq  %mm2, 16*6(%r11)
	movq  %mm3, 16*4(%r11)
	movq  %mm5, 16*2(%r11)

# column 0 ... second line		

	add $8, %r11

	movq 16*0(%r11), %mm0          # v00
	movq 16*2(%r11), %mm1          # v01
	movq 16*4(%r11), %mm2          # v02
	movq 16*6(%r11), %mm3          # v03

	movq %mm0, %mm4
	movq %mm1, %mm5
	
	paddw %mm3, %mm0               # v10: v00+v03
	psubw %mm3, %mm4	       # v13: v00-v03
	
	paddw %mm2, %mm1	       # v11: v01+v02
	psubw %mm2, %mm5	       # v12: v01-v02
	
	movq %mm0, %mm3

	paddw %mm4, %mm5               # v22: v12+v13	
	paddw %mm1, %mm0	       # v20: v10+v11
	psubw %mm1, %mm3	       # v21: v10-v11

	pmulhw WA1(%rip), %mm5               # v32~: WA1*v22
	movq  %mm4, %mm2	
	psllw  $16-NSHIFT, %mm5        # v32: compensate the coeffient scale

	movq  %mm0, 16*0(%r11)
	
	psubw %mm5, %mm2	       # v43: v13-v32	
	paddw %mm4, %mm5	       # v42: v32+v13

	movq  %mm2, 16*6(%r11)
	movq  %mm3, 16*4(%r11)
	movq  %mm5, 16*2(%r11)

# column 1 ... second line		

	add $8, %r11
	
	movq 16*0(%r11), %mm0          # v00
	movq 16*2(%r11), %mm1          # v01
	movq 16*4(%r11), %mm2          # v02
	movq 16*6(%r11), %mm3          # v03

	movq %mm0, %mm4
	movq %mm1, %mm5
	
	paddw %mm3, %mm0               # v10: v00+v03
	psubw %mm3, %mm4	       # v13: v00-v03
	
	paddw %mm2, %mm1	       # v11: v01+v02
	psubw %mm2, %mm5	       # v12: v01-v02
	
	movq %mm0, %mm3

	paddw %mm4, %mm5               # v22: v12+v13	
	paddw %mm1, %mm0	       # v20: v10+v11
	psubw %mm1, %mm3	       # v21: v10-v11

	pmulhw WA1(%rip), %mm5               # v32~: WA1*v22
	movq  %mm4, %mm2	
	psllw  $16-NSHIFT, %mm5        # v32: compensate the coeffient scale

	movq  %mm0, 16*0(%r11)
	
	psubw %mm5, %mm2	       # v43: v13-v32	
	paddw %mm4, %mm5	       # v42: v32+v13

	movq  %mm2, 16*6(%r11)
	movq  %mm3, 16*4(%r11)
	movq  %mm5, 16*2(%r11)
	
	ret

.align 8	
.global _dv_dct_248_block_mmx_x86_64_post_sum
_dv_dct_248_block_mmx_x86_64_post_sum:

/* void _dv_dct_248_block_mmx_x86_64_post_sum(int16_t* out_block); */
	
	/* argument is block=rdi */

	mov    %rdi, %r11          # source

	movq	16*0(%r11), %mm0
	movq	16*1(%r11), %mm1	
	movq	16*2(%r11), %mm2
	movq	16*3(%r11), %mm3	
	movq	16*4(%r11), %mm4
	movq	16*5(%r11), %mm5	
	movq	16*6(%r11), %mm6	
	movq	16*7(%r11), %mm7
	
	psubw	%mm1, %mm0
	paddw	16*0(%r11), %mm1 
	movq	%mm0, 16*4(%r11)
	movq	%mm1, 16*0(%r11)

	movq	%mm4, %mm1
	movq	%mm2, %mm0
	paddw	%mm5, %mm1
	psubw	%mm5, %mm4		
	paddw	%mm3, %mm0
	psubw	%mm3, %mm2

	movq	%mm0, 16*1(%r11)
	movq	%mm2, 16*5(%r11)			
	movq	%mm1, 16*2(%r11)
	movq	%mm4, 16*6(%r11)			
		
	paddw	%mm6, %mm7
	psubw	16*7(%r11), %mm6

	movq	%mm7, 16*3(%r11)
	movq	%mm6, 16*7(%r11)		
				
	add	$8, %r11

	movq	16*0(%r11), %mm0
	movq	16*1(%r11), %mm1	
	movq	16*2(%r11), %mm2
	movq	16*3(%r11), %mm3	
	movq	16*4(%r11), %mm4
	movq	16*5(%r11), %mm5	
	movq	16*6(%r11), %mm6	
	movq	16*7(%r11), %mm7
	
	psubw	%mm1, %mm0
	paddw	16*0(%r11), %mm1
	movq	%mm0, 16*4(%r11)
	movq	%mm1, 16*0(%r11)

	movq	%mm4, %mm1
	movq	%mm2, %mm0
	paddw	%mm5, %mm1
	psubw	%mm5, %mm4		
	paddw	%mm3, %mm0
	psubw	%mm3, %mm2

	movq	%mm0, 16*1(%r11)
	movq	%mm2, 16*5(%r11)			
	movq	%mm1, 16*2(%r11)
	movq	%mm4, 16*6(%r11)			
		
	paddw	%mm6, %mm7
	psubw	16*7(%r11), %mm6

	movq	%mm7, 16*3(%r11)
	movq	%mm6, 16*7(%r11)		
	
	ret		

.align 8	
.global _dv_dct_block_mmx_x86_64_postscale_248
_dv_dct_block_mmx_x86_64_postscale_248:

/* void _dv_dct_block_mmx_x86_64_postscale_248(int16_t* block, int16_t* postscale_matrix); */
	
	/* arguments are block=rdi, postscale=rsi */

	push	%r12

	mov	%rdi,%r11	# block matrix 
	mov	%rsi, %r12      # postscale matrix 

	movq	0*8(%r11), %mm0
	movq	1*8(%r11), %mm1
	movq	2*8(%r11), %mm2
	movq	3*8(%r11), %mm3

	movq	%mm0, %mm4
	movq	%mm1, %mm5
	movq	%mm2, %mm6
	movq	%mm3, %mm7
	psraw	$0xf, %mm4
	psraw	$0xf, %mm5
	psraw	$0xf, %mm6
	psraw	$0xf, %mm7

	pxor	%mm4, %mm0
	pxor	%mm5, %mm1
	pxor	%mm6, %mm2
	pxor	%mm7, %mm3

	psubw	%mm4, %mm0
	psubw	%mm5, %mm1
	psubw	%mm6, %mm2
	psubw	%mm7, %mm3
		
	pmulhw	0*8(%r12), %mm0
	pmulhw	1*8(%r12), %mm1
	pmulhw	2*8(%r12), %mm2
	pmulhw	3*8(%r12), %mm3
	
	psraw   $YUV_PRECISION, %mm0
	psraw   $YUV_PRECISION, %mm1
	psraw   $YUV_PRECISION, %mm2
	psraw   $YUV_PRECISION, %mm3

	pxor	%mm4, %mm0
	pxor	%mm5, %mm1
	pxor	%mm6, %mm2
	pxor	%mm7, %mm3

	psubw	%mm4, %mm0
	psubw	%mm5, %mm1
	psubw	%mm6, %mm2
	psubw	%mm7, %mm3
	
	movq	%mm0, 0*8(%r11)
	movq	%mm1, 1*8(%r11)
	movq	%mm2, 2*8(%r11)
	movq	%mm3, 3*8(%r11)

	
	movq	4*8(%r11), %mm0
	movq	5*8(%r11), %mm1
	movq	6*8(%r11), %mm2
	movq	7*8(%r11), %mm3

	movq	%mm0, %mm4
	movq	%mm1, %mm5
	movq	%mm2, %mm6
	movq	%mm3, %mm7
	psraw	$0xf, %mm4
	psraw	$0xf, %mm5
	psraw	$0xf, %mm6
	psraw	$0xf, %mm7

	pxor	%mm4, %mm0
	pxor	%mm5, %mm1
	pxor	%mm6, %mm2
	pxor	%mm7, %mm3

	psubw	%mm4, %mm0
	psubw	%mm5, %mm1
	psubw	%mm6, %mm2
	psubw	%mm7, %mm3
		
	pmulhw	4*8(%r12), %mm0
	pmulhw	5*8(%r12), %mm1
	pmulhw	6*8(%r12), %mm2
	pmulhw	7*8(%r12), %mm3
	
	psraw   $YUV_PRECISION, %mm0
	psraw   $YUV_PRECISION, %mm1
	psraw   $YUV_PRECISION, %mm2
	psraw   $YUV_PRECISION, %mm3

	pxor	%mm4, %mm0
	pxor	%mm5, %mm1
	pxor	%mm6, %mm2
	pxor	%mm7, %mm3

	psubw	%mm4, %mm0
	psubw	%mm5, %mm1
	psubw	%mm6, %mm2
	psubw	%mm7, %mm3
	
	movq	%mm0, 4*8(%r11)
	movq	%mm1, 5*8(%r11)
	movq	%mm2, 6*8(%r11)
	movq	%mm3, 7*8(%r11)
	

	movq	8*8(%r11), %mm0
	movq	9*8(%r11), %mm1
	movq	10*8(%r11), %mm2
	movq	11*8(%r11), %mm3

	movq	%mm0, %mm4
	movq	%mm1, %mm5
	movq	%mm2, %mm6
	movq	%mm3, %mm7
	psraw	$0xf, %mm4
	psraw	$0xf, %mm5
	psraw	$0xf, %mm6
	psraw	$0xf, %mm7

	pxor	%mm4, %mm0
	pxor	%mm5, %mm1
	pxor	%mm6, %mm2
	pxor	%mm7, %mm3

	psubw	%mm4, %mm0
	psubw	%mm5, %mm1
	psubw	%mm6, %mm2
	psubw	%mm7, %mm3
		
	pmulhw	8*8(%r12), %mm0
	pmulhw	9*8(%r12), %mm1
	pmulhw	10*8(%r12), %mm2
	pmulhw	11*8(%r12), %mm3
	
	psraw   $YUV_PRECISION, %mm0
	psraw   $YUV_PRECISION, %mm1
	psraw   $YUV_PRECISION, %mm2
	psraw   $YUV_PRECISION, %mm3

	pxor	%mm4, %mm0
	pxor	%mm5, %mm1
	pxor	%mm6, %mm2
	pxor	%mm7, %mm3

	psubw	%mm4, %mm0
	psubw	%mm5, %mm1
	psubw	%mm6, %mm2
	psubw	%mm7, %mm3
	
	movq	%mm0, 8*8(%r11)
	movq	%mm1, 9*8(%r11)
	movq	%mm2, 10*8(%r11)
	movq	%mm3, 11*8(%r11)

		
	movq	12*8(%r11), %mm0
	movq	13*8(%r11), %mm1
	movq	14*8(%r11), %mm2
	movq	15*8(%r11), %mm3

	movq	%mm0, %mm4
	movq	%mm1, %mm5
	movq	%mm2, %mm6
	movq	%mm3, %mm7
	psraw	$0xf, %mm4
	psraw	$0xf, %mm5
	psraw	$0xf, %mm6
	psraw	$0xf, %mm7

	pxor	%mm4, %mm0
	pxor	%mm5, %mm1
	pxor	%mm6, %mm2
	pxor	%mm7, %mm3

	psubw	%mm4, %mm0
	psubw	%mm5, %mm1
	psubw	%mm6, %mm2
	psubw	%mm7, %mm3
		
	pmulhw	12*8(%r12), %mm0
	pmulhw	13*8(%r12), %mm1
	pmulhw	14*8(%r12), %mm2
	pmulhw	15*8(%r12), %mm3
	
	psraw   $YUV_PRECISION, %mm0
	psraw   $YUV_PRECISION, %mm1
	psraw   $YUV_PRECISION, %mm2
	psraw   $YUV_PRECISION, %mm3

	pxor	%mm4, %mm0
	pxor	%mm5, %mm1
	pxor	%mm6, %mm2
	pxor	%mm7, %mm3

	psubw	%mm4, %mm0
	psubw	%mm5, %mm1
	psubw	%mm6, %mm2
	psubw	%mm7, %mm3
	
	movq	%mm0, 12*8(%r11)
	movq	%mm1, 13*8(%r11)
	movq	%mm2, 14*8(%r11)
	movq	%mm3, 15*8(%r11)
			
	pop	 %r12
	
	ret
