
man_MANS                  = encodedv.1 dvconnect.1 dubdv.1

EXTRA_DIST                = $(man_MANS)

MAINTAINERCLEANFILES      = Makefile.in $(AUX_DIST)

LIBTOOL = $(SHELL) $(top_builddir)/libtool --silent

CLEANFILES                = 

bin_PROGRAMS= encodedv dvconnect dubdv

noinst_PROGRAMS= fix_headers scan_packet_headers steal_header ppmqscale dvavi

encodedv_SOURCES= encodedv.c 
encodedv_LDADD= ../libdv/libdv.la $(POPT_LIB)

dubdv_SOURCES = insert_audio.c 
dubdv_LDADD= ../libdv/libdv.la $(POPT_LIB)

fix_headers_SOURCES = fix_headers.c 
fix_headers_LDADD= ../libdv/libdv.la

scan_packet_headers_SOURCES = scan_packet_headers.c

steal_header_SOURCES = steal_header.c

ppmqscale_SOURCES = ppmqscale.c

dvconnect_SOURCES = dvconnect.c
dvconnect_LDADD = $(PTHREAD_LIBS) $(POPT_LIB)

dvavi_SOURCES = dvavi.c
dvavi_LDADD= ../libdv/libdv.la
