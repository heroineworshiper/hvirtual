#include "veccprotos.h"


int str_to_punctuation(char *text)
{
	int i;
	for(i = 1; i < sizeof(punct_to_str) / sizeof(char*); i++)
	{
		if(!strcmp(text, punct_to_str[i])) return i;
	}
	return PUNCT_NONE;
}

