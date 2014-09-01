include ../../global_config

OBJS = \
	aes.o \
	af_vfs.o \
	aiff.o \
	aiffwrite.o \
	aupv.o \
	byteorder.o \
	compression.o \
	data.o \
	error.o \
	extended.o \
	format.o \
	g711.o \
	instrument.o \
	loop.o \
	marker.o \
	misc.o \
	modules.o \
	next.o \
	nextwrite.o \
	openclose.o \
	pcm.o \
	query.o \
	raw.o \
	setup.o \
	track.o \
	units.o \
	util.o \
	wave.o \
	wavewrite.o

OUTPUT = libasound.a
CC = gcc
CFLAGS += -I.

$(OUTPUT): $(OBJS)
	ar rcs $(OUTPUT) $(OBJS)

clean:
	find \( -name core \
		-o -name '*.o' \
		-o -name '*.a' \
		-o -name '*.so' \) -exec rm -f {} \;

.c.o:
	$(CC) -c $(CFLAGS) $*.c
