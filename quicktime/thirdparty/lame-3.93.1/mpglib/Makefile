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
LDADD = 
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

INCLUDES = -I$(top_srcdir)/include -I$(srcdir) -I$(top_srcdir)/libmp3lame -I$(top_builddir)

AUTOMAKE_OPTIONS = 1.5 foreign $(top_srcdir)/ansi2knr

EXTRA_DIST = \
	mpglib.dsp \
	mpglib_vc6.dsp


DEFS = -DHAVE_CONFIG_H 

noinst_LTLIBRARIES = libmpgdecoder.la

libmpgdecoder_la_SOURCES = common.c \
	dct64_i386.c \
	decode_i386.c \
	interface.c \
	layer1.c \
	layer2.c \
	layer3.c \
	tabinit.c


noinst_HEADERS = common.h \
	dct64_i386.h \
	decode_i386.h \
	huffman.h \
	interface.h \
	l2tables.h \
	layer1.h \
	layer2.h \
	layer3.h \
	mpg123.h \
	mpglib.h \
	tabinit.h


LCLINTFLAGS = \
	+posixlib \
	+showsummary \
	+showalluses \
	+whichlib \
	+forcehints \
	-fixedformalarray \
	+matchanyintegral \
	-Dlint

subdir = mpglib
mkinstalldirs = $(SHELL) $(top_srcdir)/mkinstalldirs
CONFIG_HEADER = $(top_builddir)/config.h
CONFIG_CLEAN_FILES =
LTLIBRARIES = $(noinst_LTLIBRARIES)

libmpgdecoder_la_LDFLAGS =
libmpgdecoder_la_LIBADD =
am_libmpgdecoder_la_OBJECTS = common$U.lo dct64_i386$U.lo \
	decode_i386$U.lo interface$U.lo layer1$U.lo layer2$U.lo \
	layer3$U.lo tabinit$U.lo
libmpgdecoder_la_OBJECTS = $(am_libmpgdecoder_la_OBJECTS)
DEFAULT_INCLUDES =  -I. -I$(srcdir) -I$(top_builddir)
CPPFLAGS = 
LDFLAGS = 
LIBS = 
depcomp = $(SHELL) $(top_srcdir)/depcomp
DEP_FILES = $(DEPDIR)/common$U.Plo \
	$(DEPDIR)/dct64_i386$U.Plo \
	$(DEPDIR)/decode_i386$U.Plo \
	$(DEPDIR)/interface$U.Plo $(DEPDIR)/layer1$U.Plo \
	$(DEPDIR)/layer2$U.Plo $(DEPDIR)/layer3$U.Plo \
	$(DEPDIR)/tabinit$U.Plo
COMPILE = $(CC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
	$(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS)
LTCOMPILE = $(LIBTOOL) --mode=compile $(CC) $(DEFS) $(DEFAULT_INCLUDES) \
	$(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS)
CCLD = $(CC)
LINK = $(LIBTOOL) --mode=link $(CCLD) $(AM_CFLAGS) $(CFLAGS) \
	$(AM_LDFLAGS) $(LDFLAGS) -o $@
CFLAGS =  -Wall -O3 -march=i686 -fmessage-length=0 -funroll-all-loops -fomit-frame-pointer -malign-loops=2 -malign-jumps=2 -malign-functions=2 -I/usr/local/include
DIST_SOURCES = $(libmpgdecoder_la_SOURCES)
HEADERS = $(noinst_HEADERS)

DIST_COMMON = README $(noinst_HEADERS) Makefile.am Makefile.in TODO \
	depcomp
SOURCES = $(libmpgdecoder_la_SOURCES)

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
	  $(AUTOMAKE) --foreign  mpglib/Makefile
Makefile: # $(srcdir)/Makefile.in  $(top_builddir)/config.status
	cd $(top_builddir) && \
	  CONFIG_HEADERS= CONFIG_LINKS= \
	  CONFIG_FILES=$(subdir)/$@ $(SHELL) ./config.status

clean-noinstLTLIBRARIES:
	-test -z "$(noinst_LTLIBRARIES)" || rm -f $(noinst_LTLIBRARIES)
libmpgdecoder.la: $(libmpgdecoder_la_OBJECTS) $(libmpgdecoder_la_DEPENDENCIES) 
	$(LINK)  $(libmpgdecoder_la_LDFLAGS) $(libmpgdecoder_la_OBJECTS) $(libmpgdecoder_la_LIBADD) $(LIBS)

mostlyclean-compile:
	-rm -f *.$(OBJEXT) core *.core

distclean-compile:
	-rm -f *.tab.c

ANSI2KNR = $(top_srcdir)/ansi2knr
$(top_srcdir)/ansi2knr:
	cd $(top_srcdir) && $(MAKE) $(AM_MAKEFLAGS) ansi2knr

mostlyclean-kr:
	-rm -f *_.c

include $(DEPDIR)/common$U.Plo
include $(DEPDIR)/dct64_i386$U.Plo
include $(DEPDIR)/decode_i386$U.Plo
include $(DEPDIR)/interface$U.Plo
include $(DEPDIR)/layer1$U.Plo
include $(DEPDIR)/layer2$U.Plo
include $(DEPDIR)/layer3$U.Plo
include $(DEPDIR)/tabinit$U.Plo

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
common_.c: common.c $(ANSI2KNR)
	$(CPP) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) `if test -f $(srcdir)/common.c; then echo $(srcdir)/common.c; else echo common.c; fi` | sed 's/^# \([0-9]\)/#line \1/' | $(ANSI2KNR) > common_.c || rm -f common_.c
