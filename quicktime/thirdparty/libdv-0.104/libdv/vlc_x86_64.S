	#include "asmoff.h"
.section .note.GNU-stack, "", @progbits

.text
	.align 4
.globl dv_decode_vlc
	.type	 dv_decode_vlc,@function
dv_decode_vlc:
	push %rbx
	push %rbp

	/* Args are at bits=rdi, maxbit=rsi, result=rdx */
	mov  %rdi,%rax		/* %rax is bits */
	mov  %rsi,%rbx		/* %rbx is maxbits */
	and  $0x3f,%rbx		/* limit index range STL*/
	
	/* note that BITS is left aligned */
	/* klass = dv_vlc_classes[maxbits][(bits & (dv_vlc_class_index_mask[maxbits])) >> */
	/*         (dv_vlc_class_index_rshift[maxbits])];  */
/*	xor  %rbp,%rbp */
	mov  dv_vlc_class_index_mask@GOTPCREL(%rip),%r11    /* use %rip for PIC code */
 	mov  (%r11,%rbx,4),%ebp           /* int32 */      /* dv_vlc_class_index_mask[maxbits] */
	and  %eax,%ebp                                     /* bits & */
	mov  dv_vlc_class_index_rshift@GOTPCREL(%rip),%rcx
	mov  (%rcx,%rbx,4),%ecx           /* int32 */      /* dv_vlc_class_index_rshift[maxbits] */
	sar  %cl,%ebp                                      /* >> */
	mov  dv_vlc_classes@GOTPCREL(%rip),%rcx
	mov  (%rcx,%rbx,8),%rcx           /* ptr */        /* dv_vlc_classes[maxbits], a pointer */
	movsbq  (%rcx,%rbp,1),%rbp        /* int8 */       /* klass = */

	/* *result = dv_vlc_lookups[klass][(bits & (dv_vlc_index_mask[klass])) >> */
	/*           (dv_vlc_index_rshift[klass])];   */
/*	xor  %rbx,%rbx */
	mov  dv_vlc_index_mask@GOTPCREL(%rip),%r11
	mov  (%r11,%rbp,4),%ebx          /* int32 */       /* (dv_vlc_index_mask[klass]) */
/*	xor  %rcx,%rcx */
	mov  dv_vlc_index_rshift@GOTPCREL(%rip),%r11
	mov  (%r11,%rbp,4),%ecx          /* int32 */       /* dv_vlc_index_rshift[klass] */
	and  %eax,%ebx                                     /* bits &  */
	sar  %cl,%ebx                                      /* >> */

	mov  dv_vlc_lookups@GOTPCREL(%rip),%r11
	mov  (%r11,%rbp,8),%rbp          /* ptr */         /* dv_vlc_lookups[klass] */
	mov  (%rbp,%rbx,4),%ebp          /* int32 */       /* *result = */

	/* Now %ebp holds result, a dv_vlc_t, like this:
	   bits 0-7   run
	   bits 8-15  len
	   bits 16-31 amp
	*/
	/* code needs to do this with result:
	   if ((result->lamp > 0) &&
	     if (bits & sign_mask[result->len])
	         result->lamp = -result->lamp;
	   }
	*/

	/* Form a mask from (bits & sign_mask[result->len]) */
	mov  %ebp,%ecx
	sar  $8,%ecx
	and  $0xff,%ecx                /* result->len */
	mov  sign_mask@GOTPCREL(%rip),%rbx
	mov  (%rbx,%rcx,4),%ebx        /* int32 */
	and  %ebx,%eax
	neg  %eax
	sar  $31,%eax

	mov  %ebp,%ebx
	sar  $31,%ebx                  /* result->amp */
	xor  $0xffffffff,%ebx
	and  $0xffff0000,%ebx

	and  %rbx,%rax

	/* Now %eax is 0xffff0000 if we want to negate %ebp, zero otherwise */
	xor  %eax,%ebp
	sub  %eax,%ebp

	/*
	if (maxbits < result->len)
	    *result = broken;
	Note that the 'broken' pattern is all ones (i.e. 0xffffffff)
	*/
	mov  %esi,%ebx		/* maxbits */ /* int32 */
	sub  %ecx,%ebx
	sbb  %ebx,%ebx
	or   %ebx,%ebp

	mov  %ebp,(%rdx)        /* *result = */

	pop  %rbp
	pop  %rbx

	ret

