#include "veccprotos.h"

#include <stdlib.h>



function_t* function_new()
{
	return calloc(1, sizeof(function_t));
}

void function_delete(function_t *f)
{
	if(f->arguments)
	{
		int i;
		for(i = 0; i < f->total_arguments; i++)
		{
			datatype_delete(f->arguments[i]);
		}
		free(f->arguments);
	}

	if(f->locals)
	{
		int i;
		for(i = 0; i < f->total_locals; i++)
			datatype_delete(f->locals[i]);
		free(f->locals);
	}

	if(f->regstates)
	{
		int i;
		for(i = 0; i < f->total_regstates; i++)
			regstate_delete(f->regstates[i]);
		free(f->regstates);
	}

	if(f->stackstates)
	{
		int i;
		for(i = 0; i < f->total_stackstates; i++)
			stackstate_delete(f->stackstates[i]);
		free(f->stackstates);
	}

	free(f);
}

void function_new_argument(function_t *f,
	int type,
	char *name)
{
	datatype_t **arguments = malloc((f->total_arguments + 1) * sizeof(datatype_t*));
	arguments[f->total_arguments] = datatype_new(type, name);

	if(f->arguments)
	{
		memcpy(arguments, f->arguments, f->total_arguments * sizeof(datatype_t*));
		free(f->arguments);
	}
	f->arguments = arguments;
	f->total_arguments++;
}

// Create new table entry or change type of existing entry
void function_new_local(function_t *f,
	int type,
	char *name)
{
	int i;
	for(i = 0; i < f->total_locals; i++)
	{
		if(!strcmp(f->locals[i]->name, name))
		{
			f->locals[i]->type = type;
			return;
		}
	}

	datatype_t **locals = malloc(
}
















int function_translate(int target,
	char *path, 
	parse_unit_t *name,
	parse_tree_t *arguments,
	parse_tree_t *code)
{
// Get function prototype
	function_t *function = function_new();
	int result = 0;
	int i;


// Load arguments
	for(i = 0; i < arguments->size && !result; i++)
	{
		if(arguments->units[i]->type == PARSE_MATHBLOCK)
		{
			;
		}
		else
		if(arguments->units[i]->type == PARSE_PUNCTUATION &&
			arguments->units[i]->keyword == PUNCT_COMMA)
		{
			;
		}
		else
		if(arguments->units[i]->type == PARSE_TYPE)
		{
			i++;
			if(i < arguments->size && 
				arguments->units[i]->type == PARSE_NAME)
			{
				function_new_argument(function,
					arguments->units[i - 1]->keyword,
					arguments->units[i]->name);
			}
			else
			{
				CLAMP(i, 0, arguments->size - 1);
				parse_error(path, 
					arguments->units[i]->line,
					"Expected an identifier");
				result = 1;
			}
		}
		else
		{
			char string[1024];
			sprintf(string, "Parse error at %s", parse_to_text(arguments->units[i]));
			parse_error(path, 
				arguments->units[i]->line,
				string);
			result = 1;
		}
	}



// Export C function prototype
	if(!result)
	{
		printf("void %s(", name->name);

		for(i = 0; i < function->total_arguments; i++)
		{
			datatype_t *argument = function->arguments[i];
			switch(argument->type)
			{
				case TYPE_INT_1X32:
					printf("int32_t %s", argument->name);
					break;
				case TYPE_INT_1X32P:
					printf("int32_t* %s", argument->name);
					break;
			}
			if(i < function->total_arguments - 1)
				printf(", ");
			else
				printf(")\n{\n");
		}




// Export code in desired language
		switch(target)
		{
			case TARGET_C:
				break;
			case TARGET_MMX:
				break;
			case TARGET_SSE:
				result = output_sse(path, function, code);
				break;
		}







// Terminate C function prototype
		printf("}\n");
	}





	function_delete(function);
	return result;
}


