include ../global_config

CFLAGS += -I../quicktime

# must only define here so applications using libpng don't see any of the XFT
# includes at all.  They use a conflicting setjmp.
ifeq ($(HAVE_XFT), y)
CFLAGS += -DHAVE_XFT
endif

DEST := /usr/bin

#CFLAGS += -g

ifeq ($(OBJDIR), i686)
BOOTSTRAPFLAGS := -DBOOTSTRAP="\"objcopy -B i386 -I binary -O elf32-i386\""
endif

ifeq ($(OBJDIR), x86_64)
BOOTSTRAPFLAGS := -DBOOTSTRAP="\"objcopy -B i386 -I binary -O elf64-x86-64\""
LFLAGS += -L/usr/X11R6/lib64
endif


$(shell mkdir -p $(OBJDIR) )

OBJS = \
	$(OBJDIR)/bcbar.o \
	$(OBJDIR)/bcbitmap.o \
	$(OBJDIR)/bcbutton.o \
	$(OBJDIR)/bccapture.o \
	$(OBJDIR)/bccmodel_default.o \
	$(OBJDIR)/bccmodel_float.o \
	$(OBJDIR)/bccmodels.o \
	$(OBJDIR)/bccmodel_yuv420p.o \
	$(OBJDIR)/bccmodel_yuv422.o \
	$(OBJDIR)/bccounter.o \
	$(OBJDIR)/bcclipboard.o \
	$(OBJDIR)/bcdelete.o \
	$(OBJDIR)/bcdialog.o \
	$(OBJDIR)/bcdisplay.o \
	$(OBJDIR)/bcdisplayinfo.o \
	$(OBJDIR)/bcdragwindow.o \
	$(OBJDIR)/bcfilebox.o \
	$(OBJDIR)/bclistbox.o \
	$(OBJDIR)/bclistboxitem.o \
	$(OBJDIR)/bchash.o \
	$(OBJDIR)/bcmenu.o \
	$(OBJDIR)/bcmenubar.o \
	$(OBJDIR)/bcmenuitem.o \
	$(OBJDIR)/bcmenupopup.o \
	$(OBJDIR)/bcmeter.o \
	$(OBJDIR)/bcnewfolder.o \
	$(OBJDIR)/bcpan.o \
	$(OBJDIR)/bcpbuffer.o \
	$(OBJDIR)/bcpixmap.o \
	$(OBJDIR)/bcpopup.o \
	$(OBJDIR)/bcpopupmenu.o \
	$(OBJDIR)/bcpot.o \
	$(OBJDIR)/bcprogress.o \
	$(OBJDIR)/bcprogressbox.o \
	$(OBJDIR)/bcrename.o \
	$(OBJDIR)/bcrepeater.o \
	$(OBJDIR)/bcresources.o \
	$(OBJDIR)/bcscrollbar.o \
	$(OBJDIR)/bcsignals.o \
	$(OBJDIR)/bcslider.o \
	$(OBJDIR)/bcsubwindow.o \
	$(OBJDIR)/bcsynchronous.o \
	$(OBJDIR)/bctextbox.o \
	$(OBJDIR)/bctexture.o \
	$(OBJDIR)/bctheme.o \
	$(OBJDIR)/bctitle.o \
	$(OBJDIR)/bctoggle.o \
	$(OBJDIR)/bctumble.o \
	$(OBJDIR)/bcwindow.o \
	$(OBJDIR)/bcwindow3d.o \
	$(OBJDIR)/bcwindowbase.o \
	$(OBJDIR)/bcwindowdraw.o \
	$(OBJDIR)/bcwindowevents.o \
	$(OBJDIR)/condition.o \
	$(OBJDIR)/errorbox.o \
	$(OBJDIR)/filesystem.o \
	$(OBJDIR)/mutex.o \
	$(OBJDIR)/rotateframe.o \
	$(OBJDIR)/sema.o \
	$(OBJDIR)/stringfile.o \
	$(OBJDIR)/thread.o \
	$(OBJDIR)/testobject.o \
	$(OBJDIR)/bctimer.o \
	$(OBJDIR)/units.o \
	$(OBJDIR)/vframe.o \
	$(OBJDIR)/vframe3d.o \
	$(OBJDIR)/workarounds.o

OUTPUT = $(OBJDIR)/libguicast.so
STATICOUTPUT = $(OBJDIR)/libguicast.a

