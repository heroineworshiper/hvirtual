#include "veccprotos.h"
#include <stdlib.h>

prototype_t* prototype_new()
{
	return calloc(1, sizeof(prototype_t));
}

void prototype_delete(prototype_t *p)
{
	if(p->arguments)
	{
		int i;
		for(i = 0; i < p->total_args; i++)
			datatype_delete(p->arguments[i]);
		free(p->arguments);
	}
	free(p);
}

void prototype_add_argument(prototype_t *p, int type, char *name)
{
	datatype_t **arguments = malloc(sizeof(datatype_t*) * p->total_args + 1);
	arguments[p->total_args] = datatype_new(type, name);

	if(p->arguments)
	{
		memcpy(arguments, p->arguments, sizeof(datatype_t*) * p->total_args);
		free(p->arguments);
	}
	p->arguments = arguments;
	p->total_args++;
}

void prototype_dump(prototype_t *p)
{
	int i;
	printf("prototype_dump name=%s total_args=%d\n", p->name, p->total_args);
	for(i = 0; i < p->total_args; i++)
		datatype_dump(p->arguments[i]);
}

