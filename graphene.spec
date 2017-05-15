Name:         graphene
Version:      2.5
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
graphene -- a simple time series database with nanosecond precision for scientific applications

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
%attr(2770,root,users) %dir %_sharedstatedir/graphene
%_bindir/graphene
%_bindir/graphene_http
%config %_initdir/graphene_http

%changelog
* Mon May 15 2017 Vladislav Zavjalov <slazav@altlinux.org> 2.5-alt1
- v2.5
 - use database environment to allow multiple-program operation

* Fri May 05 2017 Vladislav Zavjalov <slazav@altlinux.org> 2.4-alt1
- v2.4
 - change sync command: now it does sync() instead of closing databases
 - add close command for closing databases
 - add *idn? command
 
* Sun Apr 30 2017 Vladislav Zavjalov <slazav@altlinux.org> 2.3-alt1
- v2.3
 - change communication protocol in the interactive mode
 - interactive mode is called by -i option
 - use attr(2770,root,users) for database folder to allow users
   create their own databases

* Fri Dec 16 2016 Vladislav Zavjalov <slazav@altlinux.org> 2.2-alt1
- v.2.2
 - fix error in get_range command

* Sun Dec 11 2016 Vladislav Zavjalov <slazav@altlinux.org> 2.1-alt1
- v2.1
  - remove tcl packages (see https://github.com/slazav/exp_tcl)
  - bugfixes

* Fri Nov 18 2016 Vladislav Zavjalov <slazav@altlinux.org> 2.0-alt1
- v2.0
  - improved interactive mode
  - <seconds>.<nanoseconds> time format
  - v2 databases with nanosecond precision (v1 databases still work)
  - tcl packages

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
