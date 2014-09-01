#include <stdio.h>


int main()
{
	float input[4] = { 1, 2, 3, 4 };
	float output[4] = { 5, 6, 7, 8 };
	float opacity[4] = { .6, .6, .6, .6 };
	float transparency[4] = { .4, .4, .4, .4 };


#if 0

	input[0] *= opacity[0];
	input[1] *= opacity[1];
	input[2] *= opacity[2];
	input[3] *= opacity[3];
	output[0] *= transparency[0];
	output[1] *= transparency[1];
	output[2] *= transparency[2];
	output[3] *= transparency[3];
	output[0] += input[0];
	output[1] += input[1];
	output[2] += input[2];
	output[3] += input[3];

#else

	asm(
		"movups (%0), %%xmm0;\n"
		"movups (%1), %%xmm1;\n"
		"mulps  (%2), %%xmm0;\n"	  // input *= opacity
		"mulps  (%3), %%xmm1;\n"	  // output *= transparency
		"addps  %%xmm1, %%xmm0;\n"     // output += input
		"movups %%xmm0, (%1);\n"
	:
	: "r" (input), "r" (output), "r" (opacity), "r" (transparency));

#endif

	printf("%f %f %f %f\n", output[0], output[1], output[2], output[3]);




	return 0;
}
