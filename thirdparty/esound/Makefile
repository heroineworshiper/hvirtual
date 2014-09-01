include ../../global_config

$(shell sh -c 'if ! test -d $(OBJDIR)\; then mkdir $(OBJDIR)\; fi' )

OBJS = \
	$(OBJDIR)/audio.o \
	$(OBJDIR)/clients.o \
	$(OBJDIR)/esd_config.o \
	$(OBJDIR)/esdfile.o \
	$(OBJDIR)/esdlib.o \
	$(OBJDIR)/esdmgr.o \
	$(OBJDIR)/genrand.o \
	$(OBJDIR)/filter.o \
	$(OBJDIR)/mix.o \
	$(OBJDIR)/players.o \
	$(OBJDIR)/proto.o \
	$(OBJDIR)/samples.o



OUTPUT = $(OBJDIR)/libesound.a
SHARED_OUTPUT = $(OBJDIR)/libesound.so
CC = gcc
CFLAGS += -I. -I../audiofile -DSYSCONFDIR=\"/usr/local/etc\"

all: $(OUTPUT)

$(OUTPUT): $(OBJS)
	ar rc $(OUTPUT) $(OBJS)

$(SHARED_OUTPUT): $(OBJS)
	gcc -shared -o $(SHARED_OUTPUT) $(OBJS)

clean:
	rm -rf $(OBJDIR)

$(OBJS):
	$(GCC) -c `cat $(OBJDIR)/c_flags` $(subst $(OBJDIR)/,, $*.c) -o $*.o

$(OBJDIR)/audio.o:	 audio.c
$(OBJDIR)/clients.o:	 clients.c
$(OBJDIR)/esd_config.o:  esd_config.c
$(OBJDIR)/esdfile.o:	 esdfile.c
$(OBJDIR)/esdlib.o:	 esdlib.c
$(OBJDIR)/esdmgr.o:	 esdmgr.c
$(OBJDIR)/genrand.o:	 genrand.c
$(OBJDIR)/filter.o:	 filter.c
$(OBJDIR)/mix.o:	 mix.c
$(OBJDIR)/players.o:	 players.c
$(OBJDIR)/proto.o:	 proto.c
$(OBJDIR)/samples.o:	 samples.c













