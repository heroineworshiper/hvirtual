include ../../global_config

OPENCV_DIR := $(shell expr ../../thirdparty/opencv* )

OBJS := \
	$(OBJDIR)/hyperlapse.o \
	$(OBJDIR)/hyperlapsewindow.o

LFLAGS += $(OPENCV_DIR)/libopencv.a

OUTPUT = $(PLUGIN_DIR)/hyperlapse.plugin


CFLAGS += -I$(OPENCV_DIR)/modules/core/include/ \
	-I$(OPENCV_DIR)/modules/features2d/include/ \
	-I$(OPENCV_DIR)/modules/calib3d/include/ \
	-I$(OPENCV_DIR)/modules/flann/include/ \
	-I$(OPENCV_DIR)/modules/imgproc/include/ \
	-I$(OPENCV_DIR)/modules/legacy/include/ \
	-I$(OPENCV_DIR)/modules/highgui/include/ \
	-I$(OPENCV_DIR)/modules/objdetect/include/ \
	-I$(OPENCV_DIR)/modules/video/include/

include ../../plugin_config

$(OBJDIR)/hyperlapse.o: hyperlapse.C
$(OBJDIR)/hyperlapsewindow.o: hyperlapsewindow.C


