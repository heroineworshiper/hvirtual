# Makefile.in generated automatically by automake 1.5 from Makefile.am.

# Copyright 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2001
# Free Software Foundation, Inc.
# This Makefile.in is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.




# global section for every Makefile.am

SHELL = /bin/sh

srcdir = .
top_srcdir = ..

prefix = /usr/local
exec_prefix = ${prefix}

bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
libexecdir = ${exec_prefix}/libexec
datadir = ${prefix}/share
sysconfdir = ${prefix}/etc
sharedstatedir = ${prefix}/com
localstatedir = ${prefix}/var
libdir = ${exec_prefix}/lib
infodir = ${prefix}/info
mandir = ${prefix}/man
includedir = ${prefix}/include
oldincludedir = /usr/include
pkgdatadir = $(datadir)/lame
pkglibdir = $(libdir)/lame
pkgincludedir = $(includedir)/lame
top_builddir = ..

ACLOCAL = ${SHELL} /root/hvirtual/quicktime/lame-3.93.1/missing --run aclocal
AUTOCONF = ${SHELL} /root/hvirtual/quicktime/lame-3.93.1/missing --run autoconf
AUTOMAKE = ${SHELL} /root/hvirtual/quicktime/lame-3.93.1/missing --run automake
AUTOHEADER = ${SHELL} /root/hvirtual/quicktime/lame-3.93.1/missing --run autoheader

INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_SCRIPT = ${INSTALL}
INSTALL_HEADER = $(INSTALL_DATA)
transform = s,x,x,
NORMAL_INSTALL = :
PRE_INSTALL = :
POST_INSTALL = :
NORMAL_UNINSTALL = :
PRE_UNINSTALL = :
POST_UNINSTALL = :
host_alias = 
host_triplet = i686-pc-linux-gnu
AMTAR = ${SHELL} /root/hvirtual/quicktime/lame-3.93.1/missing --run tar
AS = @AS@
AWK = gawk
CC = gcc
CONFIG_DEFS = 
CPP = gcc -E
CPUTYPE = i386
DEPDIR = .deps
DLLTOOL = @DLLTOOL@
EXEEXT = 
FRONTEND_CFLAGS = 
FRONTEND_LDADD = -lncurses  -lm
FRONTEND_LDFLAGS = 
GTK_CFLAGS = 
GTK_CONFIG = gtk12-config
GTK_LIBS = 
INSTALL_STRIP_PROGRAM = ${SHELL} $(install_sh) -c -s
LIBTOOL = $(SHELL) $(top_builddir)/libtool
LIBTOOL_DEPS = ./ltconfig ./ltmain.sh
LIB_MAJOR_VERSION = 0
LIB_MINOR_VERSION = 0
LN_S = ln -s
MAINT = #
MAKEDEP = -M
NASM = no
NASM_FORMAT = -f elf
OBJDUMP = @OBJDUMP@
OBJEXT = o
PACKAGE = lame
RANLIB = ranlib
RM_F = rm -f
U = 
VERSION = 3.93.1
WITH_FRONTEND = lame
WITH_MP3RTP = 
WITH_MP3X = 
am__include = include
am__quote = 
install_sh = /root/hvirtual/quicktime/lame-3.93.1/install-sh

INCLUDES = -I$(top_srcdir)/libmp3lame -I$(top_srcdir)/include -I$(top_builddir)

LDADD =  \
	$(top_builddir)/libmp3lame/libmp3lame.la \
	-lncurses  -lm


AUTOMAKE_OPTIONS = 1.5 foreign $(top_srcdir)/ansi2knr

bin_PROGRAMS = lame  
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


DEFS = -DHAVE_CONFIG_H 

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

lame__EXEEXT__SOURCES = main.c $(common_sources) $(brhist_sources)
#lame__EXEEXT__SOURCES = main.c $(common_sources)
mp3rtp__EXEEXT__SOURCES = mp3rtp.c rtp.c rtp.h $(common_sources) \
	$(brhist_sources)

#mp3rtp__EXEEXT__SOURCES = mp3rtp.c rtp.c rtp.h $(common_sources)
mp3x__EXEEXT__SOURCES = mp3x.c gtkanal.c gpkplotting.c \
	$(common_sources) $(brhist_sources)

#mp3x__EXEEXT__SOURCES = mp3x.c gtkanal.c gpkplotting.c $(common_sources)