TESTLIBS := \
		$(STATICOUTPUT) \
		../quicktime/$(OBJDIR)/libquicktime.a \
		$(LFLAGS) \
		$(GLLIBS) \
		-lX11 \
		-lXext \
		-lXv \
		-lpthread \
		-lm \
		-lpng

#		-lXxf86vm \

ifeq ($(HAVE_XFT), y)
TESTLIBS += -lXft
endif	

ifeq ($(HAVE_GL), y)
TESTLIBS += -lGL
endif

UTILS = $(OBJDIR)/bootstrap $(OBJDIR)/pngtoh $(OBJDIR)/pngtoraw

$(shell echo $(CFLAGS) > $(OBJDIR)/c_flags)
$(shell echo $(OBJS) $(CXXREPOSITORY) > $(OBJDIR)/objs)


# PTHREAD DOESN'T WORK WHEN LINKED HERE
all: $(STATICOUTPUT) $(UTILS)


$(STATICOUTPUT): $(OBJS)
	ar rcs $(STATICOUTPUT) `cat $(OBJDIR)/objs`

$(OBJDIR)/bootstrap:
	gcc -O2 $(BOOTSTRAPFLAGS) bootstrap.c -o $(OBJDIR)/bootstrap

$(OBJDIR)/pngtoh: pngtoh.c
	gcc -O2 pngtoh.c -o $(OBJDIR)/pngtoh

$(OBJDIR)/pngtoraw: pngtoraw.c
	gcc -O2 pngtoraw.c -o $(OBJDIR)/pngtoraw -lpng -lz

replace:  replace.o $(STATICOUTPUT)
	$(CC) -o replace replace.o $(STATICOUTPUT) \
	../quicktime/i686/libquicktime.a \
	$(LFLAGS) \
	$(XLIBS) \
	-lpng

#	$(XXF86VM) \



test:	$(OBJDIR)/test.o $(STATICOUTPUT)
	$(CC) -o $(OBJDIR)/test \
		$(OBJDIR)/test.o \
		$(TESTLIBS)

test2:	$(OBJDIR)/test2.o $(STATICOUTPUT)
	$(CC) -o $(OBJDIR)/test2 \
		$(OBJDIR)/test2.o \
		$(TESTLIBS)

test3:	$(OBJDIR)/test3.o $(STATICOUTPUT)
	$(CC) -o $(OBJDIR)/test3 \
		$(OBJDIR)/test3.o \
		$(TESTLIBS)

clean:
	rm -rf $(OBJDIR)
	find \( -name core \
		-o -name '*.o' \
		-o -name '*.a' \
		-o -name '*.so' \) -exec rm -f {} \;

wc:
	cat *.C *.h | wc

backup: clean
	cd .. && \
	tar Icvf ~/guicast.tar.bz2 guicast

install:
	cp $(OBJDIR)/pngtoh $(OBJDIR)/bootstrap $(DEST)

$(OBJS) $(OBJDIR)/test.o $(OBJDIR)/test2.o $(OBJDIR)/test3.o $(OBJDIR)/replace.o:
	$(CC) -c `cat $(OBJDIR)/c_flags` $(subst $(OBJDIR)/,, $*.C) -o $*.o

$(OBJDIR)/bootstrap: bootstrap.c
$(OBJDIR)/pngtoh: pngtoh.c

