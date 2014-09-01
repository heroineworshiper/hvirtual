MAINTAINERCLEANFILES = Makefile.in
lib_LTLIBRARIES = libavc1394.la
libavc1394_la_LDFLAGS = @LIBRAW1394_LIBS@ \
	-version-info @lt_major@:@lt_revision@:@lt_age@ 
libavc1394_la_LIBADD = $(top_builddir)/common/raw1394util.lo 
libavc1394_la_SOURCES = \
	avc1394_simple.c avc1394_vcr.c \
	avc1394_internal.c avc1394_internal.h 
pkginclude_HEADERS = avc1394.h avc1394_vcr.h
INCLUDES = @LIBRAW1394_CFLAGS@

