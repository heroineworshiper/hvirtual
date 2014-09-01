#include "veccprotos.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



// Generate C and vector math assembly from the same
// source so it only has to be debugged once, not necessarily by someone
// with the target processor.

// Make assembly slightly easier to read.

// Enforce data types in assembly language.

// Allow operators to change the types of their arguments and
// arguments to change their own type permanently.












int main(int argc, char *argv[])
{
	int result = 0;
	char *path = 0;
	int target = TARGET_C;
	int i;
	char temp_path[1024];

	if(argc < 2)
	{
		fprintf(stderr, 

"Vectored compiler collection.  (C) 2002 Heroine Virtual Ltd.\n"
"No warranties.  Not even the guarantee of merchantability, support, or \n"
"fitness for a particular purpose.\n"
"Usage: vecc -mmx -sse -c <filename>\n"

);
		exit(1);
	}


	for(i = 1; i < argc && !result; i++)
	{
		if(!strcmp(argv[i], "-mmx"))
		{
			target = TARGET_MMX;
		}
		else
		if(!strcmp(argv[i], "-sse"))
		{
			target = TARGET_SSE;
		}
		else
		if(!strcmp(argv[i], "-c"))
		{
			target = TARGET_C;
		}
		else
			path = argv[i];
	}

	if(!path)
	{
		fprintf(stderr, "No path specified.\n");
		exit(1);
	}
	else
	{
		parse_tree_t *parse;

// Search for assembly blocks and convert to all languages.
// The resulting C code contains a switch statement and #ifdefs for every
// architecture supported.
		if(parse = vecc_parse(path))
		{
// Generate C prototype
			for(i = 0; i < parse->size && !result; i++)
			{
// Verify function prototype
				if(parse->units[i]->type == PARSE_NAME)
				{
					i++;
					if(i < parse->size &&
						parse->units[i]->type == PARSE_BRANCH)
					{
						i++;
						if(i < parse->size &&
							parse->units[i]->type == PARSE_BRANCH)
						{
							result = function_translate(target,
								path, 
								parse->units[i - 2], 
								parse->units[i - 1]->branch,
								parse->units[i]->branch);
						}
						else
						{
							CLAMP(i, 0, parse->size - 1);
							parse_error(path, parse->units[i]->line, "Expected a { } block.");
							result = 1;
						}
					}
					else
					{
						CLAMP(i, 0, parse->size - 1);
						parse_error(path, parse->units[i]->line, "Expected an argument list.");
						result = 1;
					}
				}
				else
				{
					parse_error(path, parse->units[i]->line, "Expected a function name.");
					result = 1;
				}
			}
		}
		else
			result = 1;
	}
	













}