/*
void __dv_decode_vlc(int bits, dv_vlc_t *result)
*/
	
.text
	.align 4
.globl __dv_decode_vlc
	.type	 __dv_decode_vlc,@function
__dv_decode_vlc:
	push %rbx
	push %rbp

	/* Args are bits=rdi, result=rsi  */
	mov  %rdi,%rax			/* %rax is bits */
	
	mov  %rax,%rbp
	and  $0xfe00,%ebp
	sar  $9,%ebp
	mov  dv_vlc_class_lookup5@GOTPCREL(%rip),%r11
	movsbq  (%r11,%rbp),%rbp        /* int8 klass */

	mov  dv_vlc_index_mask@GOTPCREL(%rip),%rbx
	mov  (%rbx,%rbp,4),%ebx         /* int32 */
	mov  dv_vlc_index_rshift@GOTPCREL(%rip),%rcx
	mov  (%rcx,%rbp,4),%ecx         /* int32 */
	and  %eax,%ebx
	sar  %cl,%ebx			/* %rbx is klass */

	mov  dv_vlc_lookups@GOTPCREL(%rip),%r11
	mov  (%r11,%rbp,8),%rbp         /* ptr */
	mov  (%rbp,%rbx,4),%ebp         /* int32 */

	/* Now %ebp holds result, like this:
	   bits 0-7   run
	   bits 8-15  len
	   bits 16-31 amp
	*/
	/* code needs to do this with result:
	   if ((result->amp > 0) &&
	     if ((bits >> sign_rshift[result->len]) & 1)
	         result->amp = result->-amp;
	   }
	*/
	/* if (result->amp < 0) %rbp is 0, else 0xffff0000. */
	mov  %ebp,%ecx
	sar  $8,%ecx
	and  $0xff,%ecx
	mov  sign_mask@GOTPCREL(%rip),%r11
	mov  (%r11,%rcx,4),%ecx        /* int32 */
	and  %ecx,%eax
	neg  %eax
	sar  $31,%eax

	mov  %ebp,%ebx
	sar  $31,%ebx
	xor  $0xffffffff,%ebx
	and  $0xffff0000,%ebx

	and  %ebx,%eax
	
	xor  %eax,%ebp
	sub  %eax,%ebp

	mov  %ebp,(%rsi)       /* *result = */

	pop  %rbp
	pop  %rbx
	
	ret

/*	
void dv_parse_ac_coeffs_pass0(bitstream_t *bs,
			      dv_macroblock_t *mb,
			      dv_block_t *bl)
*/
.text
	.align	4
.globl	dv_parse_ac_coeffs_pass0
	
dv_parse_ac_coeffs_pass0:
	
	/* Args are at rdi=bs, rsi=mb, rdx=bl */
	push	%r12
	push	%r13
	push	%r14
	push	%r15

	/*
	eax	scratch
	ecx     scratch
	r11     scratch
	r14	bs->buf
	r13	bl->offset
	r12	bl->reorder
	r15	bl
	*/
	mov	%rdx,%r15                     /* bl */
	mov	%rdi,%r14                     /* bs */
	mov	bitstream_t_buf(%r14),%r14    /* bs->buf */
/*	xor	%r13,%r13 */
	mov	dv_block_t_offset(%r15),%r13d  /* bl->offset */
