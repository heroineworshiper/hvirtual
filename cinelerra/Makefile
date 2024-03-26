include ../global_config

AVC_DIR := $(shell expr ../thirdparty/libavc1394* )
DV_DIR := $(shell expr ../quicktime/thirdparty/libdv-*/ )
EXRDIR := $(shell expr ../thirdparty/openexr* )
ILMDIR := $(shell expr ../thirdparty/ilmbase* )
FLAC_DIR := $(shell expr ../thirdparty/flac*/ )
FFMPEG_DIR := $(shell expr ../quicktime/thirdparty/ffmpeg* )

ifeq ($(OBJDIR), i686)
JPEG_DIR := $(shell expr ../quicktime/thirdparty/jpeg-mmx* )
else
JPEG_DIR := $(shell expr ../quicktime/thirdparty/jpeg9d )
endif

LAME_DIR := ../quicktime/thirdparty/lame-3.93.1/include
LIBIEC_DIR := $(shell expr ../thirdparty/libiec61883* )
LIBRAW1394_DIR := $(shell expr ../thirdparty/libraw1394* )
LIBRAW_DIR := $(shell expr ../thirdparty/LibRaw* )
OGG_DIR := $(shell expr ../quicktime/thirdparty/libogg* )
SNDFILE_DIR := $(shell expr ../thirdparty/libsndfile* )
THEORA_DIR := $(shell expr ../thirdparty/libtheora* )
GIF_DIR := $(shell expr ../thirdparty/giflib-* )
TIFF_DIR := ../thirdparty/tiff-v3.5.7
TOOLAME_DIR := ../thirdparty/toolame-02l
UUID_DIR := ../thirdparty/uuid
VORBIS_DIR := $(shell expr ../quicktime/thirdparty/libvorbis* )
PULSE_DIR := $(shell expr ../thirdparty/pulseaudio-* )

AVC_LIB := $(AVC_DIR)/libavc1394/.libs/libavc1394.a $(AVC_DIR)/librom1394/.libs/librom1394.a
RAW1394_LIB := $(LIBRAW1394_DIR)/src/.libs/libraw1394.a
IEC_LIB := $(LIBIEC_DIR)/src/.libs/libiec61883.a
THEORA_LIB := $(THEORA_DIR)/lib/.libs/libtheora.a
#PULSE_LIB := -L$(PULSE_DIR)/src/.libs/ -lpulse -lpulse-simple
PULSE_LIB := $(PULSE_DIR)/src/.libs/libpulse-simple.a \
	$(PULSE_DIR)/src/.libs/libpulse.a \
	$(PULSE_DIR)/src/.libs/libpulsecommon*.a 
RAW_LIB := $(LIBRAW_DIR)/libraw.a

EXR_INCLUDES := \
	-I$(ILMDIR)/Iex/ \
	-I$(ILMDIR)/Half/ \
	-I$(ILMDIR)/Imath/ \
	-I$(ILMDIR)/Imath/ \
	-I$(ILMDIR)/config/ \
	-I$(ILMDIR)/IlmThread \
	-I$(EXRDIR)/IlmImf/ \
	-I$(EXRDIR)/config/

