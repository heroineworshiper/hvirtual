include ../../global_config

OBJS = $(OBJDIR)/suv.o
THEME_DATA := $(OBJDIR)/suv_data.o

OUTPUT_THEME := $(PLUGIN_DIR)/suv.plugin
AVC_DIR := $(shell expr ../../thirdparty/libavc* )
LIBRAW_DIR := $(shell expr ../../thirdparty/libraw1394* )
JPEG_DIR := $(shell expr ../../quicktime/thirdparty/jpeg )

CFLAGS += -I$(AVC_DIR) -I$(LIBRAW_DIR) -I$(JPEG_DIR)

$(OUTPUT_THEME): $(OBJS) $(THEME_DATA)
	$(LDLINKER) -o $(OUTPUT_THEME) $(OBJS) $(THEME_DATA)
	strip $(OUTPUT_THEME)



include ../../plugin_config

$(OBJDIR)/suv.o: suv.C

$(THEME_DATA):
	cd $(OBJDIR) && \
	../../../guicast/$(OBJDIR)/bootstrap suv_data.o ../data/*.png

inst:
	rm -f ../../bin/suv.plugin
	cp $(OUTPUT_THEME) ../../bin/



