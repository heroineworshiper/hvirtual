include ../../global_config

OBJS = \
	../colors/$(OBJDIR)/plugincolors.o \
	$(OBJDIR)/histogram.o \
	$(OBJDIR)/histogramconfig.o \
	$(OBJDIR)/histogramwindow.o

OUTPUT = $(PLUGIN_DIR)/histogram.plugin

include ../../plugin_config

$(OBJDIR)/histogram.o: histogram.C
$(OBJDIR)/histogramconfig.o: histogramconfig.C
$(OBJDIR)/histogramwindow.o: histogramwindow.C
