OBJDIR := $(shell uname --machine)
USE_MMX = 0
CC = gcc
CXX = g++



ifeq ($(origin CFLAGS), environment)
HAVE_CFLAGS := y
else
HAVE_CFLAGS := n
endif



ENCORE_DIR := thirdparty/encore50
ENCORE_INCLUDE := $(ENCORE_DIR)/encore.h
MPEG3_DIR := $(shell expr ../libmpeg3* )
OBJS := 


# bug fixes for ffmpeg are required
# Add test to ffmpeg/libavcodec/svq3.c to make sure extradata is
# nonzero before memcmping it.
FFMPEG_DIR := $(shell expr thirdparty/ffmpeg*)
FFMPEG_LIB := $(OBJDIR)/ffmpeg/
FFMPEG_INCLUDE := -I$(FFMPEG_DIR) \
-I$(FFMPEG_DIR)/libavcodec \
-I$(FFMPEG_DIR)/libavformat \
-I$(FFMPEG_DIR)/libavutil

FAAD_DIR := $(shell expr thirdparty/faad*)
FAAD_LIB := $(OBJDIR)/faad/
FAAD_INCLUDE := $(FAAD_DIR)/include/

FAAC_DIR := thirdparty/faac-1.24
FAAC_LIB := $(OBJDIR)/faac/
FAAC_INCLUDE := $(FAAC_DIR)/include/

X264_DIR := $(shell expr thirdparty/x264*)
X264_LIB := $(OBJDIR)/x264
X264_INCLUDE := $(X264_DIR)

X265_DIR := $(shell expr thirdparty/x265*)
X265_LIB := $(OBJDIR)/x265
X265_INCLUDE := -I$(X265_DIR)/source -I$(X265_DIR)/build

DV_DIR := thirdparty/libdv-0.104
DV_LIB := $(OBJDIR)/libdv
DV_INCLUDE := $(DV_DIR)/

OGG_DIR := $(shell expr thirdparty/libogg* )
OGG_LIB := $(OBJDIR)/ogg

VORBIS_DIR := $(shell expr thirdparty/libvorbis* )
VORBIS_LIB := $(OBJDIR)/vorbis

ifeq ($(OBJDIR), i686)

  JPEG_DIR := $(shell expr thirdparty/jpeg-mmx* )
  JPEG_LIB := $(OBJDIR)/$(JPEG_DIR)
  JPEG_LIB2 := $(JPEG_DIR)/libjpeg-mmx.a
  JPEG_INCLUDE := $(JPEG_DIR)

else

  JPEG_DIR := thirdparty/jpeg
  JPEG_LIB := $(OBJDIR)/$(JPEG_DIR)
  JPEG_LIB2 := $(JPEG_DIR)/libjpeg.a
  JPEG_INCLUDE := $(JPEG_DIR)

endif

ifneq ($(HAVE_CFLAGS), y)


ifeq ($(OBJDIR), alpha)
  CFLAGS := -O4 -arch ev67 -ieee -accept c99_keywords -gcc_messages
endif

ifeq ($(OBJDIR), i686)
  CFLAGS := -O3 -fmessage-length=0 -funroll-all-loops -fomit-frame-pointer -falign-loops=2 -falign-jumps=2 -falign-functions=2 -I/usr/local/include
endif


ifeq ($(OBJDIR), x86_64)
  CFLAGS := -O -funroll-all-loops -fomit-frame-pointer  -I/usr/local/include
endif




endif

DIRS = $(OBJDIR)




#include Makefile.jpeg
#include Makefile.vorbis
include Makefile.lame
#include Makefile.dv
include Makefile.encore50





CFLAGS += 
CFLAGS += -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS += -DHAVE_FIREWIRE








