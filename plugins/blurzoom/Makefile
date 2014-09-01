CFLAGS += -g

include ../../global_config

OBJS = $(OBJDIR)/blurzoom.o  \
	$(OBJDIR)/blurzoomwindow.o

OUTPUT = $(PLUGIN_DIR)/blurzoom.plugin

include ../../plugin_config

$(OBJDIR)/blurzoom.o: blurzoom.C
$(OBJDIR)/blurzoomwindow.o: blurzoomwindow.C
