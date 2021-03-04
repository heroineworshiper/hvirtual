include ../../global_config

OBJS := \
	$(OBJDIR)/downsampleengine.o \
	$(OBJDIR)/motioncache.o \
	$(OBJDIR)/motionscan.o \
	$(OBJDIR)/motion.o \
	$(OBJDIR)/motionwindow.o

OUTPUT := $(PLUGIN_DIR)/motion2.plugin

include ../../plugin_config

$(OBJDIR)/downsampleengine.o: downsampleengine.C
$(OBJDIR)/motion.o: motion.C
$(OBJDIR)/motioncache.o: motioncache.C
$(OBJDIR)/motionscan.o: motionscan.C
$(OBJDIR)/motionwindow.o: motionwindow.C
