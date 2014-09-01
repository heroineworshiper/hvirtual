
noinst_PROGRAMS = test-amdtp test-dv test-mpeg2 test-plugs
bin_PROGRAMS = plugreport plugctl

test_amdtp_SOURCES = test-amdtp.c
test_dv_SOURCES = test-dv.c
test_mpeg2_SOURCES = test-mpeg2.c
test_plugs_SOURCES = test-plugs.c
plugreport_SOURCES = plugreport.c
plugctl_SOURCES = plugctl.c

INCLUDES = @LIBRAW1394_CFLAGS@
LDADD    = ../src/libiec61883.la @LIBRAW1394_LIBS@

MAINTAINERCLEANFILES = Makefile.in
