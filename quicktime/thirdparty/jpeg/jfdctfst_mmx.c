#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jdct.h"		/* Private declarations for DCT subsystem */


// took this code from intel, set DCTELEM type to short int, and ... it works


#define MULT_BITS $0x2

int64_t C4 = 0x2D412D412D412D41,  // cos(4*PI/16)*2^14+.5
        C6 = 0x187E187E187E187E,  // cos(6*PI/16)*2^14+.5
     C2mC6 = 0x22A322A322A322A3,  // [cos(6*PI/16)-cos(6*PI/16)]*2^14+.5
     C2pC6 = 0x539F539F539F539F;  // [cos(6*PI/16)+cos(6*PI/16)]*2^14+.5


/*
 * Perform the forward DCT on one block of samples.
 */
//
GLOBAL(void) jpeg_fdct_ifast_mmx (DCTELEM  * data) {
  int64_t temp6, temp7;
  
  __asm__ __volatile__ ("

        // Do fourth quadrant before any DCT

        movq 8+2*8*4(%0),%%mm7  # m03:m02|m01:m00 - first line (line 4)
        
        movq 8+2*8*6(%0),%%mm6     # m23:m22|m21:m20 - third line (line 6)

        movq %%mm7,%%mm5              # copy first line



        punpcklwd 8+2*8*5(%0),%%mm7                        # m11:m01|m10:m00 - interleave first and second lines

        movq %%mm6,%%mm2                  # copy third line



        punpcklwd 8+2*8*7(%0),%%mm6                        # m31:m21|m30:m20 - interleave third and fourth lines

        movq %%mm7,%%mm1                  # copy first intermediate result



        movq 8+2*8*5(%0),%%mm3     # m13:m12|m11:m10 - second line

        punpckldq %%mm6,%%mm7             # m30:m20|m10:m00 - interleave to produce result 1



        movq 8+2*8*7(%0),%%mm0    # m33:m32|m31:m30 - fourth line

        punpckhdq %%mm6,%%mm1             # m31:m21|m11:m01 - interleave to produce result 2



        movq %%mm7,8+2*8*4(%0)      # write result 1

        punpckhwd %%mm3,%%mm5             # m13:m03|m12:m02 - interleave first and second lines



        movq %%mm1,8+2*8*5(%0)     # write result 2

        punpckhwd %%mm0,%%mm2             # m33:m23|m32:m22 - interleave third and fourth lines



        movq %%mm5,%%mm1                  # copy first intermediate result

        punpckldq %%mm2,%%mm5             # m32:m22|m12:m02 - interleave to produce result 3



        movq 8(%0),%%mm0                                 # m03:m02|m01:m00 - first line, 4x4!=

        punpckhdq %%mm2,%%mm1                             # m33:m23|m13:m03 - interleave to produce result 4



        movq %%mm5,8+2*8*6(%0)     # write result 3

  #***********************Last 4x4 done





#do_4x4_blocks_x_and_y_not_equal:



# transpose the two mirror image 4x4 sets so that the writes 

# can be done without overwriting unused data 


        movq %%mm1,8+2*8*7(%0)     # write result 4, last 4x4




        movq 8+2*8*2(%0),%%mm2     # m23:m22|m21:m20 - third line

        movq %%mm0,%%mm6                                                  # copy first line



        punpcklwd 8+2*8*1(%0),%%mm0    # m11:m01|m10:m00 - interleave first and second lines

        movq %%mm2,%%mm7                  # copy third line



        punpcklwd 8+2*8*3(%0),%%mm2    # m31:m21|m30:m20 - interleave third and fourth lines

        movq %%mm0,%%mm4                  # copy first intermediate result

# all references for second 4 x 4 block are referred by \"n\" instead of \"m\"

        movq 2*8*4(%0),%%mm1                 # n03:n02|n01:n00 - first line 

        punpckldq %%mm2,%%mm0

        # m30:m20|m10:m00 - interleave to produce first result



        movq 2*8*6(%0),%%mm3           # n23:n22|n21:n20 - third line

        punpckhdq %%mm2,%%mm4             # m31:m21|m11:m01 - interleave to produce second result



        punpckhwd 8+2*8*1(%0),%%mm6 #m13:m03|m12:m02 - interleave first and second lines

        movq %%mm1,%%mm2                  # copy first line



        punpckhwd 8+2*8*3(%0),%%mm7     # m33:m23|m32:m22 - interleave third and fourth lines

        movq %%mm6,%%mm5                  # copy first intermediate result



        movq %%mm0,2*8*4(%0)                 # write result 1

        punpckhdq %%mm7,%%mm5                     # m33:m23|m13:m03 - produce third result



        punpcklwd 2*8*5(%0),%%mm1    # n11:n01|n10:n00 - interleave first and second lines

        movq %%mm3,%%mm0                  # copy third line



        punpckhwd 2*8*5(%0),%%mm2  # n13:n03|n12:n02 - interleave first and second lines



        movq %%mm4,2*8*5(%0)           # write result 2 out

        punpckldq %%mm7,%%mm6                  # m32:m22|m12:m02 - produce fourth result



        punpcklwd 2*8*7(%0),%%mm3 # n31:n21|n30:n20 - interleave third and fourth lines

        movq %%mm1,%%mm4                  # copy first intermediate result



        movq %%mm6,2*8*6(%0)           # write result 3 out

        punpckldq %%mm3,%%mm1            # n30:n20|n10:n00 - produce first result



        punpckhwd 2*8*7(%0),%%mm0        # n33:n23|n32:n22 - interleave third and fourth lines

        movq %%mm2,%%mm6                  # copy second intermediate result



        movq %%mm5,2*8*7(%0)             # write result 4 out

        punpckhdq %%mm3,%%mm4            # n31:n21|n11:n01- produce second result



        movq %%mm1,8+2*8*0(%0)       # write result 5 out - (first result for other 4 x 4 block)

        punpckldq %%mm0,%%mm2           # n32:n22|n12:n02- produce third result



        movq %%mm4,8+2*8*1(%0)         # write result 6 out

        punpckhdq %%mm0,%%mm6            # n33:n23|n13:n03 - produce fourth result



        movq %%mm2,8+2*8*2(%0)         # write result 7 out



        movq 2*8*0(%0),%%mm0     # m03:m02|m01:m00 - first line, first 4x4



        movq %%mm6,8+2*8*3(%0)           # write result 8 out





#Do first 4x4 quadrant, which is used in the beginning of the DCT:



        movq 2*8*2(%0),%%mm7     # m23:m22|m21:m20 - third line

        movq %%mm0,%%mm2                  # copy first line



        punpcklwd 2*8*1(%0),%%mm0# m11:m01|m10:m00 - interleave first and second lines

        movq %%mm7,%%mm4                  # copy third line



        punpcklwd 2*8*3(%0),%%mm7# m31:m21|m30:m20 - interleave third and fourth lines

        movq %%mm0,%%mm1                  # copy first intermediate result



        movq 2*8*1(%0),%%mm6     # m13:m12|m11:m10 - second line

        punpckldq %%mm7,%%mm0             # m30:m20|m10:m00 - interleave to produce result 1



        movq 2*8*3(%0),%%mm5     # m33:m32|m31:m30 - fourth line

        punpckhdq %%mm7,%%mm1             # m31:m21|m11:m01 - interleave to produce result 2



        movq %%mm0,%%mm7                              # write result 1

        punpckhwd %%mm6,%%mm2             # m13:m03|m12:m02 - interleave first and second lines



        psubw 2*8*7(%0),%%mm7    #tmp07=x0-x7    # Stage 1 

        movq %%mm1,%%mm6                                  # write result 2



        paddw 2*8*7(%0),%%mm0    #tmp00=x0+x7    # Stage 1 

        punpckhwd %%mm5,%%mm4             # m33:m23|m32:m22 - interleave third and fourth lines



        paddw 2*8*6(%0),%%mm1    #tmp01=x1+x6    # Stage 1 

        movq %%mm2,%%mm3                  # copy first intermediate result



        psubw 2*8*6(%0),%%mm6    #tmp06=x1-x6    # Stage 1 

        punpckldq %%mm4,%%mm2             # m32:m22|m12:m02 - interleave to produce result 3





        movq %%mm7,%2                         #save tmp07             # Stage 1 

        movq %%mm2,%%mm5                                  # write result 3



        movq %%mm6,%1                         #save tmp06             # Stage 1 

        punpckhdq %%mm4,%%mm3                             # m33:m23|m13:m03 - interleave to produce result 4





        paddw 2*8*5(%0),%%mm2    #tmp02=x2+x5    # Stage 1 

        movq %%mm3,%%mm4                                  # write result 4



#  Pass 1: process rows. 
#  Stage 1, above
#  dataptr = data#
#  for (ctr = 8-1# ctr >= 0# ctr--) {
#    tmp0 = dataptr[8*0] + dataptr[8*7]#
#    tmp7 = dataptr[8*0] - dataptr[8*7]#
#    tmp1 = dataptr[8*1] + dataptr[8*6]#
#    tmp6 = dataptr[8*1] - dataptr[8*6]#
#    tmp2 = dataptr[8*2] + dataptr[8*5]#
#    tmp5 = dataptr[8*2] - dataptr[8*5]#
#    tmp3 = dataptr[8*3] + dataptr[8*4]#
#    tmp4 = dataptr[8*3] - dataptr[8*4]#

# Stage 2, Even part
#    tmp10 = tmp0 + tmp3#        
#    tmp13 = tmp0 - tmp3#
#    tmp11 = tmp1 + tmp2#
#    tmp12 = tmp1 - tmp2#


        paddw 2*8*4(%0),%%mm3    #tmp03=x3+x4     Stage 1 

        movq %%mm0,%%mm7                          #copy tmp00                                     # Even 2 



        psubw 2*8*4(%0),%%mm4    #tmp04=x3-x4    # Stage 1 

        movq %%mm1,%%mm6                          #copy tmp01                                     # Even 2 



        paddw %%mm3,%%mm0                         #tmp10 = tmp00 + tmp03#         # Even 2 

        psubw %%mm3,%%mm7                         #tmp13 = tmp00 - tmp03#         # Even 2 



    psubw %%mm2,%%mm6                             #tmp12 = tmp01 - tmp02#         # Even 2 

    paddw %%mm2,%%mm1                             #tmp11 = tmp01 + tmp02#         # Even 2      



        psubw 2*8*5(%0),%%mm5    #tmp05=x2-x5    # Stage 1 

        paddw %%mm7,%%mm6                         #tmp12+tmp13                    # Even 4 





# Stage 3, Even                and                                             Stage 4 & 5, Even
#    dataptr[8*0] =y0= tmp10 + tmp11#      z1 = MULTIPLY(tmp12 + tmp13, FIX_0_707106781)#  c4 
#    dataptr[8*4] =y4= tmp10 - tmp11#      dataptr[8*2] = tmp13 + z1#


        movq %1,%%mm2                         #load tmp6                                      # Odd 2 

        movq %%mm0,%%mm3                          #copy tmp10                     # Even 3 

        psllw $0x2,%%mm6  # shift z1 by 2 bits  

        paddw %%mm1,%%mm0                         #y0=tmp10 + tmp11       # Even 3 



    pmulhw C4,%%mm6                      #z1 = (tmp12+tmp13)*c4  # Even 4 

        psubw %%mm1,%%mm3                         #y4=tmp10 - tmp11       # Even 3 



    movq %%mm0,2*8*0(%0) #save y0                        # Even 3 

        movq %%mm7,%%mm0                          #copy tmp13                             # Even 4 



# Odd part 
# Stage 2 

# mm4=tmp4, mm5=tmp5, load mm2=tmp6, load mm3=tmp7
#    tmp10 = tmp4 + tmp5#
#    tmp11 = tmp5 + tmp6#
#    tmp12 = tmp6 + tmp7#


    movq %%mm3,2*8*4(%0) #save y4                        # Even 3 

        paddw %%mm5,%%mm4                         #tmp10 = tmp4 + tmp5#           # Odd 2 



        movq %2,%%mm3                         #load tmp7                                      # Odd 2 

        paddw %%mm6,%%mm0                         #tmp32 = tmp13 + z1             # Even 5 



        paddw %%mm2,%%mm5                         #tmp11 = tmp5 + tmp6#           # Odd 2 

        psubw %%mm6,%%mm7                         #tmp33 = tmp13 - z1             # Even 5 



        movq %%mm0,2*8*2(%0)     #save tmp32=y2          # Even 5 

        paddw %%mm3,%%mm2                         #tmp12 = tmp6 + tmp7#           # Odd 2 



#      Stage 4 
#The rotator is modified from fig 4-8 to avoid extra negations. 
#    z5 = MULTIPLY(tmp10 - tmp12, FIX_0_382683433)# c6 
#    z2 = MULTIPLY(tmp10, FIX_0_541196100) + z5#  c2-c6 
#    z4 = MULTIPLY(tmp12, FIX_1_306562965) + z5#  c2+c6 
#    z3 = MULTIPLY(tmp11, FIX_0_707106781)# c4 

        movq %%mm7,2*8*6(%0)     #save tmp32=y6          # Even 5 

        movq %%mm4,%%mm1                                  #copy of tmp10                          # Odd 4 


        psubw %%mm2,%%mm1                                 #tmp10-tmp12                            # Odd 4 

        psllw $0x2,%%mm4



        movq C2mC6,%%mm0                                 #load C2mC6                                     # Odd 4 

        psllw $0x2, %%mm1



    pmulhw C6,%%mm1                                      #z5=(tmp10-tmp12)*c6            # Odd 4 

    psllw $0x2, %%mm2



    pmulhw %%mm0,%%mm4                                    #tmp10*(c2-c6)                          # Odd 4 



# Stage 5 
#    z11 = tmp7 + z3#            
#    z13 = tmp7 - z3#


    pmulhw C2pC6,%%mm2                                   #tmp12*(c2+c6)                          # Odd 4 

    psllw $0x2, %%mm5



    pmulhw C4,%%mm5                                      #z3=tmp11*c4                            # Odd 4 

        movq %%mm3,%%mm0                                  #copy tmp7                              # Stage 5 



        movq 8+2*8*0(%0),%%mm7     #load x0                # Stage 1, next set of 4 

        paddw %%mm1,%%mm4                                 #z2=tmp10*(c2-c6)+z5            # Odd 4 



        paddw %%mm1,%%mm2                                 #z4=tmp12*(c2+c6)+z5            # Odd 4 



        paddw %%mm5,%%mm0                                 #z11 = tmp7 + z3#               # Stage 5 

        psubw %%mm5,%%mm3                                 #z13 = tmp7 - z3#               # Stage 5 



# Stage 6 
#    dataptr[8*5] = z13 + z2# 
#    dataptr[8*3] = z13 - z2#
#    dataptr[8*1] = z11 + z4#
#    dataptr[8*7] = z11 - z4#

        movq %%mm3,%%mm5                                  #copy z13                               # Stage 6 

        psubw %%mm4,%%mm3                                 #y3=z13 - z2                    # Stage 6 



        paddw %%mm4,%%mm5                                 #y5=z13 + z2                    # Stage 6 

        movq %%mm0,%%mm6                                  #copy z11                               # Stage 6 



        movq %%mm3,2*8*3(%0)             #save y3                                # Stage 6 

        psubw %%mm2,%%mm0                                 #y7=z11 - z4                    # Stage 6 



        movq %%mm5,2*8*5(%0)             #save y5                                # Stage 6 

        paddw %%mm2,%%mm6                                 #y1=z11 + z4                    # Stage 6 



        movq %%mm0,2*8*7(%0)             #save y7                                # Stage 6 



# Pass 2: process rows.

# dataptr = data#
#  for (ctr = 8-1# ctr >= 0# ctr--) {
#    tmp0 = dataptr[8*0] + dataptr[8*7]#
#    tmp7 = dataptr[8*0] - dataptr[8*7]#
#    tmp1 = dataptr[8*1] + dataptr[8*6]#
#    tmp6 = dataptr[8*1] - dataptr[8*6]#
#    tmp2 = dataptr[8*2] + dataptr[8*5]#
#    tmp5 = dataptr[8*2] - dataptr[8*5]#
#    tmp3 = dataptr[8*3] + dataptr[8*4]#
#    tmp4 = dataptr[8*3] - dataptr[8*4]#

#Stage 1,even



        movq 8+2*8*1(%0),%%mm1     #load x1                # Stage 1 

        movq %%mm7,%%mm0                          #copy x0                                # Stage 1 



        movq %%mm6,2*8*1(%0)             #save y1                                # Stage 6 



        movq 8+2*8*2(%0),%%mm2     #load x2                # Stage 1 

        movq %%mm1,%%mm6                          #copy x1                                # Stage 1 



        paddw 8+2*8*7(%0),%%mm0    #tmp00=x0+x7    # Stage 1 



        movq 8+2*8*3(%0),%%mm3     #load x3                # Stage 1 

        movq %%mm2,%%mm5                          #copy x2                                # Stage 1 





        psubw 8+2*8*7(%0),%%mm7    #tmp07=x0-x7    # Stage 1 

        movq %%mm3,%%mm4                          #copy x3                                # Stage 1 



        paddw 8+2*8*6(%0),%%mm1    #tmp01=x1+x6    # Stage 1 



        movq %%mm7,%2                         #save tmp07                             # Stage 1 

        movq %%mm0,%%mm7                          #copy tmp00                                     # Even 2 



        psubw 8+2*8*6(%0),%%mm6    #tmp06=x1-x6    # Stage 1 



# Stage 2, Even part
#    tmp10 = tmp0 + tmp3#        
#    tmp13 = tmp0 - tmp3#
#    tmp11 = tmp1 + tmp2#
#    tmp12 = tmp1 - tmp2#

        paddw 8+2*8*4(%0),%%mm3    #tmp03=x3+x4    # Stage 1 



        movq %%mm6,%1                         #save tmp06             # Stage 1 

        movq %%mm1,%%mm6                          #copy tmp01                                     # Even 2 





        paddw 8+2*8*5(%0),%%mm2    #tmp02=x2+x5    # Stage 1 

        paddw %%mm3,%%mm0                         #tmp10 = tmp00 + tmp03#         # Even 2 



        psubw %%mm3,%%mm7                         #tmp13 = tmp00 - tmp03#         # Even 2 



        psubw 8+2*8*4(%0),%%mm4    #tmp04=x3-x4    # Stage 1 

    psubw %%mm2,%%mm6                             #tmp12 = tmp01 - tmp02#         # Even 2 



    paddw %%mm2,%%mm1                             #tmp11 = tmp01 + tmp02#         # Even 2      



        psubw 8+2*8*5(%0),%%mm5    #tmp05=x2-x5    # Stage 1 

        paddw %%mm7,%%mm6                         #tmp12+tmp13                    # Even 4 





# Stage 3, Even                and                                             Stage 4 & 5, Even
#    dataptr[8*0] =y0= tmp10 + tmp11#      z1 = MULTIPLY(tmp12 + tmp13, FIX_0_707106781)#  c4 
#    dataptr[8*4] =y4= tmp10 - tmp11#      dataptr[8*2] = tmp13 + z1#


        movq %1,%%mm2                         #load tmp6                                      # Odd 2 

        movq %%mm0,%%mm3                          #copy tmp10                     # Even 3 



        psllw $0x2, %%mm6

        paddw %%mm1,%%mm0                         #y0=tmp10 + tmp11       # Even 3 



    pmulhw C4,%%mm6                      #z1 = (tmp12+tmp13)*c4  # Even 4 

        psubw %%mm1,%%mm3                         #y4=tmp10 - tmp11       # Even 3 



    movq %%mm0,8+2*8*0(%0) #save y0                        # Even 3 

        movq %%mm7,%%mm0                          #copy tmp13                             # Even 4 



# Odd part 
# Stage 2 
#mm4=tmp4, mm5=tmp5, load mm2=tmp6, load mm3=tmp7
#    tmp10 = tmp4 + tmp5#
#    tmp11 = tmp5 + tmp6#
#    tmp12 = tmp6 + tmp7#

    movq %%mm3,8+2*8*4(%0) #save y4                        # Even 3 

        paddw %%mm5,%%mm4                         #tmp10 = tmp4 + tmp5#           # Odd 2 



        movq %2,%%mm3                         #load tmp7                                      # Odd 2 

        paddw %%mm6,%%mm0                         #tmp32 = tmp13 + z1             # Even 5 



        paddw %%mm2,%%mm5                         #tmp11 = tmp5 + tmp6#           # Odd 2 

        psubw %%mm6,%%mm7                         #tmp33 = tmp13 - z1             # Even 5 



        movq %%mm0,8+2*8*2(%0)     #save tmp32=y2          # Even 5 

        paddw %%mm3,%%mm2                         #tmp12 = tmp6 + tmp7#           # Odd 2 



#      Stage 4 
#The rotator is modified from fig 4-8 to avoid extra negations. 
#    z5 = MULTIPLY(tmp10 - tmp12, FIX_0_382683433)#  c6 
#    z2 = MULTIPLY(tmp10, FIX_0_541196100) + z5#  c2-c6 
#    z4 = MULTIPLY(tmp12, FIX_1_306562965) + z5#  c2+c6 
#    z3 = MULTIPLY(tmp11, FIX_0_707106781)#  c4 

        movq %%mm7,8+2*8*6(%0)     #save tmp32=y6          # Even 5 

        movq %%mm4,%%mm1                                  #copy of tmp10                          # Odd 4 



        psubw %%mm2,%%mm1                                 #tmp10-tmp12                            # Odd 4 

    psllw $0x2, %%mm4



        movq C2mC6,%%mm0                                 #load C2mC6                                     # Odd 4 

    psllw $0x2, %%mm1



    pmulhw C6,%%mm1                                      #z5=(tmp10-tmp12)*c6            # Odd 4 

    psllw $0x2, %%mm5



    pmulhw %%mm0,%%mm4                                    #tmp10*(c2-c6)                          # Odd 4 



# Stage 5 
#    z11 = tmp7 + z3#            
#    z13 = tmp7 - z3#


    pmulhw C4,%%mm5                                      #z3=tmp11*c4                            # Odd 4 

    psllw $0x2, %%mm2



    pmulhw C2pC6,%%mm2                                   #tmp12*(c2+c6)                          # Odd 4 

        movq %%mm3,%%mm0                                  #copy tmp7                              # Stage 5 



        movq 8+2*8*4(%0),%%mm7     # m03:m02|m01:m00 - first line (line 4)

        paddw %%mm1,%%mm4                                 #z2=tmp10*(c2-c6)+z5            # Odd 4 



        paddw %%mm5,%%mm0                                 #z11 = tmp7 + z3#               # Stage 5 

        psubw %%mm5,%%mm3                                 #z13 = tmp7 - z3#               # Stage 5 



# Stage 6 
#    dataptr[8*5] = z13 + z2# 
#    dataptr[8*3] = z13 - z2#
#    dataptr[8*1] = z11 + z4#
#    dataptr[8*7] = z11 - z4#


        movq %%mm3,%%mm5                                  #copy z13                               # Stage 6 

        paddw %%mm1,%%mm2                                 #z4=tmp12*(c2+c6)+z5            # Odd 4 



        movq %%mm0,%%mm6                                  #copy z11                               # Stage 6 

        psubw %%mm4,%%mm5                                 #y3=z13 - z2                    # Stage 6 



        paddw %%mm2,%%mm6                                 #y1=z11 + z4                    # Stage 6 

        paddw %%mm4,%%mm3                                 #y5=z13 + z2                    # Stage 6 



        movq %%mm5,8+2*8*3(%0)             #save y3                                # Stage 6 



        movq %%mm6,8+2*8*1(%0)             #save y1                                # Stage 6 

        psubw %%mm2,%%mm0                                 #y7=z11 - z4                    # Stage 6 








#Do fourth quadrant after DCT

        movq 8+2*8*6(%0),%%mm6     # m23:m22|m21:m20 - third line (line 6)

        movq %%mm7,%%mm5                  # copy first line



        punpcklwd %%mm3,%%mm7                             # m11:m01|m10:m00 - interleave first and second lines

        movq %%mm6,%%mm2                  # copy third line



        punpcklwd %%mm0,%%mm6                             # m31:m21|m30:m20 - interleave third and fourth lines

        movq %%mm7,%%mm1                  # copy first intermediate result



        punpckldq %%mm6,%%mm7             # m30:m20|m10:m00 - interleave to produce result 1



        punpckhdq %%mm6,%%mm1             # m31:m21|m11:m01 - interleave to produce result 2



        movq %%mm7,8+2*8*4(%0)      # write result 1

        punpckhwd %%mm3,%%mm5             # m13:m03|m12:m02 - interleave first and second lines



        movq %%mm1,8+2*8*5(%0)     # write result 2

        punpckhwd %%mm0,%%mm2             # m33:m23|m32:m22 - interleave third and fourth lines



        movq %%mm5,%%mm1                  # copy first intermediate result

        punpckldq %%mm2,%%mm5             # m32:m22|m12:m02 - interleave to produce result 3



        movq 8(%0),%%mm0                                 # m03:m02|m01:m00 - first line

        punpckhdq %%mm2,%%mm1                             # m33:m23|m13:m03 - interleave to produce result 4



        movq %%mm5,8+2*8*6(%0)     # write result 3

  #***********************Last 4x4 done





#do_4x4_blocks_x_and_y_not_equal:



# transpose the two mirror image 4x4 sets so that the writes 

# can be done without overwriting unused data 



        movq %%mm1,8+2*8*7(%0)     # write result 4



        movq 8+2*8*2(%0),%%mm2     # m23:m22|m21:m20 - third line

        movq %%mm0,%%mm6                                                  # copy first line



        punpcklwd 8+2*8*1(%0),%%mm0    # m11:m01|m10:m00 - interleave first and second lines

        movq %%mm2,%%mm7                  # copy third line



        punpcklwd 8+2*8*3(%0),%%mm2    # m31:m21|m30:m20 - interleave third and fourth lines

        movq %%mm0,%%mm4                  # copy first intermediate result

# all references for second 4 x 4 block are referred by \"n\" instead of \"m\"

        movq 2*8*4(%0),%%mm1                 # n03:n02|n01:n00 - first line 

        punpckldq %%mm2,%%mm0

        # m30:m20|m10:m00 - interleave to produce first result



        movq 2*8*6(%0),%%mm3           # n23:n22|n21:n20 - third line

        punpckhdq %%mm2,%%mm4             # m31:m21|m11:m01 - interleave to produce second result



        punpckhwd 8+2*8*1(%0),%%mm6 #m13:m03|m12:m02 - interleave first and second lines

        movq %%mm1,%%mm2                  # copy first line



        punpckhwd 8+2*8*3(%0),%%mm7     # m33:m23|m32:m22 - interleave third and fourth lines

        movq %%mm6,%%mm5                  # copy first intermediate result



        movq %%mm0,2*8*4(%0)                 # write result 1

        punpckhdq %%mm7,%%mm5                     # m33:m23|m13:m03 - produce third result



        punpcklwd 2*8*5(%0),%%mm1    # n11:n01|n10:n00 - interleave first and second lines

        movq %%mm3,%%mm0                  # copy third line



        punpckhwd 2*8*5(%0),%%mm2  # n13:n03|n12:n02 - interleave first and second lines



        movq %%mm4,2*8*5(%0)           # write result 2 out

        punpckldq %%mm7,%%mm6                  # m32:m22|m12:m02 - produce fourth result



        punpcklwd 2*8*7(%0),%%mm3 # n31:n21|n30:n20 - interleave third and fourth lines

        movq %%mm1,%%mm4                  # copy first intermediate result



        movq %%mm6,2*8*6(%0)           # write result 3 out

        punpckldq %%mm3,%%mm1            # n30:n20|n10:n00 - produce first result



        punpckhwd 2*8*7(%0),%%mm0        # n33:n23|n32:n22 - interleave third and fourth lines

        movq %%mm2,%%mm6                  # copy second intermediate result



        movq %%mm5,2*8*7(%0)             # write result 4 out

        punpckhdq %%mm3,%%mm4            # n31:n21|n11:n01- produce second result



        movq %%mm1,8+2*8*0(%0)       # write result 5 out - (first result for other 4 x 4 block)

        punpckldq %%mm0,%%mm2           # n32:n22|n12:n02- produce third result



        movq %%mm4,8+2*8*1(%0)         # write result 6 out

        punpckhdq %%mm0,%%mm6            # n33:n23|n13:n03 - produce fourth result



        movq %%mm2,8+2*8*2(%0)         # write result 7 out



        movq 2*8*0(%0),%%mm0     # m03:m02|m01:m00 - first line, first 4x4



        movq %%mm6,8+2*8*3(%0)           # write result 8 out





#Do first 4x4 quadrant, which is used in the beginning of the DCT:

        movq 2*8*2(%0),%%mm7     # m23:m22|m21:m20 - third line

        movq %%mm0,%%mm2                  # copy first line



        punpcklwd 2*8*1(%0),%%mm0# m11:m01|m10:m00 - interleave first and second lines

        movq %%mm7,%%mm4                  # copy third line



        punpcklwd 2*8*3(%0),%%mm7# m31:m21|m30:m20 - interleave third and fourth lines

        movq %%mm0,%%mm1                  # copy first intermediate result



        movq 2*8*1(%0),%%mm6     # m13:m12|m11:m10 - second line

        punpckldq %%mm7,%%mm0             # m30:m20|m10:m00 - interleave to produce result 1



        movq 2*8*3(%0),%%mm5     # m33:m32|m31:m30 - fourth line

        punpckhdq %%mm7,%%mm1             # m31:m21|m11:m01 - interleave to produce result 2





        movq %%mm0,%%mm7                              # write result 1

        punpckhwd %%mm6,%%mm2             # m13:m03|m12:m02 - interleave first and second lines





        psubw 2*8*7(%0),%%mm7    #tmp07=x0-x7    # Stage 1 

        movq %%mm1,%%mm6                                  # write result 2



        paddw 2*8*7(%0),%%mm0    #tmp00=x0+x7    # Stage 1 

        punpckhwd %%mm5,%%mm4             # m33:m23|m32:m22 - interleave third and fourth lines



        paddw 2*8*6(%0),%%mm1    #tmp01=x1+x6    # Stage 1 

        movq %%mm2,%%mm3                  # copy first intermediate result



        psubw 2*8*6(%0),%%mm6    #tmp06=x1-x6    # Stage 1 

        punpckldq %%mm4,%%mm2             # m32:m22|m12:m02 - interleave to produce result 3



        movq %%mm7,%2                         #save tmp07             # Stage 1 

        movq %%mm2,%%mm5                                  # write result 3



        movq %%mm6,%1                         #save tmp06             # Stage 1 



        punpckhdq %%mm4,%%mm3                             # m33:m23|m13:m03 - interleave to produce result 4





        paddw 2*8*5(%0),%%mm2    #tmp02=x2+x5    # Stage 1 

        movq %%mm3,%%mm4                                  # write result 4



# Pass 1: process rows. 
# Stage 1, above
#  dataptr = data#
#  for (ctr = 8-1# ctr >= 0# ctr--) {
#    tmp0 = dataptr[8*0] + dataptr[8*7]#
#    tmp7 = dataptr[8*0] - dataptr[8*7]#
#    tmp1 = dataptr[8*1] + dataptr[8*6]#
#    tmp6 = dataptr[8*1] - dataptr[8*6]#
#    tmp2 = dataptr[8*2] + dataptr[8*5]#
#    tmp5 = dataptr[8*2] - dataptr[8*5]#
#    tmp3 = dataptr[8*3] + dataptr[8*4]#
#    tmp4 = dataptr[8*3] - dataptr[8*4]#


# Stage 2, Even part
#    tmp10 = tmp0 + tmp3#        
#    tmp13 = tmp0 - tmp3#
#    tmp11 = tmp1 + tmp2#
#    tmp12 = tmp1 - tmp2#

        paddw 2*8*4(%0),%%mm3    #tmp03=x3+x4    # Stage 1 

        movq %%mm0,%%mm7                          #copy tmp00                                     # Even 2 



        psubw 2*8*4(%0),%%mm4    #tmp04=x3-x4    # Stage 1 

        movq %%mm1,%%mm6                          #copy tmp01                                     # Even 2 



        paddw %%mm3,%%mm0                         #tmp10 = tmp00 + tmp03#         # Even 2 

        psubw %%mm3,%%mm7                         #tmp13 = tmp00 - tmp03#         # Even 2 



    psubw %%mm2,%%mm6                             #tmp12 = tmp01 - tmp02#         # Even 2 

    paddw %%mm2,%%mm1                             #tmp11 = tmp01 + tmp02#         # Even 2      



        psubw 2*8*5(%0),%%mm5    #tmp05=x2-x5    # Stage 1 

        paddw %%mm7,%%mm6                         #tmp12+tmp13                    # Even 4 





# Stage 3, Even                and                                             Stage 4 & 5, Even
#    dataptr[8*0] =y0= tmp10 + tmp11#      z1 = MULTIPLY(tmp12 + tmp13, FIX_0_707106781)#  c4 
#    dataptr[8*4] =y4= tmp10 - tmp11#      dataptr[8*2] = tmp13 + z1#


        movq %1,%%mm2                         #load tmp6                                      # Odd 2 

        movq %%mm0,%%mm3                          #copy tmp10                     # Even 3 



    psllw $0x2, %%mm6

        paddw %%mm1,%%mm0                         #y0=tmp10 + tmp11       # Even 3 



    pmulhw C4,%%mm6                      #z1 = (tmp12+tmp13)*c4  # Even 4 

        psubw %%mm1,%%mm3                         #y4=tmp10 - tmp11       # Even 3 



    movq %%mm0,2*8*0(%0) #save y0                        # Even 3 

        movq %%mm7,%%mm0                          #copy tmp13                             # Even 4 



# Odd part 
# Stage 2 
#mm4=tmp4, mm5=tmp5, load mm2=tmp6, load mm3=tmp7
#    tmp10 = tmp4 + tmp5#
#    tmp11 = tmp5 + tmp6#
#    tmp12 = tmp6 + tmp7#


    movq %%mm3,2*8*4(%0) #save y4                        # Even 3 

        paddw %%mm5,%%mm4                         #tmp10 = tmp4 + tmp5#           # Odd 2 



        movq %2,%%mm3                         #load tmp7                                      # Odd 2 

        paddw %%mm6,%%mm0                         #tmp32 = tmp13 + z1             # Even 5 



        paddw %%mm2,%%mm5                         #tmp11 = tmp5 + tmp6#           # Odd 2 

        psubw %%mm6,%%mm7                         #tmp33 = tmp13 - z1             # Even 5 



        movq %%mm0,2*8*2(%0)     #save tmp32=y2          # Even 5 

        paddw %%mm3,%%mm2                         #tmp12 = tmp6 + tmp7#           # Odd 2 



#      Stage 4 
#The rotator is modified from fig 4-8 to avoid extra negations. 
#    z5 = MULTIPLY(tmp10 - tmp12, FIX_0_382683433)#  c6 
#    z2 = MULTIPLY(tmp10, FIX_0_541196100) + z5#  c2-c6 
#    z4 = MULTIPLY(tmp12, FIX_1_306562965) + z5#  c2+c6 
#    z3 = MULTIPLY(tmp11, FIX_0_707106781)#  c4 

        movq %%mm7,2*8*6(%0)     #save tmp32=y6          # Even 5 

        movq %%mm4,%%mm1                                  #copy of tmp10                          # Odd 4 



        psubw %%mm2,%%mm1                                 #tmp10-tmp12                            # Odd 4 

    psllw $0x2, %%mm4



        movq C2mC6,%%mm0                                 #load C2mC6                                     # Odd 4 

    psllw $0x2, %%mm1



    pmulhw C6,%%mm1                                      #z5=(tmp10-tmp12)*c6            # Odd 4 

    psllw $0x2, %%mm2



    pmulhw %%mm0,%%mm4                                    #tmp10*(c2-c6)                          # Odd 4 



# Stage 5 
#    z11 = tmp7 + z3#            
#    z13 = tmp7 - z3#

    pmulhw C2pC6,%%mm2                                   #tmp12*(c2+c6)                          # Odd 4 

    psllw $0x2, %%mm5



    pmulhw C4,%%mm5                                      #z3=tmp11*c4                            # Odd 4 

        movq %%mm3,%%mm0                                  #copy tmp7                              # Stage 5 



        movq 8+2*8*0(%0),%%mm7     #load x0                # Stage 1, next set of 4 

        paddw %%mm1,%%mm4                                 #z2=tmp10*(c2-c6)+z5            # Odd 4 



        paddw %%mm1,%%mm2                                 #z4=tmp12*(c2+c6)+z5            # Odd 4 



        paddw %%mm5,%%mm0                                 #z11 = tmp7 + z3#               # Stage 5 

        psubw %%mm5,%%mm3                                 #z13 = tmp7 - z3#               # Stage 5 



# Stage 6 
#    dataptr[8*5] = z13 + z2# 
#    dataptr[8*3] = z13 - z2#
#    dataptr[8*1] = z11 + z4#
#    dataptr[8*7] = z11 - z4#

        movq %%mm3,%%mm5                                  #copy z13                               # Stage 6 

        psubw %%mm4,%%mm3                                 #y3=z13 - z2                    # Stage 6 



        paddw %%mm4,%%mm5                                 #y5=z13 + z2                    # Stage 6 

        movq %%mm0,%%mm6                                  #copy z11                               # Stage 6 



        movq %%mm3,2*8*3(%0)             #save y3                                # Stage 6 

        psubw %%mm2,%%mm0                                 #y7=z11 - z4                    # Stage 6 



        movq %%mm5,2*8*5(%0)             #save y5                                # Stage 6 

        paddw %%mm2,%%mm6                                 #y1=z11 + z4                    # Stage 6 



        movq %%mm0,2*8*7(%0)             #save y7                                # Stage 6 



# Pass 2: process rows. 

#  dataptr = data#
#  for (ctr = 8-1# ctr >= 0# ctr--) {
#    tmp0 = dataptr[8*0] + dataptr[8*7]#
#    tmp7 = dataptr[8*0] - dataptr[8*7]#
#    tmp1 = dataptr[8*1] + dataptr[8*6]#
#    tmp6 = dataptr[8*1] - dataptr[8*6]#
#    tmp2 = dataptr[8*2] + dataptr[8*5]#
#    tmp5 = dataptr[8*2] - dataptr[8*5]#
#    tmp3 = dataptr[8*3] + dataptr[8*4]#
#    tmp4 = dataptr[8*3] - dataptr[8*4]#

#Stage 1,even



        movq 8+2*8*1(%0),%%mm1     #load x1                # Stage 1 

        movq %%mm7,%%mm0                          #copy x0                                # Stage 1 



        movq %%mm6,2*8*1(%0)             #save y1                                # Stage 6 



        movq 8+2*8*2(%0),%%mm2     #load x2                # Stage 1 

        movq %%mm1,%%mm6                          #copy x1                                # Stage 1 



        paddw 8+2*8*7(%0),%%mm0    #tmp00=x0+x7    # Stage 1 



        movq 8+2*8*3(%0),%%mm3     #load x3                # Stage 1 

        movq %%mm2,%%mm5                          #copy x2                                # Stage 1 





        psubw 8+2*8*7(%0),%%mm7    #tmp07=x0-x7    # Stage 1 

        movq %%mm3,%%mm4                          #copy x3                                # Stage 1 



        paddw 8+2*8*6(%0),%%mm1    #tmp01=x1+x6    # Stage 1 



        movq %%mm7,%2                         #save tmp07                             # Stage 1 

        movq %%mm0,%%mm7                          #copy tmp00                                     # Even 2 



        psubw 8+2*8*6(%0),%%mm6    #tmp06=x1-x6    # Stage 1 



# Stage 2, Even part
#    tmp10 = tmp0 + tmp3#        
#    tmp13 = tmp0 - tmp3#
#    tmp11 = tmp1 + tmp2#
#    tmp12 = tmp1 - tmp2#

        paddw 8+2*8*4(%0),%%mm3    #tmp03=x3+x4    # Stage 1 



        movq %%mm6,%1                         #save tmp06             # Stage 1 

        movq %%mm1,%%mm6                          #copy tmp01                                     # Even 2 





        paddw 8+2*8*5(%0),%%mm2    #tmp02=x2+x5    # Stage 1 

        paddw %%mm3,%%mm0                         #tmp10 = tmp00 + tmp03#         # Even 2 



        psubw %%mm3,%%mm7                         #tmp13 = tmp00 - tmp03#         # Even 2 



        psubw 8+2*8*4(%0),%%mm4    #tmp04=x3-x4    # Stage 1 

    psubw %%mm2,%%mm6                             #tmp12 = tmp01 - tmp02#         # Even 2 



    paddw %%mm2,%%mm1                             #tmp11 = tmp01 + tmp02#         # Even 2      



        psubw 8+2*8*5(%0),%%mm5    #tmp05=x2-x5    # Stage 1 

        paddw %%mm7,%%mm6                         #tmp12+tmp13                    # Even 4 





# Stage 3, Even                and                                             Stage 4 & 5, Even
#    dataptr[8*0] =y0= tmp10 + tmp11#      z1 = MULTIPLY(tmp12 + tmp13, FIX_0_707106781)#  c4 
#    dataptr[8*4] =y4= tmp10 - tmp11#      dataptr[8*2] = tmp13 + z1#


        movq %1,%%mm2                         #load tmp6                                      # Odd 2 

        movq %%mm0,%%mm3                          #copy tmp10                     # Even 3 



    psllw $0x2, %%mm6

        paddw %%mm1,%%mm0                         #y0=tmp10 + tmp11       # Even 3 



    pmulhw C4,%%mm6                      #z1 = (tmp12+tmp13)*c4  # Even 4 

        psubw %%mm1,%%mm3                         #y4=tmp10 - tmp11       # Even 3 



    movq %%mm0,8+2*8*0(%0) #save y0                        # Even 3 

        movq %%mm7,%%mm0                          #copy tmp13                             # Even 4 



# Odd part 
# Stage 2 
#
#mm4=tmp4, mm5=tmp5, load mm2=tmp6, load mm3=tmp7
#    tmp10 = tmp4 + tmp5#
#    tmp11 = tmp5 + tmp6#
#    tmp12 = tmp6 + tmp7#

    movq %%mm3,8+2*8*4(%0) #save y4                        # Even 3 

        paddw %%mm5,%%mm4                         #tmp10 = tmp4 + tmp5#           # Odd 2 



        movq %2,%%mm3                         #load tmp7                                      # Odd 2 

        paddw %%mm6,%%mm0                         #tmp32 = tmp13 + z1             # Even 5 



        paddw %%mm2,%%mm5                         #tmp11 = tmp5 + tmp6#           # Odd 2 

        psubw %%mm6,%%mm7                         #tmp33 = tmp13 - z1             # Even 5 



        movq %%mm0,8+2*8*2(%0)     #save tmp32=y2          # Even 5 

        paddw %%mm3,%%mm2                         #tmp12 = tmp6 + tmp7#           # Odd 2 



#      Stage 4 
# The rotator is modified from fig 4-8 to avoid extra negations. 
#    z5 = MULTIPLY(tmp10 - tmp12, FIX_0_382683433)# // c6 
#    z2 = MULTIPLY(tmp10, FIX_0_541196100) + z5# // c2-c6 
#    z4 = MULTIPLY(tmp12, FIX_1_306562965) + z5# // c2+c6 
#    z3 = MULTIPLY(tmp11, FIX_0_707106781)# // c4 


        movq %%mm7,8+2*8*6(%0)     #save tmp32=y6          # Even 5 

        movq %%mm4,%%mm1                                  #copy of tmp10                          # Odd 4 



        psubw %%mm2,%%mm1                                 #tmp10-tmp12                            # Odd 4 

    psllw $0x2, %%mm4



        movq C2mC6,%%mm0                                 #load C2mC6                                     # Odd 4 

    psllw $0x2, %%mm1



    pmulhw C6,%%mm1                                      #z5=(tmp10-tmp12)*c6            # Odd 4 

    psllw $0x2, %%mm5



    pmulhw %%mm0,%%mm4                                    #tmp10*(c2-c6)                          # Odd 4 



# Stage 5 
#    z11 = tmp7 + z3#            
#    z13 = tmp7 - z3#

    pmulhw C4,%%mm5                                      #z3=tmp11*c4                            # Odd 4 

    psllw $0x2, %%mm2



    pmulhw C2pC6,%%mm2                                   #tmp12*(c2+c6)                          # Odd 4 

        movq %%mm3,%%mm0                                  #copy tmp7                              # Stage 5 



        movq 8+2*8*4(%0),%%mm7     # m03:m02|m01:m00 - first line (line 4)

        paddw %%mm1,%%mm4                                 #z2=tmp10*(c2-c6)+z5            # Odd 4 



        paddw %%mm5,%%mm0                                 #z11 = tmp7 + z3#               # Stage 5 

        psubw %%mm5,%%mm3                                 #z13 = tmp7 - z3#               # Stage 5 



# Stage 6 
#    dataptr[8*5] = z13 + z2# 
#    dataptr[8*3] = z13 - z2#
#    dataptr[8*1] = z11 + z4#
#    dataptr[8*7] = z11 - z4#


        movq %%mm3,%%mm5                                  #copy z13                               # Stage 6 

        paddw %%mm1,%%mm2                                 #z4=tmp12*(c2+c6)+z5            # Odd 4 



        movq %%mm0,%%mm6                                  #copy z11                               # Stage 6 

        psubw %%mm4,%%mm5                                 #y3=z13 - z2                    # Stage 6 



        paddw %%mm2,%%mm6                                 #y1=z11 + z4                    # Stage 6 

        paddw %%mm4,%%mm3                                 #y5=z13 + z2                    # Stage 6 



        movq %%mm5,8+2*8*3(%0)             #save y3                                # Stage 6 

        psubw %%mm2,%%mm0                                 #y7=z11 - z4                    # Stage 6 



        movq %%mm3,8+2*8*5(%0)             #save y5                                # Stage 6 



        movq %%mm6,8+2*8*1(%0)             #save y1                                # Stage 6 



        movq %%mm0,8+2*8*7(%0)             #save y7                                # Stage 6 


        emms"
  :
  : "r" (data), "o" (temp6), "o" (temp7)
  );

}


