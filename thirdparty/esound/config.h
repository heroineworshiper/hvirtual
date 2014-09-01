/* config.h.  Generated automatically by configure.  */
/* config.h.in.  Generated automatically from configure.in by autoheader.  */

/* Define if using alloca.c.  */
/* #undef C_ALLOCA */

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define to one of _getb67, GETB67, getb67 for Cray-2 and Cray-YMP systems.
   This function is required for alloca.c support on those systems.  */
/* #undef CRAY_STACKSEG_END */

/* Define if you have alloca, as a function or macro.  */
#define HAVE_ALLOCA 1

/* Define if you have <alloca.h> and it should be used (not on Ultrix).  */
#define HAVE_ALLOCA_H 1

/* If using the C implementation of alloca, define if you know the
   direction of stack growth for your system; otherwise it will be
   automatically deduced at run-time.
 STACK_DIRECTION > 0 => grows toward higher addresses
 STACK_DIRECTION < 0 => grows toward lower addresses
 STACK_DIRECTION = 0 => direction of growth unknown
 */
/* #undef STACK_DIRECTION */

/* Define if your processor stores words with the most significant
   byte first (like Motorola and SPARC, unlike Intel and VAX).  */
/* #undef WORDS_BIGENDIAN */

#define DRIVER_OSS 1
/* #undef DRIVER_AIX */
/* #undef DRIVER_IRIX */
/* #undef DRIVER_HPUX */
/* #undef DRIVER_SOLARIS */
/* #undef DRIVER_MKLINUX */
/* #undef DRIVER_ALSA */
/* #define DRIVER_NEWALSA 1 */
/* #undef DRIVER_NONE */
#define HAVE_INET_ATON 1
#define HAVE_NANOSLEEP 1
/* #undef USE_LIBWRAP */
/* #undef WITH_SYMBOL_UNDERSCORE */
/* #undef ESDBG */ 

/* #undef INADDR_LOOPBACK */
#define HAVE_SUN_LEN 1

/* Define if you have the putenv function.  */
#define HAVE_PUTENV 1

/* Define if you have the setenv function.  */
#define HAVE_SETENV 1

/* Define if you have the usleep function.  */
#define HAVE_USLEEP 1

/* Define if you have the <dmedia/audio.h> header file.  */
/* #undef HAVE_DMEDIA_AUDIO_H */

/* Define if you have the <machine/soundcard.h> header file.  */
/* #undef HAVE_MACHINE_SOUNDCARD_H */

/* Define if you have the <soundcard.h> header file.  */
/* #undef HAVE_SOUNDCARD_H */

/* Define if you have the <sun/audioio.h> header file.  */
/* #undef HAVE_SUN_AUDIOIO_H */

/* Define if you have the <sys/asoundlib.h> header file.  */
#define HAVE_SYS_ASOUNDLIB_H 1

/* Define if you have the <sys/audio.h> header file.  */
/* #undef HAVE_SYS_AUDIO_H */

/* Define if you have the <sys/audio.io.h> header file.  */
/* #undef HAVE_SYS_AUDIO_IO_H */

/* Define if you have the <sys/audioio.h> header file.  */
/* #undef HAVE_SYS_AUDIOIO_H */

/* Define if you have the <sys/filio.h> header file.  */
/* #undef HAVE_SYS_FILIO_H */

/* Define if you have the <sys/ioctl.h> header file.  */
#define HAVE_SYS_IOCTL_H 1

/* Define if you have the <sys/soundcard.h> header file.  */
#define HAVE_SYS_SOUNDCARD_H 1

/* Define if you have the <sys/soundlib.h> header file.  */
/* #undef HAVE_SYS_SOUNDLIB_H */

/* Define if you have the asound library (-lasound).  */
#define HAVE_LIBASOUND 1

/* Define if you have the audio library (-laudio).  */
/* #undef HAVE_LIBAUDIO */

/* Define if you have the nsl library (-lnsl).  */
/* #undef HAVE_LIBNSL */

/* Define if you have the ossaudio library (-lossaudio).  */
/* #undef HAVE_LIBOSSAUDIO */

/* Define if you have the resolv library (-lresolv).  */
/* #undef HAVE_LIBRESOLV */

/* Define if you have the rt library (-lrt).  */
/* #undef HAVE_LIBRT */

/* Define if you have the socket library (-lsocket).  */
/* #undef HAVE_LIBSOCKET */

/* Define if you have the sound library (-lsound).  */
/* #undef HAVE_LIBSOUND */

/* Name of package */
#define PACKAGE "esound"

/* Version number of package */
#define VERSION "0.2.17"

/* keep these at the end of the generated config.h */

#ifndef HAVE_SUN_LEN
#define SUN_LEN(ptr) ((size_t)(((struct sockaddr_un *) 0)->sun_path) + strlen ((ptr)->sun_path))
#endif
