all:
	make -C graphene

prefix  ?= /usr
bindir  ?= $(prefix)/bin
sysconfdir ?= /etc
initdir    ?= $(sysconfdir)/init.d

install: all
	mkdir -p ${bindir} ${initdir}
	install graphene/graphene ${bindir}
	install graphene/graphene_http ${bindir}
	install graphene_http.init ${initdir}/graphene_http
	install scripts/graphene_tab ${bindir}
	install scripts/graphene_int ${bindir}
	install scripts/graphene_sync ${bindir}
	install scripts/graphene_sweeps ${bindir}
	install scripts/graphene_filter ${bindir}
	install scripts/graphene_mkcomm ${bindir}