/*	xor	%r12,%r12 */
	mov	dv_block_t_reorder(%r15),%r12  /* bl->reorder */

	/* I think it would be better to zero out the coeffs as we're
	copying them into the framebuffer.  But that optimization is
	for another day. */
	
	movq    dv_block_t_coeffs(%r15),%mm1
	pxor    %mm0,%mm0
	pand    const_f_0_0_0(%rip),%mm1
	movq    %mm1,dv_block_t_coeffs(%r15)  /* bl->coeffs[0] */

	/* memset(&bl->coeffs[1],'\0',sizeof(bl->coeffs)-sizeof(bl->coeffs[0])); */
	movq    %mm0,(dv_block_t_coeffs + 8)(%r15)
	movq    %mm0,(dv_block_t_coeffs + 16)(%r15)
	movq    %mm0,(dv_block_t_coeffs + 24)(%r15)
	movq    %mm0,(dv_block_t_coeffs + 32)(%r15)
	movq    %mm0,(dv_block_t_coeffs + 40)(%r15)
	movq    %mm0,(dv_block_t_coeffs + 48)(%r15)
	movq    %mm0,(dv_block_t_coeffs + 56)(%r15)
	movq    %mm0,(dv_block_t_coeffs + 64)(%r15)
	movq    %mm0,(dv_block_t_coeffs + 72)(%r15)
	movq    %mm0,(dv_block_t_coeffs + 80)(%r15)
	movq    %mm0,(dv_block_t_coeffs + 88)(%r15)
	movq    %mm0,(dv_block_t_coeffs + 96)(%r15)
	movq    %mm0,(dv_block_t_coeffs + 104)(%r15)
	movq    %mm0,(dv_block_t_coeffs + 112)(%r15)
	movq    %mm0,(dv_block_t_coeffs + 120)(%r15)
	
readloop:
	/* bits = bitstream_show(bs,16); */
	mov	%r13,%rcx           /* bl->offset */
	shr	$3,%rcx             /* divide by 8 bits/byte */
	movzbq	(%r14,%rcx,1),%rax    /* bs->(buf+offset) */
	movzbq	1(%r14,%rcx,1),%r11   /* bs->(buf+offset+1) */
	movzbq	2(%r14,%rcx,1),%rcx   /* bs->(buf+offset+2) */
	shl	$16,%rax
	shl	$8,%r11
	or	%rcx,%rax
	or	%r11,%rax           /* rax contains the 3 bitstream bytes */
	mov	%r13,%r11           /* bl->offset */
	and	$7,%r11             /* num_bits = 3 lsb's of bl->offset */
	mov	$8,%rcx
	sub	%r11,%rcx           /* 8 - num_bits */
	shr	%cl,%rax            /* bits = >> to remove bits already processed */

	/* bits_left = bl->end - bl->offset; */
	mov	dv_block_t_end(%r15),%r11d
	sub	%r13d,%r11d	/* r11 is bits_left */

	/* if(bits_left < 16) */
	cmp	$16,%r11d
	jl	slowpath

	/* ecx is most significant 7 bits */
	mov	%rax,%rcx
	and	$0xfe00,%rcx
	sar	$9,%rcx

	/* Attempt to use the shortcut first.  If it hits, then
	   this vlc term has been decoded. */
	mov	dv_vlc_class1_shortcut@GOTPCREL(%rip),%r10
	mov	(%r10,%rcx,4),%r11d    /* record32 dv_vlc_tab_t */
	test	$0x80,%r11d
	
	je	done_decode

	/* */
	
	/* fast path:	 use inlined version of __dv_decode_vlc */
	/* ---------------------- */
	mov	%r12,dv_block_t_reorder(%r15)

	/* %rax is bits */
	
	mov  dv_vlc_class_lookup5@GOTPCREL(%rip),%r10
	movsbq  (%r10,%rcx,1),%rcx     /* int8 */

