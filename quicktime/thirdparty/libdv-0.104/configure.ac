dnl Process this file with autoconf to produce a configure script.
AC_INIT(libdv/parse.c)
dnl AC_CONFIG_AUX_DIR(config)
AM_CONFIG_HEADER(config.h)

RPM_RELEASE=1
AC_CANONICAL_HOST
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE(libdv, 0.104)

AM_MAINTAINER_MODE
AC_LIBTOOL_PICMODE(no)

dnl Checks for programs.

AC_SUBST(CFLAGS)
AC_PROG_CC
AM_PROG_AS
AS="${CC}"	# stupid automake bug

dnl CPPFLAGS=-I'$(top_srcdir)'/src

dnl Add -Wall if gcc
if test "$GCC" = "yes"; then
   CFLAGS="$CFLAGS -Wall"
fi

AH_TEMPLATE([_GNU_SOURCE],
            [Define to enable GNU extensions of glibc, notably large file support])
           
AH_TEMPLATE([YUV_420_USE_YV12],
            [Define to 1 to use planar YUV format for 420 blocks in IEC PAL])

AH_TEMPLATE([ARCH_X86],
            [Define as 1 if host is an IA32])

AH_TEMPLATE([ARCH_X86_64],
            [Define as 1 if host is an AMD64])

AH_TEMPLATE([DEBUG], 
	    [Define to indicate DEBUGGING is enabled possibly with a level])

AH_TEMPLATE([HAVE_SDL], 
	    [Define as 1 if you have libsdl])

AH_TEMPLATE([HAVE_GTK], 
	    [Define as 1 if you have gtk])

