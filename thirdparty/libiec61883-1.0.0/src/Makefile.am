
lib_LTLIBRARIES = libiec61883.la

INCLUDES =					\
	@LIBRAW1394_CFLAGS@

libiec61883_la_LDFLAGS =					\
	@LIBRAW1394_LIBS@					\
	-version-info @lt_current@:@lt_revision@:@lt_age@

libiec61883_la_SOURCES = \
	cip.c \
	amdtp.c \
	plug.c \
	cmp.c \
	cooked.c \
	cooked.h \
	dv.c \
	deque.c \
	deque.h \
	tsbuffer.c \
	tsbuffer.h \
	mpeg2.c \
	iec61883-private.h

# headers to be installed
pkginclude_HEADERS = iec61883.h

MAINTAINERCLEANFILES = Makefile.in
