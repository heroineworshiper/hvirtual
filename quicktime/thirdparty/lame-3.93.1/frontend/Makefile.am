## $Id: Makefile.am,v 1.1.1.1 2003/10/14 07:54:37 heroine Exp $

include $(top_srcdir)/Makefile.am.global

bin_PROGRAMS = @WITH_FRONTEND@ @WITH_MP3RTP@ @WITH_MP3X@
EXTRA_PROGRAMS = lame$(EXEEXT) mp3rtp$(EXEEXT) mp3x$(EXEEXT)

brhist_sources = brhist.c brhist.h

EXTRA_DIST = \
	$(brhist_sources) \
	console.c \
	console.h \
	lame.dsp \
	lame.dsw \
	lame_vc6.dsp \
	mp3x.dsp \
	mp3x.dsw \
	amiga_mpega.c

DEFS = @DEFS@ @CONFIG_DEFS@

common_sources = \
	get_audio.c \
	lametime.c \
	parse.c \
	portableio.c \
	timestatus.c

noinst_HEADERS = get_audio.h \
	gtkanal.h \
	gpkplotting.h \
	lametime.h \
	main.h \
	parse.h \
	portableio.h \
	timestatus.h

EXTRA_lame__EXEEXT__SOURCES = $(brhist_sources)
EXTRA_mp3rtp__EXEEXT__SOURCES = $(brhist_sources)
EXTRA_mp3x__EXEEXT__SOURCES = $(brhist_sources)

if WITH_BRHIST
lame__EXEEXT__SOURCES = main.c $(common_sources) $(brhist_sources)
mp3rtp__EXEEXT__SOURCES = mp3rtp.c rtp.c rtp.h $(common_sources) \
	$(brhist_sources)
mp3x__EXEEXT__SOURCES = mp3x.c gtkanal.c gpkplotting.c \
	$(common_sources) $(brhist_sources)
else
lame__EXEEXT__SOURCES = main.c $(common_sources)
mp3rtp__EXEEXT__SOURCES = mp3rtp.c rtp.c rtp.h $(common_sources)
mp3x__EXEEXT__SOURCES = mp3x.c gtkanal.c gpkplotting.c $(common_sources)
endif

CFLAGS = @CFLAGS@ @GTK_CFLAGS@ @FRONTEND_CFLAGS@
LDFLAGS = @LDFLAGS@ @FRONTEND_LDFLAGS@ -static

INCLUDES = -I$(top_srcdir)/libmp3lame -I$(top_srcdir)/include -I$(top_builddir)

LDADD = @LDADD@ \
	$(top_builddir)/libmp3lame/libmp3lame.la \
	@FRONTEND_LDADD@

mp3x__EXEEXT__LDADD = $(LDADD) @GTK_LIBS@

CLEANFILES = lclint.txt $(EXTRA_PROGRAMS)

LCLINTFLAGS= \
	+posixlib \
	+showsummary \
	+showalluses \
	+whichlib \
	+forcehints \
	-fixedformalarray \
	+matchanyintegral \
	-Dlint

lclint.txt: ${lame_SOURCES} ${noinst_HEADERS}
	@lclint ${LCLINTFLAGS} ${INCLUDES} ${DEFS} ${lame_SOURCES} 2>&1 >lclint.txt || true

lclint: lclint.txt
	more lclint.txt
