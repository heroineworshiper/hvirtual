#include "veccprotos.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void block_error(block_t *block, char *offset, char *string)
{
	char string2[1024];
	char *ptr = block->source;
	int line = 1;

	while(offset && ptr < offset && ptr < block->end)
	{
		if(*ptr++ == 0xa) line++;
	}

	fflush(stdout);
	fprintf(stderr, "%s: %d: %s\n", block->path, line, string);
}


int str_to_block(char *string)
{
	int i;
	for(i = 1; i < sizeof(block_to_str) / sizeof(char*); i++)
	{
		if(!strcmp(string, block_to_str[i])) return i;
	}
	return BLOCK_NONE;
}


// Read file and return new block
block_t* block_read_file(char *path)
{
	FILE *file = fopen(path, "r");
	block_t *block;
	int size;

	if(!file)
	{
		fprintf(stderr, "%s: %s", path, strerror(errno));
		return 0;
	}

	block = calloc(1, sizeof(block_t));
	block->path = path;
	fseek(file, 0, SEEK_END);
	size = ftell(file);
	fseek(file, 0, SEEK_SET);
	block->source = malloc(size + 1);
	fread(block->source, size, 1, file);
	block->source[size] = 0;
	block->end = block->source + size + 1;
	block->offset = block->source;
	block->shared = 0;

	fclose(file);

	return block;
}

void block_delete(block_t *block)
{
	if(!block->shared)
		free(block->source);
	free(block);
}


// Return next character in block or 0 if end of block;
char block_getchar(block_t *block)
{
	if(block->offset < block->end) 
		return *block->offset++;
	else
		return 0;
}

// Return next character in block or 0 if end of block;
char block_showchar(block_t *block)
{
	if(block->offset < block->end) 
		return *block->offset;
	else
		return 0;
}

void block_ungetchar(block_t *block)
{
	block->offset--;
}

void block_dump(block_t *block)
{
	fwrite(block->offset, block->end - block->offset, 1, stdout);
	printf("\n");
}

// Find pointer to next occurance of string after offset and before end.
char* block_strstr(block_t *block, char *string)
{
	if(block->offset)
	{
		block->offset = strstr(block->offset, string);
		if(block->offset >= block->end) 
			block->offset = 0;
	}

	return block->offset;
}

// Find either of two strings and return 1 or 2 for the string it found.
// Return 0 if nothing was found.
int block_strstrstr(block_t *block, char *string1, char *string2)
{
	if(block->offset)
	{
		char *ptr1 = strstr(block->offset, string1);
		char *ptr2 = strstr(block->offset, string2);
		if(ptr1 >= block->end) ptr1 = 0;
		if(ptr2 >= block->end) ptr2 = 0; 

		if(ptr1 && ptr2)
		{
			if(ptr1 < ptr2)
			{
				block->offset = ptr1;
				return 1;
			}
			else
			{
				block->offset = ptr2;
				return 2;
			}
		}
		else
		if(ptr1)
		{
			block->offset = ptr1;
			return 1;
		}
		else
		if(ptr2)
		{
			block->offset = ptr2;
			return 2;
		}
	}

	block->offset = 0;

	return 0;
}

block_t* block_find_block(block_t *block, char *start_key, char *end_key)
{
	block_t *result = 0;
	char *ptr = 0;
	char string_result = 0;
	char *start = 0, *end = 0;
	int total_starts = 0;

//printf("block_find_block 1\n");
	string_result = block_strstrstr(block, start_key, end_key);
//printf("block_find_block 1\n");

// Got start key
	if(string_result == 1)
	{
		block->offset += strlen(start_key);
		start = block->offset;
		total_starts++;
//printf("block_find_block 1\n");

		while(total_starts > 0)
		{
			string_result = block_strstrstr(block, start_key, end_key);
//printf("block_find_block 2 %d\n", string_result);

			if(string_result == 1)
			{
				block->offset += strlen(start_key);
				total_starts++;
			}
			else
			if(string_result == 2)
			{
				total_starts--;
				if(total_starts > 0)
					block->offset += strlen(end_key);
			}
			else
			{
				char string[1024];
				sprintf(string, "Unmatched %s", start_key);
				block_error(block, block->offset, string);
				block->offset += strlen(start_key);
				break;
			}
		}
//printf("block_find_block 3\n");

		if(total_starts == 0)
		{
			end = block->offset;
			result = calloc(1, sizeof(block_t));
			result->path = block->path;
			result->source = block->source;
			result->offset = start;
			result->end = end;
			result->shared = 1;
		}
//printf("block_find_block 1\n");
	}
	else
// Got end key
	if(string_result == 2)
	{
		char string[1024];
		sprintf(string, "Unmatched %s", end_key);
		block_error(block, block->offset, string);
	}
//printf("block_find_block 2\n");

	return result;
}

int block_is_whitespace(char c)
{
	return (c == ' ' || 
			c == 0xa || 
			c == 0xd ||
			c == 0x9);
}