OBJS = \
	$(OBJDIR)/aattachmentpoint.o \
	$(OBJDIR)/aautomation.o \
	$(OBJDIR)/aboutprefs.o \
	$(OBJDIR)/adeviceprefs.o \
	$(OBJDIR)/aedit.o \
	$(OBJDIR)/aedits.o \
	$(OBJDIR)/affine.o \
	$(OBJDIR)/amodule.o \
	$(OBJDIR)/apatchgui.o \
	$(OBJDIR)/aplugin.o \
	$(OBJDIR)/apluginarray.o \
	$(OBJDIR)/apluginset.o \
	$(OBJDIR)/arender.o \
	$(OBJDIR)/assetedit.o \
	$(OBJDIR)/assetpopup.o \
	$(OBJDIR)/assetremove.o \
	$(OBJDIR)/assets.o \
	$(OBJDIR)/asset.o \
	$(OBJDIR)/atrack.o \
	$(OBJDIR)/attachmentpoint.o \
	$(OBJDIR)/audio1394.o \
	$(OBJDIR)/audioalsa.o \
	$(OBJDIR)/audiocine.o \
	$(OBJDIR)/audiodevice.o \
	$(OBJDIR)/audiodvb.o \
	$(OBJDIR)/audioesound.o \
	$(OBJDIR)/audioidevice.o \
	$(OBJDIR)/audioodevice.o \
	$(OBJDIR)/audiooss.o \
	$(OBJDIR)/auto.o \
	$(OBJDIR)/autos.o \
	$(OBJDIR)/autoconf.o \
	$(OBJDIR)/automation.o \
	$(OBJDIR)/avc1394control.o \
	$(OBJDIR)/avc1394transport.o \
	$(OBJDIR)/awindow.o \
	$(OBJDIR)/awindowgui.o \
	$(OBJDIR)/awindowmenu.o \
	$(OBJDIR)/batch.o \
	$(OBJDIR)/batchrender.o \
	$(OBJDIR)/bitspopup.o \
	$(OBJDIR)/brender.o \
	$(OBJDIR)/browsebutton.o \
	$(OBJDIR)/byteorderpopup.o \
	$(OBJDIR)/cache.o \
	$(OBJDIR)/cachebase.o \
	$(OBJDIR)/canvas.o \
	$(OBJDIR)/canvastools.o \
	$(OBJDIR)/channel.o \
	$(OBJDIR)/channeldb.o \
	$(OBJDIR)/channeledit.o \
	$(OBJDIR)/channelpicker.o \
	$(OBJDIR)/chantables.o \
	$(OBJDIR)/clipedit.o \
	$(OBJDIR)/cicolors.o \
	$(OBJDIR)/colorpicker.o \
	$(OBJDIR)/commonrender.o \
        $(OBJDIR)/compressortools.o \
	$(OBJDIR)/confirmsave.o \
	$(OBJDIR)/confirmquit.o \
	$(OBJDIR)/cpanel.o \
	$(OBJDIR)/cplayback.o \
	$(OBJDIR)/ctimebar.o \
	$(OBJDIR)/cursor.o \
	$(OBJDIR)/cwindow.o \
	$(OBJDIR)/cwindowgui.o \
	$(OBJDIR)/cwindowtool.o \
	$(OBJDIR)/device1394input.o \
	$(OBJDIR)/devicedvbinput.o \
	$(OBJDIR)/device1394output.o \
	$(OBJDIR)/deleteallindexes.o \
	$(OBJDIR)/dvbtune.o \
	$(OBJDIR)/drivesync.o \
	$(OBJDIR)/edit.o \
        $(OBJDIR)/editinfo.o \
        $(OBJDIR)/editkeyframe.o \
	$(OBJDIR)/editlength.o \
	$(OBJDIR)/editpanel.o \
	$(OBJDIR)/editpopup.o \
	$(OBJDIR)/edits.o \
	$(OBJDIR)/edl.o \
	$(OBJDIR)/edlfactory.o \
	$(OBJDIR)/edlsession.o \
        $(OBJDIR)/eqcanvas.o \
	$(OBJDIR)/fadeengine.o \
	$(OBJDIR)/file.o \
	$(OBJDIR)/fileavi.o \
	$(OBJDIR)/filebase.o \
	$(OBJDIR)/filebaseaudio.o \
	$(OBJDIR)/filebaseima4.o \
	$(OBJDIR)/filebaseulaw.o \
	$(OBJDIR)/filecr2.o \
	$(OBJDIR)/filefork.o \
	$(OBJDIR)/fileformat.o \
	$(OBJDIR)/filegif.o \
	$(OBJDIR)/filelist.o \
	$(OBJDIR)/filejpeg.o \
	$(OBJDIR)/filemov.o \
	$(OBJDIR)/filempeg.o \
	$(OBJDIR)/fileogg.o \
	$(OBJDIR)/filepng.o \
	$(OBJDIR)/filepreviewer.o \
	$(OBJDIR)/filescene.o \
	$(OBJDIR)/fileserver.o \
	$(OBJDIR)/filesndfile.o \
	$(OBJDIR)/filestdout.o \
	$(OBJDIR)/filetga.o \
	$(OBJDIR)/filethread.o \
	$(OBJDIR)/filetiff.o \
	$(OBJDIR)/filevorbis.o \
	$(OBJDIR)/filexml.o \
	$(OBJDIR)/floatauto.o \
	$(OBJDIR)/floatautos.o \
	$(OBJDIR)/forkwrapper.o \
	$(OBJDIR)/formatcheck.o \
	$(OBJDIR)/formatpresets.o \
	$(OBJDIR)/formatpopup.o \
	$(OBJDIR)/formattools.o \
	$(OBJDIR)/fourier.o \
	$(OBJDIR)/framecache.o \
	$(OBJDIR)/garbage.o \
	$(OBJDIR)/gwindow.o \
	$(OBJDIR)/gwindowgui.o \
	$(OBJDIR)/iec61883input.o \
	$(OBJDIR)/iec61883output.o \
	$(OBJDIR)/indexable.o \
	$(OBJDIR)/indexfile.o \
	$(OBJDIR)/indexstate.o \
	$(OBJDIR)/indexthread.o \
	$(OBJDIR)/intauto.o \
	$(OBJDIR)/intautos.o \
	$(OBJDIR)/interfaceprefs.o \
	$(OBJDIR)/keyframe.o \
	$(OBJDIR)/keyframegui.o \
	$(OBJDIR)/keyframepopup.o \
	$(OBJDIR)/keyframes.o \
	$(OBJDIR)/keyframehandles.o \
	$(OBJDIR)/labelnavigate.o \
	$(OBJDIR)/labels.o \
	$(OBJDIR)/levelwindow.o \
	$(OBJDIR)/levelwindowgui.o \
	$(OBJDIR)/loadbalance.o \
	$(OBJDIR)/loadfile.o \
	$(OBJDIR)/loadmode.o \
	$(OBJDIR)/localsession.o \
	$(OBJDIR)/main.o \
	$(OBJDIR)/mainclock.o \
	$(OBJDIR)/maincursor.o \
	$(OBJDIR)/mainerror.o \
	$(OBJDIR)/mainindexes.o \
	$(OBJDIR)/mainmenu.o \
	$(OBJDIR)/mainprogress.o \
	$(OBJDIR)/mainsession.o \
	$(OBJDIR)/mainundo.o \
	$(OBJDIR)/maskauto.o \
	$(OBJDIR)/maskautos.o \
	$(OBJDIR)/maskengine.o \
	$(OBJDIR)/mbuttons.o \
	$(OBJDIR)/menuaeffects.o \
	$(OBJDIR)/menuattacheffect.o \
	$(OBJDIR)/menueditlength.o \
	$(OBJDIR)/menueffects.o \
	$(OBJDIR)/menutransitionlength.o \
	$(OBJDIR)/menuveffects.o \
	$(OBJDIR)/meterpanel.o \
	$(OBJDIR)/module.o \
        $(OBJDIR)/motioncache.o \
        $(OBJDIR)/motionscan.o \
	$(OBJDIR)/mtimebar.o \
	$(OBJDIR)/mwindow.o \
	$(OBJDIR)/mwindowedit.o \
	$(OBJDIR)/mwindowgui.o \
	$(OBJDIR)/mwindowmove.o \
	$(OBJDIR)/nestededls.o \
	$(OBJDIR)/new.o \
	$(OBJDIR)/newfolder.o \
	$(OBJDIR)/overlayframe.o \
	$(OBJDIR)/packagedispatcher.o \
	$(OBJDIR)/packagerenderer.o \
	$(OBJDIR)/panauto.o \
	$(OBJDIR)/panautos.o \
	$(OBJDIR)/panedividers.o \
	$(OBJDIR)/patchbay.o \
	$(OBJDIR)/patchgui.o \
	$(OBJDIR)/performanceprefs.o \
	$(OBJDIR)/picture.o \
	$(OBJDIR)/playabletracks.o \
	$(OBJDIR)/playback3d.o \
	$(OBJDIR)/playbackconfig.o \
	$(OBJDIR)/playbackengine.o \
	$(OBJDIR)/playbackprefs.o \
	$(OBJDIR)/playtransport.o \
	$(OBJDIR)/plugin.o \
	$(OBJDIR)/pluginaclient.o \
	$(OBJDIR)/pluginaclientlad.o \
	$(OBJDIR)/pluginarray.o \
	$(OBJDIR)/pluginclient.o \
	$(OBJDIR)/plugindialog.o \
	$(OBJDIR)/pluginpopup.o \
	$(OBJDIR)/pluginset.o \
	$(OBJDIR)/pluginserver.o \
	$(OBJDIR)/plugintclient.o \
	$(OBJDIR)/plugintoggles.o \
	$(OBJDIR)/pluginvclient.o \
	$(OBJDIR)/preferences.o \
	$(OBJDIR)/preferencesthread.o \
	$(OBJDIR)/presets.o \
	$(OBJDIR)/presetsgui.o \
        $(OBJDIR)/previewer.o \
	$(OBJDIR)/proxy.o \
	$(OBJDIR)/question.o \
	$(OBJDIR)/quit.o \
	$(OBJDIR)/recconfirmdelete.o \
	$(OBJDIR)/record.o \
	$(OBJDIR)/recordableatracks.o \
	$(OBJDIR)/recordablevtracks.o \
	$(OBJDIR)/recordaudio.o \
	$(OBJDIR)/recordconfig.o \
	$(OBJDIR)/recordgui.o \
	$(OBJDIR)/recordlabel.o \
	$(OBJDIR)/recordmonitor.o \
	$(OBJDIR)/recordprefs.o \
	$(OBJDIR)/recordscopes.o \
	$(OBJDIR)/recordthread.o \
	$(OBJDIR)/recordtransport.o \
	$(OBJDIR)/recordvideo.o \
	$(OBJDIR)/recordwindow.o \
	$(OBJDIR)/removethread.o \
	$(OBJDIR)/render.o \
	$(OBJDIR)/renderfarm.o \
	$(OBJDIR)/renderfarmclient.o \
	$(OBJDIR)/renderengine.o \
	$(OBJDIR)/resample.o \
	$(OBJDIR)/resizetrackthread.o \
	$(OBJDIR)/resourcepixmap.o \
	$(OBJDIR)/resourcethread.o \
	$(OBJDIR)/samples.o \
	$(OBJDIR)/samplescroll.o \
	$(OBJDIR)/save.o \
	$(OBJDIR)/scenegraph.o \
	$(OBJDIR)/scopewindow.o \
	$(OBJDIR)/setformat.o \
	$(OBJDIR)/sharedlocation.o \
	$(OBJDIR)/shmemory.o \
	$(OBJDIR)/sighandler.o \
	$(OBJDIR)/splashgui.o \
	$(OBJDIR)/statusbar.o \
	$(OBJDIR)/swapasset.o \
	$(OBJDIR)/theme.o \
	$(OBJDIR)/threadexec.o \
	$(OBJDIR)/threadloader.o \
	$(OBJDIR)/threadfork.o \
	$(OBJDIR)/timelinepane.o \
	$(OBJDIR)/timebar.o \
	$(OBJDIR)/timeentry.o \
	$(OBJDIR)/tipwindow.o \
	$(OBJDIR)/track.o \
	$(OBJDIR)/trackcanvas.o \
	$(OBJDIR)/tracking.o \
	$(OBJDIR)/tracks.o \
	$(OBJDIR)/trackscroll.o \
	$(OBJDIR)/tracksedit.o \
	$(OBJDIR)/transition.o \
	$(OBJDIR)/transitiondialog.o \
	$(OBJDIR)/transitionpopup.o \
	$(OBJDIR)/transportque.o \
	$(OBJDIR)/tunerserver.o \
	$(OBJDIR)/undostack.o \
	$(OBJDIR)/vattachmentpoint.o \
	$(OBJDIR)/vautomation.o \
	$(OBJDIR)/vdevice1394.o \
	$(OBJDIR)/vdevicebase.o \
	$(OBJDIR)/vdevicebuz.o \
	$(OBJDIR)/vdevicedvb.o \
	$(OBJDIR)/vdeviceprefs.o \
	$(OBJDIR)/vdevicepreview.o \
	$(OBJDIR)/vdevicev4l.o \
	$(OBJDIR)/vdevicev4l2.o \
	$(OBJDIR)/vdevicev4l2jpeg.o \
	$(OBJDIR)/vdevicex11.o \
	$(OBJDIR)/vedit.o \
	$(OBJDIR)/vedits.o \
	$(OBJDIR)/videodevice.o \
	$(OBJDIR)/viewmenu.o \
	$(OBJDIR)/virtualnode.o \
	$(OBJDIR)/virtualaconsole.o \
	$(OBJDIR)/virtualanode.o \
	$(OBJDIR)/virtualconsole.o \
	$(OBJDIR)/virtualvconsole.o \
	$(OBJDIR)/virtualvnode.o \
	$(OBJDIR)/vmodule.o \
	$(OBJDIR)/vpatchgui.o \
	$(OBJDIR)/vplayback.o \
	$(OBJDIR)/vpluginarray.o \
	$(OBJDIR)/vtimebar.o \
	$(OBJDIR)/vrender.o \
	$(OBJDIR)/vtrack.o \
	$(OBJDIR)/vwindow.o \
	$(OBJDIR)/vwindowgui.o \
	$(OBJDIR)/wavecache.o \
	$(OBJDIR)/zoombar.o \
	$(OBJDIR)/zoompanel.o