/*	xor  %r12,%r12 */
	mov  dv_vlc_index_mask@GOTPCREL(%rip),%r10
	mov  (%r10,%rcx,4),%r12d       /* int32 */
	
	mov  dv_vlc_lookups@GOTPCREL(%rip),%r10
	mov  (%r10,%rcx,8),%r11       /* ptr->record32 */
	
	mov  dv_vlc_index_rshift@GOTPCREL(%rip),%r10
	mov  (%r10,%rcx,4),%ecx        /* int32 */
	
	and  %eax,%r12d
	sar  %cl,%r12d

	mov  (%r11,%r12,4),%r11d       /* int32 */

	/* Now %r11 holds result, like this:
	   bits 0-7   run
	   bits 8-15  len
	   bits 16-31 amp
	*/
	test	$0x80,%r11d	/* If (vlc.run < 0) break */
	jne	escape1
	/* code needs to do this with result:
	   if ((amp > 0) &&
	     if ((bits >> sign_rshift[result->len]) & 1)
	         amp = -amp;
	   }
	*/
	/* if (amp < 0) %r11 is 0, else 0xffff0000. */
/*	xor  %rcx,%rcx */
	mov  %r11d,%ecx
	sar  $8,%ecx
	and  $0xff,%ecx
	mov  sign_mask@GOTPCREL(%rip),%r10
	mov  (%r10,%rcx,4),%ecx      /* int32 */
	and  %ecx,%eax
	neg  %eax
	sar  $31,%eax

	mov  %r11d,%r12d
	sar  $31,%r12d
	xor  $0xffffffff,%r12d
	and  $0xffff0000,%r12d
	and  %r12d,%eax

	xor  %eax,%r11d
	sub  %eax,%r11d

	mov  dv_block_t_reorder(%r15),%r12    /* ptr */
	/* ---------------------- */
	
done_decode:
	/* bl->offset += vlc.len */
	mov	%r11d,%eax
	shr	$8,%eax
	and	$255,%eax
	add	%eax,%r13d

	/* bl->reorder += vlc.run */
/*	xor	%rax,%rax */
	mov	%r11d,%eax   /* int32 */
	and	$255,%eax
	add	%rax,%r12    /* ptr */
	
	/* SET_COEFF(bl->coeffs, bl->reorder, vlc.amp); */
	movzbq	(%r12),%rax
	inc	%r12

	shr	$16,%r11d
	movw	%r11w,(dv_block_t_coeffs)(%r15,%rax,1)   /* int16 */
	
	jmp	readloop

escape1:
	mov	dv_block_t_reorder(%r15),%r12
escape:
	/* if (vlc.amp == 0) */
	test	$0xffff0000,%r11d
	jne	ampnonzero
	/* bl->reorder = bl->reorder_sentinel; */
	mov	dv_block_t_reorder_sentinel(%r15),%r12  /* ptr */
	/* bl->offset += 4; */
	add	$4,%r13d
	/* bl->eob = 1; */
	movl	$1,dv_block_t_eob(%r15)           /* int32 */
	/* mb->eob_count++; */
	mov	%rsi,%r11
	incl	dv_macroblock_t_eob_count(%r11)   /* int32 */
	
	jmp	alldone
	/* else if(vlc.len == VLC_ERROR) */
ampnonzero:
	and	$0x0000ff00,%r11d
	cmp	$0x0000fe00,%r11d	/* VLC_ERROR */
	jne	alldone
	/* mb->vlc_error = TRUE; */
	mov	%rsi,%r11
	movl	$1,dv_macroblock_t_vlc_error(%r11); /* int32 */
alldone:
	mov	%r12,dv_block_t_reorder(%r15)   /* ptr */
	mov	%r13d,dv_block_t_offset(%r15)   /* int32 */
	
	pop	%r15
	pop	%r14
	pop	%r13
	pop	%r12

	ret

