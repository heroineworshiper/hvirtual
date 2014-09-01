#include "veccprotos.h"



// This could be recursive.
static int process_token(char *path, 
	function_t *function, 
	parse_tree_t *code, 
	int *offset)
{
	while(1)
	{
		parse_unit_t *unit;
		if((*offset) >= code->size) break;

		unit = code->units[(*offset)];
		(*offset)++;

		switch(unit->type)
		{
// Declare new identifier
			case PARSE_TYPE:
				if((*offset) < code->size && 
					code->units[(*offset)]->type == PARSE_NAME)
				{
					parse_unit_t *unit2 = code->units[(*offset)];
					(*offset)++;
					if(*offset < code->size && 
						code->units[(*offset)]->type == PARSE_PUNCTUATION &&
						code->units[(*offset)]->keyword == PUNCT_SEMI)
					{
						(*offset)++;
						function_new_local(function, 
							unit->keyword, 
							unit2->name);
					}
					else
					{
						parse_error(path, unit->line, "Expected a ;");
					}
				}
				else
				{
					parse_error(path, unit->line, "Expected an identifier");
					return 1;
				}
				break;
			case PARSE_NAME:
				
				break;
			case PARSE_PUNCTUATION:
				if(unit->keyword == PUNCT_SEMI)
					return 0;
				else
				{
					char string[1024];
					sprintf(string, "Parse error at %s", parse_to_text(unit));
					parse_error(path, unit->line, string);
				}
				break;
			case PARSE_BRANCH:
				process_token(path, function, code, offset);
				break;
		}

	}
	return 0;
}


int output_sse(char *path, function_t *function, parse_tree_t *code)
{
	int result = 0;
	int i;

// Generate inline block
	printf("\tasm(\n");



// Generate instructions
	for(i = 0; i < code->size ; i++)
	{
		process_token(path, function, code, &i);
	}








// Finish inline block
	printf("\t:\n\t: ");
	for(i = 0; i < function->total_arguments; i++)
	{
		printf("\"r\" %s", function->arguments[i]->name);
		if(i < function->total_arguments - 1)
			printf(", ");
		else
			printf(");\n");
	}



	return result;
}