#	$(OBJDIR)/edithandles.o \
#	$(OBJDIR)/dcoffset.o \
#	$(OBJDIR)/transitionhandles.o \
#	$(OBJDIR)/renderfarmfsclient.o \
#	$(OBJDIR)/renderfarmfsserver.o \

# some files need different compilation options
FILEEXR := $(OBJDIR)/fileexr.o
FILEFLAC := $(OBJDIR)/fileflac.o
FFMPEG_OBJS := $(OBJDIR)/fileffmpeg.o \
	$(OBJDIR)/fileac3.o
FILECR3 := $(OBJDIR)/filecr3.o
DCRAW := $(OBJDIR)/dcraw.o
CFUNCTIONS := $(OBJDIR)/cfunctions.o
AUDIOPULSE := $(OBJDIR)/audiopulse.o

THEME_DATA := $(OBJDIR)/theme_data.o


OUTPUT = ../bin/cinelerra


#ifeq ($(HAVE_AVIFILE), y)
AVIFILE_DIR := $(shell expr ../avifile* )
#endif



STATICLIBS = \
	$(AVC_LIB) \
	$(RAW_LIB) \
        $(RAW1394_LIB) \
	$(IEC_LIB) \
        ../guicast/$(OBJDIR)/libcmodel.a \
	../quicktime/$(OBJDIR)/libquicktime.a \
        $(ESOUNDLIBS) \
	$(EXRDIR)/$(OBJDIR)/libexr.a \
	$(FLAC_DIR)/src/libFLAC/.libs/libFLAC.a \
	$(GIF_DIR)/$(OBJDIR)/libgif.a \
	$(TIFF_DIR)/$(OBJDIR)/libtiff.a \
	../guicast/$(OBJDIR)/libguicast.a \
	../libmpeg3/$(OBJDIR)/libmpeg3.a \
	$(SNDFILE_DIR)/src/.libs/libsndfile.a \
	$(UUID_DIR)/$(OBJDIR)/libuuid.a \
	../mpeg2enc/$(OBJDIR)/mpeg2enc.a \
	$(TOOLAME_DIR)/$(OBJDIR)/toolame.a \
	$(THEME_DATA) \
	$(THEORA_LIB) \
        $(PULSE_LIB)

