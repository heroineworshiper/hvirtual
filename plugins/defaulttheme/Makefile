include ../../global_config

OBJS = $(OBJDIR)/defaulttheme.o
THEME_DATA := $(OBJDIR)/defaulttheme_data.o

OUTPUT_THEME := $(PLUGIN_DIR)/defaulttheme.plugin

CFLAGS += -I../../$(AVC_DIR) -I../../$(LIBRAW_DIR)

$(OUTPUT_THEME): $(OBJS) $(THEME_DATA)
	$(LDLINKER) -o $(OUTPUT_THEME) $(OBJS) $(THEME_DATA)
	strip $(OUTPUT_THEME)



include ../../plugin_config

$(OBJDIR)/defaulttheme.o: defaulttheme.C

$(THEME_DATA):
	cd $(OBJDIR) && \
	../../../guicast/$(OBJDIR)/bootstrap defaulttheme_data.o ../data/*.png

inst:
	rm -f $(PLUGINDIR)/defaulttheme.plugin
	cp $(OUTPUT_THEME) $(PLUGINDIR)
