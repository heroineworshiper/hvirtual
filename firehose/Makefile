# i686 or mips

TARGET = i686
#TARGET = mips
VERSION = 0.6.0

ifeq ($(origin CFLAGS), environment)
HAVE_CFLAGS := y
else
HAVE_CFLAGS := n
endif


ifeq ($(TARGET), i686)
GCC = gcc
ifneq ($(HAVE_CFLAGS), y)
  CFLAGS := -O3 -fmessage-length=0 -funroll-all-loops -fomit-frame-pointer -falign-loops=2 -falign-jumps=2 -falign-functions=2 -I/usr/local/include
endif
endif

ifeq ($(TARGET), mips)
TOOLCHAIN := /usr/local/uclib-mipsel/
GCC = $(TOOLCHAIN)/bin/mipsel-linux-uclibc-gcc
CFLAGS := -mips2 -O \
	-I$(TOOLCHAIN)/include \
	-I$(TOOLCHAIN)/lib/gcc-lib/mipsel-linux/3.0.4/include
endif


OUTPUT = $(TARGET)/libfirehose.a
RECV = $(TARGET)/firerecv
SEND = $(TARGET)/firesend
FIREPIPE = $(TARGET)/firepipe
DATESERVE = $(TARGET)/dateserve
DATEGET = $(TARGET)/dateget
SWAPON = $(TARGET)/swapon
BOTTOM = $(TARGET)/bottom
EXTRACT = $(TARGET)/extract

LIBS = $(OUTPUT) -lpthread 
CFLAGS += -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
COMPILER := $(GCC) `cat $(TARGET)/cflags`



$(shell mkdir -p $(TARGET) )
$(shell echo $(CFLAGS) > $(TARGET)/cflags )


all: $(OUTPUT) $(RECV) $(SEND) $(FIREPIPE) $(DATESERVE) $(DATEGET) $(BOTTOM) $(EXTRACT)

$(OUTPUT): firehose.c
	$(COMPILER) -c firehose.c -o $(TARGET)/firehose.o
	ar rcs $(OUTPUT) $(TARGET)/firehose.o

$(RECV): $(OUTPUT) firerecv.c
	$(COMPILER) -o $(RECV) firerecv.c $(LIBS)

$(SEND): $(OUTPUT) firesend.c
	$(COMPILER) -o $(SEND) firesend.c $(LIBS)

$(FIREPIPE): $(OUTPUT) firepipe.c
	$(COMPILER) -o $(FIREPIPE) firepipe.c $(LIBS)

$(DATESERVE): dateserve.c
	$(COMPILER) -o $(DATESERVE) dateserve.c

$(DATEGET): dateget.c
	$(COMPILER) -o $(DATEGET) dateget.c

$(SWAPON): swapon.c
	$(COMPILER) -o $(SWAPON) swapon.c

$(BOTTOM): bottom.c
	$(COMPILER) -o $(BOTTOM) bottom.c

$(EXTRACT): extract.c
	$(COMPILER) -o $(EXTRACT) extract.c

clean: 
	rm -rf $(TARGET)

tar: clean
	cd .. && \
	tar jcf firehose-$(VERSION)-src.tar.bz2 firehose

install:
ifdef HOST
	scp $(RECV) $(SEND) $(FIREPIPE) $(DATESERVE) $(DATEGET) $(BOTTOM) $(EXTRACT) root@$(HOST):/bin
else
	cp $(RECV) $(SEND) $(FIREPIPE) $(DATESERVE) $(DATEGET) $(BOTTOM) $(EXTRACT) /usr/bin
endif




