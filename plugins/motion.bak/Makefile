include ../../global_config

OBJS := \
	../downsample/$(OBJDIR)/downsampleengine.o \
	$(OBJDIR)/motion.o \
	$(OBJDIR)/motionscan.o \
	$(OBJDIR)/motionwindow.o \
	$(OBJDIR)/opencvwrapper.o

OUTPUT = $(PLUGIN_DIR)/motion.plugin

OPENCV_DIR := $(shell expr ../../thirdparty/OpenCV* )

LFLAGS += $(OPENCV_DIR)/libopencv.a

CFLAGS += -I$(OPENCV_DIR)/modules/core/include/ \
	-I$(OPENCV_DIR)/modules/features2d/include/ \
	-I$(OPENCV_DIR)/modules/calib3d/include/ \
	-I$(OPENCV_DIR)/modules/flann/include/ \
	-I$(OPENCV_DIR)/modules/imgproc/include/ \
	-I$(OPENCV_DIR)/modules/highgui/include/ \
	-I$(OPENCV_DIR)/modules/objdetect/include/

include ../../plugin_config

$(OBJDIR)/motion.o: motion.C
$(OBJDIR)/motionscan.o: motionscan.C
$(OBJDIR)/motionwindow.o: motionwindow.C
$(OBJDIR)/opencvwrapper.o: opencvwrapper.C




