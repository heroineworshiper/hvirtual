TARGET_DIR := ../bin/doc

# images to copy from the theme
# these need the latest date stamp to get the browser to take the latest image
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
        razor.png \
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

# custom images from this directory
# Images requiring scaling go in fix_images.py
CUSTOM_IMAGES := \
        cwindow_context.png \
        cwindow_tools.png \
        apatches.png \
        autozoom.png \
        bezier.png \
        channelpositions.png \
        cleaning1.png \
        compositing_pipeline2.png \
        compress.png \
        cwindow_light.png \
        cwindow_zoom.png \
        editing_mode.png \
        fit_curves.png \
        ibeam_mode.png \
        inout_points.png \
        in_point_button.png \
        insertion_strategy.png \
        label_button.png \
        label_traversal.png \
        main_timebar.png \
        out_point_button.png \
        overwrite_button.png \
        preview.png \
        previewregion.png \
        splice_button.png \
        timebar_label.png \
        timebarslider.png \
        title.png \
        toclip_button.png \
        track_attributes.png \
        transport_panel.png \
        viewer_labels.png \
        vpatches.png \
        zoompanel.png


all:
	mkdir -p $(TARGET_DIR)
	for i in $(IMAGES) ; \
		do if ! test -e $(TARGET_DIR)/$$i ; \
			then cp ../plugins/brighttheme/data/$$i $(TARGET_DIR) ; \
		fi ; \
	done
	cp $(CUSTOM_IMAGES) $(TARGET_DIR)
# use this to create a single document.
	cd $(TARGET_DIR) && makeinfo --html --no-split ../../doc/cinelerra.texi
	./fix_images.py
# use this to create multiple HTML pages.  Not searchable
#	cd $(TARGET_DIR) && makeinfo --html ../../doc/cinelerra.texi


#install:
#	mkdir -p $(TARGET_DIR)
#	cp $(IMAGES) $(TARGET_DIR)
#	cp cinelerra.html $(TARGET_DIR)

clean:
	rm -rf $(TARGET_DIR)
