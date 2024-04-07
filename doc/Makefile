TARGET_DIR := ../bin/doc

# images to copy from the theme
IMAGES = \
	arrow.png \
	autokeyframe.png \
	camera.png \
	channel.png \
	crop.png \
	cut.png \
	expandpatch_checked.png \
	eyedrop.png \
	fitautos.png \
	ibeam.png \
	left_justify.png \
	loadmode_cat.png \
	loadmode_new.png \
	loadmode_nested.png \
	loadmode_newcat.png \
	loadmode_newtracks.png \
	loadmode_none.png \
	loadmode_paste.png \
	loadmode_resource.png \
	magnify.png \
	mask.png \
	mutepatch_up.png \
	paste.png \
	projector.png \
	protect.png \
	record.png \
	recordpatch.png \
	rewind.png \
	singleframe.png \
	show_meters.png \
	titlesafe.png \
	toolwindow.png \
	top_justify.png \
	wrench.png \
	magnify.png

# custom images for documentation
CUSTOM_IMAGES := \
	chromakey.png \
	chromakeyhsv1.png \
	chromakeyhsv2.png \
	chromakeyhsv3.png \
	chromakeyhsv4.png \
	chromakeyhsv5.png \
	chromakeyhsv6.png \
	chromakeyhsv7.png \
	chromakeyhsv8.png


all:
	mkdir -p $(TARGET_DIR)
	for i in $(IMAGES) ; \
		do if ! test -e $(TARGET_DIR)/$$i ; \
			then cp -a ../plugins/suv/data/$$i $(TARGET_DIR) ; \
		fi ; \
	done
	cp $(CUSTOM_IMAGES) $(TARGET_DIR)
# use this to create a single document.
	cd $(TARGET_DIR) && makeinfo --html --no-split ../../doc/cinelerra.texi
	cd $(TARGET_DIR) && ../../doc/fix_images.py cinelerra.html
# use this to create multiple HTML pages.  Not searchable
#	cd $(TARGET_DIR) && makeinfo --html ../../doc/cinelerra.texi


#install:
#	mkdir -p $(TARGET_DIR)
#	cp $(IMAGES) $(TARGET_DIR)
#	cp cinelerra.html $(TARGET_DIR)

clean:
	rm -rf $(TARGET_DIR)
