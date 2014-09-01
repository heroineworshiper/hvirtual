OBJS = \
	block.o \
	command.o \
	datatype.o \
	function.o \
	operator.o \
	output_c.o \
	output_mmx.o \
	output_sse.o \
	parse.o \
	prototype.o \
	punctuation.o \
	regstate.o \
	stackstate.o \
	tables.o \
	vecc.o

LEX_OBJS = \
	lexicon.o

all: $(OBJS) $(LEX_OBJS)
	gcc -o vecc $(OBJS) $(LEX_OBJS) -lfl

$(LEX_OBJS):
	flex $*.f
	gcc -c lex.yy.c -o $*.o

$(OBJS):
	gcc -c $*.c -o $*.o

clean:
	rm -f *.o




block.o: block.c
command.o: command.c
datatype.o: datatype.c
function.o: function.c
lexicon.o: lexicon.f
operator.o: operator.c
output_c.o: output_c.c
output_mmx.o: output_mmx.c
output_sse.o: output_sse.c
prototype.o: prototype.c
punctuation.o: punctuation.c
parse.o: parse.c
regstate.o: regstate.c
stackstate.o: stackstate.c
tables.o: tables.c
vecc.o: vecc.c



