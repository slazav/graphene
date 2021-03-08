Name:         graphene
Version:      2.9
Release:      alt1

Summary:      Simple time series database.
Group:        System
URL:          https://github.com/slazav/graphene
License:      GPL

Packager:     Vladislav Zavjalov <slazav@altlinux.org>

Source:       %name-%version.tar
BuildRequires: libmicrohttpd-devel libjansson-devel libdb4.7-devel db4.7-utils tcl-devel
Requires:      libmicrohttpd libjansson libdb4.7

%description
graphene -- a simple time series database with nanosecond precision for scientific applications

%prep
%setup -q

%build
tar -xvf modules.tar
%make

%install
%makeinstall initdir=%buildroot%_initdir
mkdir -p %buildroot%_sharedstatedir/graphene

%post
%post_service graphene_http

%preun
%preun_service graphene_http

%files
%attr(2770,root,users) %dir %_sharedstatedir/graphene
%_bindir/graphene
%_bindir/graphene_*
%config %_initdir/graphene_http
%_datadir/graphene/tcllib

%changelog
* Thu Apr 09 2020 Vladislav Zavjalov <slazav@altlinux.org> 2.9-alt1
- v2.9
 - Allow human-readable timestamps on input (YYYY-mm-dd HH:MM:SS).
 - Input and output filters (embedded TCL scripts stored in the database).
   set_filter, print_filter, put_flt commands, <dbname>:f<n> notation.
 - Rewrite backup methods, add backup_reset and backup_print commands.
 - graphene_http server:
    - HTTP GET interface with text output,
    - do not reopen databases on each request,
    - add server tests.
 - graphene_sync script: faster operation (read all points,
   then write all points).
 - Use read_words module to parse commands in interactive mode.
   Quoted strings, newlines, comments, etc are allowed now.
 - Remove old-style filters.
 - Fix env_type in read-only mode.
 - Fix build on systems without bash.
 - Rewrite a large part of code.
 - Add GPL3 license.
 - Use mapsoft2-libs as git-submodule.

* Mon Dec 30 2019 Vladislav Zavjalov <slazav@altlinux.org> 2.8-alt1
- v2.8
 - Rename -E lock parameter according to help message.
   Change default environment type to "lock". (It seems that "txn"
   environment type is broken and not a good choice in most cases).
   This change will require accurate migration from v2.7 to v2.8:
   - Stop all programs working with databases (including graphene_http).
   - Check that theseprograms do not have -E parameter. It is not
     recommended to use any other setting then the default one.
   - Backup your databases.
   - Do db_checkpoint -1 on the database envirnment.
   - Remove log fies (db_archive -d). Delete environment files (__*)
   - Update graphene and start.

 - add backup_start and backup_end commands (database syncronization)
 - add libdb_version command (print libdb version)
 - add get_time command (print current time)
 - list command produces sorted output

 - change error message produced by del command (this fixes tests for
   libdb5 where libdb messages are different)

 - Support for +Inf, -Inf and NaN values in Float and Double databases.
   This modification affects only parsing user input, storage and
   ouput did not change, old and new versions are fully compatable here.

 - avoid infinite loops on some broken databases

 - graphene_http:
   - support for simple search query
   - produce HTTP errors when it is needed
   - fix brokn -l option (set log-file)
   - add -P option (set pid-file)
   - add -E option (envirnment type)
   - set defult environment type to "lock"

 - add new scripts:
   - graphene_mkcomm -- read numerical database and update human-readable comments
   - graphene_filter -- filter points or time ranges through external program
   - graphene_sweeps -- extract parameter sweeps from a database
   - graphene_sync -- syncronize two databases
   - graphene_int: trivial graphene -i wrapper to be used as user shells

* Thu Apr 18 2019 Vladislav Zavjalov <slazav@altlinux.org> 2.7-alt1
- v2.7
 - New commands:
    load -- load file in db_dump format (instead of db_load utility)
    dump -  (analog of db_dump utility)
    add list_dbs/list_logs commands (analogs of db_archive -s, db_archive -l)
 - Use logs and transactions, -E option for choosing environment type (txn, lock, none).
   process registrationin env; snapshot isolation for reading; deadlock
   detection.
 - -R option: read-only mode.
 - Set default database path to ".".
 - Symbol / is not allowed in db names, subfolders are not supported.
 - json.cpp: allow requests without format=json field
 - add graphene_tab script: collecting data from different databases
   to a single text table
 - protect # in interactive mode (for SPP protocol)

* Fri Mar 02 2018 Vladislav Zavjalov <slazav@altlinux.org> 2.6-alt1
- v2.6
 - test_cli.sh: fix test to use only local folder
 - json interface: never return more then MaxAnnotations=500 annotations
 - allow +/- suffixes in timestamps
 - text databases: only first line of text is printed in get_range output, no '\n' -> ' ' conversion
 - -s option: unix socket mode
 - -r option: output relative times (seconds from requested time) instead of absolute timestamps

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
