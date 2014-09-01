TARGET_DIR := ../bin/doc

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

$(OUTPUT):
	cinelerra.html

all:
	for i in $(IMAGES) ; \
		do if ! test -e $$i ; \
			then cp -a ../plugins/suv/data/$$i . ; \
		fi ; \
	done
	makeinfo --html --no-split cinelerra.texi
#	makeinfo --html cinelerra.texi

install:
	mkdir -p $(TARGET_DIR)
	cp $(IMAGES) $(TARGET_DIR)
	cp cinelerra.html $(TARGET_DIR)

clean:
	rm -f $(IMAGES) $(OUTPUT)
