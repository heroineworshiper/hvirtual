include ../../global_config

OPENCV_DIR := $(shell expr ../../thirdparty/opencv* )

OBJS := \
	$(OBJDIR)/hyperlapse.o \
	$(OBJDIR)/hyperlapsewindow.o

LFLAGS += \
	$(OPENCV_DIR)/bin/lib/libopencv_video.a \
	$(OPENCV_DIR)/bin/lib/libopencv_videostab.a \
	$(OPENCV_DIR)/bin/lib/libopencv_imgproc.a \
	$(OPENCV_DIR)/bin/lib/libopencv_core.a \
	$(OPENCV_DIR)/bin/lib/libopencv_calib3d.a \
	$(OPENCV_DIR)/bin/lib/libopencv_features2d.a \
	$(OPENCV_DIR)/bin/lib/libopencv_flann.a \
	$(OPENCV_DIR)/3rdparty/ippicv/unpack/ippicv_lnx/lib/intel64/libippicv.a \
	$(OPENCV_DIR)/bin/lib/libopencv_imgproc.a \
	$(OPENCV_DIR)/bin/lib/libopencv_core.a \
	$(OPENCV_DIR)/bin/lib/libopencv_calib3d.a \
	$(OPENCV_DIR)/3rdparty/ippicv/unpack/ippicv_lnx/lib/intel64/libippicv.a


OUTPUT = $(PLUGIN_DIR)/hyperlapse.plugin


CFLAGS += -I$(OPENCV_DIR)/modules/core/include/ \
	-I$(OPENCV_DIR)/modules/features2d/include/ \
	-I$(OPENCV_DIR)/modules/calib3d/include/ \
	-I$(OPENCV_DIR)/modules/flann/include/ \
	-I$(OPENCV_DIR)/modules/imgproc/include/ \
	-I$(OPENCV_DIR)/modules/legacy/include/ \
	-I$(OPENCV_DIR)/modules/highgui/include/ \
	-I$(OPENCV_DIR)/modules/objdetect/include/ \
	-I$(OPENCV_DIR)/modules/video/include/ \
	-I$(OPENCV_DIR)/modules/videostab/include/ \
	-I$(OPENCV_DIR)/bin/

include ../../plugin_config

$(OBJDIR)/hyperlapse.o: hyperlapse.C
$(OBJDIR)/hyperlapsewindow.o: hyperlapsewindow.C


