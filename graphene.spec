Name:         graphene
Version:      2.13
Release:      alt1

Summary:      Simple time series database.
Group:        System
URL:          https://github.com/slazav/graphene
License:      GPL

Packager:     Vladislav Zavjalov <slazav@altlinux.org>

Source:       %name-%version.tar
BuildRequires: libmicrohttpd-devel libjansson-devel libdb4.7-devel db4.7-utils tcl-devel
BuildRequires: wget
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
* Tue Nov 14 2023 Vladislav Zavjalov <slazav@altlinux.org> 2.13-alt1
- v2.13
  fix Makefile for Debian builds, add clean rule; use DESTDIR var
  add script for building deb packages
  data.h: include missing cstdlib header
  update Readme.md
  graphene_sync: fix error introduced in 473386

* Wed Mar 22 2023 Vladislav Zavjalov <slazav@altlinux.org> 2.12-alt1
- v2.12
 - graphene:
   - add get_wrange command, equivalent of get_prev + get_range + get_next
   - add backup_list command
   - add help command -- same as cmdlist
 - tcl filters:
   - graphene_get_prev/graphene_get_next functions in TCL filters
   - tcllib/table_lookup: allow nan values
   - add tcllib/table_lookup function to use it separately from flt_table_lookup filter
   - add help/cmdlist command to HTTP GET interface
 - graphene_http
   - fix for old versions of libmicrohttpd (without MHD_Result)
   - check for invalid parameters in the simple GET interface
   - fix test: wait for 1s after starting graphene_http daemon to be sure that it is running
 - graphene_sync script:
   - use backup_list commands to skip non-modified databases
   - fix problem with quickly-updated databases

* Fri May 28 2021 Vladislav Zavjalov <slazav@altlinux.org> 2.11-alt1
- v2.11:
 - GrapheneEnv class implements programming interface for working with databases.
   Rearrangings and renamings in the code.
 - Get values from multiple databases by using <name1>+<name2>+... syntax.
   Remove graphene_tab script.
 - Add graphene_get function availble from TCL filters: getting values from any database.
   Use a single TCL interpreter for the whole database environment.
   No caching of filters.
 - Simple_json interface: return null for NaN values.
 - Do not set environment type to none for read-only operations.
   This fixes PAGE_NOTFOUND errors when using graphene_http.
 - Add graphene_meas program - tool for measuring time usage
   and simple example of the programming interface,
 - Do not do sync after each put_flt command. This makes it about 1000 times
   faster but can require extra precautions if the input filter uses storage
   variable. Use one graphene process (or subsequent calls to graphene program)
   for writing to a database, or use sync command after put_flt.

* Sun Apr 18 2021 Vladislav Zavjalov <slazav@altlinux.org> 2.10-alt1
- v2.10:
 - safe database closing when catching signals
 - close all databases on errors, in some cases reopening/recovery is needed
 - always use tm_isdst=-1 in mktime (this also fixes aarch64 build)
 - fix libdb_version command: add newline char at the end to avoid problems in interactive mode
 - new commands:
   - get_count: get limited amount of points starting from a timestamp
   - clear_f0data: clear storage of the input filter
 - graphene_http:
   - fix build with gcc10, with old and new libmicrohttpd (MHD_Result return type)
   - fix error in HTTP response initialization
   - when stopping a server wait until the process exits
   - use umask(022) in daemon mode
   - fix error with filters + json datasource, always give all columns to filter input
 - filters:
   - fix error in set_filter/print_filter commands (filter range checking)
   - fix error handling in filters
   - always define storage variable in filters
   - use a single TCL interpreter for each database (faster, less isolation)
   - output filters can return false to skip data
   - fully rewrite TCL filter library
 - graphene_sync script:
   - copy large amounts of data in small steps (use get_count command)
   - add -f option for copying filters
   - do sync on the destination db after backup
 - fix/update documentation

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
