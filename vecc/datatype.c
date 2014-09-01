#include "veccprotos.h"
#include <stdlib.h>

int str_to_datatype(char *string)
{
	int i;
	for(i = 1; i < sizeof(type_to_str) / sizeof(char*); i++)
	{
		if(!strcmp(string, type_to_str[i])) return i;
	}
	return TYPE_INVALID;
}


datatype_t* datatype_new(int type, char *name)
{
	datatype_t *result = calloc(1, sizeof(datatype_t));
	result->type = type;
	result->name = malloc(strlen(name) + 1);
	strcpy(result->name, name);
	return result;
}

void datatype_delete(datatype_t *d)
{
	if(d->name) free(d->name);
	free(d);
}

void datatype_dump(datatype_t *d)
{
	printf("datatype_dump type=%d name=%s\n", d->type, d->name);
}