LIBS := \
	-L./ \
	-L../guicast
        


ifeq ($(OBJDIR), x86_64)
LIBS += -L/usr/X11R6/lib64
else
LIBS += -L/usr/X11R6/lib
endif

ifeq ($(HAVE_GL), y)
LIBS += -lGL
endif


ifeq ($(HAVE_ALSA), y)
CFLAGS += -DHAVE_ALSA
LIBS += -lasound
endif



LIBS += \
	-lX11 \
	-lXext \
	-lXfixes \
	-lXv \
	-lpthread \
	-lm \
	-lpng \
	-lz \
	-ldl \
	-lbz2 \
	-llzma \
	-lrt

#	-lfontconfig \
#	-lXxf86vm \

ifeq ($(HAVE_XFT), y)
LIBS += -lXft
endif



CFLAGS += \
	-c \
	-I../guicast \
	-I../quicktime \
	-I$(JPEG_DIR) \
	-I$(DV_DIR) \
	-I$(OGG_DIR)/include \
	-I$(VORBIS_DIR)/include \
	-I$(LIBRAW1394_DIR)/ \
	-I$(AVC_DIR) \
	-I$(LIBIEC_DIR)/src/ \
	-I$(GIF_DIR)/lib \
	-I$(TIFF_DIR)/libtiff \
	-I$(UUID_DIR) \
	-I$(TOOLAME_DIR) \
	-I$(LAME_DIR) \
	-I../libmpeg3 \
	-I$(SNDFILE_DIR)/src \
	$(ESOUNDCFLAGS) \
	-I$(THEORA_DIR)/include


# Speed up linking with this linking sequence
ifeq ($(OBJDIR), alpha)



LDFLAGS1 = \
	--demangle=compaq -export-dynamic -L./ \
	-L../guicast -L/usr/X11R6/lib \
	-L`expr /usr/lib/compaq/cxx-*/alpha-linux/`lib -L/usr/local/lib \
	-L`expr /usr/lib/gcc-lib/alpha-redhat-linux/*` \
	-rpath `expr /usr/lib/compaq/cxx-*/alpha-linux/`bin/ -m elf64alpha \
	-L`expr /usr/lib/compaq/cxx-*/alpha-linux/`bin/ -dynamic-linker \
	/lib/ld-linux.so.2 `expr /usr/lib/compaq/cxx-*/alpha-linux/`bin/crt1.o \
	`expr /usr/lib/compaq/cxx-*/alpha-linux/`bin/crti.o \
	`expr /usr/lib/compaq/cxx-*/alpha-linux/`bin/crtbegin.o \
	`expr /usr/lib/compaq/cxx-*/alpha-linux/`bin/_main.o

LDFLAGS2 = -lcpml -lcxxstdma_rh60 -lcxxma_rh60 -lc -lots \
	`expr /usr/lib/compaq/cxx-*/alpha-linux/`bin/crtend.o \
	`expr /usr/lib/compaq/cxx-*/alpha-linux/`bin/crtn.o --no-demangle \
	--warn-once
LINKER = ld -o $(OUTPUT)
CFLAGS += -DUSE_ALPHA



else



LDFLAGS1 = -export-dynamic
LDFLAGS2 = 
LINKER = g++ -o $(OUTPUT)


endif




# Enable AVIFile support
ifeq ($(HAVE_AVIFILE), y)

CFLAGS += -DUSE_AVIFILE -I$(AVIFILE_DIR)/include
LIBS += \
	$(AVIFILE_DIR)/libmmxnow/src/.libs/libmmxnow.a \
	$(AVIFILE_DIR)/lib/.libs/libaviplay.a

endif


FFMPEG_CFLAGS := \
	$(CFLAGS) \
        $(FFMPEG_CFLAGS)

FFMPEG_LIBS := -lavformat -lavcodec -lavutil -lswresample

CFLAGS += -g
CXXFLAGS := $(CFLAGS) -std=c++11

$(shell echo $(CFLAGS) > $(OBJDIR)/c_flags)
$(shell echo $(CXXFLAGS) > $(OBJDIR)/cxx_flags)
$(shell echo $(FFMPEG_CFLAGS) -D__STDC_CONSTANT_MACROS > $(OBJDIR)/ffmpeg_cflags)
$(shell echo $(LDFLAGS1) $(OBJS) $(CFUNCTIONS) $(DCRAW) $(FILEEXR) $(FILEFLAC) $(AUDIOPULSE) $(FFMPEG_OBJS) $(FILECR3) $(AVIOBJS) $(CXXREPOSITORY) $(FFMPEG_LFLAGS) $(FFMPEG_LIBS) $(STATICLIBS) $(LIBS) $(LDFLAGS2) > $(OBJDIR)/objs)





# Static linking is not possible because the plugins depend on symbols
# in the main executable.
# Also VFS only overrides the C library when dynamic linking is used.
$(OUTPUT): $(OBJS) $(CFUNCTIONS) $(DCRAW) $(FILEEXR) $(FFMPEG_OBJS) $(FILECR3) $(FILEFLAC) $(AUDIOPULSE) $(THEME_DATA) ../guicast/$(OBJDIR)/libguicast.a
	$(LINKER) `cat $(OBJDIR)/objs`
#	strip $(OUTPUT)

$(OBJDIR)/test: 
	$(CC) -shared -o $(OBJDIR)/test.so \
		$(OBJDIR)/threadfork.o \
		../guicast/$(OBJDIR)/thread.o \
		../guicast/$(OBJDIR)/mutex.o
	$(CC) -o $(OBJDIR)/test test.C \
		`cat $(OBJDIR)/c_flags` \
		$(OBJDIR)/test.so \
		$(LIBS)

