%define prefix /usr

Name: libraw1394
Version: 1.2.0
Release: 1
Copyright: LGPL
Group: Libraries
Source: http://linux1394.org/libraw1394/libraw1394-1.2.0.tar.gz
URL: http://linux1394.org
BuildRoot: /var/tmp/libraw1394-1.2.0-root
Summary: Streaming library for IEEE1394
BuildRequires: openjade

%changelog

%description 

The Linux kernel's IEEE 1394 subsystem provides access to the raw 1394
bus through the raw1394 module.  This includes the standard 1394
transactions (read, write, lock) on the active side, isochronous
stream receiving and sending and dumps of data written to the
FCP_COMMAND and FCP_RESPONSE registers.  raw1394 uses a character
device to communicate to user programs using a special protocol.

libraw1394 was created with the intent to hide that protocol from
applications so that

- the protocol has to be implemented correctly only once.

- all work can be done using easy to understand functions instead of
  handling a complicated command structure.

- only libraw1394 has to be changed when raw1394's interface changes.

%package devel
Summary:  Development libs for libraw1394
Group:    Development/Libraries
Requires: %{name} = %{version}


%description devel
Development libraries needed to build applications against libraw1394

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

%doc AUTHORS COPYING.LIB NEWS README
%{prefix}/lib/libraw1394.so.*

%files devel
%defattr(-, root, root)

%{prefix}/bin/*
%{prefix}/lib/*.so
%{prefix}/lib/*a
%{prefix}/include/*
%{prefix}/man/*
%{prefix}/lib/pkgconfig/libraw1394.pc
