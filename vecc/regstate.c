#include "veccprotos.h"

#include <stdlib.h>

regstate_t* new_regstate(datatype_t *d, char *name)
{
	regstate_t *result = calloc(1, sizeof(regstate_t));
	result->datatype = d;
	result->name = malloc(strlen(name) + 1);
	strcpy(result->name, name);
	return result;
}

void regstate_delete(regstate_t *r)
{
	free(r->name);
	free(r);
}