#$(SNDFILE_LIB):
#	mkdir -p $(SNDFILE_LIB) && \
#	cd $(SNDFILE_LIB) && \
#	ar x ../../$(SNDFILE_DIR)/src/.libs/libsndfile.a

clean:
	rm -rf $(OBJDIR)
	find \( -name core \
		-o -name '*.o' \
		-o -name '*.a' \
		-o -name '*.so' \) -exec rm -f {} \; -prune

wc:
	cat *.C *.h | wc

#install:
#	rm -f /usr/bin/cinelerra
#	cp $(OUTPUT) ../bin


$(LIBOBJS) $(OBJS):
	$(CC) `cat $(OBJDIR)/cxx_flags` $(subst $(OBJDIR)/,, $*.C) -o $*.o

$(AUDIOPULSE): audiopulse.C
	$(CC) `cat $(OBJDIR)/cxx_flags` -I$(PULSE_DIR)/src/ $(subst $(OBJDIR)/,, $*.C) -o $*.o


$(FILEEXR): fileexr.C
	$(CC) `cat $(OBJDIR)/cxx_flags` $(EXR_INCLUDES) $(subst $(OBJDIR)/,, $*.C) -o $*.o

$(FILEFLAC): fileflac.C
	$(CC) `cat $(OBJDIR)/cxx_flags` -I$(FLAC_DIR)/include $(subst $(OBJDIR)/,, $*.C) -o $*.o

$(FILECR3): filecr3.C
	$(CC) `cat $(OBJDIR)/cxx_flags` -I$(LIBRAW_DIR) $(subst $(OBJDIR)/,, $*.C) -o $*.o

$(FFMPEG_OBJS):
	$(CC) `cat $(OBJDIR)/ffmpeg_cflags` $(subst $(OBJDIR)/,, $*.C) -o $*.o

$(DCRAW): dcraw.c
	$(GCC) `cat $(OBJDIR)/c_flags` dcraw.c -o $*.o

$(CFUNCTIONS): cfunctions.c
	$(GCC) `cat $(OBJDIR)/c_flags` cfunctions.c -o $*.o

$(THEME_DATA):
	cd $(OBJDIR) && \
	../../guicast/$(OBJDIR)/bootstrap theme_data.o ../data/mode_*.png


