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
Source2:      graphene.xinetd
BuildRequires: libmicrohttpd-devel libjansson-devel
Requires:      libmicrohttpd libjansson

%description
graphene -- a simple time series database

%prep
%setup -q

%build
%makeinstall
install -pD -m755 %_sourcedir/graphene_http.init %buildroot%_initdir/graphene_http
install -pD -m644 %_sourcedir/graphene.xinetd %buildroot/etc/xinetd.d/graphene

%pre
%_sbindir/useradd -c 'Graphene server' -d /var/lib/graphene -s '/dev/null' \
  -r graphene 2>/dev/null || :

%post
%post_service graphene_http

%preun
%preun_service graphene_http

%files
%attr(0700,graphene,graphene) %dir %_sharedstatedir/graphene
%_bindir/graphene
%_bindir/graphene_http
%config %_initdir/graphene_http
%config(noreplace) /etc/xinetd.d/graphene

%changelog
* Wed Jun 01 2016 Vladislav Zavjalov <slazav@altlinux.org> 1.0-alt1
- v1.0

