include ../../global_config

OBJS := \
	$(OBJDIR)/motion.o \
	$(OBJDIR)/motioncache.o \
	$(OBJDIR)/motionscan.o \
	$(OBJDIR)/motionwindow.o

OUTPUT = $(PLUGIN_DIR)/motion.plugin

include ../../plugin_config

$(OBJDIR)/motion.o: motion.C
$(OBJDIR)/motioncache.o: motioncache.C
$(OBJDIR)/motionscan.o: motionscan.C
$(OBJDIR)/motionwindow.o: motionwindow.C
$(OBJDIR)/rotatescan.o: rotatescan.C
