%define prefix /usr

Name: libiec61883
Version: 1.0.0
Release: 1
License: LGPL
Group: Libraries
Source: http://linux1394.org/dl/libiec61883-1.0.0.tar.gz
URL: http://linux1394.org
BuildRoot: /var/tmp/libiec61883-1.0.0-root
Summary: Streaming library for IEEE1394

%changelog

%description 

The libiec61883 library provides an higher level API for streaming DV,
MPEG-2 and audio over IEEE1394.  Based on the libraw1394 isochronous
functionality, this library acts as a filter that accepts DV-frames,
MPEG-2 frames or audio samples from the application and breaks these
down to isochronous packets, which are transmitted using libraw1394.

Requires: libraw1394 >= 1.2.0

%package devel
Summary:  Development libs for libiec61883
Group:    Development/Libraries
Requires: %{name} = %{version}


%description devel
Development libraries needed to build applications against libiec61883

%changelog

%prep
%setup

%build
CFLAGS="$RPM_OPT_FLAGS" ./configure --prefix=%{prefix}
make

%install
rm -rf $RPM_BUILD_ROOT

make prefix=$RPM_BUILD_ROOT%{prefix} install

%clean
rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-, root, root)

%doc AUTHORS COPYING NEWS README
%{prefix}/lib/libiec61883.so.*

%files devel
%defattr(-, root, root)

%{prefix}/lib/*.so
%{prefix}/lib/*a
%{prefix}/include/*
%{prefix}/lib/pkgconfig/libiec61883.pc

%doc examples/*.c