CFLAGS =  -Wall -O3 -march=i686 -fmessage-length=0 -funroll-all-loops -fomit-frame-pointer -malign-loops=2 -malign-jumps=2 -malign-functions=2 -I/usr/local/include  
LDFLAGS =   -static

mp3x__EXEEXT__LDADD = $(LDADD) 

CLEANFILES = lclint.txt $(EXTRA_PROGRAMS)

LCLINTFLAGS = \
	+posixlib \
	+showsummary \
	+showalluses \
	+whichlib \
	+forcehints \
	-fixedformalarray \
	+matchanyintegral \
	-Dlint

subdir = frontend
mkinstalldirs = $(SHELL) $(top_srcdir)/mkinstalldirs
CONFIG_HEADER = $(top_builddir)/config.h
CONFIG_CLEAN_FILES =
EXTRA_PROGRAMS = lame$(EXEEXT) mp3rtp$(EXEEXT) mp3x$(EXEEXT)
bin_PROGRAMS = lame  
PROGRAMS = $(bin_PROGRAMS)

am_lame__EXEEXT__OBJECTS = main$U.$(OBJEXT) \
	get_audio$U.$(OBJEXT) lametime$U.$(OBJEXT) \
	parse$U.$(OBJEXT) portableio$U.$(OBJEXT) \
	timestatus$U.$(OBJEXT) brhist$U.$(OBJEXT)
#am_lame__EXEEXT__OBJECTS = main$U.$(OBJEXT) \
#	get_audio$U.$(OBJEXT) lametime$U.$(OBJEXT) \
#	parse$U.$(OBJEXT) portableio$U.$(OBJEXT) \
#	timestatus$U.$(OBJEXT)
lame__EXEEXT__OBJECTS = $(am_lame__EXEEXT__OBJECTS)
lame__EXEEXT__LDADD = $(LDADD)
lame__EXEEXT__DEPENDENCIES = $(top_builddir)/libmp3lame/libmp3lame.la
lame__EXEEXT__LDFLAGS =
am_mp3rtp__EXEEXT__OBJECTS = mp3rtp$U.$(OBJEXT) \
	rtp$U.$(OBJEXT) get_audio$U.$(OBJEXT) \
	lametime$U.$(OBJEXT) parse$U.$(OBJEXT) \
	portableio$U.$(OBJEXT) timestatus$U.$(OBJEXT) \
	brhist$U.$(OBJEXT)
#am_mp3rtp__EXEEXT__OBJECTS = mp3rtp$U.$(OBJEXT) \
#	rtp$U.$(OBJEXT) get_audio$U.$(OBJEXT) \
#	lametime$U.$(OBJEXT) parse$U.$(OBJEXT) \
#	portableio$U.$(OBJEXT) \
#	timestatus$U.$(OBJEXT)
mp3rtp__EXEEXT__OBJECTS = $(am_mp3rtp__EXEEXT__OBJECTS)
mp3rtp__EXEEXT__LDADD = $(LDADD)
mp3rtp__EXEEXT__DEPENDENCIES = $(top_builddir)/libmp3lame/libmp3lame.la
mp3rtp__EXEEXT__LDFLAGS =
am_mp3x__EXEEXT__OBJECTS = mp3x$U.$(OBJEXT) \
	gtkanal$U.$(OBJEXT) gpkplotting$U.$(OBJEXT) \
	get_audio$U.$(OBJEXT) lametime$U.$(OBJEXT) \
	parse$U.$(OBJEXT) portableio$U.$(OBJEXT) \
	timestatus$U.$(OBJEXT) brhist$U.$(OBJEXT)
#am_mp3x__EXEEXT__OBJECTS = mp3x$U.$(OBJEXT) \
#	gtkanal$U.$(OBJEXT) gpkplotting$U.$(OBJEXT) \
#	get_audio$U.$(OBJEXT) lametime$U.$(OBJEXT) \
#	parse$U.$(OBJEXT) portableio$U.$(OBJEXT) \
#	timestatus$U.$(OBJEXT)
mp3x__EXEEXT__OBJECTS = $(am_mp3x__EXEEXT__OBJECTS)
mp3x__EXEEXT__DEPENDENCIES = $(top_builddir)/libmp3lame/libmp3lame.la
mp3x__EXEEXT__LDFLAGS =
DEFAULT_INCLUDES =  -I. -I$(srcdir) -I$(top_builddir)
CPPFLAGS = 
LIBS = 
depcomp = $(SHELL) $(top_srcdir)/depcomp
DEP_FILES = $(DEPDIR)/brhist$U.Po $(DEPDIR)/get_audio$U.Po \
	$(DEPDIR)/gpkplotting$U.Po $(DEPDIR)/gtkanal$U.Po \
	$(DEPDIR)/lametime$U.Po $(DEPDIR)/main$U.Po \
	$(DEPDIR)/mp3rtp$U.Po $(DEPDIR)/mp3x$U.Po \
	$(DEPDIR)/parse$U.Po $(DEPDIR)/portableio$U.Po \
	$(DEPDIR)/rtp$U.Po $(DEPDIR)/timestatus$U.Po
