#include "veccprotos.h"

#include <stdlib.h>

command_t* command_new()
{
	command_t *result = calloc(1, sizeof(command_t));
	return result;
}

void command_delete(command_t *command)
{
	if(command->name) free(command->name);
	if(command->target) command_delete(command->target);
	if(command->argument) command_delete(command->argument);
	free(command);
}


void command_set_name(command_t *command, char *name)
{
	if(command->name) free(command->name);
	command->name = malloc(strlen(name) + 1);
	strcpy(command->name, name);
}