slowpath:
	/* slow path:	 use dv_decode_vlc */;
	/* Args are at rdi=bits, rsi=bits_left, rdx=*vlc */
	push	%rdi
	push	%rsi
	push	%rdx
	mov	%r11,%rsi        /* bits */
	mov	%rax,%rdi        /* bits_left */
	lea	vlc(%rip),%rdx   /* *vlc */
	mov	dv_decode_vlc@GOTPCREL(%rip),%r11
	call	*%r11
	pop	%rdx
	pop	%rsi
	pop	%rdi

	mov	vlc(%rip),%r11
	test	$0x80,%r11	/* If (vlc.run < 0) break */
	jne	escape
	
	jmp	done_decode
	
show16:                         /* not used */
	mov	%rbx,%rcx
	mov	%rbx,%r11
	shr	$3,%rcx
	and	$7,%r11
	mov	(%r14,%rcx,1),%rax
	mov	1(%r14,%rcx,1),%rbx
	mov	2(%r14,%rcx,1),%rcx
	shl	$16,%rax
	shl	$8,%rbx
	or	%rcx,%rax
	or	%rbx,%rax
	mov	$8,%rcx
	sub	%r11,%rcx
	shr	%cl,%rax
	ret


/*
gint dv_parse_video_segment(dv_videosegment_t *seg, guint quality) {
*/
	.globl dv_parse_video_segment
dv_parse_video_segment:
	
	/* Args are at rdi=seg, rsi=quality */
	push	%r12
	push	%r13
	push	%r14
	push	%r15

	mov	%rsi,%rax			/* quality */
	mov	$4,%r12
	test	$DV_QUALITY_COLOR,%rax
	jz	its_mono
	mov	$6,%r12
its_mono:
	mov	%r12d,n_blocks(%rip)            /* int32 */
	
	/*
	 *	r12	seg,m
	 *
	 *	
	 *      r14	bs->buf
	 *	r13	mb
	 *	r15	bl
	 */
	mov	%rdi,%r12                         /* seg */
	mov	dv_videosegment_t_bs(%r12),%r14   /* seg->bs */
	mov	bitstream_t_buf(%r14),%r14        /* seg->bs->t_buf */
	lea	dv_videosegment_t_mb(%r12),%r13   /* seg->mb */

	xor	%rax,%rax
	xor	%rcx,%rcx
macloop:
	mov	%eax,m(%rip)                      /* int32 */
	mov	%ecx,mb_start(%rip)               /* int32 */

	mov	%rdi,%r12                         /* seg */
	
	/* bitstream_seek_set(bs,mb_start+28); */
	/* mb->qno = bitstream_get(bs,4); */
	mov	%rcx,%r11
	shr	$3,%r11
	movzbq	3(%r14,%r11,1),%r11
	and	$0xf,%r11
	movl	%r11d,dv_macroblock_t_qno(%r13)    /* int32 */

	/* mb->vlc_error = 0;
           mb->eob_count = 0; */
	xor	%r11,%r11
	movl	%r11d,dv_macroblock_t_vlc_error(%r13) /* int32 */
	movl	%r11d,dv_macroblock_t_eob_count(%r13) /* int32 */

	/* mb->i = (seg->i + dv_super_map_vertical[m]) % (seg->isPAL?12:10); */
	mov	dv_super_map_vertical@GOTPCREL(%rip),%r11
	movl	(%r11,%rax,4),%r11d                     /* int32 */
/*	xor	%rcx,%rcx */
	movl	dv_videosegment_t_i(%r12),%ecx          /* int32 */
	add	%rcx,%r11

