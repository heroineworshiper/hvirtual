man_MANS                  = playdv.1 

EXTRA_DIST                = $(man_MANS)

MAINTAINERCLEANFILES      = Makefile.in $(AUX_DIST)

CLEANFILES                = 

LIBTOOL = $(SHELL) $(top_builddir)/libtool --silent

AM_CFLAGS = $(SDL_CFLAGS) $(GTK_CFLAGS)

bin_PROGRAMS= playdv

noinst_HEADERS=  display.h oss.h

playdv_SOURCES= playdv.c display.c display.h oss.c
playdv_LDADD= $(SDL_LIBS) $(GTK_LIBS) $(XV_LIB) ../libdv/libdv.la $(POPT_LIB)
