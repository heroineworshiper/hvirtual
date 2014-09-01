include ../../global_config

OBJS = \
	jcapimin.o \
	jcapistd.o \
	jctrans.o \
	jcparam.o \
	jdatadst.o \
	jcinit.o \
	jcmaster.o \
	jcmarker.o \
	jcmainct.o \
	jcprepct.o \
	jccoefct.o \
	jccolor.o \
	jcsample.o \
	jchuff.o \
	jcphuff.o \
	jcdctmgr.o \
	jfdctfst.o \
	jfdctflt.o \
	jfdctint.o \
	jdapimin.o \
	jdapistd.o \
	jdtrans.o \
	jdatasrc.o \
	jdmaster.o \
	jdinput.o \
	jdmarker.o \
	jdhuff.o \
	jdphuff.o \
	jdmainct.o \
	jdcoefct.o \
	jdpostct.o \
	jddctmgr.o \
	jidctfst.o \
	jidctflt.o \
	jidctint.o \
	jidctred.o \
	jdsample.o \
	jdcolor.o \
	jquant1.o \
	jquant2.o \
	jdmerge.o \
	jcomapi.o \
	jutils.o \
	jerror.o \
	jmemmgr.o \
	jmemnobs.o


libjpeg.a: $(OBJS)
	ar rcs libjpeg.a $(OBJS)

.c.o:
	$(CC) -c `./c_flags` $*.c

clean:
	rm -f *.o core *.a
