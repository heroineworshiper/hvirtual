	
%{

#include "veccprotos.h"
#include <errno.h>
#include <string.h>

%}

%option yylineno

	static int active = 0;
	static int result = 0;
	static parse_tree_t *parse_tree = 0;
	static parse_tree_t *current_branch = 0;
	static char *path;

%%

VECC_BEGIN          active = 1;

VECC_END            active = 0;

"//"                {
						if(active)
						{
							int c;
							while(1)
							{
								c = input();
								if(c == '\n') break;
								if(c == EOF) break;
							}
						}
						else
							printf("%s", yytext);
					}

"{"                 {
						if(active)
						{
							current_branch = parse_new_branch(current_branch, yylineno);
							parse_new_codeblk(current_branch, yytext, yylineno);
						}
						else
							printf("%s", yytext);
					}

"}"                 {
						if(active)
						{
							parse_new_codeblk(current_branch, yytext, yylineno);
							if(current_branch->parent)
							{
								current_branch = current_branch->parent;
							}
							else
							{
								fprintf(stderr, "%s: %d: Unmatched {", path, current_branch->line);
								result = 1;
							}
						}
						else
							printf("%s", yytext);
					}

"("                 {
						if(active)
						{
							current_branch = parse_new_branch(current_branch, yylineno);
							parse_new_mathblk(current_branch, yytext, yylineno);
						}
						else
							printf("%s", yytext);
					}

")"                 {
						if(active)
						{
							parse_new_mathblk(current_branch, yytext, yylineno);
							if(current_branch->parent)
							{
								current_branch = current_branch->parent;
							}
							else
							{
								fprintf(stderr, "%s: %d: Unmatched (", path, current_branch->line);
								result = 1;
							}
						}
						else
							printf("%s", yytext);
					}

"["                 {
						if(active)
						{
							current_branch = parse_new_branch(current_branch, yylineno);
							parse_new_vectblk(current_branch, yytext, yylineno);
						}
						else
							printf("%s", yytext);
					}

"]"                 {
						if(active)
						{
							parse_new_vectblk(current_branch, yytext, yylineno);
							if(current_branch->parent)
							{
								current_branch = current_branch->parent;
							}
							else
							{
								fprintf(stderr, "%s: %d: Unmatched (", path, current_branch->line);
								result = 1;
							}
						}
						else
							printf("%s", yytext);
					}

{DATATYPES} 		{
						if(active)
							parse_new_type(current_branch, yytext, yylineno);
						else
							printf("%s", yytext);
					}


{OPERATORS} 		{
						if(active)
							parse_new_operator(current_branch, yytext, yylineno);
						else
							printf("%s", yytext);
					}


","|";"             {
						if(active)
							parse_new_punctuation(current_branch, yytext, yylineno);
						else
							printf("%s", yytext);
					}

[A-Za-z][a-z0-9_]*	{
						if(active)
							parse_new_name(current_branch, yytext, yylineno);
						else
							printf("%s", yytext);
					}

\n					if(!active) printf("%s", yytext);

.					if(!active) printf("%s", yytext);

%%




parse_tree_t* vecc_parse(char *in_path)
{
	FILE *fd = fopen(in_path, "r");
	path = in_path;

	if(!fd)
	{
		fprintf(stderr, "vecc_parse: open %s: %s\n", path, strerror(errno));
		return 0;
	}

	current_branch = parse_tree = parse_new();
	yyin = fd;
	yylex();

	fclose(fd);
	
	if(!result)
		return parse_tree;
	else
	{
		parse_delete(parse_tree);
		return 0;
	}
}



