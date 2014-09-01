#AVIDIR = ../avifile
#include ../avifile/makefile_objs
include ../global_config
SNDFILE_DIR := $(shell expr ../libsndfile* )

PREFIX=/usr

OBJS = \
	$(OBJDIR)/arender.o \
	$(OBJDIR)/asset.o \
	$(OBJDIR)/audiodevice.o \
	$(OBJDIR)/file.o \
	$(OBJDIR)/fileavi.o \
	$(OBJDIR)/filebase.o \
	$(OBJDIR)/filemov.o \
	$(OBJDIR)/filempeg.o \
	$(OBJDIR)/filesndfile.o \
	$(OBJDIR)/load.o \
	$(OBJDIR)/mainmenu.o \
	$(OBJDIR)/mwindow.o \
	$(OBJDIR)/mwindowgui.o \
	$(OBJDIR)/playbackengine.o \
	$(OBJDIR)/playbackscroll.o \
	$(OBJDIR)/playlist.o \
	$(OBJDIR)/renderengine.o \
	$(OBJDIR)/theme.o \
	$(OBJDIR)/transportque.o \
	$(OBJDIR)/settings.o \
	$(OBJDIR)/vrender.o \
	$(OBJDIR)/xmovie.o

THEME_DATA := $(OBJDIR)/suv_data.o

LIBS += $(THEME_DATA)


OUTPUT = $(OBJDIR)/xmovie

CFLAGS += -I../guicast \
	-I../quicktime \
	-I../libmpeg3 \
	-I$(SNDFILE_DIR)/src

ifeq ($(OBJDIR), x86_64)
LIBS += -L/usr/X11R6/lib64
endif

ifeq ($(OBJDIR), i686)
LIBS += -L/usr/X11R6/lib
endif

LIBS += ../libmpeg3/$(OBJDIR)/libmpeg3.a \
	../quicktime/$(OBJDIR)/libquicktime.a \
	../guicast/$(OBJDIR)/libguicast.a \
	$(SNDFILE_DIR)/src/.libs/libsndfile.a \
 	$(GLLIBS) \
	-lX11 \
	-lXext \
	-lXv \
	-lpthread \
	-lm \
	-lpng \
	-lz \
	-ldl \
	-lXxf86vm

ifeq ($(HAVE_XFT), y)
LIBS += -lXft
endif

# Static libraries for distribution
ifeq ($(USE_STATIC),y)
LIBS += -static
endif

$(shell echo $(OBJS) $(AVIOBJS) $(CXXREPOSITORY) $(LFLAGS) $(LIBS) > $(OBJDIR)/libs )
$(shell echo $(CFLAGS) > $(OBJDIR)/c_flags )

$(OUTPUT): $(OBJS) $(THEME_DATA)
	$(CC) -o $(OUTPUT) `cat $(OBJDIR)/libs`

$(THEME_DATA):
	cd $(OBJDIR) && \
	../../guicast/$(OBJDIR)/bootstrap suv_data.o ../suv/*.png

test: test.o
	$(CC) -o $(OBJDIR)/test $(OBJDIR)/test.o $(LFLAGS) $(LIBS)

clean:
	find -follow \( -name core \
		-o -name '*.o' \
		-o -name '*.a' \
		-o -name '*.so' \) -exec rm -f {} \; -prune
	rm -rf $(OBJDIR)

wc:
	cat *.C *.h | wc

install:
	strip $(OUTPUT)
	rm -f $(PREFIX)/bin/xmovie
	cp $(OUTPUT) $(PREFIX)/bin

dist:
	strip $(OUTPUT) && \
	tar zcf $(OUTPUT).tar.gz $(OUTPUT)

$(OBJS):
	@echo C++ $*.o
	@$(CC) -c `cat $(OBJDIR)/c_flags` $(subst $(OBJDIR)/,, $*.C) -o $*.o

$(OBJDIR)/arender.o: 		       arender.C
$(OBJDIR)/asset.o: 		       asset.C
$(OBJDIR)/audiodevice.o: 	       audiodevice.C
$(OBJDIR)/file.o: 		       file.C
$(OBJDIR)/fileavi.o: 		       fileavi.C
$(OBJDIR)/filebase.o: 		       filebase.C
$(OBJDIR)/filemov.o: 		       filemov.C
$(OBJDIR)/filempeg.o: 		       filempeg.C
$(OBJDIR)/filesndfile.o: 	       filesndfile.C
$(OBJDIR)/load.o: 		       load.C
$(OBJDIR)/mainmenu.o: 		       mainmenu.C
$(OBJDIR)/mwindow.o: 		       mwindow.C
$(OBJDIR)/mwindowgui.o: 	       mwindowgui.C
$(OBJDIR)/playbackengine.o: 	       playbackengine.C
$(OBJDIR)/playbackscroll.o: 	       playbackscroll.C
$(OBJDIR)/playlist.o: 		       playlist.C
$(OBJDIR)/renderengine.o: 	       renderengine.C
$(OBJDIR)/theme.o: 		       theme.C
$(OBJDIR)/transportque.o: 	       transportque.C
$(OBJDIR)/settings.o: 		       settings.C
$(OBJDIR)/vrender.o: 		       vrender.C
$(OBJDIR)/xmovie.o: 		       xmovie.C

