
include ../../global_config

OBJS = $(OBJDIR)/aging.o  \
	$(OBJDIR)/agingwindow.o \
	$(OBJDIR)/effecttv.o

OUTPUT = $(PLUGIN_DIR)/aging.plugin
CFLAGS += -I../burn

include ../../plugin_config

$(OBJDIR)/aging.o: aging.C
$(OBJDIR)/agingwindow.o: agingwindow.C
$(OBJDIR)/effecttv.o: effecttv.C
