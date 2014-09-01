include ../../global_config

OBJS := \
	$(OBJDIR)/chromakey.o

SHADER := $(OBJDIR)/chromakey_sl.o

OUTPUT_ = $(PLUGIN_DIR)/chromakeyhsv.plugin

$(OUTPUT_): $(OBJS) $(SHADER)
	$(LDLINKER) -o $(OUTPUT_) $(OBJS) $(SHADER)
	strip $(OUTPUT_)


include ../../plugin_config

inst:
	rm -f $(PLUGINDIR)/chromakeyhsv.plugin
	cp $(OUTPUT_) $(PLUGINDIR)


$(OBJDIR)/chromakey.o: chromakey.C
$(SHADER): chromakey.sl
	cd $(OBJDIR) && \
	../../../guicast/$(OBJDIR)/bootstrap -s chromakey_sl.o ../chromakey.sl
