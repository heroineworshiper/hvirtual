#include "../transferidct.h"





void EncTransferIDCT_add(uint8_t* pDest, int16_t* pSrc, int stride)


{


	__asm


	{


		mov ecx, pDest


		mov edx, pSrc


		mov eax, stride


		mov edi, 8


		pxor mm2, mm2


p1:


		movq mm0, [edx]


		movq mm1, [edx+8]


		movq mm3, [ecx]


		movq mm4, mm3


		punpcklbw mm3, mm2


		punpckhbw mm4, mm2


		paddsw mm0, mm3


		paddsw mm1, mm4


		packuswb mm0, mm1


		movq [ecx], mm0


		add edx, 16


		add ecx, eax


		dec edi


		jnz p1


		emms


    }


}





void EncTransferIDCT_copy(uint8_t* pDest, int16_t* pSrc, int stride)


{


	_asm {





	; not sure about the state handling here - there must be a better way


	push eax


	push ebx


	push edi





	mov eax, pSrc           ;  parameter 1, *sourceS16


	mov ebx, pDest              ;  parameter 2, *destU8


	mov edi, stride              ;  parameter 3, stride





; lines 0 to 7 schedueled into each other...


	movq mm0, qword ptr [eax]       ;  move first four words into mm0





	packuswb mm0, qword ptr [eax+8] ;  pack mm0 and the next four words into mm0





	movq mm1, qword ptr [eax+16]    ;  move first four words into mm1





	packuswb mm1, qword ptr [eax+24];  pack mm0 and the next four words into mm1





	movq mm2, qword ptr [eax+32]    ;  move first four words into mm2





	packuswb mm2, qword ptr [eax+40];  pack mm0 and the next four words into mm2





	movq mm3, qword ptr [eax+48]    ;  move first four words into mm3





	packuswb mm3, qword ptr [eax+56] ;  pack mm3 and the next four words into mm3





	movq qword ptr [ebx], mm0       ;  copy output to destination


	add ebx, edi                    ;  add +stride to dest ptr





	movq qword ptr [ebx], mm1       ;  copy output to destination


	add ebx, edi                    ;  add +stride to dest ptr





	movq qword ptr [ebx], mm2       ;  copy output to destination


	add ebx, edi                    ;  add +stride to dest ptr





	movq qword ptr [ebx], mm3       ;  copy output to destination


	add ebx, edi                    ;  add +stride to dest ptr


	


	movq mm0, qword ptr [eax+64]    ;  move first four words into mm0


	add eax, 64                     ;  add 64 to source ptr                





	packuswb mm0, qword ptr [eax+8] ;  pack mm0 and the next four words into mm0





	movq mm1, qword ptr [eax+16]    ;  move first four words into mm1





	packuswb mm1, qword ptr [eax+24];  pack mm0 and the next four words into mm1





	movq mm2, qword ptr [eax+32]    ;  move first four words into mm2





	packuswb mm2, qword ptr [eax+40];  pack mm0 and the next four words into mm2





	movq mm3, qword ptr [eax+48]    ;  move first four words into mm3





	packuswb mm3, qword ptr [eax+56];  pack mm3 and the next four words into mm3





	movq qword ptr [ebx], mm0       ;  copy output to destination


	add ebx, edi                    ;  add +stride to dest ptr





	movq qword ptr [ebx], mm1       ;  copy output to destination


	add ebx, edi                    ;  add +stride to dest ptr





	movq qword ptr [ebx], mm2       ;  copy output to destination


	add ebx, edi                    ;  add +stride to dest ptr





	movq qword ptr [ebx], mm3       ;  copy output to destination





	pop edi


	pop ebx 


	pop eax





	emms





	}


}





void EncTransferFDCT_sub(const uint8_t* pSrc, uint8_t* pSrc2,  


    int16_t* pDest, int stride)


{


	__asm


	{


		mov edx, pSrc2


	 	mov	ecx, pDest


		mov eax, stride


		mov esi, pSrc


		mov edi, 8


		pxor mm1, mm1


p2:


		movq mm0, [edx]


		movq mm2, mm0


		punpcklbw mm2, mm1


		punpckhbw mm0, mm1


		movq mm3, [esi]


		movq mm4, mm3


		movq mm5, mm3


		punpcklbw mm4, mm1


		punpckhbw mm3, mm1





		psubsw mm0, mm3


		psubsw mm2, mm4





		movq [ecx], mm2


		movq [ecx+8], mm0


		movq [edx], mm5





		add ecx, 16


		add edx, eax


		add esi, eax


		dec edi


		jnz p2


		emms


	}


}





void EncTransferFDCT_copy(uint8_t* pSrc, int16_t* pDest, int stride)


{


    __asm


	{


		mov ecx, pDest


		mov edx, pSrc


		mov eax, stride


		mov edi, 8


		pxor mm1, mm1


p3:


		movq mm0, [edx]


		movq mm2, mm0


		punpcklbw mm2, mm1


		movq [ecx], mm2


		punpckhbw mm0, mm1


		movq [ecx+8], mm0


		add ecx, 16


		add edx, eax


		dec edi


		jnz p3


		emms


	}


}


