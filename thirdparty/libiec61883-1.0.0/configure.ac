# process this file with autoconf to get a configure script

AC_INIT(Makefile.am)
AM_INIT_AUTOMAKE(libiec61883, 1.0.0)
AM_CONFIG_HEADER(config.h)

AC_PROG_CC
AM_PROG_LIBTOOL
AC_PROG_INSTALL

AC_C_CONST
AC_C_BIGENDIAN

PKG_CHECK_MODULES(LIBRAW1394, libraw1394 >= 1.2.0)

# set the libtool so version numbers
lt_current=0
lt_revision=0
lt_age=0

AC_SUBST(lt_current)
AC_SUBST(lt_revision)
AC_SUBST(lt_age)

CFLAGS=${CFLAGS:-"-Wall"}
AC_OUTPUT([
	Makefile src/Makefile examples/Makefile
	libiec61883.pc libiec61883.spec
])
