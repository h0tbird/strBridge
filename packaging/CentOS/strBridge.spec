%define name     strBridge
%define ver      0.1
%define rel      1

Name:           %name
Version:        %ver
Release:        %rel%{?dist}
Summary:        A high-performance UDP packet reflector.
Group:          Applications/Internet
License:        GPL2
URL:            https://github.com/h0tbird/strBridge
Source0:        %{name}-%{version}.tar.gz
Packager:       Marc Villacorta Morera <marc.villacorta@gmail.com>
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}

%description
strBridge is the strManager bridge endpoint used to comunicate
CallControl and strManager.

%prep
%setup -q

%build
make

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf "$RPM_BUILD_ROOT"
make install basedir=$RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/etc/rc.d/init.d
install -m755 packaging/CentOS/strBridge.init $RPM_BUILD_ROOT/etc/rc.d/init.d/strBridge

%post
/sbin/chkconfig --add strBridge

%preun
/sbin/service strBridge stop
/sbin/chkconfig --del strBridge

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf "$RPM_BUILD_ROOT"

%files
%defattr(-,root,root)
%dir /etc/strBridge
%config(noreplace) /etc/strBridge/*
%config(noreplace) /etc/rc.d/init.d/*
/usr/bin/strBridge