OBJS += \
	$(OBJDIR)/atom.o \
	$(OBJDIR)/avcc.o \
	$(OBJDIR)/avi_hdrl.o \
	$(OBJDIR)/avi_ix.o \
	$(OBJDIR)/avi_idx1.o \
	$(OBJDIR)/avi_indx.o \
	$(OBJDIR)/avi_movi.o \
	$(OBJDIR)/avi_odml.o \
	$(OBJDIR)/avi_riff.o \
	$(OBJDIR)/avi_strl.o \
	$(OBJDIR)/cmodel_default.o \
	$(OBJDIR)/cmodel_float.o \
	$(OBJDIR)/cmodel_yuv420p.o \
	$(OBJDIR)/cmodel_yuv422.o \
	$(OBJDIR)/codecs.o \
	$(OBJDIR)/colormodels.o \
	$(OBJDIR)/ctab.o \
	$(OBJDIR)/dinf.o \
	$(OBJDIR)/dref.o \
	$(OBJDIR)/edts.o \
	$(OBJDIR)/elst.o \
	$(OBJDIR)/esds.o \
	$(OBJDIR)/frma.o \
	$(OBJDIR)/graphics.o \
	$(OBJDIR)/hdlr.o \
	$(OBJDIR)/ima4.o \
	$(OBJDIR)/jpeg.o \
	$(OBJDIR)/libdv.o \
	$(OBJDIR)/libmjpeg.o \
	$(OBJDIR)/matrix.o \
	$(OBJDIR)/mdat.o \
	$(OBJDIR)/mdhd.o \
	$(OBJDIR)/mdia.o \
	$(OBJDIR)/minf.o \
	$(OBJDIR)/moov.o \
	$(OBJDIR)/mp4a.o \
	$(OBJDIR)/mvhd.o \
	$(OBJDIR)/plugin.o \
	$(OBJDIR)/qtasf.o \
	$(OBJDIR)/qtcache.o \
	$(OBJDIR)/qtdv.o \
	$(OBJDIR)/qtpng.o \
	$(OBJDIR)/qtmp3.o \
	$(OBJDIR)/quicktime.o \
	$(OBJDIR)/raw.o \
	$(OBJDIR)/rawaudio.o \
	$(OBJDIR)/rle.o \
	$(OBJDIR)/smhd.o \
	$(OBJDIR)/stbl.o \
	$(OBJDIR)/stco.o \
	$(OBJDIR)/stsc.o \
	$(OBJDIR)/stsd.o \
	$(OBJDIR)/stsdtable.o \
	$(OBJDIR)/stss.o \
	$(OBJDIR)/stsz.o \
	$(OBJDIR)/stts.o \
	$(OBJDIR)/tkhd.o \
	$(OBJDIR)/trak.o \
	$(OBJDIR)/twos.o \
	$(OBJDIR)/udta.o \
	$(OBJDIR)/ulaw.o \
	$(OBJDIR)/util.o \
	$(OBJDIR)/v308.o \
	$(OBJDIR)/v408.o \
	$(OBJDIR)/v410.o \
	$(OBJDIR)/vbraudio.o \
	$(OBJDIR)/vmhd.o \
	$(OBJDIR)/vorbis.o \
	$(OBJDIR)/wmx2.o \
	$(OBJDIR)/workarounds.o \
	$(OBJDIR)/yuv2.o \
	$(OBJDIR)/yuv4.o \
	$(OBJDIR)/yv12.o

FFMPEG_OBJS := \
	$(OBJDIR)/mpeg4.o \
	$(OBJDIR)/qdm2.o \
	$(OBJDIR)/qtffmpeg.o \
	$(OBJDIR)/qth264.o \
	$(OBJDIR)/qth265.o \
	$(OBJDIR)/wma.o



UTILS := \
	$(OBJDIR)/qtdump \
	$(OBJDIR)/make_streamable \
	$(OBJDIR)/qtinfo \
	$(OBJDIR)/dechunk \
	$(OBJDIR)/yuv2mov \
	$(OBJDIR)/yuv4toyuv \
	$(OBJDIR)/recover \
	$(OBJDIR)/rechunk


