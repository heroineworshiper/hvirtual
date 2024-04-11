include ../../global_config

OBJS = \
	$(OBJDIR)/curves.o \
	$(OBJDIR)/curveswindow.o
SHADER := $(OBJDIR)/curves_sl.o

OUTPUT_ := $(PLUGIN_DIR)/curves.plugin

$(OUTPUT_): $(OBJS) $(SHADER)
	$(LDLINKER) -o $(OUTPUT_) $(OBJS) $(SHADER)
	strip $(OUTPUT_)

include ../../plugin_config

$(OBJDIR)/curves.o: curves.C
$(OBJDIR)/curveswindow.o: curveswindow.C
$(SHADER): curves.sl
	cd $(OBJDIR) && \
	../../../guicast/$(OBJDIR)/bootstrap -s curves_sl.o ../curves.sl
