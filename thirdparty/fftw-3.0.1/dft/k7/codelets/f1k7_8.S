/*
 * Copyright (c) 2003 Matteo Frigo
 * Copyright (c) 2003 Massachusetts Institute of Technology
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/* This file was automatically generated --- DO NOT EDIT */
/* Generated on Sat Jul  5 21:52:44 EDT 2003 */

#include "config.h"



/* cheap-mode: VECTGRADE_FULL succeeded. (136 steps) */
/* Generated by: /homee/stevenj/cvs/fftw3.0.1/genfft-k7/gen_twiddle -no-randomized-cse -dif -n 8 -name f1k7_8 */

/*
 * Generator Id's : 
 * $Id: algsimp.ml,v 1.3 2003/03/15 20:29:42 stevenj Exp $
 * $Id: fft.ml,v 1.3 2003/03/15 20:29:43 stevenj Exp $
 * $Id: gen_twiddle.ml,v 1.13 2003/04/18 01:21:45 athena Exp $
 */

/* The following asm code is Copyright (c) 2000-2001 Stefan Kral */
.section .rodata
	.balign 64
chs_lo: .long 0x80000000, 0x00000000
chs_hi: .long 0x00000000, 0x80000000
KP707106781KP707106781: .float +0.707106781186547524400844362104849039284835938, +0.707106781186547524400844362104849039284835938
.text
.text
	.balign 64
f1k7_8:
	subl $60, %esp
	femms 
	movl %ebx, 56(%esp)
	movl 76(%esp), %edx
	movl 84(%esp), %ebx
	movl 64(%esp), %ecx
	movl %esi, 52(%esp)
	movl 72(%esp), %eax
	movl %edi, 48(%esp)
	movl 80(%esp), %esi
	movl %ebp, 44(%esp)
	leal (,%edx,4), %edx
	leal (,%ebx,4), %ebx
	.p2align 4,,7