#	$(OBJDIR)/mts2mp4





OUTPUT = $(OBJDIR)/libquicktime.a
SHAREDOUTPUT = $(OBJDIR)/libquicktime.so

CFLAGS += \
	-I. \
	-I$(ENCORE_DIR) \
	-I$(FAAC_INCLUDE) \
	-I$(FAAD_INCLUDE) \
	-DENCORE_INCLUDE=\"$(ENCORE_INCLUDE)\" \
	-I$(X264_INCLUDE) \
	$(X265_INCLUDE) \
	-I$(DV_INCLUDE) \
	-I$(JPEG_INCLUDE) \
	-I$(MPEG3_DIR) \
	-I$(LAME_DIR)/include \
	-I$(OGG_DIR)/include \
	-I$(VORBIS_DIR)/include \
	-ffast-math \
	-mno-ieee-fp

FFMPEG_CFLAGS := $(CFLAGS) \
	$(FFMPEG_INCLUDE) \

#DIRS += \
#	$(OBJDIR)/$(DECORE_DIR)/intel_mmx/postprocess

DIRS += \
	$(OBJDIR)/$(ENCORE_DIR) \
	$(OBJDIR)/$(ENCORE_DIR)/att_mmx \
	$(OBJDIR)/$(ENCORE_DIR)/colorspace \
	$(OBJDIR)/$(ENCORE_DIR)/colorspace/intel_mmx \
	$(OBJDIR)/$(ENCORE_DIR)/intel_mmx \
	$(OBJDIR)/$(ENCORE_DIR)/mbcoding \
	$(OBJDIR)/$(ENCORE_DIR)/mbmotionestcomp \
	$(OBJDIR)/$(ENCORE_DIR)/mbmotionestcomp/intel_mmx \
	$(OBJDIR)/$(ENCORE_DIR)/mbprediction \
	$(OBJDIR)/$(ENCORE_DIR)/mbtransquant \
	$(OBJDIR)/$(ENCORE_DIR)/mbtransquant/intel_mmx


LIBS := $(OUTPUT) $(MPEG3_DIR)/$(OBJDIR)/libmpeg3.a -lpng -lz -lpthread -ldl -lm -llzma -lrt


