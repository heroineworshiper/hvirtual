include ../global_config

CFLAGS += -I../guicast -I../quicktime
PREFIX=/usr


OBJS := \
	$(OBJDIR)/main.o \
	$(OBJDIR)/mixer.o





ifeq ($(OBJDIR), x86_64)
LIBS := -L/usr/X11R6/lib64
else
LIBS := -L/usr/X11R6/lib
endif


LIBS += $(GLLIBS) -lX11 -lXext -lXv -lpthread -lm -lpng -lz -ldl -lXxf86vm -lGL

ifeq ($(HAVE_XFT), y)
LIBS += -lXft
endif


# Static libraries for distribution
ifeq ($(USE_STATIC),y)
LIBS += -static
endif

OUTPUT = $(OBJDIR)/mix2000

$(OUTPUT): $(OBJS)
	$(CC) -o $(OUTPUT) \
		$(OBJS) \
		../guicast/$(OBJDIR)/libguicast.a \
		../quicktime/$(OBJDIR)/libquicktime.a \
		../libmpeg3/$(OBJDIR)/libmpeg3.a \
		$(LIBS)

$(OBJS):
	$(CC) -c $(CFLAGS) $(subst $(OBJDIR)/,, $*.C) -o $*.o

install:
	rm -f $(PREFIX)/bin/mix2000
	cp $(OUTPUT) $(PREFIX)/bin


clean:
	find -follow \( -name core \
		-o -name '*.o' \
		-o -name '*.a' \
		-o -name '*.so' \) -exec rm -f {} \;
	rm -rf $(OBJDIR)

wc:
	cat *.C *.h | wc

$(OBJDIR)/main.o: main.C
$(OBJDIR)/mixer.o: mixer.C

