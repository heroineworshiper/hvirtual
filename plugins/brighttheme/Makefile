include ../../global_config

OBJS = $(OBJDIR)/brighttheme.o
THEME_DATA := $(OBJDIR)/brighttheme_data.o

OUTPUT_THEME := $(PLUGIN_DIR)/brighttheme.plugin
AVC_DIR := $(shell expr ../../thirdparty/libavc* )
LIBRAW_DIR := $(shell expr ../../thirdparty/libraw1394* )
JPEG_DIR := $(shell expr ../../quicktime/thirdparty/jpeg )

CFLAGS += -I$(AVC_DIR) -I$(LIBRAW_DIR) -I$(JPEG_DIR)

$(OUTPUT_THEME): $(OBJS) $(THEME_DATA)
	$(LDLINKER) -o $(OUTPUT_THEME) $(OBJS) $(THEME_DATA)
	strip $(OUTPUT_THEME)



include ../../plugin_config

$(OBJDIR)/brighttheme.o: brighttheme.C

$(THEME_DATA):
	cd $(OBJDIR) && \
	../../../guicast/$(OBJDIR)/bootstrap brighttheme_data.o ../data/*.png

inst:
	rm -f ../../bin/brighttheme.plugin
	cp $(OUTPUT_THEME) ../../bin/