skarly:
/*	xor	%rcx,%rcx */
	movl	dv_videosegment_t_isPAL(%r12),%ecx     /* int32 */
	add	$-1,%rcx
	sbb	%rcx,%rcx
	and	$1,%rcx
	shl	$5,%rcx		/* rcx = (isPAL ? 32 : 0) */

	add	%r11,%rcx       /* rcx = offset from mod10 */
	lea	mod_10(%rip),%r11
	movzbq	(%r11,%rcx,1),%r11	/* uses mod_12 for PAL */ /* int8 */
	movl	%r11d,dv_macroblock_t_i(%r13)         /* int32 */

	/*  mb->j = dv_super_map_horizontal[m]; */		
	mov	dv_super_map_horizontal@GOTPCREL(%rip),%r11
	movl	(%r11,%rax,4),%r11d                   /* int32 */
	movl	%r11d,dv_macroblock_t_j(%r13)         /* int32 */

	/* mb->k = seg->k; */
	movl	dv_videosegment_t_k(%r12),%r11d       /* int32 */
	movl	%r11d,dv_macroblock_t_k(%r13)         /* int32 */

	xor	%r12,%r12                        /* b=0 */
	lea	dv_macroblock_t_b(%r13),%r15     /* mb->b */
	
blkloop:
	/*
		+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
	        |15 |   |   |   |   |   |   |   | 7 | 6 | 5 | 4 |   |   |   | 0 |
	        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
	        |                 dc                |mde| class |               |
	        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
	*/
	/* dc coefficient = bitstream_get(bs,9); */
	mov	mb_start(%rip),%ecx     /* int32 */
	shr	$3,%rcx
	lea	blk_start(%rip),%r11
	movzbq	(%r11,%r12),%r11        /* int8 */
	add	%rcx,%r11
	movzbq	(%r14,%r11,1),%rax	/* hi byte */
	movzbq	1(%r14,%r11,1),%rcx	/* lo byte */
	shl	$8,%rax
	or	%rcx,%rax               /* int16 */

        mov     %rax,%r11
        /* if(dc > 255) dc -= 512;
           just do an arithmetric shift right 7bits*/
        sarw     $7,%r11w               /* dc in %r11, 9 bits */
        movw    %r11w,dv_block_t_coeffs(%r15)     /* int16 */

	/* bl->class_no = bitstream_get(bs,2); */
	mov	%rax,%rcx
	shr	$4,%rcx
	and	$3,%rcx
	movl	%ecx,dv_block_t_class_no(%r15)    /* int32 */

	/* bl->eob=0 */
 	xor	%rcx,%rcx
	movl	%ecx,dv_block_t_eob(%r15)         /* int32 */

	/* bl->dct_mode = bitstream_get(bs,1); */
	shr	$6,%rax
	and	$1,%rax
	movl	%eax,dv_block_t_dct_mode(%r15)    /* int32 */

	/* bl->reorder = &dv_reorder[bl->dct_mode][1]; */
	shl	$6,%rax                          /* *64 */
	mov	dv_reorder@GOTPCREL(%rip),%rcx
	add	$1,%rcx
	add	%rcx,%rax
	mov	%rax,dv_block_t_reorder(%r15)    /* ptr */

	/* bl->reorder_sentinel = bl->reorder + 63; */
	add	$63,%rax
	mov	%rax,dv_block_t_reorder_sentinel(%r15) /* ptr */

	/* bl->offset= mb_start + dv_parse_bit_start[b]; */