COMPILE = $(CC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
	$(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS)
LTCOMPILE = $(LIBTOOL) --mode=compile $(CC) $(DEFS) $(DEFAULT_INCLUDES) \
	$(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS)
CCLD = $(CC)
LINK = $(LIBTOOL) --mode=link $(CCLD) $(AM_CFLAGS) $(CFLAGS) \
	$(AM_LDFLAGS) $(LDFLAGS) -o $@
DIST_SOURCES = $(lame__EXEEXT__SOURCES) $(EXTRA_lame__EXEEXT__SOURCES) \
	$(mp3rtp__EXEEXT__SOURCES) $(EXTRA_mp3rtp__EXEEXT__SOURCES) \
	$(mp3x__EXEEXT__SOURCES) $(EXTRA_mp3x__EXEEXT__SOURCES)
HEADERS = $(noinst_HEADERS)

DIST_COMMON = $(noinst_HEADERS) Makefile.am Makefile.in depcomp
SOURCES = $(lame__EXEEXT__SOURCES) $(EXTRA_lame__EXEEXT__SOURCES) $(mp3rtp__EXEEXT__SOURCES) $(EXTRA_mp3rtp__EXEEXT__SOURCES) $(mp3x__EXEEXT__SOURCES) $(EXTRA_mp3x__EXEEXT__SOURCES)

all: all-am

.SUFFIXES:
.SUFFIXES: .c .lo .o .obj

mostlyclean-libtool:
	-rm -f *.lo

clean-libtool:
	-rm -rf .libs _libs

distclean-libtool:
	-rm -f libtool
$(srcdir)/Makefile.in: # Makefile.am $(top_srcdir)/Makefile.am.global $(top_srcdir)/configure.in $(ACLOCAL_M4)
	cd $(top_srcdir) && \
	  $(AUTOMAKE) --foreign  frontend/Makefile
Makefile: # $(srcdir)/Makefile.in  $(top_builddir)/config.status
	cd $(top_builddir) && \
	  CONFIG_HEADERS= CONFIG_LINKS= \
	  CONFIG_FILES=$(subdir)/$@ $(SHELL) ./config.status
install-binPROGRAMS: $(bin_PROGRAMS)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(bindir)
	@list='$(bin_PROGRAMS)'; for p in $$list; do \
	  p1=`echo $$p|sed 's/$(EXEEXT)$$//'`; \
	  if test -f $$p \
	     || test -f $$p1 \
	  ; then \
	    f=`echo $$p1|sed '$(transform);s/$$/$(EXEEXT)/'`; \
	   echo " $(INSTALL_PROGRAM_ENV) $(LIBTOOL) --mode=install $(INSTALL_PROGRAM) $$p $(DESTDIR)$(bindir)/$$f"; \
	   $(INSTALL_PROGRAM_ENV) $(LIBTOOL) --mode=install $(INSTALL_PROGRAM) $$p $(DESTDIR)$(bindir)/$$f; \
	  else :; fi; \
	done

uninstall-binPROGRAMS:
	@$(NORMAL_UNINSTALL)
	@list='$(bin_PROGRAMS)'; for p in $$list; do \
	  f=`echo $$p|sed 's/$(EXEEXT)$$//;$(transform);s/$$/$(EXEEXT)/'`; \
	  echo " rm -f $(DESTDIR)$(bindir)/$$f"; \
	  rm -f $(DESTDIR)$(bindir)/$$f; \
	done

clean-binPROGRAMS:
	-test -z "$(bin_PROGRAMS)" || rm -f $(bin_PROGRAMS)
lame$(EXEEXT): $(lame__EXEEXT__OBJECTS) $(lame__EXEEXT__DEPENDENCIES) 
	@rm -f lame$(EXEEXT)
	$(LINK) $(lame__EXEEXT__LDFLAGS) $(lame__EXEEXT__OBJECTS) $(lame__EXEEXT__LDADD) $(LIBS)
mp3rtp$(EXEEXT): $(mp3rtp__EXEEXT__OBJECTS) $(mp3rtp__EXEEXT__DEPENDENCIES) 
	@rm -f mp3rtp$(EXEEXT)
	$(LINK) $(mp3rtp__EXEEXT__LDFLAGS) $(mp3rtp__EXEEXT__OBJECTS) $(mp3rtp__EXEEXT__LDADD) $(LIBS)
mp3x$(EXEEXT): $(mp3x__EXEEXT__OBJECTS) $(mp3x__EXEEXT__DEPENDENCIES) 
	@rm -f mp3x$(EXEEXT)
	$(LINK) $(mp3x__EXEEXT__LDFLAGS) $(mp3x__EXEEXT__OBJECTS) $(mp3x__EXEEXT__LDADD) $(LIBS)

mostlyclean-compile:
	-rm -f *.$(OBJEXT) core *.core

distclean-compile:
	-rm -f *.tab.c

ANSI2KNR = $(top_srcdir)/ansi2knr
$(top_srcdir)/ansi2knr:
	cd $(top_srcdir) && $(MAKE) $(AM_MAKEFLAGS) ansi2knr

mostlyclean-kr:
	-rm -f *_.c

include $(DEPDIR)/brhist$U.Po
include $(DEPDIR)/get_audio$U.Po
include $(DEPDIR)/gpkplotting$U.Po
include $(DEPDIR)/gtkanal$U.Po
include $(DEPDIR)/lametime$U.Po
include $(DEPDIR)/main$U.Po
include $(DEPDIR)/mp3rtp$U.Po
include $(DEPDIR)/mp3x$U.Po
include $(DEPDIR)/parse$U.Po
include $(DEPDIR)/portableio$U.Po
include $(DEPDIR)/rtp$U.Po
include $(DEPDIR)/timestatus$U.Po

distclean-depend:
	-rm -rf $(DEPDIR)

.c.o:
	source='$<' object='$@' libtool=no \
	depfile='$(DEPDIR)/$*.Po' tmpdepfile='$(DEPDIR)/$*.TPo' \
	$(CCDEPMODE) $(depcomp) \
	$(COMPILE) -c `test -f $< || echo '$(srcdir)/'`$<

.c.obj:
	source='$<' object='$@' libtool=no \
	depfile='$(DEPDIR)/$*.Po' tmpdepfile='$(DEPDIR)/$*.TPo' \
	$(CCDEPMODE) $(depcomp) \
	$(COMPILE) -c `cygpath -w $<`

.c.lo:
	source='$<' object='$@' libtool=yes \
	depfile='$(DEPDIR)/$*.Plo' tmpdepfile='$(DEPDIR)/$*.TPlo' \
	$(CCDEPMODE) $(depcomp) \
	$(LTCOMPILE) -c -o $@ `test -f $< || echo '$(srcdir)/'`$<
CCDEPMODE = depmode=gcc3
brhist_.c: brhist.c $(ANSI2KNR)
	$(CPP) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) `if test -f $(srcdir)/brhist.c; then echo $(srcdir)/brhist.c; else echo brhist.c; fi` | sed 's/^# \([0-9]\)/#line \1/' | $(ANSI2KNR) > brhist_.c || rm -f brhist_.c
