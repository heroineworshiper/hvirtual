include ../../global_config

OBJS = $(OBJDIR)/microtheme.o

OUTPUT_THEME = $(PLUGIN_DIR)/microtheme.plugin

OUTPUT_THEME: $(OBJS)
	$(LDLINKER) -o $(OUTPUT_THEME) $(OBJS)
	strip $(OUTPUT_THEME)
	../../cinelerra/$(OBJDIR)/bootstrap $(OUTPUT_THEME) data/*.png

include ../../plugin_config

$(OBJDIR)/microtheme.o: microtheme.C



inst:
	rm -f $(PLUGINDIR)/microtheme.plugin
	cp $(OUTPUT_THEME) $(PLUGINDIR)