$(OBJDIR)/aattachmentpoint.o: 			  aattachmentpoint.C
$(OBJDIR)/aautomation.o: 			  aautomation.C
$(OBJDIR)/aboutprefs.o:                           aboutprefs.C
$(OBJDIR)/adeviceprefs.o: 			  adeviceprefs.C
$(OBJDIR)/aedit.o: 				  aedit.C
$(OBJDIR)/aedits.o: 				  aedits.C
$(OBJDIR)/affine.o:                               affine.C
$(OBJDIR)/amodule.o: 				  amodule.C
$(OBJDIR)/apanel.o: 				  apanel.C
$(OBJDIR)/apatchgui.o:  			  apatchgui.C
$(OBJDIR)/apluginarray.o: 			  apluginarray.C
$(OBJDIR)/arender.o: 				  arender.C
$(OBJDIR)/asset.o:                                asset.C
$(OBJDIR)/assetedit.o:  			  assetedit.C
$(OBJDIR)/assetmanager.o: 			  assetmanager.C
$(OBJDIR)/assetpopup.o: 			  assetpopup.C
$(OBJDIR)/assetremove.o: 			  assetremove.C
$(OBJDIR)/assets.o: 				  assets.C
$(OBJDIR)/atrack.o: 				  atrack.C
$(OBJDIR)/attachmentpoint.o: 			  attachmentpoint.C
$(OBJDIR)/audio1394.o:  			  audio1394.C
$(OBJDIR)/audioalsa.o:  			  audioalsa.C
$(OBJDIR)/audiocine.o:                            audiocine.C
$(OBJDIR)/audioconfig.o: 			  audioconfig.C
$(OBJDIR)/audiodevice.o: 			  audiodevice.C
$(OBJDIR)/audiodvb.o: 			    	  audiodvb.C
$(OBJDIR)/audioesound.o: 			  audioesound.C
$(OBJDIR)/audioidevice.o: 			  audioidevice.C
$(OBJDIR)/audioinprefs.o: 			  audioinprefs.C
$(OBJDIR)/audioodevice.o: 			  audioodevice.C
$(OBJDIR)/audiooss.o: 				  audiooss.C
$(OBJDIR)/audiooutprefs.o: 			  audiooutprefs.C
$(OBJDIR)/auto.o: 				  auto.C
$(OBJDIR)/autos.o: 				  autos.C
$(OBJDIR)/autoconf.o: 				  autoconf.C
$(OBJDIR)/automation.o: 			  automation.C
$(OBJDIR)/avc1394control.o:		          avc1394control.C
$(OBJDIR)/avc1394transport.o:                     avc1394transport.C
$(OBJDIR)/awindow.o: 				  awindow.C
$(OBJDIR)/awindowgui.o: 			  awindowgui.C
$(OBJDIR)/awindowmenu.o: 			  awindowmenu.C
$(OBJDIR)/batch.o: 				  batch.C
$(OBJDIR)/batchrender.o:			  batchrender.C
$(OBJDIR)/bitspopup.o:  			  bitspopup.C
$(OBJDIR)/brender.o:                              brender.C
$(OBJDIR)/browsebutton.o: 			  browsebutton.C
$(OBJDIR)/byteorderpopup.o: 			  byteorderpopup.C
$(OBJDIR)/cache.o: 				  cache.C
$(OBJDIR)/cachebase.o:                            cachebase.C
$(OBJDIR)/canvas.o: 				  canvas.C
$(OBJDIR)/canvastools.o: 			  canvastools.C
$(OBJDIR)/cfunctions.o: 			  cfunctions.c
$(OBJDIR)/channel.o: 				  channel.C
$(OBJDIR)/channeldb.o: 				  channeldb.C
$(OBJDIR)/channeledit.o: 			  channeledit.C
$(OBJDIR)/channelpicker.o: 			  channelpicker.C
$(OBJDIR)/chantables.o: 			  chantables.C
$(OBJDIR)/clipedit.o: 			  	  clipedit.C
$(OBJDIR)/cicolors.o:                             cicolors.C
$(OBJDIR)/colorpicker.o:                          colorpicker.C
$(OBJDIR)/commonrender.o: 			  commonrender.C
$(OBJDIR)/compressortools.o:                      compressortools.C
$(OBJDIR)/confirmsave.o: 			  confirmsave.C
$(OBJDIR)/confirmquit.o: 			  confirmquit.C
$(OBJDIR)/console.o: 				  console.C
$(OBJDIR)/consolescroll.o: 			  consolescroll.C
$(OBJDIR)/cpanel.o: 				  cpanel.C
$(OBJDIR)/cplayback.o:  			  cplayback.C
$(OBJDIR)/cropvideo.o:  			  cropvideo.C
$(OBJDIR)/ctimebar.o:                             ctimebar.C
$(OBJDIR)/cursor.o: 				  cursor.C
$(OBJDIR)/cwindow.o: 				  cwindow.C
$(OBJDIR)/cwindowgui.o: 			  cwindowgui.C
$(OBJDIR)/cwindowtool.o: 			  cwindowtool.C
$(OBJDIR)/dcoffset.o: 				  dcoffset.C
$(OBJDIR)/dcraw.o:                                dcraw.c
$(OBJDIR)/deleteallindexes.o: 			  deleteallindexes.C
$(OBJDIR)/device1394input.o:                      device1394input.C
$(OBJDIR)/devicedvbinput.o:                       devicedvbinput.C
$(OBJDIR)/device1394output.o:                     device1394output.C
$(OBJDIR)/drivesync.o:  			  drivesync.C
$(OBJDIR)/dvbtune.o:  			  	  dvbtune.C
$(OBJDIR)/edit.o: 				  edit.C
$(OBJDIR)/editinfo.o: 				  editinfo.C
$(OBJDIR)/editkeyframe.o: 			  editkeyframe.C
$(OBJDIR)/edithandles.o: 			  edithandles.C
$(OBJDIR)/editlength.o:                           editlength.C
$(OBJDIR)/editpanel.o:  			  editpanel.C
$(OBJDIR)/editpopup.o:  			  editpopup.C
$(OBJDIR)/edits.o: 				  edits.C
$(OBJDIR)/edl.o: 				  edl.C
$(OBJDIR)/edlfactory.o: 			  edlfactory.C
$(OBJDIR)/edlsession.o: 			  edlsession.C
$(OBJDIR)/eqcanvas.o:                             eqcanvas.C
$(OBJDIR)/fadeengine.o: 			  fadeengine.C
$(OBJDIR)/file.o: 				  file.C
$(OBJDIR)/fileac3.o:                              fileac3.C
$(OBJDIR)/fileavi.o: 				  fileavi.C
$(OBJDIR)/filebase.o: 				  filebase.C
$(OBJDIR)/filebaseaudio.o: 			  filebaseaudio.C
$(OBJDIR)/filebaseima4.o: 			  filebaseima4.C
$(OBJDIR)/filebaseulaw.o: 			  filebaseulaw.C
$(OBJDIR)/filebasevideo.o: 			  filebasevideo.C
$(OBJDIR)/filecr2.o:                              filecr2.C
$(OBJDIR)/filecr3.o:                              filecr3.C
$(OBJDIR)/fileffmpeg.o:                           fileffmpeg.C
$(OBJDIR)/filefork.o:	 			  filefork.C
$(OBJDIR)/filescene.o:                            filescene.C
$(OBJDIR)/fileserver.o:                           fileserver.C
$(OBJDIR)/fileformat.o: 			  fileformat.C
$(OBJDIR)/filegif.o: 				  filegif.C
$(OBJDIR)/filejpeg.o: 				  filejpeg.C
$(OBJDIR)/filelist.o: 				  filelist.C
$(OBJDIR)/filemov.o: 				  filemov.C
$(OBJDIR)/filempeg.o:                             filempeg.C
$(OBJDIR)/fileogg.o:                              fileogg.C
$(OBJDIR)/filepng.o: 				  filepng.C
$(OBJDIR)/filepreviewer.o:                        filepreviewer.C
$(OBJDIR)/filesndfile.o: 			  filesndfile.C
$(OBJDIR)/filestdout.o: 			  filestdout.C
$(OBJDIR)/filetga.o: 			  	  filetga.C
$(OBJDIR)/filethread.o: 			  filethread.C
$(OBJDIR)/filetiff.o: 				  filetiff.C
$(OBJDIR)/filevorbis.o:                           filevorbis.C
$(OBJDIR)/filexml.o: 				  filexml.C
$(OBJDIR)/floatauto.o:  			  floatauto.C
$(OBJDIR)/floatautos.o: 			  floatautos.C
$(OBJDIR)/forkwrapper.o:                          forkwrapper.C
$(OBJDIR)/formatcheck.o: 			  formatcheck.C
$(OBJDIR)/formatpopup.o: 			  formatpopup.C
$(OBJDIR)/formatpresets.o:                        formatpresets.C
$(OBJDIR)/formattools.o: 			  formattools.C
$(OBJDIR)/fourier.o:                              fourier.C
$(OBJDIR)/framecache.o:                           framecache.C
$(OBJDIR)/garbage.o:                              garbage.C
$(OBJDIR)/gwindow.o:                              gwindow.C
$(OBJDIR)/gwindowgui.o:                           gwindowgui.C
$(OBJDIR)/iec61883input.o:                        iec61883input.C
$(OBJDIR)/iec61883output.o:                       iec61883output.C
$(OBJDIR)/indexfile.o:  			  indexfile.C
$(OBJDIR)/indexable.o:                            indexable.C
$(OBJDIR)/indexstate.o:                           indexstate.C
$(OBJDIR)/indexthread.o: 			  indexthread.C
$(OBJDIR)/intauto.o:                              intauto.C
$(OBJDIR)/intautos.o:                             intautos.C
$(OBJDIR)/interfaceprefs.o: 			  interfaceprefs.C
$(OBJDIR)/keyframe.o: 				  keyframe.C
$(OBJDIR)/keyframegui.o: 			  keyframegui.C
$(OBJDIR)/keyframepopup.o:                        keyframepopup.C
$(OBJDIR)/keyframes.o:  			  keyframes.C
$(OBJDIR)/keyframehandles.o: 			  keyframehandles.C
$(OBJDIR)/labelnavigate.o: 			  labelnavigate.C
$(OBJDIR)/labels.o: 				  labels.C
$(OBJDIR)/levelwindow.o: 			  levelwindow.C
$(OBJDIR)/levelwindowgui.o: 			  levelwindowgui.C
$(OBJDIR)/loadbalance.o: 			  loadbalance.C
$(OBJDIR)/loadfile.o: 				  loadfile.C
$(OBJDIR)/loadmode.o: 				  loadmode.C
$(OBJDIR)/localsession.o: 			  localsession.C
$(OBJDIR)/main.o:	  			  main.C
$(OBJDIR)/mainclock.o:  			  mainclock.C
$(OBJDIR)/maincursor.o: 			  maincursor.C
$(OBJDIR)/mainerror.o:                            mainerror.C
$(OBJDIR)/mainindexes.o: 			  mainindexes.C
$(OBJDIR)/mainmenu.o: 				  mainmenu.C
$(OBJDIR)/mainprogress.o: 			  mainprogress.C
$(OBJDIR)/mainsession.o: 			  mainsession.C
$(OBJDIR)/mainundo.o: 				  mainundo.C
$(OBJDIR)/maskauto.o:                             maskauto.C
$(OBJDIR)/maskautos.o:                            maskautos.C
$(OBJDIR)/maskengine.o:                           maskengine.C
$(OBJDIR)/mbuttons.o: 				  mbuttons.C
$(OBJDIR)/menuattacheffect.o:                     menuattacheffect.C
$(OBJDIR)/menuaeffects.o: 			  menuaeffects.C
$(OBJDIR)/menueditlength.o:                       menueditlength.C
$(OBJDIR)/menueffects.o: 			  menueffects.C
$(OBJDIR)/menutransitionlength.o:                 menutransitionlength.C
$(OBJDIR)/menuveffects.o: 			  menuveffects.C
$(OBJDIR)/messages.o: 				  messages.C
$(OBJDIR)/meterpanel.o: 			  meterpanel.C
$(OBJDIR)/module.o: 				  module.C
$(OBJDIR)/motioncache.o:                          motioncache.C
$(OBJDIR)/motionscan.o:                           motionscan.C
$(OBJDIR)/mtimebar.o:                             mtimebar.C
$(OBJDIR)/mwindow.o: 				  mwindow.C
$(OBJDIR)/mwindowedit.o: 			  mwindowedit.C
$(OBJDIR)/mwindowgui.o: 			  mwindowgui.C
$(OBJDIR)/mwindowmove.o: 			  mwindowmove.C
$(OBJDIR)/nestededls.o: 			  nestededls.C
$(OBJDIR)/new.o: 				  new.C
$(OBJDIR)/newfolder.o:  			  newfolder.C
$(OBJDIR)/overlayframe.o: 			  overlayframe.C
$(OBJDIR)/packagedispatcher.o:                    packagedispatcher.C
$(OBJDIR)/packagerenderer.o: 			  packagerenderer.C
$(OBJDIR)/panauto.o: 			          panauto.C
$(OBJDIR)/panautos.o: 			          panautos.C
$(OBJDIR)/panedividers.o: 			  panedividers.C
$(OBJDIR)/patch.o: 				  patch.C
$(OBJDIR)/patchbay.o: 				  patchbay.C
$(OBJDIR)/patchgui.o: 				  patchgui.C
$(OBJDIR)/performanceprefs.o: 			  performanceprefs.C
$(OBJDIR)/picture.o:                              picture.C
$(OBJDIR)/playabletracks.o: 			  playabletracks.C
$(OBJDIR)/playback3d.o:                           playback3d.C
$(OBJDIR)/playbackconfig.o: 			  playbackconfig.C
$(OBJDIR)/playbackengine.o: 			  playbackengine.C
$(OBJDIR)/playbackprefs.o:                        playbackprefs.C
$(OBJDIR)/playtransport.o: 			  playtransport.C
$(OBJDIR)/plugin.o: 				  plugin.C
$(OBJDIR)/pluginaclient.o: 			  pluginaclient.C
$(OBJDIR)/pluginaclientlad.o: 			  pluginaclientlad.C
$(OBJDIR)/pluginarray.o: 			  pluginarray.C
$(OBJDIR)/pluginclient.o: 			  pluginclient.C
$(OBJDIR)/plugindialog.o: 			  plugindialog.C
$(OBJDIR)/pluginpopup.o: 			  pluginpopup.C
$(OBJDIR)/pluginprefs.o: 			  pluginprefs.C
$(OBJDIR)/pluginset.o:  			  pluginset.C
$(OBJDIR)/pluginserver.o: 			  pluginserver.C
$(OBJDIR)/plugintclient.o: 			  plugintclient.C
$(OBJDIR)/plugintoggles.o:                        plugintoggles.C
$(OBJDIR)/pluginvclient.o: 			  pluginvclient.C
$(OBJDIR)/preferences.o: 			  preferences.C
$(OBJDIR)/preferencesthread.o:  		  preferencesthread.C
$(OBJDIR)/presets.o:                              presets.C
$(OBJDIR)/presetsgui.o:                           presetsgui.C
$(OBJDIR)/previewer.o: 			          previewer.C
$(OBJDIR)/proxy.o: 			  	  proxy.C
$(OBJDIR)/question.o: 				  question.C
$(OBJDIR)/quit.o: 				  quit.C
$(OBJDIR)/recconfirmdelete.o: 			  recconfirmdelete.C
$(OBJDIR)/record.o: 				  record.C
$(OBJDIR)/recordableatracks.o:  		  recordableatracks.C
$(OBJDIR)/recordablevtracks.o:  		  recordablevtracks.C
$(OBJDIR)/recordaudio.o: 			  recordaudio.C
$(OBJDIR)/recordconfig.o: 			  recordconfig.C
$(OBJDIR)/recordgui.o:  			  recordgui.C
$(OBJDIR)/recordlabel.o: 			  recordlabel.C
$(OBJDIR)/recordmonitor.o: 			  recordmonitor.C
$(OBJDIR)/recordprefs.o:                          recordprefs.C
$(OBJDIR)/recordpreview.o: 			  recordpreview.C
$(OBJDIR)/recordscopes.o: 			  recordscopes.C
$(OBJDIR)/recordthread.o: 			  recordthread.C
$(OBJDIR)/recordtransport.o: 			  recordtransport.C
$(OBJDIR)/recordvideo.o: 			  recordvideo.C
$(OBJDIR)/recordwindow.o: 			  recordwindow.C
$(OBJDIR)/removethread.o:                         removethread.C
$(OBJDIR)/render.o: 				  render.C
$(OBJDIR)/renderengine.o: 			  renderengine.C
$(OBJDIR)/renderfarm.o: 			  renderfarm.C
$(OBJDIR)/renderfarmclient.o: 			  renderfarmclient.C
$(OBJDIR)/renderfarmfsclient.o: 		  renderfarmfsclient.C
$(OBJDIR)/renderfarmfsserver.o: 		  renderfarmfsserver.C
$(OBJDIR)/resample.o: 				  resample.C
$(OBJDIR)/resizetrackthread.o:                    resizetrackthread.C
$(OBJDIR)/resourcepixmap.o: 			  resourcepixmap.C
$(OBJDIR)/resourcethread.o:                       resourcethread.C
$(OBJDIR)/samples.o: 			          samples.C
$(OBJDIR)/samplescroll.o: 			  samplescroll.C
$(OBJDIR)/save.o: 				  save.C
$(OBJDIR)/scopewindow.o:                          scopewindow.C
$(OBJDIR)/scenegraph.o:                           scenegraph.C
$(OBJDIR)/setformat.o:                            setformat.C
$(OBJDIR)/sharedlocation.o: 			  sharedlocation.C
$(OBJDIR)/shmemory.o: 				  shmemory.C
$(OBJDIR)/sighandler.o:                           sighandler.C
$(OBJDIR)/splashgui.o: 				  splashgui.C
$(OBJDIR)/statusbar.o:  			  statusbar.C
$(OBJDIR)/swapasset.o:  			  swapasset.C
$(OBJDIR)/theme.o: 				  theme.C
$(OBJDIR)/threadexec.o:                           threadexec.C
$(OBJDIR)/threadfork.o:                           threadfork.C
$(OBJDIR)/threadloader.o: 			  threadloader.C
$(OBJDIR)/timebar.o: 				  timebar.C
$(OBJDIR)/timebomb.o: 				  timebomb.C
$(OBJDIR)/timeentry.o:  			  timeentry.C
$(OBJDIR)/timelinepane.o:  			  timelinepane.C
$(OBJDIR)/tipwindow.o:                            tipwindow.C
$(OBJDIR)/toggleauto.o: 			  toggleauto.C
$(OBJDIR)/toggleautos.o: 			  toggleautos.C
$(OBJDIR)/track.o: 				  track.C
$(OBJDIR)/trackcanvas.o: 			  trackcanvas.C
$(OBJDIR)/tracking.o: 				  tracking.C
$(OBJDIR)/tracks.o: 				  tracks.C
$(OBJDIR)/trackscroll.o: 			  trackscroll.C
$(OBJDIR)/tracksedit.o: 			  tracksedit.C
$(OBJDIR)/transition.o: 			  transition.C
$(OBJDIR)/transitiondialog.o:                     transitiondialog.C
$(OBJDIR)/transitionhandles.o:  		  transitionhandles.C
$(OBJDIR)/transitionpopup.o: 			  transitionpopup.C
$(OBJDIR)/transportque.o: 			  transportque.C
$(OBJDIR)/tunerserver.o: 			  tunerserver.C
$(OBJDIR)/undostack.o:  			  undostack.C
$(OBJDIR)/vattachmentpoint.o: 			  vattachmentpoint.C
$(OBJDIR)/vautomation.o: 			  vautomation.C
$(OBJDIR)/vdevice1394.o: 			  vdevice1394.C
$(OBJDIR)/vdevicebase.o: 			  vdevicebase.C
$(OBJDIR)/vdevicebuz.o: 			  vdevicebuz.C
$(OBJDIR)/vdevicedvb.o:                           vdevicedvb.C
$(OBJDIR)/vdeviceprefs.o: 			  vdeviceprefs.C
$(OBJDIR)/vdevicepreview.o: 			  vdevicepreview.C
$(OBJDIR)/vdevicev4l.o: 			  vdevicev4l.C
$(OBJDIR)/vdevicev4l2.o:                          vdevicev4l2.C
$(OBJDIR)/vdevicev4l2jpeg.o:                      vdevicev4l2jpeg.C
$(OBJDIR)/vdevicex11.o: 			  vdevicex11.C
$(OBJDIR)/vedit.o: 				  vedit.C
$(OBJDIR)/vedits.o: 				  vedits.C
$(OBJDIR)/videoconfig.o: 			  videoconfig.C
$(OBJDIR)/videodevice.o: 			  videodevice.C
$(OBJDIR)/videoinprefs.o: 			  videoinprefs.C
$(OBJDIR)/videooutprefs.o: 			  videooutprefs.C
$(OBJDIR)/videowindow.o: 			  videowindow.C
$(OBJDIR)/videowindowgui.o: 			  videowindowgui.C
$(OBJDIR)/viewmenu.o: 				  viewmenu.C
$(OBJDIR)/virtualnode.o: 			  virtualnode.C
$(OBJDIR)/virtualaconsole.o: 			  virtualaconsole.C
$(OBJDIR)/virtualanode.o: 			  virtualanode.C
$(OBJDIR)/virtualconsole.o: 			  virtualconsole.C
$(OBJDIR)/virtualvconsole.o: 			  virtualvconsole.C
$(OBJDIR)/virtualvnode.o: 			  virtualvnode.C
$(OBJDIR)/vmodule.o: 				  vmodule.C
$(OBJDIR)/vpatchgui.o:  			  vpatchgui.C
$(OBJDIR)/vplayback.o:  			  vplayback.C
$(OBJDIR)/vplugin.o: 				  vplugin.C
$(OBJDIR)/vpluginarray.o: 			  vpluginarray.C
$(OBJDIR)/vpluginset.o: 			  vpluginset.C
$(OBJDIR)/vtimebar.o:                             vtimebar.C
$(OBJDIR)/vrender.o: 				  vrender.C
$(OBJDIR)/vtrack.o: 				  vtrack.C
$(OBJDIR)/vwindow.o: 				  vwindow.C
$(OBJDIR)/vwindowgui.o: 			  vwindowgui.C
$(OBJDIR)/wavecache.o:                            wavecache.C
$(OBJDIR)/zoombar.o: 				  zoombar.C
$(OBJDIR)/zoompanel.o:  			  zoompanel.C


#$(OBJDIR)/aplugin.o: 				  aplugin.C
#$(OBJDIR)/vplugin.o \
#$(OBJDIR)/vpluginset.o \
#$(OBJDIR)/apluginset.o: 			  apluginset.C