get_audio_.c: get_audio.c $(ANSI2KNR)
	$(CPP) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) `if test -f $(srcdir)/get_audio.c; then echo $(srcdir)/get_audio.c; else echo get_audio.c; fi` | sed 's/^# \([0-9]\)/#line \1/' | $(ANSI2KNR) > get_audio_.c || rm -f get_audio_.c
gpkplotting_.c: gpkplotting.c $(ANSI2KNR)
	$(CPP) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) `if test -f $(srcdir)/gpkplotting.c; then echo $(srcdir)/gpkplotting.c; else echo gpkplotting.c; fi` | sed 's/^# \([0-9]\)/#line \1/' | $(ANSI2KNR) > gpkplotting_.c || rm -f gpkplotting_.c
gtkanal_.c: gtkanal.c $(ANSI2KNR)
	$(CPP) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) `if test -f $(srcdir)/gtkanal.c; then echo $(srcdir)/gtkanal.c; else echo gtkanal.c; fi` | sed 's/^# \([0-9]\)/#line \1/' | $(ANSI2KNR) > gtkanal_.c || rm -f gtkanal_.c
lametime_.c: lametime.c $(ANSI2KNR)
	$(CPP) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) `if test -f $(srcdir)/lametime.c; then echo $(srcdir)/lametime.c; else echo lametime.c; fi` | sed 's/^# \([0-9]\)/#line \1/' | $(ANSI2KNR) > lametime_.c || rm -f lametime_.c
