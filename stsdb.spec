Name:         stsdb
Version:      1.0
Release:      alt1

Summary:      Simple time series database.
Group:        System
URL:          https://github.com/slazav/stsdb
License:      GPL

Packager:     Vladislav Zavjalov <slazav@altlinux.org>

Source:       %name-%version.tar
Source1:      stsdb_http.init
BuildRequires: libmicrohttpd-devel libjansson-devel
Requires:      libmicrohttpd libjansson

%description
STSDB -- a simple time series database

%prep
%setup -q

%build
%makeinstall
install -pD -m755 %_sourcedir/stsdb_http.init %buildroot%_initdir/stsdb_http

%post
%post_service stsdb_http

%preun
%preun_service stsdb_http

%files
%dir %_sharedstatedir/stsdb
%_bindir/stsdb
%_bindir/stsdb_http
%config %_initdir/stsdb_http

%changelog
