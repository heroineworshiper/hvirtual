#include "veccprotos.h"

#include <stdio.h>
#include <stdlib.h>

parse_tree_t* parse_new()
{
	parse_tree_t *result = calloc(1, sizeof(parse_tree_t));
	result->allocated = 1;
	result->line = 1;
	result->units = calloc(result->allocated, sizeof(parse_unit_t*));
	return result;
}

void parse_delete(parse_tree_t *p)
{
	if(p->units)
	{
		int i;
		for(i = 0; i < p->size; i++)
			parse_delete_unit(p->units[i]);
		free(p->units);
	}

	free(p);
}

parse_unit_t* parse_new_unit(parse_tree_t *p)
{
	parse_unit_t *result;
	if(p->size >= p->allocated)
	{
		int allocated = p->allocated * 2;
		parse_unit_t **units = calloc(allocated, sizeof(parse_unit_t*));
		memcpy(units, p->units, sizeof(parse_unit_t*) * p->size);
		free(p->units);
		p->units = units;
		p->allocated = allocated;
	}

	result = p->units[p->size] = calloc(1, sizeof(parse_unit_t));
	result->line = 1;
	p->size++;

	return result;
}

void parse_delete_unit(parse_unit_t *u)
{
	if(u->name) free(u->name);
	if(u->branch) parse_delete(u->branch);
}

void parse_set_name(parse_unit_t *p, char *name)
{
	if(p->name) free(p->name);
	p->name = malloc(strlen(name) + 1);
	strcpy(p->name, name);
//printf("parse_set_name %s\n", name);
	p->type = PARSE_NAME;
}




parse_tree_t* parse_new_branch(parse_tree_t *p, int line)
{
	parse_unit_t *u = parse_new_unit(p);
	u->branch = parse_new();
	u->type = PARSE_BRANCH;
	u->line = line;
	((parse_tree_t*)u->branch)->parent = p;
	return u->branch;
}


void parse_new_name(parse_tree_t *p, char *text, int line)
{
	parse_unit_t *u = parse_new_unit(p);
	parse_set_name(u, text);
	u->line = line;
}

void parse_new_type(parse_tree_t *p, char *text, int line)
{
	parse_unit_t *u = parse_new_unit(p);
	u->keyword = str_to_datatype(text);
	u->type = PARSE_TYPE;
	u->line = line;
}
void parse_new_keyword(parse_tree_t *p, char *text, int line)
{
}
void parse_new_operator(parse_tree_t *p, char *text, int line)
{
	parse_unit_t *u = parse_new_unit(p);
	u->keyword = str_to_operator(text);
	u->type = PARSE_OPERATOR;
	u->line = line;
}
void parse_new_punctuation(parse_tree_t *p, char *text, int line)
{
	parse_unit_t *u = parse_new_unit(p);
	u->keyword = str_to_punctuation(text);
	u->type = PARSE_PUNCTUATION;
	u->line = line;
}
void parse_new_codeblk(parse_tree_t *p, char *text, int line)
{
	parse_unit_t *u = parse_new_unit(p);
	u->keyword = str_to_block(text);
	u->type = PARSE_CODEBLOCK;
	u->line = line;
}
void parse_new_mathblk(parse_tree_t *p, char *text, int line)
{
	parse_unit_t *u = parse_new_unit(p);
	u->keyword = str_to_block(text);
	u->type = PARSE_MATHBLOCK;
	u->line = line;
}
void parse_new_vectblk(parse_tree_t *p, char *text, int line)
{
	parse_unit_t *u = parse_new_unit(p);
	u->keyword = str_to_block(text);
	u->type = PARSE_VECTBLOCK;
	u->line = line;
}

char* parse_to_text(parse_unit_t *u)
{
	switch(u->type)
	{
		case PARSE_NAME:
			return u->name;
			break;
		case PARSE_KEYWORD:
			break;
		case PARSE_TYPE:
			return type_to_str[u->keyword];
			break;
		case PARSE_PUNCTUATION:
			return punct_to_str[u->keyword];
			break;
		case PARSE_CODEBLOCK:
		case PARSE_MATHBLOCK:
		case PARSE_VECTBLOCK:
			return block_to_str[u->keyword];
			break;
	}
	return "";
}





static void print_indent(int indent)
{
	int i;
	for(i = 0; i < indent; i++)
		printf("    ");
}

static void parse_dump_branch(int indent, parse_tree_t *p)
{
	int i;
//printf("parse_dump_branch %p %p %p\n", p, p->units, p->units[0]);
	for(i = 0; i < p->size; i++)
	{
		parse_unit_t *u = p->units[i];

		switch(u->type)
		{
			case PARSE_NAME:
				printf("name=%s ", u->name);
				break;
			case PARSE_KEYWORD:
				break;
			case PARSE_TYPE:
				printf("type=%s ", type_to_str[u->keyword]);
				break;
			case PARSE_PUNCTUATION:
				printf("punct=%s ", punct_to_str[u->keyword]);
				if(u->keyword == PUNCT_SEMI)
				{
					printf("\n");
					print_indent(indent);
				}
				break;
			case PARSE_CODEBLOCK:
			case PARSE_MATHBLOCK:
			case PARSE_VECTBLOCK:
				printf("block=%s ", block_to_str[u->keyword]);
				break;
			case PARSE_BRANCH:
				printf("\n");
				print_indent(indent + 1);
				printf("branch=");
				parse_dump_branch(indent + 1, u->branch);
				break;
		}
	}
}

void parse_dump(parse_tree_t *p)
{
printf("*** parse_dump\n");
	parse_dump_branch(0, p);
printf("\n");
}



void parse_error(char *path, int line, char *string)
{
	fprintf(stderr, "%s: %d: %s\n", path, line, string);
}