$(shell mkdir -p $(DIRS) )
$(shell echo  $(CFLAGS) > $(OBJDIR)/c_flags)
$(shell echo  $(LAME_CFLAGS) > $(OBJDIR)/lame_flags)
$(shell echo  $(OBJS) $(FFMPEG_OBJS) $(JPEG_LIB)/*.o $(DV_LIB)/*.o $(FAAD_LIB)/*.o $(FAAC_LIB)/*.o $(X264_LIB)/*.o $(X265_LIB)/*.o $(OGG_LIB)/*.o $(VORBIS_LIB)/*.o $(LAME_OBJS) > $(OBJDIR)/objs)
$(shell echo  $(FFMPEG_CFLAGS) > $(OBJDIR)/ffmpeg_cflags)

all: $(OBJDIR) $(OUTPUT) $(UTILS)

$(OUTPUT): $(OBJS) $(FFMPEG_OBJS) $(ENCOREMMX_OBJS) $(JPEG_NASM) $(LAME_OBJS) $(FAAD_LIB) $(FAAC_LIB) $(FFMPEG_LIB) $(X264_LIB) $(X265_LIB) $(DV_LIB) $(JPEG_LIB) $(OGG_LIB) $(VORBIS_LIB)
	find $(FFMPEG_DIR)/libavcodec/ -name *.o >> $(OBJDIR)/objs
	find $(FFMPEG_DIR)/libavformat/ -name *.o >> $(OBJDIR)/objs
	find $(FFMPEG_DIR)/libavutil/ -name *.o >> $(OBJDIR)/objs
	find $(FFMPEG_DIR)/libswresample/ -name *.o >> $(OBJDIR)/objs
	ar rcs $(OUTPUT) `cat $(OBJDIR)/objs`

# this is the only way we figured out to merge .a files
$(FFMPEG_LIB):
	$(MAKE) -C $(FFMPEG_DIR)
	mkdir -p $(FFMPEG_LIB)/avcodec \
		$(FFMPEG_LIB)/avformat \
		$(FFMPEG_LIB)/avutil
	cd $(FFMPEG_LIB)/avcodec && \
	ar x ../../../$(FFMPEG_DIR)/libavcodec/libavcodec.a &&  \
	cd ../avformat && \
	ar x ../../../$(FFMPEG_DIR)/libavformat/libavformat.a &&  \
	cd ../avutil && \
	ar x ../../../$(FFMPEG_DIR)/libavutil/libavutil.a \
	cd ../avutil && \
	ar x ../../../$(FFMPEG_DIR)/libswresample/libswresample.a

$(FAAD_LIB):
	$(MAKE) -C $(FAAD_DIR) && \
	mkdir -p $(FAAD_LIB) && \
	cd $(FAAD_LIB) && \
	ar x ../../$(FAAD_DIR)/libfaad/.libs/libfaad.a

$(FAAC_LIB):
	$(MAKE) -C $(FAAC_DIR) && \
	mkdir -p $(FAAC_LIB) && \
	cd $(FAAC_LIB) && \
	ar x ../../$(FAAC_DIR)/libfaac/.libs/libfaac.a

$(DV_LIB):
	$(MAKE) -C $(DV_DIR) && \
	mkdir -p $(DV_LIB) && \
	cd $(DV_LIB) && \
	ar x ../../$(DV_DIR)/libdv/.libs/libdv.a

$(JPEG_LIB):
	$(MAKE) -C $(JPEG_DIR) && \
	mkdir -p $(JPEG_LIB) && \
	cd $(JPEG_LIB) && \
	ar x ../../../$(JPEG_LIB2)

# need some trickery to get the duplicated filenames out
$(X264_LIB):
	$(MAKE) -C $(X264_DIR) && \
	mkdir -p $(X264_LIB) && \
	cd $(X264_LIB) && \
	ar x ../../$(X264_DIR)/libx264.a && \
	mv predict.o predict2.o && \
	mv cabac.o cabac2.o && \
	mv macroblock.o macroblock2.o && \
	mv set.o set2.o && \
	ar xN 1 ../../$(X264_DIR)/libx264.a predict.o cabac.o macroblock.o set.o


$(X265_LIB):
	$(MAKE) -C $(X265_DIR)/build && \
	mkdir -p $(X265_LIB) && \
	cd $(X265_LIB) && \
	ar x ../../$(X265_DIR)/build/libx265.a



$(OGG_LIB):
	$(MAKE) -C $(OGG_DIR) && \
	mkdir -p $(OGG_LIB) && \
	cd $(OGG_LIB) && \
	ar x ../../$(OGG_DIR)/src/.libs/libogg.a

$(VORBIS_LIB):
	$(MAKE) -C $(VORBIS_DIR) && \
	mkdir -p $(VORBIS_LIB) && \
	cd $(VORBIS_LIB) && \
	ar x ../../$(VORBIS_DIR)/lib/.libs/libvorbis.a && \
	ar x ../../$(VORBIS_DIR)/lib/.libs/libvorbisenc.a && \
	ar x ../../$(VORBIS_DIR)/lib/.libs/libvorbisfile.a



$(OBJDIR)/mts2mp4: $(OBJDIR)/mts2mp4.o $(OUTPUT)
	$(CXX) -o $(OBJDIR)/mts2mp4 $(OBJDIR)/mts2mp4.o $(LIBS)

$(OBJDIR)/qtdump: $(OBJDIR)/dump.o $(OUTPUT)
	$(CXX) -o $(OBJDIR)/qtdump $(OBJDIR)/dump.o $(LIBS)

$(OBJDIR)/make_streamable: $(OBJDIR)/make_streamable.o $(OUTPUT)
	$(CXX) -o $(OBJDIR)/make_streamable $(OBJDIR)/make_streamable.o $(LIBS)

$(OBJDIR)/dechunk: $(OBJDIR)/dechunk.o $(OUTPUT)
	$(CXX) -o $(OBJDIR)/dechunk $(OBJDIR)/dechunk.o $(LIBS)

$(OBJDIR)/rechunk: $(OBJDIR)/rechunk.o $(OUTPUT)
	$(CXX) -o $(OBJDIR)/rechunk $(OBJDIR)/rechunk.o $(LIBS)

$(OBJDIR)/recover: $(OBJDIR)/recover.o $(OUTPUT)
	$(CXX) -o $(OBJDIR)/recover $(OBJDIR)/recover.o $(LIBS)

$(OBJDIR)/yuv4toyuv: $(OBJDIR)/yuv4toyuv.o $(OUTPUT)
	$(CXX) -o $(OBJDIR)/yuv4toyuv $(OBJDIR)/yuv4toyuv.o $(LIBS)

$(OBJDIR)/yuv2mov: $(OBJDIR)/yuv2mov.o $(OUTPUT)
	$(CXX) -o $(OBJDIR)/yuv2mov $(OBJDIR)/yuv2mov.o $(LIBS)

$(OBJDIR)/qtinfo: $(OBJDIR)/qtinfo.o $(OUTPUT)
	$(CXX) -o $(OBJDIR)/qtinfo $(OBJDIR)/qtinfo.o $(LIBS)

util:  $(UTILS)
	

clean:
	rm -rf  $(X265_DIR)/build
	make -i -C $(FFMPEG_DIR) clean
	make -i -C $(FAAD_DIR) clean
	make -i -C $(FAAC_DIR) clean
	make -i -C $(FAAD_DIR) clean
	make -i -C $(X264_DIR) clean
	make -i -C $(DV_DIR) clean
	make -i -C $(OGG_DIR) clean
	make -i -C $(VORBIS_DIR) clean
	make -i -C $(JPEG_DIR) clean
	find \( -name 'core*' \
		-o -name '*.lo' \
		-o -name '*.o' \
		-o -name '*.la' \
		-o -name '*.a' \
		-o -name '*.plugin' \
		-o -name '*.so' \) -exec rm -f {} \; -prune
	find \( -name $(OBJDIR) \) -exec rm -rf {} \; -prune

wc:
	cat *.c *.h | wc

backup: clean
	cd .. && \
	tar zcvf quicktime.tar.gz quicktime

install:
	cp $(UTILS) /usr/bin

$(OBJDIR)/dump.o \
$(OBJDIR)/make_streamable.o \
$(OBJDIR)/dechunk.o \
$(OBJDIR)/recover.o \
$(OBJDIR)/yuv4toyuv.o \
$(OBJDIR)/yuv2mov.o \
$(OBJDIR)/qtinfo.o \
$(OBJDIR)/rechunk.o \
$(OBJS):
	$(CC) -c `cat $(OBJDIR)/c_flags` $(subst $(OBJDIR)/,, $*.c) -o $*.o

$(OBJDIR)/mts2mp4.o:
	$(CC) -c `cat $(OBJDIR)/c_flags` $(FFMPEG_INCLUDE) $(subst $(OBJDIR)/,, $*.c) -o $*.o

$(JPEG_FAIL):
	$(CC) -c -O3 -march=i686 -fmessage-length=0 $(subst $(OBJDIR)/,, $*.c) -o $*.o

$(DV_OBJS):
	$(CC) -c `cat $(OBJDIR)/c_flags` $(subst $(OBJDIR)/,, $*.c) -o $*.o

#$(FAAD_OBJS):
#	$(CC) -c `cat $(OBJDIR)/faad_flags` $(subst $(OBJDIR)/,, $*.c) -o $*.o

#$(FAAC_OBJS):
#	$(CC) -c `cat $(OBJDIR)/faac_flags` $(subst $(OBJDIR)/,, $*.c) -o $*.o

$(DVMMXOBJS):
	$(CC) -c `cat $(OBJDIR)/c_flags` $(subst $(OBJDIR)/,, $*.S) -o $*.o

$(DVMMXOBJ): $(DV_DIR)/libdv/gasmoff.c
	$(CC) `cat $(OBJDIR)/c_flags` $(DV_DIR)/libdv/gasmoff.c -o $(OBJDIR)/$(DV_DIR)/libdv/gasmoff -lm
	$(OBJDIR)/$(DV_DIR)/libdv/gasmoff > $(DVMMXOBJ)

$(JPEG_NASM):
	nasm -f elf $(subst $(OBJDIR)/,, $*.s) -o $*.o

$(ENCOREMMX_OBJS):
	nasm -f elf $(subst $(OBJDIR)/,, $*.asm) -o $*.o

#$(DECOREMMX_OBJS):
#	$(CC) -c `cat $(OBJDIR)/c_flags` $(subst $(OBJDIR)/,, $*.S) -o $*.o


$(FFMPEG_OBJS):
	$(CC) `cat $(OBJDIR)/ffmpeg_cflags` -c $(subst $(OBJDIR)/,, $*.c) -o $*.o

$(LAME_OBJS):
	$(CC) `cat $(OBJDIR)/lame_flags` -c $(subst $(OBJDIR)/,, $*.c) -o $*.o

#$(FFMPEGIA32_OBJS):
#	nasm -f elf $(subst $(OBJDIR)/,, $*.s) -o $*.o




#$(OBJDIR)/div3.o:                         div3.c
#$(OBJDIR)/divx.o:			  divx.c


$(OBJDIR)/atom.o:			  atom.c
$(OBJDIR)/avcc.o:                         avcc.c
$(OBJDIR)/avi_hdrl.o:                     avi_hdrl.c
$(OBJDIR)/avi_ix.o:                       avi_ix.c
$(OBJDIR)/avi_idx1.o:                     avi_idx1.c
$(OBJDIR)/avi_indx.o:                     avi_indx.c
$(OBJDIR)/avi_movi.o:                     avi_movi.c
$(OBJDIR)/avi_odml.o:                     avi_odml.c
$(OBJDIR)/avi_riff.o:                     avi_riff.c
$(OBJDIR)/avi_strl.o:                     avi_strl.c
$(OBJDIR)/cmodel_default.o:		  cmodel_default.c cmodel_permutation.h
$(OBJDIR)/cmodel_float.o:		  cmodel_float.c cmodel_permutation.h
$(OBJDIR)/cmodel_yuv420p.o:		  cmodel_yuv420p.c cmodel_permutation.h
$(OBJDIR)/cmodel_yuv422.o:		  cmodel_yuv422.c cmodel_permutation.h
$(OBJDIR)/codecs.o:			  codecs.c
$(OBJDIR)/colormodels.o:		  colormodels.c
$(OBJDIR)/ctab.o:			  ctab.c
$(OBJDIR)/dechunk.o:			  dechunk.c
$(OBJDIR)/dinf.o:			  dinf.c
$(OBJDIR)/dref.o:			  dref.c
$(OBJDIR)/dump.o:			  dump.c
$(OBJDIR)/edts.o:			  edts.c
$(OBJDIR)/elst.o:			  elst.c
$(OBJDIR)/esds.o:                         esds.c
$(OBJDIR)/frma.o:                         frma.c
$(OBJDIR)/graphics.o:			  graphics.c
$(OBJDIR)/hdlr.o:			  hdlr.c
$(OBJDIR)/ima4.o:			  ima4.c
$(OBJDIR)/jpeg.o:			  jpeg.c
$(OBJDIR)/libdv.o:			  libdv.c
$(OBJDIR)/libmjpeg.o:			  libmjpeg.c
$(OBJDIR)/make_streamable.o:		  make_streamable.c
$(OBJDIR)/matrix.o:			  matrix.c
$(OBJDIR)/mdat.o:			  mdat.c
$(OBJDIR)/mdhd.o:			  mdhd.c
$(OBJDIR)/mdia.o:			  mdia.c
$(OBJDIR)/minf.o:			  minf.c
$(OBJDIR)/moov.o:			  moov.c
$(OBJDIR)/mpeg4.o:			  mpeg4.c
$(OBJDIR)/mp4a.o:                         mp4a.c
$(OBJDIR)/mvhd.o:			  mvhd.c
$(OBJDIR)/plugin.o:			  plugin.c
$(OBJDIR)/qdm2.o:                         qdm2.c
$(OBJDIR)/qtasf.o:                        qtasf.c
$(OBJDIR)/qtcache.o:                      qtcache.c
$(OBJDIR)/qtdv.o:			  qtdv.c
$(OBJDIR)/qtffmpeg.o:                     qtffmpeg.c
$(OBJDIR)/qth264.o:                       qth264.c
$(OBJDIR)/qth265.o:                       qth265.c
$(OBJDIR)/qtinfo.o:			  qtinfo.c
$(OBJDIR)/qtmp3.o:                        qtmp3.c
$(OBJDIR)/qtpng.o:			  qtpng.c
$(OBJDIR)/quicktime.o: 		  	  quicktime.c
$(OBJDIR)/raw.o:			  raw.c
$(OBJDIR)/rawaudio.o:			  rawaudio.c
$(OBJDIR)/rechunk.o:			  rechunk.c
$(OBJDIR)/mts2mp4.o:			  mts2mp4.c
$(OBJDIR)/recover.o:			  recover.c
$(OBJDIR)/rle.o:                          rle.c
$(OBJDIR)/smhd.o:			  smhd.c
$(OBJDIR)/stbl.o:			  stbl.c
$(OBJDIR)/stco.o:			  stco.c
$(OBJDIR)/strl.o:			  strl.c
$(OBJDIR)/stsc.o:			  stsc.c
$(OBJDIR)/stsd.o:			  stsd.c
$(OBJDIR)/stsdtable.o: 		  	  stsdtable.c
$(OBJDIR)/stss.o:			  stss.c
$(OBJDIR)/stsz.o:			  stsz.c
$(OBJDIR)/stts.o:			  stts.c
$(OBJDIR)/tkhd.o:			  tkhd.c
$(OBJDIR)/trak.o:			  trak.c
$(OBJDIR)/twos.o:			  twos.c
$(OBJDIR)/udta.o:			  udta.c
$(OBJDIR)/ulaw.o:			  ulaw.c
$(OBJDIR)/util.o:			  util.c
$(OBJDIR)/v308.o:                         v308.c
$(OBJDIR)/v408.o:                         v408.c
$(OBJDIR)/v410.o:                         v410.c
$(OBJDIR)/vbraudio.o:                     vbraudio.c
$(OBJDIR)/vmhd.o:			  vmhd.c
$(OBJDIR)/vorbis.o:                       vorbis.c
$(OBJDIR)/wma.o:		          wma.c
$(OBJDIR)/wmx2.o:		          wmx2.c
$(OBJDIR)/workarounds.o:		  workarounds.c
$(OBJDIR)/yuv2.o:			  yuv2.c
$(OBJDIR)/yuv2mov.o:                      yuv2mov.c
$(OBJDIR)/yuv4.o:			  yuv4.c
$(OBJDIR)/yuv4toyuv.o:			  yuv4toyuv.c
$(OBJDIR)/yv12.o:			  yv12.c









































#include depend.vorbis
#include depend.ffmpeg
#include depend.jpeg
include depend.lame
#include depend.dv
include depend.encore50
#include depend.faad
#include depend.faac