main_.c: main.c $(ANSI2KNR)
	$(CPP) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) `if test -f $(srcdir)/main.c; then echo $(srcdir)/main.c; else echo main.c; fi` | sed 's/^# \([0-9]\)/#line \1/' | $(ANSI2KNR) > main_.c || rm -f main_.c
mp3rtp_.c: mp3rtp.c $(ANSI2KNR)
	$(CPP) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) `if test -f $(srcdir)/mp3rtp.c; then echo $(srcdir)/mp3rtp.c; else echo mp3rtp.c; fi` | sed 's/^# \([0-9]\)/#line \1/' | $(ANSI2KNR) > mp3rtp_.c || rm -f mp3rtp_.c
mp3x_.c: mp3x.c $(ANSI2KNR)
	$(CPP) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) `if test -f $(srcdir)/mp3x.c; then echo $(srcdir)/mp3x.c; else echo mp3x.c; fi` | sed 's/^# \([0-9]\)/#line \1/' | $(ANSI2KNR) > mp3x_.c || rm -f mp3x_.c
parse_.c: parse.c $(ANSI2KNR)
	$(CPP) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) `if test -f $(srcdir)/parse.c; then echo $(srcdir)/parse.c; else echo parse.c; fi` | sed 's/^# \([0-9]\)/#line \1/' | $(ANSI2KNR) > parse_.c || rm -f parse_.c
portableio_.c: portableio.c $(ANSI2KNR)
	$(CPP) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) `if test -f $(srcdir)/portableio.c; then echo $(srcdir)/portableio.c; else echo portableio.c; fi` | sed 's/^# \([0-9]\)/#line \1/' | $(ANSI2KNR) > portableio_.c || rm -f portableio_.c
rtp_.c: rtp.c $(ANSI2KNR)
	$(CPP) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) `if test -f $(srcdir)/rtp.c; then echo $(srcdir)/rtp.c; else echo rtp.c; fi` | sed 's/^# \([0-9]\)/#line \1/' | $(ANSI2KNR) > rtp_.c || rm -f rtp_.c
timestatus_.c: timestatus.c $(ANSI2KNR)
	$(CPP) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) `if test -f $(srcdir)/timestatus.c; then echo $(srcdir)/timestatus.c; else echo timestatus.c; fi` | sed 's/^# \([0-9]\)/#line \1/' | $(ANSI2KNR) > timestatus_.c || rm -f timestatus_.c
brhist_.$(OBJEXT) brhist_.lo get_audio_.$(OBJEXT) get_audio_.lo \
gpkplotting_.$(OBJEXT) gpkplotting_.lo gtkanal_.$(OBJEXT) gtkanal_.lo \
lametime_.$(OBJEXT) lametime_.lo main_.$(OBJEXT) main_.lo \
mp3rtp_.$(OBJEXT) mp3rtp_.lo mp3x_.$(OBJEXT) mp3x_.lo parse_.$(OBJEXT) \
parse_.lo portableio_.$(OBJEXT) portableio_.lo rtp_.$(OBJEXT) rtp_.lo \
timestatus_.$(OBJEXT) timestatus_.lo : $(ANSI2KNR)
uninstall-info-am:

tags: TAGS

ID: $(HEADERS) $(SOURCES) $(LISP) $(TAGS_FILES)
	list='$(SOURCES) $(HEADERS) $(TAGS_FILES)'; \
	unique=`for i in $$list; do \
	    if test -f "$$i"; then echo $$i; else echo $(srcdir)/$$i; fi; \
	  done | \
	  $(AWK) '    { files[$$0] = 1; } \
	       END { for (i in files) print i; }'`; \
	mkid -fID $$unique $(LISP)

TAGS:  $(HEADERS) $(SOURCES)  $(TAGS_DEPENDENCIES) \
		$(TAGS_FILES) $(LISP)
	tags=; \
	here=`pwd`; \
	list='$(SOURCES) $(HEADERS) $(TAGS_FILES)'; \
	unique=`for i in $$list; do \
	    if test -f "$$i"; then echo $$i; else echo $(srcdir)/$$i; fi; \
	  done | \
	  $(AWK) '    { files[$$0] = 1; } \
	       END { for (i in files) print i; }'`; \
	test -z "$(ETAGS_ARGS)$$unique$(LISP)$$tags" \
	  || etags $(ETAGS_ARGS) $$tags  $$unique $(LISP)

