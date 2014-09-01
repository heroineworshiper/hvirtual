/* 
 *  encode_x86.S
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

.data
ALLONE:		.word 1,1,1,1
VLCADDMASK:	.byte 255,0,0,0,255,0,0,0
		
.section .note.GNU-stack, "", @progbits

.text

.global _dv_vlc_encode_block_mmx_x86_64
_dv_vlc_encode_block_mmx_x86_64:
	
/* extern unsigned long _dv_vlc_encode_block_mmx(dv_coeff_t* coeffs,
			                	 dv_vlc_entry_t ** out); */

	/* arguments are coeffs=rdi, out=rsi */
	
	push	%rbx
	push	%rbp

	xor	%rax,%rax
	mov	%rsi,%rdx 
	mov	(%rdx),%rdx                      # o = out->coeffs
	add	$2, %rdi                         # skip the first entry, DC coeff

	mov	$63, %rcx                        # loop counter

	mov	vlc_encode_lookup@GOTPCREL(%rip), %r11
	mov	(%r11),%r11

	pxor	%mm0, %mm0
	pxor	%mm2, %mm2
	movq	VLCADDMASK(%rip), %mm1
	xor	%rbp, %rbp
	sub	$8, %rdx
vlc_encode_block_mmx_loop:
	pand	%mm1, %mm0
	movw	(%rdi), %ax                     /* amp = *z */
	add	$8, %rdx                        /* point to next vlc entry */
	paddd	%mm0, %mm2
	cmpw	$0, %ax
	jz	vlc_encode_block_amp_zero
	
	addw	$255, %ax
	add	$2, %rdi                        /* z++ */
	movq	(%r11, %rax, 8), %mm0           /* get the vlc code from the table */
	movq	%mm0, (%rdx)
	dec	%rcx
	jnz	vlc_encode_block_mmx_loop
	
	pand	%mm1, %mm0
	paddd	%mm0, %mm2
	jmp     vlc_encode_block_out
	
vlc_encode_block_amp_zero:
        mov     %rcx, %rbp
        inc     %rcx
        repz    scasw                 /* scans for eax (zero) up to rcx times, increments rdi */
        jecxz   vlc_encode_block_out

	/* inline vlc_encode(int run, int amp, int sign, dv_vlc_entry_t * o) */
	/* vlc_encode_lookup + 2 * ((amp + 255) | (run << 9)); */
        movw    -2(%rdi), %ax
        sub     %rcx, %rbp                /* the run length of zeros */
	addw	$255, %ax
        shl     $9, %rbp
	or	%rbp, %rax
	
	movq	(%r11, %rax, 8), %mm0     /* vlc_encode_lookup + 2 * (...) */
	movq	%mm0, (%rdx)
	
	dec	%rcx
	jnz	vlc_encode_block_mmx_loop

	pand	%mm1, %mm0
	paddd	%mm0, %mm2
	
vlc_encode_block_out:
	movq	%mm2, %mm0
	psrlq	$32, %mm0
	paddd	%mm0, %mm2

	mov	%rsi, %rbx             /* update the output pointer */
	mov	%rdx, (%rbx)
	
	movd	%mm2, %rax             # return value, number of bits
	
	pop	%rbp
	pop	%rbx
	
	ret	

.global _dv_vlc_num_bits_block_x86_64
_dv_vlc_num_bits_block_x86_64:
	
	/* extern unsigned long _dv_vlc_num_bits_block_x86_64(dv_coeff_t* coeffs); */

	/* arguments are coeffs=rdi */
	
	push	%rbx
	push	%rbp

	xor	%rax, %rax
	xor	%rdx, %rdx
	xor	%rbx, %rbx
	xor	%rbp, %rbp
	
	add	$2, %rdi           /* skip the DC coefficient */
	mov	$63, %rcx

	mov	vlc_num_bits_lookup@GOTPCREL(%rip), %r11
	mov	(%r11),%r11
	
vlc_num_bits_block_x86_loop:
	movw	(%rdi), %ax        /* get the next coeffiecient */
	add	%rbx, %rdx         /* accumulate the number of VLC bits */
	cmpw	$0, %ax
	jz	vlc_num_bits_block_amp_zero
	addw	$255, %ax
	add	$2, %rdi
	movb	(%r11, %rax), %bl  /* lookup the number of VLC bits in the table */
	
	dec	%rcx
	jnz	vlc_num_bits_block_x86_loop
	
	add	%rbx, %rdx         /* accumulate the number of VLC bits */
	jmp	vlc_num_bits_block_out
	
