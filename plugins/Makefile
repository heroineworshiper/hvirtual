include ../global_config

# burn must come before any other effecttv plugin
# colors must come before any plugin
# parametric must come before fourier plugins
# motion must come before perspective

DIRS = \
	1080to540 \
	720to480 \
	audioscope \
        background \
	bandslide \
	bandwipe \
	blur \
	brightness \
	brighttheme \
	burn \
	parametric \
	aging \
	cdripper \
	chorus \
	chromakey \
	chromakeyhsv \
        clip \
	colorbalance \
	color3way \
	compressor \
        compressormulti \
	crossfade \
        cubemap \
	dcoffset \
	decimate \
	deinterlace \
	delayaudio \
	delayvideo \
	denoise \
	denoisefft \
	denoiseseltempavg \
	denoisevideo \
	despike \
	diffkey \
	dissolve \
	dot \
	downsample \
        dropout \
	edge \
	fieldframe \
	flanger \
	flash \
	flip \
	framefield \
	freeverb \
	freezeframe \
	gain \
	gamma \
	gradient \
	graphic \
	histogram \
	holo \
	huesaturation \
	interpolate \
	interpolateaudio \
	interpolatevideo \
	invertaudio \
	invertvideo \
	irissquare \
	ivtc \
	lens \
	level \
	linearblur \
	liveaudio \
	livevideo \
	loopaudio \
	loopvideo \
	motion \
	motion2point \
	motionblur \
        mute \
	normalize \
	oilpainting \
	overlay \
	overlayaudio \
	perspective \
	photoscale \
	pitch \
	polar \
        posterize \
	radialblur \
	reframe \
	reframert \
	removegaps \
	resample \
	resamplert \
	reverb \
	reverseaudio \
	reversevideo \
	rgb601 \
	rotate \
	scale \
	shapewipe \
	sharpen \
	shiftinterlace \
	slide \
	spectrogram \
	spherecam \
	spheretranslate \
	suv \
	swapchannels \
	swapframes \
        swatch \
	synthesizer \
	threshold \
	timeavg \
	timefront \
        timelapsehelper \
	timestretch \
	timestretchrt \
	titler \
	translate \
        tremolo \
	unsharp \
	videoscope \
	vocoder \
	wave \
	whirl \
	wipe \
	yuv \
	zoom \
	zoomblur

#	findobject \

# Make static table of all plugins
ifeq ($(DO_STATIC), y)

endif


all: $(DIRS)

# force parallel compilation of all directories
.PHONY: FORCE
$(DIRS): FORCE
	$(MAKE) -C $@

clean:
	find \( -name core \
		-o -name '*.o' \
		-o -name '*.plugin' \) -exec rm -rf {} \;

wc:
	cat *.C *.h | wc
	$(LOOP_BEGIN) \
		 $(MAKE) -C $$i wc; \
	$(LOOP_END)


# Can't strip the themes so instead the plugins strip themselves
install:
#	strip ../bin/*.plugin
#	rm -rf ../bin/*.plugin
#	cp $(OBJDIR)/*.plugin ../bin/
	mkdir -p ../bin/fonts
	cp fonts/* ../bin/fonts
	cp -a previews ../bin
	mkdir -p ../bin/shapes
	cp shapes/* ../bin/shapes
	cp ../thirdparty/mjpegtools*/mpeg2enc/mpeg2enc ../bin/mpeg2enc.plugin