.L0:
	/* promise simd cell size = 8 */ 
	movq (%ecx), %mm1
	movq (%ecx,%edx,4), %mm0
	leal (%edx,%edx,2), %edi
	movq (%ecx,%edx,2), %mm4
	leal (%edx,%edx,4), %ebp
	movq (%ecx,%edi,2), %mm3
	movq (%ecx,%ebp), %mm7
	movq %mm1, %mm2
	pfadd %mm0, %mm1
	movq %mm4, %mm5
	pfsub %mm0, %mm2
	movq (%ecx,%edx), %mm0
	pfadd %mm3, %mm4
	movq %mm1, %mm6
	pfsub %mm3, %mm5
	movq %mm2, 8(%esp)
	movq (%ecx,%edi), %mm2
	leal (%ebp,%edx,2), %edi
	pfsub %mm4, %mm1
	movq %mm0, %mm3
	leal (%edx,%edx,2), %ebp
	pfadd %mm4, %mm6
	movq (%ecx,%edi), %mm4
	pswapd %mm5, %mm5
	pfsub %mm7, %mm0
	leal (%edx,%ebp,2), %edi
	pfadd %mm7, %mm3
	pxor chs_lo, %mm5
	movq %mm1, 0(%esp)
	movq %mm4, %mm7
	pfsub %mm2, %mm4
	pfadd %mm2, %mm7
	movq %mm3, %mm1
	movq %mm4, %mm2
	pfadd %mm7, %mm3
	pfpnacc %mm0, %mm2
	pswapd %mm0, %mm0
	pfsub %mm7, %mm1
	movq %mm6, %mm7
	pfpnacc %mm4, %mm0
	movq 8(%esp), %mm4
	pfadd %mm3, %mm6
	pfsub %mm3, %mm7
	pswapd %mm1, %mm1
	movq %mm4, %mm3
	pfsub %mm5, %mm4
	movq %mm6, 16(%esp)
	movq %mm0, %mm6
	pfnacc %mm2, %mm0
	pfadd %mm5, %mm3
	movq 0(%esp), %mm5
	pxor chs_hi, %mm1
	pfacc %mm6, %mm2
	pfmul KP707106781KP707106781, %mm0
	movq %mm5, %mm6
	pfadd %mm1, %mm5
	pfmul KP707106781KP707106781, %mm2
	pfsub %mm1, %mm6
	movq %mm3, %mm1
	movq %mm5, 24(%esp)
	movq 16(%eax), %mm5
	pfadd %mm0, %mm1
	pfsub %mm0, %mm3
	pswapd %mm1, %mm0
	pfmul %mm5, %mm1
	pfmul %mm5, %mm0
	movq 48(%eax), %mm5
	pfpnacc %mm1, %mm0
	pswapd %mm3, %mm1
	pfmul %mm5, %mm3
	pfmul %mm5, %mm1
	movq 24(%eax), %mm5
	pswapd %mm0, %mm0
	pfpnacc %mm3, %mm1
	pswapd %mm7, %mm3
	pfmul %mm5, %mm7
	pfmul %mm5, %mm3
	movq %mm4, %mm5
	pfsub %mm2, %mm4
	pfadd %mm2, %mm5
	movq 32(%eax), %mm2
	pswapd %mm1, %mm1
	pfpnacc %mm7, %mm3
	pswapd %mm4, %mm7
	pfmul %mm2, %mm4
	pfmul %mm2, %mm7
	movq (%eax), %mm2
	pswapd %mm3, %mm3
	pfpnacc %mm4, %mm7
	pswapd %mm5, %mm4
	pfmul %mm2, %mm5
	pfmul %mm2, %mm4
	pswapd %mm6, %mm2
	movq %mm7, 32(%esp)
	movq 40(%eax), %mm7
	pfpnacc %mm5, %mm4
	movq 16(%esp), %mm5
	pfmul %mm7, %mm2
	pfmul %mm7, %mm6
	movq 8(%eax), %mm7
	/* simd data load/store barrier */ 
	movq %mm5, (%ecx)
	movq 24(%esp), %mm5
	pswapd %mm4, %mm4
	movq %mm0, (%ecx,%ebp)
	addl $56, %eax
	movq %mm3, (%ecx,%edx,4)
	movq 32(%esp), %mm3
	pfpnacc %mm6, %mm2
	movq %mm1, (%ecx,%edi)
	pswapd %mm5, %mm0
	pfmul %mm7, %mm5
	movq %mm4, (%ecx,%edx)
	pfmul %mm7, %mm0
	pswapd %mm3, %mm3
	pswapd %mm2, %mm2
	movq %mm2, (%ecx,%ebp,2)
	leal (%edx,%edx,4), %ebp
	pfpnacc %mm5, %mm0
	movq %mm3, (%ecx,%ebp)
	pswapd %mm0, %mm0
	movq %mm0, (%ecx,%edx,2)
	addl %ebx, %ecx
	decl %esi
	jnz .L0
	femms 
	movl 56(%esp), %ebx
	movl 52(%esp), %esi
	movl 48(%esp), %edi
	movl 44(%esp), %ebp
	addl $60, %esp
	ret 

.section .rodata
nam:
	.string "f1k7_8"
	.align 4
twinstr:
	.byte 4
	.byte 0
	.value 8
	.byte 3
	.byte 1
	.value 0
	.align 4
desc:
	.long 8
	.long nam
	.long twinstr
	.zero 4
	.double 33
	.double 16
	.double 0
	.double 0
	.long fftwf_kdft_ct_k7_mgenus
	.long 0
	.long 0
	.long 0

.text
	.align 4
.globl fftwf_codelet_f1k7_8
fftwf_codelet_f1k7_8:
	subl $12,%esp
	movl 16(%esp),%eax
	addl $-4,%esp
	pushl $desc
	pushl $f1k7_8
	pushl %eax
	call fftwf_kdft_dif_register
	addl $16,%esp
	addl $12,%esp
	ret