vlc_num_bits_block_amp_zero:
        mov     %rcx, %rbp
        inc     %rcx
        repz    scasw              /* scans for eax (zero) up to rcx times, increments rdi */
        jecxz   vlc_num_bits_block_out
	
        sub     %rcx, %rbp         /* The run length of zero coefficients */
        movw    -2(%rdi), %ax      /* The first nonzero coefficient */
        shl     $9, %rbp
	addw	$255, %ax
	or 	%rbp, %rax
	movb	(%r11, %rax), %bl  /* lookup the number of VLC bits in the table */
	
	dec	%rcx
	jnz	vlc_num_bits_block_x86_loop
	add	%rbx, %rdx
	
vlc_num_bits_block_out:
	mov	%rdx, %rax              # return value
		
	pop	%rbp
	pop	%rbx
	ret	

.global _dv_vlc_encode_block_pass_1_x86_64
_dv_vlc_encode_block_pass_1_x86_64:

/*
extern void _dv_vlc_encode_block_pass_1_x86_64(dv_vlc_entry_t** start,        rdi
					dv_vlc_entry_t*         end,          rsi
					long*                   bit_budget,   rdx
					long*                   bit_offset,   rcx
					unsigned char*          vsbuffer);    r8
*/
	
	push	%rbx
	push	%r12
	push	%rbp
	push	%r15
	push	%rsi
	
	mov	%rsi, %r12		   # end
	mov	(%rdi), %rsi               # *start
	mov	(%rdx), %r15               # local bit_budget = *bit_budget
	mov	(%rcx), %ebp		   # *bit_offset
	mov	%rcx, %r11
	xor	%rcx,%rcx
	xor	%r10, %r10
	xor	%rax,%rax
	
vlc_encode_block_pass_1_x86_loop:	
	lodsl	(%rsi),%eax
	mov	%al, %cl                 # len
	
	sub	%rcx, %r15               # local bit_budget -= len
	jl	vlc_encode_block_pass1_x86_out

	mov	%rbp, %rbx               # bit_offset
	neg	%ecx                     # (-len)
	
	and	$7, %rbx                 # bit_offset & 7
	add	$32, %ecx                # 32-len
	
	movb	%al, %r10b               # len
	sub	%rbx, %rcx               # 32-len-(bit_offset & 7)
	
	shr	$8, %eax                 # value
	mov	%rbp, %rbx               # bit_offset
	
	shl	%cl, %eax                # value <<= 32-len-(bit_offset & 7)
	shr	$3, %rbx                 # bit_offset >> 3
	
	bswap	%eax
	add	%r8, %rbx                # vsbuffer + bit_offset >> 3
	
	add	%r10, %rbp               # bit_offset += len
	or	%eax, (%rbx)             # store value
		
	cmp	%rsi, %r12
	jnz	vlc_encode_block_pass_1_x86_loop
	
	xor	%ecx, %ecx
	add	$4, %rsi
	
vlc_encode_block_pass1_x86_out:
	sub	$4, %rsi
	add	%r15, %rcx             # + local bit_budget

	mov	%rsi, (%rdi)           # *start =
	
	mov	%ecx, (%rdx)          # *bit_budget =

	mov	%r11, %rcx
	mov	%ebp, (%rcx)           # *bit_offset =

	pop	%rsi
	pop	%r15
	pop	%rbp	
	pop	%r12
	pop	%rbx	
	ret		
		
