# Note that this is NOT a relocatable package
%define ver      0.2.8
%define rel      2
%define prefix   /usr

Summary:   The Enlightened Sound Daemon
Name:      esound
Version:   %ver
Release:   %rel
Copyright: GPL
Group:     Daemons
Source0:   esound-%{PACKAGE_VERSION}.tar.gz
URL:       http://www.tux.org/~ricdude/EsounD.html
BuildRoot: /tmp/esound-%{PACKAGE_VERSION}-root
Packager:  Michael Fulbright <msf@redhat.com>
Docdir: %{prefix}/doc
Summary(es): Demonio de sonido de Enlightment
Summary(fr): Démon audio de Enlightment

%description
The Enlightened Sound Daemon is a server process that allows multiple
applications to share a single sound card.

%description -l es
El demonio de sonido de Enlightment es un proceso servidor que permite
a más de una aplicación de usar la tarjeta de sonido al mismo tiempo

%description -l fr
Le démon audio de Enlightment est un processus serveur qui permets à
plusieures applications d'utilsier la carte son à la fois.

%package devel
Summary: Libraries, includes, etc to develop EsounD applications
Group: Libraries
Requires: esound
Summary(es): Bibliotecas, includes, etc para desarrollar programas para EsounD
Summary(fr): Bibliothèques, includes, etc pour programmer pour EsounD

%description devel
Libraries, include files, etc you can use to develop EsounD applications.

%description devel -l es
Bibliotecas, archivos *.h, etc necesarios para escribir programas con
soporte para EsounD

%description devel -l fr
Bibliothèques, fichiers d'en-têtes, etc. necessaires pour écrire des
applications avec support EsounD
 
%prep
%setup

%build
# Needed for snapshot releases.
if [ ! -f configure ]; then
  CFLAGS="$RPM_OPT_FLAGS" ./autogen.sh --prefix=%prefix
else
  CFLAGS="$RPM_OPT_FLAGS" ./configure --prefix=%prefix
fi

if [ "$SMP" != "" ]; then
  (make "MAKE=make -k -j $SMP"; exit 0)
  make
else
  make
fi

%install
rm -rf $RPM_BUILD_ROOT

make prefix=$RPM_BUILD_ROOT%{prefix} install

%post
/sbin/ldconfig

%postun -p /sbin/ldconfig

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-, root, root)
%doc AUTHORS COPYING ChangeLog EsounD.html INSTALL NEWS README TIPS TODO
%{prefix}/bin/*
%{prefix}/lib/lib*.so.*

%files devel
%defattr(-, root, root)

%{prefix}/lib/lib*.so
%{prefix}/lib/*a
%{prefix}/include/*
%{prefix}/share/aclocal/*

%changelog
* Sat Nov 21 1998 Pablo Saratxaga <srtxg@chanae.alphanet.ch>

- added %{prefix}/share/aclocal/* to %files devel
- added spanish and french translations for rpm

* Thu Oct 1 1998 Ricdude <ericmit@ix.netcom.com>

- make autoconf do the version updating for us.

* Wed May 13 1998 Michael Fulbright <msf@redhat.com>

- First try at an RPM
