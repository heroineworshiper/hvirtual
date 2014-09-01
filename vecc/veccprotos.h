#ifndef VECCPROTOS_H
#define VECCPROTOS_H

#include "veccprivate.h"





#define CLAMP(x, y, z) ((x) = ((x) < (y) ? (y) : ((x) > (z) ? (z) : (x))))

parse_tree_t* parse_new();
void parse_delete(parse_tree_t *p);
parse_unit_t* parse_new_unit(parse_tree_t *p);
void parse_delete_unit(parse_unit_t *u);
void parse_set_name(parse_unit_t *p, char *name);
parse_tree_t* parse_new_branch(parse_tree_t *p, int line);
void parse_new_name(parse_tree_t *p, char *text, int line);
void parse_new_type(parse_tree_t *p, char *text, int line);
void parse_new_keyword(parse_tree_t *p, char *text, int line);
void parse_new_operator(parse_tree_t *p, char *text, int line);
void parse_new_punctuation(parse_tree_t *p, char *text, int line);
void parse_new_codeblk(parse_tree_t *p, char *text, int line);
void parse_new_mathblk(parse_tree_t *p, char *text, int line);
void parse_new_vectblk(parse_tree_t *p, char *text, int line);
void parse_dump(parse_tree_t *p);
void parse_error(char *path, int line, char *string);
char* parse_to_text(parse_unit_t *u);

parse_tree_t* vecc_parse(char *in_path);



int str_to_datatype(char *string);
datatype_t* datatype_new(int type, char *name);
void datatype_delete(datatype_t *d);
void datatype_dump(datatype_t *d);

prototype_t* prototype_new();
void prototype_delete(prototype_t *p);
void prototype_add_argument(prototype_t *p, int type, char *name);
void prototype_dump(prototype_t *p);

block_t* block_read_file(char *path);
void block_delete(block_t *block);
char block_getchar(block_t *block);
char block_showchar(block_t *block);
void block_error(block_t *block, char *offset, char *string);
void block_ungetchar(block_t *block);
void block_dump(block_t *block);
char* block_strstr(block_t *block, char *string);
int block_strstrstr(block_t *block, char *string1, char *string2);
block_t* block_find_block(block_t *block, char *start_key, char *end_key);
void block_skip_whitespace(block_t *block);
void block_next_token_text(char *string, block_t *block);
void block_next_token(char *string, block_t *block);
command_t* block_next_command(block_t *block);
int block_get_keyword(block_t *block);
int block_is_whitespace(char c);
int block_is_eol(char c);
int str_to_block(char *string);

int str_to_punctuation(char *text);

function_t* function_new();
void function_delete(function_t *f);
int function_translate(int target,
	char *path, 
	parse_unit_t *name,
	parse_tree_t *arguments,
	parse_tree_t *code);
void function_new_argument(function_t *f,
	int type,
	char *name);

int output_c(char *path, function_t *function, parse_tree_t *code);
int output_mmx(char *path, function_t *function, parse_tree_t *code);
int output_sse(char *path, function_t *function, parse_tree_t *code);

regstate_t* new_regstate(datatype_t *d, char *name);
void regstate_delete(regstate_t *r);

stackstate_t* new_stackstate();
void stackstate_delete(stackstate_t *s);

command_t* command_new();
void command_delete(command_t *command);
void command_set_name(command_t *command, char *name);

int str_to_operator(char *string);

#endif
