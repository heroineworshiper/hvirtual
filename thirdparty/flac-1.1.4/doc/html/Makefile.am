#  FLAC - Free Lossless Audio Codec
#  Copyright (C) 2001,2002,2003,2004,2005,2006,2007  Josh Coalson
#
#  This file is part the FLAC project.  FLAC is comprised of several
#  components distributed under difference licenses.  The codec libraries
#  are distributed under Xiph.Org's BSD-like license (see the file
#  COPYING.Xiph in this distribution).  All other programs, libraries, and
#  plugins are distributed under the GPL (see COPYING.GPL).  The documentation
#  is distributed under the Gnu FDL (see COPYING.FDL).  Each file in the
#  FLAC distribution contains at the top the terms under which it may be
#  distributed.
#
#  Since this particular file is relevant to all components of FLAC,
#  it may be distributed under the Xiph.Org license, which is the least
#  restrictive of those mentioned above.  See the file COPYING.Xiph in this
#  distribution.

AUTOMAKE_OPTIONS = foreign

SUBDIRS = ru images

docdir = $(datadir)/doc/$(PACKAGE)-$(VERSION)/html

doc_DATA = \
	changelog.html \
	comparison.html \
	developers.html \
	documentation.html \
	documentation_bugs.html \
	documentation_format_overview.html \
	documentation_tasks.html \
	documentation_tools.html \
	documentation_tools_flac.html \
	documentation_tools_metaflac.html \
	documentation_tools_plugins.html \
	download.html \
	faq.html \
	favicon.ico \
	features.html \
	flac.css \
	format.html \
	id.html \
	index.html \
	itunes.html \
	license.html \
	links.html \
	news.html \
	ogg_mapping.html

EXTRA_DIST = $(doc_DATA) api

# The install targets don't copy whole directories so we have to
# handle 'api/' specially:
install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(docdir)/api
	(cd api && $(INSTALL_DATA) * $(DESTDIR)$(docdir)/api)
uninstall-local:
	rm -rf $(DESTDIR)$(docdir)/api
maintainer-clean-local:
	rm -rf api
