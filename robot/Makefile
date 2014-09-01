include ../global_config

PREFIX := /home/artemis/root/usr/bin
DEST_DIR := /usr/bin

TARGET := $(OBJDIR)/heroine1120

OBJS := \
	$(OBJDIR)/allocationgui.o \
	$(OBJDIR)/checkin.o \
	$(OBJDIR)/checkingui.o \
	$(OBJDIR)/checkout.o \
	$(OBJDIR)/checkoutgui.o \
	$(OBJDIR)/delete.o \
	$(OBJDIR)/deletegui.o \
	$(OBJDIR)/filegui.o \
	$(OBJDIR)/filethread.o \
	$(OBJDIR)/import.o \
	$(OBJDIR)/importgui.o \
	$(OBJDIR)/manual.o \
	$(OBJDIR)/manualgui.o \
	$(OBJDIR)/move.o \
	$(OBJDIR)/movegui.o \
	$(OBJDIR)/mwindow.o \
	$(OBJDIR)/mwindowgui.o \
	$(OBJDIR)/options.o \
	$(OBJDIR)/optionsgui.o \
	$(OBJDIR)/robotalloc.o \
	$(OBJDIR)/robotclient.o \
	$(OBJDIR)/robotdb.o \
	$(OBJDIR)/robotprefs.o \
	$(OBJDIR)/robottheme.o \
	$(OBJDIR)/search.o \
	$(OBJDIR)/searchgui.o \
	$(OBJDIR)/splashgui.o \
	$(OBJDIR)/textfile.o



CFLAGS += \
	-I../guicast \
	-I../quicktime

ifeq ($(OBJDIR), x86_64)
LIBS := -L/usr/X11R6/lib64
else
LIBS := -L/usr/X11R6/lib
endif

LIBS += ../guicast/$(OBJDIR)/libguicast.a \
	../quicktime/$(OBJDIR)/libquicktime.a \
	$(GLLIBS) \
	-lX11 \
	-lXext \
	-lXv \
	-lXxf86vm \
	-lpthread \
	-lm \
	-lpng \
	-lz \
	-ldl





LINKER := g++ -o $(TARGET)

UTIL_TARGET := $(OBJDIR)/robot
UTIL_OBJS := $(OBJDIR)/robot.o \
	$(OBJDIR)/robotserver.o
UTIL_CFLAGS := -O2 -Iparapin
UTIL_LIBS = parapin/libparapin.a -lpthread


# microprocessor interface library
MICRO_OBJS := $(OBJDIR)/lib68hc11e1.o
MICRO_OUTPUT := $(OBJDIR)/lib68hc11e1.a


TEST_OBJS := $(OBJDIR)/tracker.o
TEST_OUTPUT := $(OBJDIR)/tracker
#TEST_OBJS := $(OBJDIR)/loader.o
#TEST_OUTPUT := $(OBJDIR)/loader


# program to be run from RAM
ASM_PROGRAM := networth
ASM_PROGRAM := adc
ASM_PROGRAM := beer
ASM_PROGRAM := animation
ASM_PROGRAM := distance
ASM_PROGRAM := dumpeeprom
ASM_PROGRAM := vector
ASM_PROGRAM := fortune
ASM_PROGRAM := angle
ASM_PROGRAM := ic
ASM_PROGRAM := echo
ASM_PROGRAM := sensor
ASM_PROGRAM := oscillate
ASM_PROGRAM := tracker3
ASM_PROGRAM := pwm2
ASM_PROGRAM := loader
ASM_PROGRAM := pulse
ASM_PROGRAM := hello
ASM_PROGRAM := chargepump

$(shell echo $(CFLAGS) > $(OBJDIR)/c_flags)
$(shell echo $(OBJS) $(LIBS) > $(OBJDIR)/objs)

all: $(TARGET) $(UTIL_TARGET)

