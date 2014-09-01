# process this file with automake to create a Makefile.in

SUBDIRS        = src examples

aclocaldir     = @datadir@/aclocal
pkgconfigdir   = $(libdir)/pkgconfig
pkgconfig_DATA = libiec61883.pc

MAINTAINERCLEANFILES = Makefile.in aclocal.m4 configure config.h.in

EXTRA_DIST = libiec61883.spec

dist-hook:
	svn log >$(distdir)/ChangeLog

doc:
	/usr/src/linux/scripts/kernel-doc -man src/iec61883.h | nroff -man | less