.global _dv_classify_mmx_x86_64
_dv_classify_mmx_x86_64:

	/* extern int _dv_classify_mmx_x86_64(dv_coeff_t *    a,          rdi
	                                      unsigned short* amp_ofs,    rsi
		                              unsigned short* amp_cmp);   rdx
	*/
	
	mov	%rsi, %r11
	movq	(%r11), %mm7            # amp_ofs
	mov	%rdx, %r11
	movq	(%r11), %mm6            # amp_cmp

	mov     %rdi, %r11              # a

	movq	%mm7, %mm5
	movq	%mm6, %mm4
	
	pxor	%mm3, %mm3
	pxor	%mm2, %mm2

	movq	0*8(%r11), %mm0
	movq	1*8(%r11), %mm1
	
	paddw	%mm7, %mm0
	paddw	%mm5, %mm1
	pcmpgtw	%mm6, %mm0
	pcmpgtw %mm4, %mm1
	paddw	%mm0, %mm3 
	paddw	%mm1, %mm2 

	movq	2*8(%r11), %mm0
	movq	3*8(%r11), %mm1
	paddw	%mm7, %mm0
	paddw	%mm5, %mm1
	pcmpgtw	%mm6, %mm0
	pcmpgtw %mm4, %mm1
	paddw	%mm0, %mm3 
	paddw	%mm1, %mm2 

	movq	4*8(%r11), %mm0
	movq	5*8(%r11), %mm1
	paddw	%mm7, %mm0
	paddw	%mm5, %mm1
	pcmpgtw	%mm6, %mm0
	pcmpgtw %mm4, %mm1
	paddw	%mm0, %mm3 
	paddw	%mm1, %mm2 

	movq	6*8(%r11), %mm0
	movq	7*8(%r11), %mm1
	paddw	%mm7, %mm0
	paddw	%mm5, %mm1
	pcmpgtw	%mm6, %mm0
	pcmpgtw %mm4, %mm1
	paddw	%mm0, %mm3 
	paddw	%mm1, %mm2 

	movq	8*8(%r11), %mm0
	movq	9*8(%r11), %mm1
	paddw	%mm7, %mm0
	paddw	%mm5, %mm1
	pcmpgtw	%mm6, %mm0
	pcmpgtw %mm4, %mm1
	paddw	%mm0, %mm3 
	paddw	%mm1, %mm2 

	movq	10*8(%r11), %mm0
	movq	11*8(%r11), %mm1
	paddw	%mm7, %mm0
	paddw	%mm5, %mm1
	pcmpgtw	%mm6, %mm0
	pcmpgtw %mm4, %mm1
	paddw	%mm0, %mm3 
	paddw	%mm1, %mm2 

	movq	12*8(%r11), %mm0
	movq	13*8(%r11), %mm1
	paddw	%mm7, %mm0
	paddw	%mm5, %mm1
	pcmpgtw	%mm6, %mm0
	pcmpgtw %mm4, %mm1
	paddw	%mm0, %mm3 
	paddw	%mm1, %mm2 

	movq	14*8(%r11), %mm0
	movq	15*8(%r11), %mm1
	paddw	%mm7, %mm0
	paddw	%mm5, %mm1
	pcmpgtw	%mm6, %mm0
	pcmpgtw %mm4, %mm1
	paddw	%mm0, %mm3 
	paddw	%mm1, %mm2 
							
	paddw	%mm2, %mm3
	packsswb %mm3, %mm3

	movd	%mm3, %rax         # retun value

	ret

/* FIXME: _dv_reorder_block_mmx isn't really _that_ faster than the C version... 
	 don't know why... */
	
.global _dv_reorder_block_mmx_x86_64
_dv_reorder_block_mmx_x86_64:
	
/*extern int _dv_reorder_block_mmx_x86_64(dv_coeff_t *          a,               rdi
		                          const unsigned short* reorder_table);  rsi
*/
				
	push	%rbp
	push	%r12
	push	%rbx
	push	%rcx

	mov	%rdi, %r11      # a
	mov	%rsi, %r12      # reorder_table

	xor	%rbp, %rbp
	xor	%rax, %rax
	xor	%rbx, %rbx
	xor	%rcx, %rcx
	xor	%rdx, %rdx

	sub	$128, %rsp      # allocate some stack space, the redzone
	
reorder_loop:	
	
	movw	 (%r11, %rbp), %ax
	movw	2(%r11, %rbp), %bx

	movw	 (%r12, %rbp), %cx
	movw	2(%r12, %rbp), %dx

	movw	 %ax, (%rsp,%rcx)
	movw	 %bx, (%rsp,%rdx)

	movw	4(%r11, %rbp), %ax
	movw	6(%r11, %rbp), %bx

	movw	4(%r12, %rbp), %cx
	movw	6(%r12, %rbp), %dx

	movw	 %ax, (%rsp,%rcx)
	movw	 %bx, (%rsp,%rdx)

	movw	 8(%r11, %rbp), %ax
	movw	10(%r11, %rbp), %bx

	movw	8(%r12, %rbp), %cx
	movw	10(%r12, %rbp), %dx

	movw	 %ax, (%rsp,%rcx)
	movw	 %bx, (%rsp,%rdx)

	movw	12(%r11, %rbp), %ax
	movw	14(%r11, %rbp), %bx

	movw	12(%r12, %rbp), %cx
	movw	14(%r12, %rbp), %dx

	movw	 %ax, (%rsp,%rcx)
	movw	 %bx, (%rsp,%rdx)

	add	$16, %rbp
	
	cmp	$128, %rbp
	jne	reorder_loop
	
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

	add	$128, %rsp             # restore the stack
	
	movq	%mm0, 96(%r11)
	movq	%mm1, 96+8(%r11)
	movq	%mm2, 96+16(%r11)
	movq	%mm3, 96+24(%r11)
	
	pop	%rcx
	pop	%rbx
	pop	%r12
	pop	%rbp
	ret