/*	xor	%rcx,%rcx */
	movl	mb_start(%rip),%ecx             /* int32 */
	mov	dv_parse_bit_start@GOTPCREL(%rip),%rax
	mov	(%rax,%r12,4),%eax              /* int32 */
	add	%rcx,%rax
	movl	%eax,dv_block_t_offset(%r15)    /* int32 */

	/* bl->end= mb_start + dv_parse_bit_end[b]; */
	mov	dv_parse_bit_end@GOTPCREL(%rip),%rax
	mov	(%rax,%r12,4),%eax            /* int32 */
	add	%ecx,%eax
	mov	%eax,dv_block_t_end(%r15)     /* int32 */

	/* dv_parse_ac_coeffs_pass0(bs,mb,bl); */
	mov	%rsi,%rcx	/* quality */
	test	$DV_QUALITY_AC_MASK,%rcx
	jnz	do_ac_pass
	
	/* no AC pass.  Just zero out the remaining coeffs */
	movq    dv_block_t_coeffs(%r15),%mm1
	pxor    %mm0,%mm0
	pand    const_f_0_0_0(%rip),%mm1
	movq    %mm1,dv_block_t_coeffs(%r15)
	movq    %mm0,(dv_block_t_coeffs + 8)(%r15)
	movq    %mm0,(dv_block_t_coeffs + 16)(%r15)
	movq    %mm0,(dv_block_t_coeffs + 24)(%r15)
	movq    %mm0,(dv_block_t_coeffs + 32)(%r15)
	movq    %mm0,(dv_block_t_coeffs + 40)(%r15)
	movq    %mm0,(dv_block_t_coeffs + 48)(%r15)
	movq    %mm0,(dv_block_t_coeffs + 56)(%r15)
	movq    %mm0,(dv_block_t_coeffs + 64)(%r15)
	movq    %mm0,(dv_block_t_coeffs + 72)(%r15)
	movq    %mm0,(dv_block_t_coeffs + 80)(%r15)
	movq    %mm0,(dv_block_t_coeffs + 88)(%r15)
	movq    %mm0,(dv_block_t_coeffs + 96)(%r15)
	movq    %mm0,(dv_block_t_coeffs + 104)(%r15)
	movq    %mm0,(dv_block_t_coeffs + 112)(%r15)
	movq    %mm0,(dv_block_t_coeffs + 120)(%r15)
	jmp	done_ac
	
do_ac_pass:
	/* dv_parse_ac_coeffs_pass0(bs,mb,bl);   Args are at rdi=bs, rsi=mb, rdx=bl */
	push	%rdx
	push	%rsi
	push	%rdi
	mov	dv_videosegment_t_bs(%rdi),%rdi   /* passed in rdi was seg, now passing seg->bs */
	mov	%r13,%rsi                         /* mb */
	mov	%r15,%rdx                         /* bl */
	mov     dv_parse_ac_coeffs_pass0@GOTPCREL(%rip),%r11
	call	*%r11
	pop	%rdi
	pop	%rsi
	pop	%rdx
	
done_ac:

	movl	n_blocks(%rip),%eax          /* int32 */
	add	$dv_block_t_size,%r15        /* point to next block */
	inc	%r12                         /* b++ */
	cmp	%eax,%r12d
	jnz	blkloop

	mov	m(%rip),%eax                 /* int32 */
	mov	mb_start(%rip),%ecx          /* int32 */
	add	$(8 * 80),%ecx
	add	$dv_macroblock_t_size,%r13   /* point to next macroblock */
	inc	%eax                         /* m++ */
	cmp	$5,%eax
	jnz	macloop
	
	pop	%r15
	pop	%r14
	pop	%r13
	pop	%r12

	emms

	/* if ((quality & DV_QUALITY_AC_MASK) == DV_QUALITY_AC_2) */
	mov	%rsi,%rax	            /* quality */
	and	$DV_QUALITY_AC_MASK,%rax
	cmp	$DV_QUALITY_AC_2,%rax

	jne	done
	mov	dv_parse_ac_coeffs@GOTPCREL(%rip),%r11
	jmp	*%r11
	
done:	mov	$0,%rax
	
	ret

.data
vlc:
	.long	0
m:
	.long	0
mb_start:
	.long	0
n_blocks:
	.long	0	/* 4 for monochrome, 6 for color */
blk_start:
	.byte	4,18,32,46,60,70
	
	/* mod tables, 32 bytes apart */
mod_10:
	.byte	0,1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7
	.byte	0,0,0,0,0,0,0,0,0,0,0,0,0,0	/* spacer, see above */
mod_12:
	.byte	0,1,2,3,4,5,6,7,8,9,10,11,0,1,2,3,4,5,6,7,8
	
	.align 16
const_f_0_0_0:
	.short	0xffff,0,0,0