AH_TEMPLATE([HAVE_LIBXV],
            [Define if you have the `Xv' library (-lXv).])

AC_DEFINE(_GNU_SOURCE)

AC_PROG_INSTALL
AC_PROG_LIBTOOL

use_asm=:
arch_x86=false
arch_x86_64=false

AC_ARG_ENABLE(asm,
[  --disable-asm           disable use of architecture specific assembly code],
[	if test "$enableval" = "no"; then
		use_asm=false
	fi
])

use_sdl=false
AC_ARG_ENABLE(sdl,
[  --enable-sdl            enable use of SDL for display],
[     if test "$enableval" = "yes"; then
               use_sdl=:
       fi
])

use_gtk=:
AC_ARG_ENABLE(gtk,
[  --disable-gtk           disable use of gtk for display],
[     if test "$enableval" = "no"; then
               use_gtk=false
       fi
])

use_xv=:
AC_ARG_ENABLE(xv,
[  --disable-xv            disable use of XVideo extension for display],
[     if test "$enableval" = "no"; then
               use_xv=false
       fi
])

AC_ARG_ENABLE(gprof,
[  --enable-gprof          enable compiler options for gprof profiling],
[	if test "$enableval" = "yes"; then
	  CFLAGS="$CFLAGS -g -pg"
	fi
])

if $use_asm; then
	case "$host_cpu" in
		i?86)
			arch_x86=true
			AC_DEFINE(ARCH_X86)
			;;
		x86_64)
			arch_x86_64=true
			AC_DEFINE(ARCH_X86_64)
			;;
		*)
			;;
	esac
fi

AC_ARG_WITH(pal-yuv,
[  --with-pal-yuv=(YV12|YUY2)  Select YUV format for IEC PAL (YUY2 by default) ],
[
	case "$with_pal_yuv" in
	     YV12)
		AC_MSG_RESULT(YV12)
		AC_DEFINE(YUV_420_USE_YV12)
		;;
	     *)
            	AC_MSG_RESULT(YUY2)
		;;
	esac
]
)

AC_MSG_CHECKING(whether to enable debugging)
AC_ARG_WITH(debug,
[  --with-debug[=level]    Enable debugging.
  --without-debug	  Disable debugging (default). ],
[
	case "$with_debug" in
	     no)
		AC_MSG_RESULT(no)
		;;
	     yes)
	        AC_MSG_RESULT(yes)
		AC_DEFINE(DEBUG)		
		CFLAGS="$CFLAGS -g"
		;;
	     *)
	        AC_MSG_RESULT(DEBUG=$with_debug)
		AC_DEFINE_UNQUOTED(DEBUG,$with_debug)
		CFLAGS="$CFLAGS -g"
	        ;;
	esac
],
[ AC_MSG_RESULT(default=yes)
  AC_DEFINE(DEBUG)
  CFLAGS="$CFLAGS -g" 
]
)


AM_CONDITIONAL(HOST_X86, test x$arch_x86 = xtrue)
AM_CONDITIONAL(HOST_X86_64, test x$arch_x86_64 = xtrue)



dnl Checks for libraries.

if $use_gtk; then
	REQUIRES='glib >= 1.2.4 gtk+ >= 1.2.4'
	PKG_CHECK_MODULES(GTK,$REQUIRES)
fi

dnl used in Makefile.am
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)
dnl CPPFLAGS="$CPPFLAGS $GTK_CFLAGS"
dnl LDFLAGS="$LDFLAGS $GTK_LIBS"

if $use_sdl; then
	AM_PATH_SDL(1.1.6,
	[
		AC_DEFINE(HAVE_SDL) 
 	])
fi

if $use_gtk; then
	AM_PATH_GTK(1.2.4,
	[
		AC_DEFINE(HAVE_GTK)
	])
fi

if [ $use_gtk && $use_xv ]; then
	AC_CHECK_LIB(Xv, XvQueryAdaptors,
		[AC_DEFINE(HAVE_LIBXV)
			AC_SUBST(XV_LIB)
			XV_LIB='-lXv'
		],
		[AC_MSG_ERROR([Could not find Xv Lib])],
		$GTK_LIBS
	)
fi

if $use_gtk; then
	PLAYDV_SUBDIR='playdv'
else
	PLAYDV_SUBDIR=''
fi
AC_SUBST(PLAYDV_SUBDIR)


dnl *********************************************************************
dnl Check for the pthread lib
dnl
AC_SUBST(PTHREAD_LIBS)
AC_CHECK_LIB(pthread, pthread_create, [ PTHREAD_LIBS="-lpthread" ],,)

OS_ARCH=`uname -s | sed -e 's|/|_|g'`
# FreeBSD and OpenBSD have neither -lpthread nor integrated pthreads in libc.
# Instead -pthread is used to the compiler/linker to get pthreads working right.
if test x$OS_ARCH = xFreeBSD -o x$OS_ARCH = xOpenBSD; then
   CFLAGS="$CFLAGS -pthread"
   CXXFLAGS="$CXXFLAGS -pthread"
fi


AC_CHECK_LIB(m, sqrt)

dnl Don't bother with popt on non-gcc build, because we make heavy use of 
dnl gcc struct initialization extension in our popt code.  (Could be fixed)
REQUIRES_NOPKGCONFIG=""
if test "$GCC" = "yes"; then
      AC_CHECK_LIB(popt, poptGetContext,
      [
        POPT_LIB="-lpopt"
        AC_DEFINE(HAVE_LIBPOPT, 1, [Define if you have the 'popt' library])
      ],
      [
        AC_MSG_WARN(playdv needs the popt package for command line parsing.
 		    The source archive for popt is available at:  
			ftp://ftp.redhat.com/pub/redhat/code/popt)
      ])
fi
AC_SUBST(POPT_LIB)
AC_SUBST(REQUIRES_NOPKGCONFIG)

dnl Checks for header files.
AC_PATH_X
AC_HEADER_STDC
AC_CHECK_HEADERS(fcntl.h sys/time.h unistd.h linux/videodev.h sys/soundcard.h \
endian.h machine/endian.h inttypes.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_HEADER_TIME

dnl Checks for library functions.
AC_FUNC_MMAP
AC_CHECK_FUNCS(gettimeofday)

AC_SUBST(ac_aux_dir)
AC_SUBST(RPM_RELEASE)

AC_CONFIG_FILES(Makefile libdv.spec libdv.pc libdv/Makefile playdv/Makefile encodedv/Makefile)
AC_OUTPUT
