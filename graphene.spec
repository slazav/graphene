Name:         graphene
Version:      1.1
Release:      alt1

Summary:      Simple time series database.
Group:        System
URL:          https://github.com/slazav/graphene
License:      GPL

Packager:     Vladislav Zavjalov <slazav@altlinux.org>

Source:       %name-%version.tar
Source1:      graphene_http.init
BuildRequires: libmicrohttpd-devel libjansson-devel
Requires:      libmicrohttpd libjansson

%description
graphene -- a simple time series database

%prep
%setup -q

%build
%makeinstall
install -pD -m755 %_sourcedir/graphene_http.init %buildroot%_initdir/graphene_http

%post
%post_service graphene_http

%preun
%preun_service graphene_http

%files
%attr(0755,root,root) %dir %_sharedstatedir/graphene
%_bindir/graphene
%_bindir/graphene_http
%config %_initdir/graphene_http

%changelog
* Sun Jun 05 2016 Vladislav Zavjalov <slazav@altlinux.org> 1.1-alt1
- v1.1
  - rename: stsdb -> graphene
  - user-defined filter programs
  - interactive mode (read commands from stdin without reopening databases)
  - xinetd network service

* Wed Jun 01 2016 Vladislav Zavjalov <slazav@altlinux.org> 1.0-alt1
- v1.0
  - command line interface: reading and writing data, manipulating databases
  - http + simple json interface
