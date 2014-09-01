Name: libavc1394
Version: 0.5.1
Release: 1
Copyright: GPL
Group: Multimedia
Source0: http://download.sourceforge.net/libavc1394/%{name}-%{version}.tar.gz
Buildroot: %{_tmppath}/%{name}-%{version}-root
Requires: libraw1394
BuildRequires: libraw1394-devel

Summary: FireWire AV/C interface
Summary(pt_BR): FireWire AV/C interface
Summary(es): FireWire AV/C interface

%description
libavc1394 is a programming interface to the AV/C specification from the
1394 Trade Assocation. AV/C stands for Audio/Video Control.  Currently,
applications use the library to control the tape transport mechansim
on DV camcorders. However, there are many devices and functions of
devices that can be controlled via AV/C. Eventually, the library will
be expanded to implement more of the specification and to provide high
level interfaces to various devices.

%package devel
Summary: Development and include files for libavc1394
Summary(pt_BR): Arquivos de desenvolvimento e cabeçalhos para o libavc1394
Summary(es): Development and include files for libavc1394
Group: Development
Group(pt_BR): Desenvolvimento
Group(es): Desarrollo
Requires: %{name} == %{version}-%{release}
AutoProv: no

%package devel-static
Summary: Development components for libavc1394
Summary(pt_BR): Componentes estáticos de desenvolvimento para o libavc1394
Summary(es): Development components for libavc1394
Group: Development
Group(pt_BR): Desenvolvimento
Group(es): Desarrollo
Requires: %{name}-devel == %{version}-%{release}

%description -n libavc1394-devel
libavc1394 is a programming interface to the AV/C specification from the
1394 Trade Assocation. AV/C stands for Audio/Video Control.  Currently,
applications use the library to control the tape transport mechansim
on DV camcorders. However, there are many devices and functions of
devices that can be controlled via AV/C. Eventually, the library will
be expanded to implement more of the specification and to provide high
level interfaces to various devices.

This archive contains the header files for libavc1394 development.

%description -n libavc1394-devel-static
libavc1394 is a programming interface to the AV/C specification from the
1394 Trade Assocation. AV/C stands for Audio/Video Control.  Currently,
applications use the library to control the tape transport mechansim
on DV camcorders. However, there are many devices and functions of
devices that can be controlled via AV/C. Eventually, the library will
be expanded to implement more of the specification and to provide high
level interfaces to various devices.

This archive contains the static libraries (.a) for libavc1394 development.


%prep
rm -rf %{buildroot}

%setup -q

%build
%configure --prefix=/usr
make

%install
make install DESTDIR=%{buildroot}

%post
ldconfig

%postun
ldconfig

%clean
rm -rf %{buildroot}

%files
%defattr(0644,root,root)
%doc README NEWS INSTALL COPYING AUTHORS TODO
%{_libdir}/libavc1394.so
%{_libdir}/libavc1394.so.0
%{_libdir}/libavc1394.so.0.1.1
%{_libdir}/librom1394.so
%{_libdir}/librom1394.so.0
%{_libdir}/librom1394.so.0.1.1
%attr(0755,root,root) %{_bindir}/dvcont
%attr(0755,root,root) %{_bindir}/mkrfc2734
%{_mandir}/man1/*.gz

%files -n libavc1394-devel
%defattr(0644,root,root)
%attr(0755,root,root) %dir %{_includedir}/libavc1394
%{_includedir}/libavc1394/avc1394.h
%{_includedir}/libavc1394/avc1394_vcr.h
%{_includedir}/libavc1394/rom1394.h
%{_libdir}/pkgconfig/libavc1394.pc

%files -n libavc1394-devel-static
%defattr(0644,root,root)
%{_libdir}/libavc1394.a
%{_libdir}/libavc1394.la
%{_libdir}/librom1394.a
%{_libdir}/librom1394.la


%changelog
* Thu Feb 17 2005 Dan Dennedy <dan@dennedy.org>
- added pkg-config file
- added mkrfc2734 binary

* Fri May 21 2004 Steven Boswell <ulatec@users.sourceforge.net>
- initial spec