$(OBJDIR)/bcbar.o:                                            bcbar.C
$(OBJDIR)/bcbitmap.o: 	   				      bcbitmap.C
$(OBJDIR)/bcbutton.o: 	   				      bcbutton.C
$(OBJDIR)/bccapture.o:     				      bccapture.C
$(OBJDIR)/bccmodel_default.o:				      bccmodel_default.C bccmodel_permutation.h
$(OBJDIR)/bccmodel_float.o:				      bccmodel_float.C
$(OBJDIR)/bccmodels.o:					      bccmodels.C
$(OBJDIR)/bccmodel_yuv420p.o:				      bccmodel_yuv420p.C
$(OBJDIR)/bccmodel_yuv422.o:				      bccmodel_yuv422.C
$(OBJDIR)/bccounter.o:                                        bccounter.C
$(OBJDIR)/bcclipboard.o:   				      bcclipboard.C
$(OBJDIR)/bcdelete.o:                                         bcdelete.C
$(OBJDIR)/bcdialog.o:                                         bcdialog.C
$(OBJDIR)/bcdisplay.o: 				      	      bcdisplay.C
$(OBJDIR)/bcdisplayinfo.o: 				      bcdisplayinfo.C
$(OBJDIR)/bcdragwindow.o:  				      bcdragwindow.C
$(OBJDIR)/bcfilebox.o:     				      bcfilebox.C
$(OBJDIR)/bchash.o:                                           bchash.C
$(OBJDIR)/bcipc.o: 	   				      bcipc.C
$(OBJDIR)/bclistbox.o:     				      bclistbox.C
$(OBJDIR)/bclistboxitem.o:     				      bclistboxitem.C
$(OBJDIR)/bcmenu.o:                                           bcmenu.C
$(OBJDIR)/bcmenubar.o:                                        bcmenubar.C
$(OBJDIR)/bcmenuitem.o:                                       bcmenuitem.C
$(OBJDIR)/bcmenupopup.o:                                      bcmenupopup.C
$(OBJDIR)/bcmeter.o: 	   				      bcmeter.C
$(OBJDIR)/bcnewfolder.o:                                      bcnewfolder.C
$(OBJDIR)/bcpan.o: 	   				      bcpan.C
$(OBJDIR)/bcpbuffer.o:                                        bcpbuffer.C
$(OBJDIR)/bcpixmap.o: 	   				      bcpixmap.C
$(OBJDIR)/bcpopup.o: 	   				      bcpopup.C
$(OBJDIR)/bcpopupmenu.o:   				      bcpopupmenu.C
$(OBJDIR)/bcpot.o: 	   				      bcpot.C
$(OBJDIR)/bcprogress.o:    				      bcprogress.C
$(OBJDIR)/bcprogressbox.o: 				      bcprogressbox.C
$(OBJDIR)/bcrename.o:    				      bcrename.C
$(OBJDIR)/bcrepeater.o:    				      bcrepeater.C
$(OBJDIR)/bcresources.o:   				      bcresources.C
$(OBJDIR)/bcscrollbar.o:   				      bcscrollbar.C
$(OBJDIR)/bcsignals.o:     				      bcsignals.C
$(OBJDIR)/bcslider.o: 	   				      bcslider.C
$(OBJDIR)/bcsubwindow.o:   				      bcsubwindow.C
$(OBJDIR)/bcsynchronous.o:                                    bcsynchronous.C
$(OBJDIR)/bctextbox.o:     				      bctextbox.C
$(OBJDIR)/bctexture.o:                                        bctexture.C
$(OBJDIR)/bctitle.o: 	   				      bctitle.C
$(OBJDIR)/bctheme.o: 	   				      bctheme.C
$(OBJDIR)/bctoggle.o: 	   				      bctoggle.C
$(OBJDIR)/bctumble.o: 	   				      bctumble.C
$(OBJDIR)/bcwindow3d.o:                                       bcwindow3d.C
$(OBJDIR)/bcwindow.o: 	   				      bcwindow.C
$(OBJDIR)/bcwindowbase.o:  				      bcwindowbase.C
$(OBJDIR)/bcwindowdraw.o:  				      bcwindowdraw.C
$(OBJDIR)/bcwindowevents.o:  				      bcwindowevents.C
$(OBJDIR)/condition.o:  				      condition.C
$(OBJDIR)/errorbox.o: 	   				      errorbox.C
$(OBJDIR)/defaults.o: 	   				      defaults.C
$(OBJDIR)/filesystem.o:    				      filesystem.C
$(OBJDIR)/mutex.o: 	   				      mutex.C
$(OBJDIR)/rotateframe.o:                                      rotateframe.C
$(OBJDIR)/sema.o: 	   				      sema.C
$(OBJDIR)/stringfile.o:    				      stringfile.C
$(OBJDIR)/test.o: 	   				      test.C
$(OBJDIR)/test2.o: 	   				      test2.C
$(OBJDIR)/test3.o: 	   				      test3.C
$(OBJDIR)/testobject.o:                                       testobject.C
$(OBJDIR)/thread.o: 	   				      thread.C
$(OBJDIR)/bctimer.o: 	   				      bctimer.C
$(OBJDIR)/units.o: 	   				      units.C
$(OBJDIR)/vframe.o: 	   				      vframe.C
$(OBJDIR)/vframe3d.o: 	   				      vframe3d.C
$(OBJDIR)/workarounds.o:   				      workarounds.C






