.section .note.GNU-stack, "", @progbits

.text
.global _dv_transpose_mmx_x86_64
	
_dv_transpose_mmx_x86_64:
	
/* extern void _dv_transpose_mmx_x86_64(short * dst); */
	
	/* argument dst=rdi */
	
	push	%r12
        push	%rbx
        push	%rcx
        push	%rdx

	mov     %rdi, %r11      # dst

        mov     $8, %rbx        # rbx is x_size
        mov     %rbx, %rcx
        mov     %r11, %r12      # pointer to the matrix

        sal     $2, %rcx
        mov     %rbx, %rax
        add     %rbx, %rcx
        sub     $4, %rax        # rax is the inner loop variable

        add     %rbx, %rcx      # rcx is 6*row size
        mov     %rax, %rdx      # rdx is the outer loop variable

do_4x4_block_where_x_equals_y: 

        movq    (%r11), %mm0            # m03:m02|m01:m00 - first line

        movq    (%r11,%rbx,4), %mm2     # m23:m22|m21:m20 - third line
        movq    %mm0, %mm6              # copy first line

        punpcklwd (%r11,%rbx,2), %mm0
        # m11:m01|m10:m00 - interleave first and second lines
        movq    %mm2, %mm7              # copy third line

        punpcklwd (%r11,%rcx,), %mm2
        # m31:m21|m30:m20 - interleave third and fourth lines
        movq    %mm0, %mm4              # copy first intermediate result

        movq    (%r11,%rbx,2), %mm1     # m13:m12|m11:m10 - second line
        punpckldq %mm2, %mm0
        # m30:m20|m10:m00 - interleave to produce result 1

        movq    (%r11,%rcx,), %mm3      # m33:m32|m31:m30 - fourth line
        punpckhdq %mm2, %mm4
        # m31:m21|m11:m01 - interleave to produce result 2

        movq    %mm0, (%r11)            # write result 1
        punpckhwd %mm1, %mm6
        # m13:m03|m12:m02 - interleave first and second lines

        movq    %mm4, (%r11,%rbx,2)     # write result 2
        punpckhwd %mm3, %mm7
        # m33:m23|m32:m22 - interleave third and fourth lines

        movq    %mm6, %mm5              # copy first intermediate result
        punpckldq %mm7, %mm6
        # m32:m22|m12:m02 - interleave to produce result 3

        lea     (%r12,%rbx,8), %r12
        # reload r12 to point to a 4x4 set 4 rows down
        punpckhdq %mm7, %mm5
        # m33:m23|m13:m03 - interleave to produce result 4

        movq    %mm6, (%r11,%rbx,4)     # write result 3

        movq    %mm5, (%r11,%rcx,)      # write result 4

        cmp     $0, %rdx
        # check to see if the number of rows left is zero
        je      all_done_ready_to_exit
        #last time through you are done and ready to exit

do_4x4_blocks_x_and_y_not_equal: 

# transpose the two mirror image 4x4 sets so that the writes 
# can be done without overwriting unused data 

        movq    8(%r11), %mm0           # m03:m02|m01:m00 - first line

        movq    8(%r11,%rbx,4), %mm2     # m23:m22|m21:m20 - third line
        movq    %mm0, %mm6              # copy first line

        punpcklwd 8(%r11,%rbx,2), %mm0
        # m11:m01|m10:m00 - interleave first and second lines
        movq    %mm2, %mm7              # copy third line

        punpcklwd 8(%r11,%rcx,), %mm2
        # m31:m21|m30:m20 - interleave third and fourth lines
        movq    %mm0, %mm4              # copy first intermediate result
# all references for second 4 x 4 block are referred by "n" instead of "m"
        movq    (%r12), %mm1            # n03:n02|n01:n00 - first line 
        punpckldq %mm2, %mm0
        # m30:m20|m10:m00 - interleave to produce first result

        movq    (%r12,%rbx,4), %mm3     # n23:n22|n21:n20 - third line
        punpckhdq %mm2, %mm4
        # m31:m21|m11:m01 - interleave to produce second result

        punpckhwd 8(%r11,%rbx,2), %mm6
        # m13:m03|m12:m02 - interleave first and second lines
        movq    %mm1, %mm2              # copy first line

        punpckhwd 8(%r11,%rcx,), %mm7
        # m33:m23|m32:m22 - interleave third and fourth lines
        movq    %mm6, %mm5              # copy first intermediate result

        movq    %mm0, (%r12)            # write result 1
        punpckhdq %mm7, %mm5
        # m33:m23|m13:m03 - produce third result

        punpcklwd (%r12,%rbx,2), %mm1
        # n11:n01|n10:n00 - interleave first and second lines
        movq    %mm3, %mm0              # copy third line

        punpckhwd (%r12,%rbx,2), %mm2
        # n13:n03|n12:n02 - interleave first and second lines

        movq    %mm4, (%r12,%rbx,2)     # write result 2 out
        punpckldq %mm7, %mm6
        # m32:m22|m12:m02 - produce fourth result

        punpcklwd (%r12,%rcx,), %mm3
        # n31:n21|n30:n20 - interleave third and fourth lines
        movq    %mm1, %mm4              # copy first intermediate result

        movq    %mm6, (%r12,%rbx,4)     # write result 3 out
        punpckldq %mm3, %mm1
        # n30:n20|n10:n00 - produce first result

        punpckhwd (%r12,%rcx,), %mm0
        # n33:n23|n32:n22 - interleave third and fourth lines
        movq    %mm2, %mm6              # copy second intermediate result

        movq    %mm5, (%r12,%rcx,)      # write result 4 out
        punpckhdq %mm3, %mm4
        # n31:n21|n11:n01- produce second result

        movq    %mm1, 8(%r11)
        # write result 5 out - (first result for other 4 x 4 block)
        punpckldq %mm0, %mm2
        # n32:n22|n12:n02- produce third result

        movq    %mm4, 8(%r11,%rbx,2)     # write result 6 out
        punpckhdq %mm0, %mm6
        # n33:n23|n13:n03 - produce fourth result

        movq    %mm2, 8(%r11,%rbx,4)     # write result 7 out

        movq    %mm6, 8(%r11,%rcx,)     # write result 8 out

        add     $8, %r11
        # increment r11 to point to next 4 x 4 block in same row
        lea     (%r12,%rbx,8), %r12
        # increment r12 to point to next 4 x 4 block below current one

        sub     $4, %rax                # decrement inner loop variable
        jnz     do_4x4_blocks_x_and_y_not_equal
        # rax points to start of the second row in block we just finished

        sal     %rdx
        lea     8(%r11,%rbx,8), %r11     # reload r11 to point four rows down

        sub     %rdx, %r11
        # subtract the number of bytes in last row 
        # now we point to spot where row = col 
        sub     $8, %rdx                # sub 4 from row number

        sar     %rdx
        mov     %r11, %r12

        mov     %rdx, %rax
        # reset x_size to outer loop variable to start new row
	
        jmp     do_4x4_block_where_x_equals_y

all_done_ready_to_exit:
	
        pop	%rdx
        pop	%rcx
        pop	%rbx
	pop	%r12

	ret     $0
