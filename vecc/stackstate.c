#include "veccprotos.h"

#include <stdlib.h>



stackstate_t* new_stackstate()
{
	return calloc(1, sizeof(stackstate_t));
}

void stackstate_delete(stackstate_t *s)
{
	free(s);
}
