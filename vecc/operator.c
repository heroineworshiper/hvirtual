#include "veccprotos.h"
#include <stdlib.h>


// Operators are not delimited by spaces so they need a string len
int str_to_operator(char *string)
{
	int i;

	for(i = 1; i < sizeof(operator_to_str) / sizeof(char*); i++)
	{
		if(!strcmp(operator_to_str[i], string)) return i;
	}


	return OP_INVALID;
}
