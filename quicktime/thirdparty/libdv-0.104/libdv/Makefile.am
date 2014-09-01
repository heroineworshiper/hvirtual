
MAINTAINERCLEANFILES      = Makefile.in $(AUX_DIST)

CLEANFILES                = asmoff.h 

LIBTOOL = $(SHELL) $(top_builddir)/libtool --silent

lib_LTLIBRARIES= libdv.la

if HOST_X86
GASMOFF=gasmoff
endif # HOST_X86

if HOST_X86_64
GASMOFF=gasmoff
endif # HOST_X86_64


noinst_PROGRAMS= dovlc testvlc testbitstream $(GASMOFF) recode reppm enctest

#
# If HOST_X86 is set, we build all the x86 asm stuff..
#
if HOST_X86

BUILT_SOURCES = asmoff.h

libdv_la_ASMS= vlc_x86.S quant_x86.S idct_block_mmx.S dct_block_mmx.S rgbtoyuv.S encode_x86.S transpose_x86.S
libdv_la_ASM_HS=asmoff.h  mmx.h 

gasmoff_SOURCES= gasmoff.c bitstream.h 

endif # HOST_X86

#
# If HOST_X86_64 is set, we build all the x86_64 asm stuff..
#
if HOST_X86_64

BUILT_SOURCES = asmoff.h

libdv_la_ASMS= vlc_x86_64.S quant_x86_64.S idct_block_mmx_x86_64.S dct_block_mmx_x86_64.S rgbtoyuv_x86_64.S encode_x86_64.S transpose_x86_64.S
libdv_la_ASM_HS=asmoff.h  mmx.h

gasmoff_SOURCES= gasmoff.c bitstream.h

endif # HOST_X86_64

pkginclude_HEADERS=  dv_types.h dv.h

noinst_HEADERS=  YUY2.h      bitstream.h parse.h  rgb.h \
	YV12.h   dct.h       idct_248.h  place.h  vlc.h \
	quant.h  weighting.h audio.h     rgb.h    audio.h \
	encode.h enc_input.h enc_audio_input.h 	  enc_output.h \
        headers.h 	     util.h   $(libdv_la_ASM_HS)

libdv_la_SOURCES= dv.c dct.c idct_248.c weighting.c quant.c vlc.c place.c \
	parse.c bitstream.c YUY2.c YV12.c rgb.c audio.c util.c \
        encode.c headers.c enc_input.c enc_audio_input.c enc_output.c \
	$(libdv_la_ASMS)

libdv_la_LDFLAGS = -version-info 4:2:0

dovlc_SOURCES= dovlc.c 
dovlc_LDADD= libdv.la

testvlc_SOURCES= testvlc.c 
testvlc_LDADD=libdv.la

testbitstream_SOURCES= testbitstream.c  bitstream.h
testbitstream_LDADD=libdv.la

recode_SOURCES=recode.c
recode_LDADD=libdv.la

reppm_SOURCES=reppm.c
reppm_LDADD=libdv.la

enctest_SOURCES=enctest.c
enctest_LDADD=libdv.la $(PTHREAD_LIBS) 

if HOST_X86

# Automake doesn't do dependency tracking for asm
quant_x86.lo vlc_x86.lo: asmoff.h

asmoff.h: gasmoff
	./gasmoff > asmoff.h

endif

if HOST_X86_64

# Automake doesn't do dependency tracking for asm
quant_x86_64.lo vlc_x86_64.lo: asmoff.h

asmoff.h: gasmoff
	./gasmoff > asmoff.h

endif