.global _dv_need_dct_248_mmx_x86_64_rows
_dv_need_dct_248_mmx_x86_64_rows:
	
	/* extern int _dv_need_dct_248_mmx_x86_64_rows(dv_coeff_t * bl);  rdi */

	mov	%rdi, %r11      # bl

	movq	(0*8+0)*2(%r11), %mm0
	movq	(0*8+4)*2(%r11), %mm1
	psubw	(1*8+0)*2(%r11), %mm0
	psubw	(1*8+4)*2(%r11), %mm1
	movq	%mm0, %mm2
	movq	%mm1, %mm3
	psraw	$15, %mm2
	psraw	$15, %mm3
	pxor	%mm2, %mm0
	pxor	%mm3, %mm1
	psubw	%mm2, %mm0
	psubw	%mm3, %mm1
	
	movq	(1*8+0)*2(%r11), %mm4
	movq	(1*8+4)*2(%r11), %mm5
	psubw	(2*8+0)*2(%r11), %mm4
	psubw	(2*8+4)*2(%r11), %mm5
	movq	%mm4, %mm6
	movq	%mm5, %mm7
	psraw	$15, %mm6
	psraw	$15, %mm7
	pxor	%mm6, %mm4
	pxor	%mm7, %mm5
	psubw	%mm6, %mm4
	psubw	%mm7, %mm5

	paddw	%mm4, %mm0
	paddw	%mm5, %mm1

	movq	(2*8+0)*2(%r11), %mm4
	movq	(2*8+4)*2(%r11), %mm5
	psubw	(3*8+0)*2(%r11), %mm4
	psubw	(3*8+4)*2(%r11), %mm5
	movq	%mm4, %mm6
	movq	%mm5, %mm7
	psraw	$15, %mm6
	psraw	$15, %mm7
	pxor	%mm6, %mm4
	pxor	%mm7, %mm5
	psubw	%mm6, %mm4
	psubw	%mm7, %mm5

	paddw	%mm4, %mm0
	paddw	%mm5, %mm1

	movq	(3*8+0)*2(%r11), %mm4
	movq	(3*8+4)*2(%r11), %mm5
	psubw	(4*8+0)*2(%r11), %mm4
	psubw	(4*8+4)*2(%r11), %mm5
	movq	%mm4, %mm6
	movq	%mm5, %mm7
	psraw	$15, %mm6
	psraw	$15, %mm7
	pxor	%mm6, %mm4
	pxor	%mm7, %mm5
	psubw	%mm6, %mm4
	psubw	%mm7, %mm5

	paddw	%mm4, %mm0
	paddw	%mm5, %mm1

	movq	(4*8+0)*2(%r11), %mm4
	movq	(4*8+4)*2(%r11), %mm5
	psubw	(5*8+0)*2(%r11), %mm4
	psubw	(5*8+4)*2(%r11), %mm5
	movq	%mm4, %mm6
	movq	%mm5, %mm7
	psraw	$15, %mm6
	psraw	$15, %mm7
	pxor	%mm6, %mm4
	pxor	%mm7, %mm5
	psubw	%mm6, %mm4
	psubw	%mm7, %mm5

	paddw	%mm4, %mm0
	paddw	%mm5, %mm1

	movq	(5*8+0)*2(%r11), %mm4
	movq	(5*8+4)*2(%r11), %mm5
	psubw	(6*8+0)*2(%r11), %mm4
	psubw	(6*8+4)*2(%r11), %mm5
	movq	%mm4, %mm6
	movq	%mm5, %mm7
	psraw	$15, %mm6
	psraw	$15, %mm7
	pxor	%mm6, %mm4
	pxor	%mm7, %mm5
	psubw	%mm6, %mm4
	psubw	%mm7, %mm5

	paddw	%mm4, %mm0
	paddw	%mm5, %mm1

	movq	(6*8+0)*2(%r11), %mm4
	movq	(6*8+4)*2(%r11), %mm5
	psubw	(7*8+0)*2(%r11), %mm4
	psubw	(7*8+4)*2(%r11), %mm5
	movq	%mm4, %mm6
	movq	%mm5, %mm7
	psraw	$15, %mm6
	psraw	$15, %mm7
	pxor	%mm6, %mm4
	pxor	%mm7, %mm5
	psubw	%mm6, %mm4
	psubw	%mm7, %mm5

	paddw	%mm4, %mm0
	paddw	%mm5, %mm1

	paddw	%mm1, %mm0
	
	pmaddwd	ALLONE(%rip), %mm0	
	movq	%mm0, %mm1
	psrlq	$32, %mm1
	paddd	%mm1, %mm0
	
	movd	%mm0, %rax

	ret