GTAGS:
	here=`CDPATH=: && cd $(top_builddir) && pwd` \
	  && cd $(top_srcdir) \
	  && gtags -i $(GTAGS_ARGS) $$here

distclean-tags:
	-rm -f TAGS ID GTAGS GRTAGS GSYMS GPATH

DISTFILES = $(DIST_COMMON) $(DIST_SOURCES) $(TEXINFOS) $(EXTRA_DIST)

top_distdir = ..
distdir = $(top_distdir)/$(PACKAGE)-$(VERSION)

distdir: $(DISTFILES)
	@for file in $(DISTFILES); do \
	  if test -f $$file; then d=.; else d=$(srcdir); fi; \
	  dir=`echo "$$file" | sed -e 's,/[^/]*$$,,'`; \
	  if test "$$dir" != "$$file" && test "$$dir" != "."; then \
	    $(mkinstalldirs) "$(distdir)/$$dir"; \
	  fi; \
	  if test -d $$d/$$file; then \
	    cp -pR $$d/$$file $(distdir) \
	    || exit 1; \
	  else \
	    test -f $(distdir)/$$file \
	    || cp -p $$d/$$file $(distdir)/$$file \
	    || exit 1; \
	  fi; \
	done
check-am: all-am
check: check-am
all-am: Makefile $(PROGRAMS) $(HEADERS)

installdirs:
	$(mkinstalldirs) $(DESTDIR)$(bindir)

install: install-am
install-exec: install-exec-am
install-data: install-data-am
uninstall: uninstall-am

install-am: all-am
	@$(MAKE) $(AM_MAKEFLAGS) install-exec-am install-data-am

installcheck: installcheck-am
install-strip:
	$(MAKE) $(AM_MAKEFLAGS) INSTALL_PROGRAM="$(INSTALL_STRIP_PROGRAM)" \
	  `test -z '$(STRIP)' || \
	    echo "INSTALL_PROGRAM_ENV=STRIPPROG='$(STRIP)'"` install
mostlyclean-generic:

clean-generic:
	-test -z "$(CLEANFILES)" || rm -f $(CLEANFILES)

distclean-generic:
	-rm -f Makefile $(CONFIG_CLEAN_FILES) stamp-h stamp-h[0-9]*

maintainer-clean-generic:
	@echo "This command is intended for maintainers to use"
	@echo "it deletes files that may require special tools to rebuild."
clean: clean-am

clean-am: clean-binPROGRAMS clean-generic clean-libtool mostlyclean-am

distclean: distclean-am

distclean-am: clean-am distclean-compile distclean-depend \
	distclean-generic distclean-libtool distclean-tags

dvi: dvi-am

dvi-am:

info: info-am

info-am:

install-data-am:

install-exec-am: install-binPROGRAMS

install-info: install-info-am

install-man:

installcheck-am:

maintainer-clean: maintainer-clean-am

maintainer-clean-am: distclean-am maintainer-clean-generic

mostlyclean: mostlyclean-am

mostlyclean-am: mostlyclean-compile mostlyclean-generic mostlyclean-kr \
	mostlyclean-libtool

uninstall-am: uninstall-binPROGRAMS uninstall-info-am

.PHONY: GTAGS all all-am check check-am clean clean-binPROGRAMS \
	clean-generic clean-libtool distclean distclean-compile \
	distclean-depend distclean-generic distclean-libtool \
	distclean-tags distdir dvi dvi-am info info-am install \
	install-am install-binPROGRAMS install-data install-data-am \
	install-exec install-exec-am install-info install-info-am \
	install-man install-strip installcheck installcheck-am \
	installdirs maintainer-clean maintainer-clean-generic \
	mostlyclean mostlyclean-compile mostlyclean-generic \
	mostlyclean-kr mostlyclean-libtool tags uninstall uninstall-am \
	uninstall-binPROGRAMS uninstall-info-am


# end global section

lclint.txt: ${lame_SOURCES} ${noinst_HEADERS}
	@lclint ${LCLINTFLAGS} ${INCLUDES} ${DEFS} ${lame_SOURCES} 2>&1 >lclint.txt || true

lclint: lclint.txt
	more lclint.txt
# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