int block_is_eol(char c)
{
	return (c == 0xa) || (c == 0xd);
}

// Advance offset to next relevant character.
void block_skip_whitespace(block_t *block)
{
	int done = 0;

//printf("block_skip_whitespace ");
	while(!done)
	{
		done = 1;

//if(block->offset < block->end) printf("%c", *block->offset);
// Comment begins here.
		if(block->offset < block->end &&
			*block->offset == '/' && 
			block->offset + 1 < block->end && *(block->offset + 1) == '/')
		{
			while(block->offset < block->end && 
				!block_is_eol(*block->offset))
				block->offset++;

			done = 0;
		}
		else
// Space, line feed, or carriage return.  Skip it.
		if(block->offset < block->end &&
			block_is_whitespace(*block->offset))
		{
			block->offset++;
			done = 0;
		}
	}
//printf("\n");
}

// Copy next text token terminated by non alphanumeric character or end of block
void block_next_token_text(char *string, block_t *block)
{
	char *ptr = string;

	block_skip_whitespace(block);

	while(1)
	{
		*ptr = block_getchar(block);

// End of text
		if(!*ptr)
		{
			break;
		}

		if(!isalnum(*ptr) && *ptr != '_')
		{
			block_ungetchar(block);
			*ptr = 0;
			break;
		}

		ptr++;
	}
}

// Copy next text token terminated by space or end of block
void block_next_token(char *string, block_t *block)
{
	char *ptr = string;

	block_skip_whitespace(block);

	while(1)
	{
		*ptr = block_getchar(block);

// End of text
		if(!*ptr)
		{
			break;
		}

		if(*ptr == ' ')
		{
			block_ungetchar(block);
			*ptr = 0;
			break;
		}

		ptr++;
	}
}






// Test current offset against every table to see if it is a keyword.
// Only advance offset if it is a keyword.
// Return the keyword enum or 0 if none was found.
int block_get_keyword(block_t *block)
{
	char string[1024];
	char *ptr1 = string;
	char *ptr2 = block->offset;
	int keyword = 0;
	int operator = 0;

// Retrieve the next complete word
	while(ptr2 < block->end && (isalnum(*ptr2) || *ptr2 == '_'))
		*ptr1++ = *ptr2++;

	*ptr1 = 0;

// Compare to data types
//printf("block_get_keyword %s\n", string);
	if((keyword = str_to_datatype(string)) != TYPE_INVALID)
	{
		block->offset += strlen(type_to_str[keyword]);
		return keyword;
	}

	return 0;
}

// Compare to operators
int block_get_operator(block_t *block)
{
	int operator = OP_INVALID;
	if((operator = str_to_operator(block->offset)) !=
		OP_INVALID)
	{
		block->offset += strlen(operator_to_str[operator]);
		return operator;
	}
	return operator;
}







// Decode next command
command_t* block_next_command(block_t *block)
{
	command_t *result = 0;
	char string[1024];

	block_skip_whitespace(block);


// Got beginning of a command
	if(block_showchar(block) == '_' ||
		(block_showchar(block) >= 'A' && block_showchar(block) <= 'z'))
	{
		char *begin = block->offset;

// Read entire command
		while(block->offset < block->end && *block->offset != ';')
			block->offset++;

		if(block->offset >= block->end)
		{
			block_error(block, begin, "No semicolon found after command.");
		}
		else
		{
			block_t *command_block;
			command_t *command[3];
			char *ptr;
			int keyword = 0;
			int operator = 0;
			int i;

printf("block_next_command ");
			command_block = calloc(1, sizeof(block_t));
			command_block->source = block->source;
			command_block->offset = begin;
			command_block->end = block->offset;
			command_block->shared = 1;
// Skip semicolon
			block->offset++;

			for(i = 0; i < 3; i++)
			{
				command[i] = command_new();
				block_skip_whitespace(command_block);

				if((keyword = block_get_keyword(command_block)) != 0)
				{
					command[i]->keyword = keyword;
printf("keyword=%d ", keyword);
				}
				else
				if((operator = block_get_operator(command_block)) != 0)
				{
					command[i]->operator = operator;
printf("operator=%d ", operator);
				}
				else
				{
// Scan until space, operator or end of block
					ptr = string;
					while(1)
					{
						if(command_block->offset >= command_block->end ||
							str_to_operator(command_block->offset) != 
									OP_INVALID)
						{
							*ptr = 0;
							break;
						}
						else
						{
							*ptr++ = *command_block->offset++;
						}
					}

					if(strlen(string))
					{
						ptr = string + strlen(string) - 1;
						while(block_is_whitespace(*ptr) && ptr > string)
							*ptr-- = 0;
						command_set_name(command[i], string);
printf("name=%s ", string);
					}
				}

				command[i]->offset = command_block->offset;
			}

			result = command[1];
			result->target = command[0];
			result->argument = command[2];

printf("\n");
		}
	}

	return result;
}






