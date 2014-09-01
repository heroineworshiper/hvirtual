#include "veccprotos.h"




// Tables should line up with enums in veccprivate.h
char *type_to_str[] = 
{
	"", 
	"uint_1x64_t",   // MMX
	"uint_2x32_t",
	"uint_4x16_t",
	"uint_8x8_t",
	"int_1x64_t",
	"int_2x32_t",
	"int_4x16_t",
	"int_8x8_t",
	"int_1x8_t",
	"int_1x8p_t",
	"int_1x16_t",
	"int_1x16p_t",
	"int_1x32_t",
	"int_1x32p_t",
	"float_1x32_t",   // SSE
	"float_1x32p_t",
	"float_4x32_t",
	"float_4x32p_t"
};

char *operator_to_str[] = 
{
	"",
	"=",        // Assign destination to source (MMX) (SSE)
	"+=",       // Add source to destination (MMX) (SSE)
	"limit+=",  // Add source to destination with limiting (MMX)
	"&=",       // And source to destination (MMX) (SSE)
	"not&=",    // Invert destination and AND source to it (MMX) (SSE)
	"==",       // Set destination to all 1 if equal to source (MMX)
	">",        // Set destination to all 1 if greater than source (MMX)
	"add*=",    // Multiply 4 words and add 2 high results and 2 low results into 2 doublewords. (MMX)
	"high*=",   // Multiply 4 words and return 4 high words of the results. (MMX)
	"low*=",    // Multiply 4 words and return 4 low words of the results. (MMX)
	"|=",       // Or (MMX)
	"<<=",      // Shift  (MMX)
	">>=",      // Shift (MMX)
	"-=",       // Subtract (MMX) (SSE)
	"limit-=",  // Subtract and limit (MMX)
	"^=",       // Xor (MMX)
	"1/=",      // Get reciprocal (SSE)
	"min=",     // Get minimum (MMX) (SSE)
	"max=",     // Get maximum (MMX) (SSE)
	""
};

char *block_to_str[] = 
{
	"",
	"{",
	"}",
	"(",
	")",
	"[",
	"]"
};

char *punct_to_str[] =
{
	"",
	",",
	";"
};
