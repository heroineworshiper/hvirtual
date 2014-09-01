#ifndef VECCPRIVATE_H
#define VECCPRIVATE_H



#define VECC_BEGIN "VECC_BEGIN"
#define VECC_END "VECC_END"


// Data types
enum
{
	TYPE_INVALID,
	TYPE_UINT_1X64,
	TYPE_UINT_2X32,
	TYPE_UINT_4X16,
	TYPE_UINT_8X8,
	TYPE_INT_1X64,
	TYPE_INT_2X32,
	TYPE_INT_4X16,
	TYPE_INT_8X8,
	TYPE_INT_1X8,
	TYPE_INT_1X8P,
	TYPE_INT_1X16,
	TYPE_INT_1X16P,
	TYPE_INT_1X32,
	TYPE_INT_1X32P,
	TYPE_FLOAT_1X32,
	TYPE_FLOAT_1X32P,
	TYPE_FLOAT_4X32,
	TYPE_FLOAT_4X32P
};

// Operators
enum
{
	OP_INVALID,
	OP_EQUALS,
	OP_ADD,
	OP_ADDLIMIT,
	OP_AND,
	OP_ANDNOT,
	OP_EQUAL,
	OP_GREATER,
	OP_MULTIPLYADD,
	OP_MULTIPLYHIGH,
	OP_MULTIPLYLOW,
	OP_OR,
	OP_SHIFTLEFT,
	OP_SHIFTRIGHT,
	OP_SUBTRACT,
	OP_SUBTRACTLIMIT,
	OP_XOR,
	OP_RECIPROCAL,
	OP_MIN,
	OP_MAX
};

// Blocks
enum
{
	BLOCK_NONE,
	BLOCK_CODE_BEGIN,
	BLOCK_CODE_END,
	BLOCK_MATH_BEGIN,
	BLOCK_MATH_END,
	BLOCK_VECTOR_BEGIN,
	BLOCK_VECTOR_END
};

enum
{
	PUNCT_NONE,
	PUNCT_COMMA,
	PUNCT_SEMI
};

// Parser types
enum
{
	PARSE_NOTHING,
	PARSE_NAME,
	PARSE_TYPE,
	PARSE_KEYWORD,
	PARSE_OPERATOR,
	PARSE_PUNCTUATION,
	PARSE_CODEBLOCK,   // { }
	PARSE_MATHBLOCK,   // ( )
	PARSE_VECTBLOCK,   // [ ]
	PARSE_BRANCH
};

// Language targets
enum
{
	TARGET_C,
	TARGET_MMX,
	TARGET_SSE
};



// Tables should line up with enums
extern char *type_to_str[19];

extern char *operator_to_str[21];

extern char *block_to_str[7];

extern char *punct_to_str[3];






typedef struct
{
	int type;
	char *name;
} datatype_t;

typedef struct
{
	char name[1024];
	datatype_t **arguments;
	int total_args;
} prototype_t;

typedef struct
{
// Pointer to data type currently held
	datatype_t *datatype;
// Hardware name of register
	char *name;
} regstate_t;

typedef struct
{
// Pointer to data type currently held
	datatype_t *datatype;
} stackstate_t;


typedef struct
{
	datatype_t **arguments;
	int total_arguments;
	datatype_t **locals;
	int total_locals;
	regstate_t **regstates;
	int total_regstates;
	stackstate_t **stackstates;
	int total_stackstates;
} function_t;

typedef struct
{
// Only one of these
	char *name;
	void *branch;
	int keyword;

// and all of these need to be defined
	int line;
	int type;
} parse_unit_t;

typedef struct
{
	parse_unit_t **units;
	int size;
	int allocated;
	int line;

	void *parent;
} parse_tree_t;

typedef struct
{
	char *path;
	char *source;
	char *offset;
	char *end;
	int shared;
} block_t;

typedef struct
{
	void *target;
	void *argument;

// The function is either a keyword or argument
	char *name;
	int keyword;
	int operator;
// Offset in function block
	char *offset;
} command_t;









#endif
