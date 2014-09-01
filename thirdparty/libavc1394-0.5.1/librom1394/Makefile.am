MAINTAINERCLEANFILES = Makefile.in
lib_LTLIBRARIES = librom1394.la
librom1394_la_LDFLAGS = @LIBRAW1394_LIBS@ \
	-version-info @lt_major@:@lt_revision@:@lt_age@  -lm
librom1394_la_LIBADD = $(top_builddir)/common/raw1394util.lo
librom1394_la_SOURCES = \
	rom1394_main.c \
	rom1394_internal.c rom1394_internal.h
pkginclude_HEADERS = rom1394.h
INCLUDES = @LIBRAW1394_CFLAGS@

