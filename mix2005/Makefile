HAVE_LIBXXF86VM := $(shell if [ -f /usr/X11R6/include/X11/extensions/xf86vmode.h ]; then echo yes; fi )

include ../global_config

CFLAGS += -I../guicast \
	-I../quicktime

CFLAGS += -g

PREFIX=/usr

ifeq ($(HAVE_LIBXXF86VM), yes)

XXF86VM = -lXxf86vm
endif


OBJS := \
	$(OBJDIR)/audiodriver.o \
	$(OBJDIR)/audioalsa.o \
	$(OBJDIR)/configure.o \
	$(OBJDIR)/main.o \
	$(OBJDIR)/mixer.o \
	$(OBJDIR)/mixergui.o \
	$(OBJDIR)/mixerpopup.o \
	$(OBJDIR)/mixertree.o \
	$(OBJDIR)/theme.o

THEME_DATA := $(OBJDIR)/theme_data.o

ifeq ($(OBJDIR), x86_64)
LIBS := -L/usr/X11R6/lib64
else
LIBS := -L/usr/X11R6/lib
endif

ifeq ($(HAVE_GL), y)
LIBS += -lGL
endif


LIBS += -lX11 \
	-lXext \
	-lXv  \
	-lpthread  \
	-lm \
	-lpng  \
	-lz  \
	-ldl  \
	-lasound \
	$(XXF86VM) \
	$(THEME_DATA)

ifeq ($(HAVE_XFT), y)
LIBS += -lXft
endif

OUTPUT = $(OBJDIR)/mix2005

$(shell echo $(CFLAGS) > $(OBJDIR)/c_flags)

$(OUTPUT): $(OBJS) $(THEME_DATA)
	$(CC) -o $(OUTPUT) \
		$(OBJS) \
		../guicast/$(OBJDIR)/libguicast.a \
                ../guicast/$(OBJDIR)/libcmodel.a \
		$(LIBS)

#		../quicktime/$(OBJDIR)/libquicktime.a \

$(OBJS):
	$(CC) -c `cat $(OBJDIR)/c_flags` $(subst $(OBJDIR)/,, $*.C) -o $*.o

install:
	rm -f $(PREFIX)/bin/mix2005
	cp $(OUTPUT) $(PREFIX)/bin


$(THEME_DATA):
	cd $(OBJDIR) && \
	../../guicast/$(OBJDIR)/bootstrap theme_data.o ../theme/*.png


clean:
	rm -rf $(OBJDIR)

wc:
	cat *.C *.h | wc

$(OBJDIR)/audiodriver.o: audiodriver.C
$(OBJDIR)/audioalsa.o:	 audioalsa.C
$(OBJDIR)/configure.o:	 configure.C
$(OBJDIR)/main.o:	 main.C
$(OBJDIR)/mixer.o:	 mixer.C
$(OBJDIR)/mixergui.o:	 mixergui.C
$(OBJDIR)/mixerpopup.o:  mixerpopup.C
$(OBJDIR)/mixertree.o:   mixertree.C
$(OBJDIR)/theme.o:   theme.C