$(TARGET): $(OBJS)
	cd $(OBJDIR) && ../../guicast/$(OBJDIR)/bootstrap data.o ../data/*.png
	$(LINKER) `cat $(OBJDIR)/objs`  $(OBJDIR)/data.o
	strip $(TARGET)
	

$(UTIL_TARGET): $(UTIL_OBJS)
	$(MAKE) -C parapin
	gcc -o $(UTIL_TARGET) $(UTIL_OBJS) $(UTIL_LIBS)


micro: $(MICRO_OUTPUT) $(TEST_OUTPUT) $(ASM_PROGRAM)

$(TEST_OUTPUT): $(TEST_OBJS) $(MICRO_OUTPUT)
	gcc -o $(TEST_OUTPUT) $(TEST_OBJS) $(MICRO_OUTPUT)

$(MICRO_OUTPUT): $(MICRO_OBJS)
	ar rcs $(MICRO_OUTPUT) $(MICRO_OBJS)

$(ASM_PROGRAM): $(ASM_PROGRAM).s
	cp ram.x memory.x
	m6811-elf-as -o $(ASM_PROGRAM).o $(ASM_PROGRAM).s
	m6811-elf-ld -m m68hc11elfb --oformat srec --defsym _start=0 -o $(ASM_PROGRAM).s19 $(ASM_PROGRAM).o

clean:
	rm -rf $(OBJDIR)
	rm -f run *.s19 *.o

install:
	cp $(TARGET) $(DEST_DIR)

$(UTIL_OBJS) $(MICRO_OBJS) $(TEST_OBJS):
	gcc -c $(UTIL_CFLAGS) $(subst $(OBJDIR)/,, $*.c) -o $*.o

$(OBJS):
	$(CC) -c `cat $(OBJDIR)/c_flags` $(subst $(OBJDIR)/,, $*.C) -o $*.o


$(OBJDIR)/allocationgui.o:  allocationgui.C
$(OBJDIR)/checkin.o: 	    checkin.C
$(OBJDIR)/checkingui.o:     checkingui.C
$(OBJDIR)/checkout.o: 	    checkout.C
$(OBJDIR)/checkoutgui.o:    checkoutgui.C
$(OBJDIR)/delete.o: 	    delete.C
$(OBJDIR)/deletegui.o:      deletegui.C
$(OBJDIR)/filegui.o: 	    filegui.C
$(OBJDIR)/filethread.o:     filethread.C
$(OBJDIR)/import.o: 	    import.C
$(OBJDIR)/importgui.o:      importgui.C
$(OBJDIR)/manual.o:         manual.C
$(OBJDIR)/manualgui.o:      manualgui.C
$(OBJDIR)/move.o: 	    move.C
$(OBJDIR)/movegui.o: 	    movegui.C
$(OBJDIR)/mwindow.o: 	    mwindow.C
$(OBJDIR)/mwindowgui.o:     mwindowgui.C
$(OBJDIR)/options.o: 	    options.C
$(OBJDIR)/optionsgui.o:     optionsgui.C
$(OBJDIR)/robotalloc.o:     robotalloc.C
$(OBJDIR)/robotclient.o:    robotclient.C
$(OBJDIR)/robotdb.o: 	    robotdb.C
$(OBJDIR)/robotoptions.o:   robotoptions.C
$(OBJDIR)/robotprefs.o:     robotprefs.C
$(OBJDIR)/robottheme.o:     robottheme.C
$(OBJDIR)/search.o: 	    search.C
$(OBJDIR)/searchgui.o:      searchgui.C
$(OBJDIR)/splashgui.o:      splashgui.C
$(OBJDIR)/textfile.o:       textfile.C




$(OBJDIR)/robot.o: robot.c
$(OBJDIR)/robotserver.o: robotserver.c


$(OBJDIR)/lib68hc11e1.o: lib68hc11e1.c

$(OBJDIR)/tracker.o:     tracker.c
$(OBJDIR)/loader.o:      loader.c