dct64_i386_.c: dct64_i386.c $(ANSI2KNR)
	$(CPP) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) `if test -f $(srcdir)/dct64_i386.c; then echo $(srcdir)/dct64_i386.c; else echo dct64_i386.c; fi` | sed 's/^# \([0-9]\)/#line \1/' | $(ANSI2KNR) > dct64_i386_.c || rm -f dct64_i386_.c
decode_i386_.c: decode_i386.c $(ANSI2KNR)
	$(CPP) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) `if test -f $(srcdir)/decode_i386.c; then echo $(srcdir)/decode_i386.c; else echo decode_i386.c; fi` | sed 's/^# \([0-9]\)/#line \1/' | $(ANSI2KNR) > decode_i386_.c || rm -f decode_i386_.c
interface_.c: interface.c $(ANSI2KNR)
	$(CPP) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) `if test -f $(srcdir)/interface.c; then echo $(srcdir)/interface.c; else echo interface.c; fi` | sed 's/^# \([0-9]\)/#line \1/' | $(ANSI2KNR) > interface_.c || rm -f interface_.c
layer1_.c: layer1.c $(ANSI2KNR)
	$(CPP) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) `if test -f $(srcdir)/layer1.c; then echo $(srcdir)/layer1.c; else echo layer1.c; fi` | sed 's/^# \([0-9]\)/#line \1/' | $(ANSI2KNR) > layer1_.c || rm -f layer1_.c
layer2_.c: layer2.c $(ANSI2KNR)
	$(CPP) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) `if test -f $(srcdir)/layer2.c; then echo $(srcdir)/layer2.c; else echo layer2.c; fi` | sed 's/^# \([0-9]\)/#line \1/' | $(ANSI2KNR) > layer2_.c || rm -f layer2_.c
layer3_.c: layer3.c $(ANSI2KNR)
	$(CPP) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) `if test -f $(srcdir)/layer3.c; then echo $(srcdir)/layer3.c; else echo layer3.c; fi` | sed 's/^# \([0-9]\)/#line \1/' | $(ANSI2KNR) > layer3_.c || rm -f layer3_.c
tabinit_.c: tabinit.c $(ANSI2KNR)
	$(CPP) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) `if test -f $(srcdir)/tabinit.c; then echo $(srcdir)/tabinit.c; else echo tabinit.c; fi` | sed 's/^# \([0-9]\)/#line \1/' | $(ANSI2KNR) > tabinit_.c || rm -f tabinit_.c
common_.$(OBJEXT) common_.lo dct64_i386_.$(OBJEXT) dct64_i386_.lo \
decode_i386_.$(OBJEXT) decode_i386_.lo interface_.$(OBJEXT) \
interface_.lo layer1_.$(OBJEXT) layer1_.lo layer2_.$(OBJEXT) layer2_.lo \
layer3_.$(OBJEXT) layer3_.lo tabinit_.$(OBJEXT) tabinit_.lo : \
$(ANSI2KNR)
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
all-am: Makefile $(LTLIBRARIES) $(HEADERS)

installdirs:

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

distclean-generic:
	-rm -f Makefile $(CONFIG_CLEAN_FILES) stamp-h stamp-h[0-9]*

maintainer-clean-generic:
	@echo "This command is intended for maintainers to use"
	@echo "it deletes files that may require special tools to rebuild."
clean: clean-am

clean-am: clean-generic clean-libtool clean-noinstLTLIBRARIES \
	mostlyclean-am

distclean: distclean-am

distclean-am: clean-am distclean-compile distclean-depend \
	distclean-generic distclean-libtool distclean-tags

dvi: dvi-am

dvi-am:

info: info-am

info-am:

install-data-am:

install-exec-am:

install-info: install-info-am

install-man:

installcheck-am:

maintainer-clean: maintainer-clean-am

maintainer-clean-am: distclean-am maintainer-clean-generic

mostlyclean: mostlyclean-am

mostlyclean-am: mostlyclean-compile mostlyclean-generic mostlyclean-kr \
	mostlyclean-libtool

uninstall-am: uninstall-info-am

.PHONY: GTAGS all all-am check check-am clean clean-generic \
	clean-libtool clean-noinstLTLIBRARIES distclean \
	distclean-compile distclean-depend distclean-generic \
	distclean-libtool distclean-tags distdir dvi dvi-am info \
	info-am install install-am install-data install-data-am \
	install-exec install-exec-am install-info install-info-am \
	install-man install-strip installcheck installcheck-am \
	installdirs maintainer-clean maintainer-clean-generic \
	mostlyclean mostlyclean-compile mostlyclean-generic \
	mostlyclean-kr mostlyclean-libtool tags uninstall uninstall-am \
	uninstall-info-am


# end global section

lclint.txt: ${libmpgdecoder_la_SOURCES} ${noinst_HEADERS}
	@lclint ${LCLINTFLAGS} ${INCLUDES} ${DEFS} ${libmpgdecoder_la_SOURCES} 2>&1 >lclint.txt || true

lclint: lclint.txt
	more lclint.txt

#$(OBJECTS): libtool
#libtool: $(LIBTOOL_DEPS)
#	$(SHELL) $(top_builddir)/config.status --recheck
# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
