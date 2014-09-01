include ../../global_config

OBJS = $(OBJDIR)/holo.o  \
	$(OBJDIR)/holowindow.o \
	$(OBJDIR)/effecttv.o

OUTPUT = $(PLUGIN_DIR)/holo.plugin

CFLAGS += -I../burn 

include ../../plugin_config

$(OBJDIR)/holo.o: holo.C
$(OBJDIR)/holowindow.o: holowindow.C
$(OBJDIR)/effecttv.o: effecttv.C
