SUBDIRS                   = libdv encodedv @PLAYDV_SUBDIR@

AUX_DIST                  = $(ac_aux_dir)/config.guess \
                                   $(ac_aux_dir)/config.sub \
                                   $(ac_aux_dir)/install-sh \
                                   $(ac_aux_dir)/ltmain.sh \
                                   $(ac_aux_dir)/missing \
                                   $(ac_aux_dir)/depcomp 

EXTRA_DIST                = bootstrap COPYRIGHT README.encoder README.dvconnect \
                            @PACKAGE@.spec @PACKAGE@.spec.in @PACKAGE@.pc @PACKAGE@.pc.in \
							Makefile.in aclocal.m4 configure config.h.in \
							install-sh depcomp missing config.guess config.sub \
							ltmain.sh libtool

MAINTAINERCLEANFILES      = Makefile.in aclocal.m4 configure config.h.in \
                            stamp-h.in config.log config.status $(AUX_DIST)

CLEANFILES                = 

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libdv.pc

rpm: dist
	rpmbuild -ta @PACKAGE@-@VERSION@.tar.gz

dist-hook:
	(cd $(distdir) && if [ ! -d $(ac_aux_dir); then mkdir $(ac_aux_dir); fi)
	for file in $(AUX_DIST) $(AUX_DIST_EXTRA); do \
		cp $$file $(distdir)/$$file; \
	done

maintainer-clean-local:
	rm -f libdv*.tar.gz
	rm -rf autom*.cache
