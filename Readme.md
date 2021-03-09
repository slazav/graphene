## Graphene -- a simple time-series database with nanosecond precision for scientific applications

Source code: https://github.com/slazav/graphene

E-mail: Vladislav Zavjalov <slazav@altlinux.org>

### Features

- based on BerkleyDB
- store integer, floating point or text values with nanosecond-precision timestamps
- fast access to data, interpolation, downsampling, time ranges
- multi-column numerical values
- command line and socket interfaces for reading/writing data
- http `simple_json` interface for [Grafana viewer](https://grafana.com/)
- user filters for data processing, calibration tables etc. (to be removed?)

### Data storage

Data are stored as databases inside a BerkleyDB environment directory (which
can be chosen via `-d` command-line option). Database name can not
contain symbols `.:|+ \t\n/`. [Some BerkleyDB notes](BerkleyDB.md)

Each database contains a set of sorted key-value pairs. Key is a
timestamp, one or two 32-bit unsigned integers: a number of seconds from
1970-01-01 UTC, and optional number of nanoseconds. Largest possible
timestamp is on 2106-02-07.

Duplicated timestamps are not allowed, but user can choose what to do
with duplicates (see -D option of the graphene program):
- replace -- replace the old record (default),
- skip    -- skip the new record if timestamp already exists,
- error   -- skip the new record and return error message
- sshift  -- increase time by 1 second step until it will be possible
             to put the record (no loss of data)
- nsshift -- same, but with nanosecond steps.

Value can contain an array of numbers of arbitrary length (data columns)
or some text. The data format can be chosen during the database
creation. Possible variants are: TEXT, INT8, UINT8, INT16, UINT16,
INT32, UINT32, INT64, UINT64, FLOAT, DOUBLE.

Records with 8-bit keys are reserved for database information: data
format, database version, description, filters. Records with 16-bit keys
are reserved for arbitrary user data. These records are not affected by
regular get/put commands.


### Command line interface

The program `graphene` is used to access data from command line.

Usage: `graphene [options] <command> <parameters>`

#### Options:

- `-d <path> --` database directory (default `.`)
- `-D <word> --` what to do with duplicated timestamps:
                 replace, skip, error, sshift, nsshift (default: replace)
- `-E <word> --` environment type: none, lock, txn (default: lock)
- `-h        --` write help message and exit
- `-i        --` interactive mode, read commands from stdin
- `-s <name> --` socket mode: use unix socket <name> for communications
- `-r        --` output relative times (seconds from requested time) instead of absolute timestamps
- `-R        --` read-only mode

#### Environment type

Graphene supports three database environment types (set by `-E`
command-line option).

- `none` -- No environment, each database is an independent file. In this mode
only one program can access database at a time. This setting can be
useful if you want to do massive and fast operations with a database and
no other programs use this database. For example, importing large amount
of data to a database can be done faster without using environment.

- `lock` -- Default mode. Environment (a few additional files) provides
database locking to allow multiple programs work with the database
simultaneously.

- `txn` -- Full transaction and logging support. At the moment there are
a few issues with this mode, and it is not recommended to use it.

It is not good to use different environment type when accessing one
database, even for read-only operations. It is strongly recommended to
use the default setting.

Further information about database environments can be found in BerkleyDB
documentation.

#### Interactive mode:

Use -i option to enter the interactive mode. Then commands are read from
stdin, answers are written to stdout. This allows making many requests
without reopening databases. Opening and closing of databases are long,
it can be useful to open the connection once and do many operations.

In interactive mode input lines are splitted to argument lists similarly
to shell splitting.
 - Comments (everything from # symbol to end of the line) are skipped.
 - Empty lines are skipped.
 - Words are splitted by ' ' or '\t' symbols, or by '\' + '\n' sequence.
 - Words can be quoted by " or '. Quoting can be used to enter multi-line text.
 - Any symbol (including newline) can be escaped by '\'. Protected newline
   symbol works as word separator, not as literal \n.

The program implements a Simple Pipe Protocol (see somewhere in my
`tcl_device` package): When it is started successfully  a prompt message is
printed to stdout started with "#SPP001" and followed by "#OK" line. In
case of an error `#Error: <...>` line is printed and program exits. Then
the program reads commands from stdin and sends answers to stdout followed
by "#OK" or "#Error: <...>" lines until the user closes the connection.
If answer contains symbol "#" in the beginning of a line, it is protected
with a second "#".

Socket mode is similar to the interactive mode. Graphene program acts as
a server which accepts connections (one at a time) through a unix-domain
socket.


#### Commands for manipulating databases:

- `create <name> [<data_fmt>] [<description>]` -- Create a database file.

- `load <name> <file>` -- Create a database and load file in `db_dump`
  format (note that it is not possible to use `db_load` utility because of
  non-standard comparison function in graphene databases).

- `dump <name> <file>` -- Dump a database to a file which can be loaded
  by `load` command. Same thing (with various options) can be done by
  `db_dump` utility and it it is recommended to use it.

- `delete <name>` -- Delete a database.

- `rename <old_name> <new_name>` -- Rename a database.
   A database can be renamed only if the destination does not exists.

- `set_descr <name> <description>` -- Change database description.

- `info <name>` -- Print database format and description.

- `list` -- List all databases in the data directory.

- `list_dbs`  -- print environment database files for archiving (same as db_archive -s).
   Works only for `txn` environment type.

- `list_logs`  -- print environment log files (same as db_archive -l)
   Works only for `txn` environment type.

#### Commands for reading and writing data:

- `put <name> <time> <value1> ... <valueN>` -- Write a data point.
  For TEXT values all arguments are joined with a single spaces between
  then. If you do not want this, use quoted argument.

- `put_flt <name> <time> <value1> ... <valueN>` -- Write a data point
  using database input filter (see below).

- `get_next <extended name> [<time1>]` -- Get first point with t>=time1.

- `get_prev <extended name> [<time2>]` -- Get last point with t<=time2.

- `get <extended name> [<time2>]` -- For integer and text databases
  get is equivalent to get_prev. For double databases it does linear
  interpolation between points, or return the last point if time is
  larger then that of the latest point.

- `get_range <extended name> [<time1>] [<time2>] [<dt>]` -- Get
  points in the time range. If parameter `dt>0` then data are filtered,
  only points with distance >dt between them are shown. This works fast
  for any ratio of dt and interpoint distance. For text data only first
  lines are shown.

- `get_count <extended name> [<time1>] [<cnt>]` -- Get
  up to `cnt` points (default 1000) starting from `time1`.


Supported timestamp forms:

- `<seconds>.<fraction>` -- Base format, nanosecond precision.

- `<seconds>.<fraction>[+|-]` -- add or subtract 1 ns to the value.This
is convenient if you know a timestamp of some value and want to read next
or previous one.

- `yyyy-mm-dd`, `yyyy-mm-dd HH`, `yyyy-mm-dd HH:MM`, `yyyy-mm-dd
HH:MM:SS`, `yyyy-mm-dd HH:MM:SS.<fraction>` -- Timestamp in a human-readable form.

- `inf` -- The largest timestamp.

- `now` -- Current time with microsecond precision

- `now_s` -- Current time with second precision

Default value for `<time1>` is 0, for `<time2>` is "inf".

The "extended name" used in get_* commands have the following format:
`<name>[:<column>]` or `<name>[:f<filter>]`

`<column>` is a column number (0,1,..), if it exists, then only this
column is shown. If a certain column is requested but data array is not
long enough, a "NaN" value is returned. Columns are ignored for text data.

`<filter>` is a filter number (1..15), if it exists data will be processed
by the filter (see below).

#### Commands for deleting data:

- `del <name> <time>` -- Delete a data point. Returns an error if there is
no such point.

- `del_range  <name> <time1> <time2>` -- Delete all points in the range.

#### Command for syncing databases in interactive mode:

- `sync` -- This command flushes any cached information to disk. It is
useful if you keep a connection (for example via ssh) for a long time,
but want to commit your changes to the databases regularly.

- `sync <name>` -- Same, but for one database. If database is not opened
  command does nothing and returns without error.

- `close` -- This command closes all previously opened databases. It can be
used if you want to close unused databases and sync data.

- `close <name>` -- Same, but for one database. If database is not opened
  command does nothing and returns without error.

#### Information:

- `cmdlist` -- print list of commands.

- `*idn?`   -- print ID string: `Graphene database <version>`.

- `get_time` -- print current time (unix seconds with microsecond precision).

- `libdb_version` -- print libdb version.

#### Backup  system:

Graphene database supports incremental backups. This can be done using
`backup_*` commands.

It is assumed that there is one backup process for a database. It notifies
the database before and after data transfer. Database answers which time
range was modified since the last successful transfer.

- `backup_start <name>` -- Notify that we want to backup the database `name`,
get time value of the earliest change since last backup or zero if
no backup was done or backup timer was reset.

- `backup_end <name> [<t>]` -- Notify that backup finished successfully
up to timestamp `<t>` (`inf` by default).

- `backup_print <name>` -- Print backup timer value.

- `backup_reset <name>` -- Reset backup timer value.

Internally there are two timers which contain earliest time of database
modification: main and temporary one. Each database modification command
(`put`, `del`, or `del_range`) decreases both timers to the
smallest time of modified record: (`timer = min(timer,
modification_time)`).

The temporary timer is reset before backup starts (`backup_start`
command) and committed to the main timer after backup process is
successfully finished (`backup_end` command). Main timer value is
returned by `backup_start` and `backup_print` commands.

For incremental backup the following procedure can be done:
- `backup_start` in the master database, save timer value
- `del_range <timer>` in the secondary database
- `get_range <timer>` (or a few calls to `get_count`) in the master database, put all values to the secondary one.
- `backup_end` in the master database

Such backup should be efficient in normal operation, when records with
continuously-increasing timestamps are added to the master database and
modifications of old values happens rarely.

If backup process fails and `backup_end` command is not executed then the
main backup timer will not be reset and the next backup will work
correctly.

There is a script `graphene_sync` for implementing incremental synchronization
of databases.

#### Filters

Each database can have up to sixteen data filters.
Filter 0 is input filter, incoming data can be filtered
through it to remove repeated points or do averaging.
Filters 1..15 are output filters.

- `set_filter <name> <N> <tcl code>` -- set code of filter N

- `print_filter <name> <N>` -- print code of filter N

- `print_f0data <name>` -- print data of filter 0 (input filter)

- `clear_f0data <name>` -- clear data of filter 0 (input filter)

- `put_flt <name> <timestamp> <data> ...` -- put data to the database through the filter 0

Filter is a piece of TCL code executed in a safe TCL interpreter.
Three variables are defined:

- `time` -- timestamp in `<seconds>.<nanoseconds>` format. Filter
can modify this variable to change the timestamp. Note that the timestamp
format is wider then `double` value. Do not convert it to number if you
want to keep precision.

- `data` -- list of data to be written to the database. Filter can
modify this list.

- `storage` -- For filter 0 it is a filter-specific data which is kept
in the database and can be used to save filter state. It can be TCL
list, but not an array. For filters 1..15 this data is not stored.

If a filter returns false value (`0`, `off`, `false`) data point will be skipped.

Simple example:
```
code='
  # use storage variable as a counter: 1,2,3,4:
  incr storage
  # round time to integer value (this will save some space):
  set time [expr int($time)]
  # change data: add one to the first element,
  # replace others by the counter value
  set data [list [expr [lindex $data 0] + 1] $storage]
  # put every third element, skip others
  return [expr $storage%3==1]
'
graphene set_filter mydb 0 "$code"
graphene put_flt mydb 123.456 10 20 30
```

Here `11 1` will be written with timestamp `123`.


### Examples

See `examples/*` in the source folder


### HTTP + Simple JSON interface

The simple JSON interface can be used with Grafana frontend to access
data (simple_json plugin is needed). Text databases can be viewed as
annotations, and numerical as metrics. Columns can be specified after
database name: <name>:<column>, default column is 0.

Usage: `graphene_http [options]`
Options:
```
 -p <port>  -- tcp port for connections (default 8081)
 -d <path>  -- database path (default /var/lib/graphene/)
 -E <word>  -- environment type:
               none, lock, txn (default: lock)
 -v <level> -- be verbose
                0 - write nothing
                1 - write some information on start
                2 - write info about connections
                3 - write input data
                4 - write output data
 -l <file>  -- log file, use '-' for stdout
               (default /var/log/graphene.log in daemon mode, '-' in
                normal mode)
 -P <file>  -- Pid file (default: /var/run/graphene_http.pid)
 -f         -- do fork and run as a daemon
 -S         -- stop running server
 -h         -- write this help message and exit
```

In addition to simple JSON interface `graphene_http` also implements
a simple GET read-only interface to access data:
- URL is graphene command, one of `get`, `get_prev`,
  `get_next`, `get_range`, `get_count`, or `list`
- `name` parameter is a database name
- `t1` parameter is timestamp for all `get_*` commands
- `t2` and `dt` parameters are second timestamp and time interval
  for `get_range` command
- `cnt` parameter is count for `get_count` command
- `tfmt` parameter is time format `def`, or `rel`.

Example:
```
wget localhost:8182/tmp_db'?cmd=get_range&t1=10&t2=12&tfmt=rel' -O - -o /dev/null
```

###  Matlab/octave interface

Nothing is ready yet. You can use something like this to get data using the
graphene program:

```
  [r, out] = system('graphene get_range my_dataset 1464260400 now 60000');
  [t val1 val2] = strread(out, '%f %f %f');
```

### `graphene_tab` script

A common problem is to make a single table using values from a few databases
(to plot one parameter as a function of another). This is not supported yet
by graphene program itself, but there is a simple script for doing this, `graphene_tab`.

Usage: `graphene_tab -D <db_prog> -t <t1> -u <t2> db1 db2 db3 ...`
- db_prog -- graphene command: 'graphene -i' (default), 'device -d db' etc.
- t1, t2  -- time range
- db2, db2, db3 -- databases

Values from db2, db3, etc. are interpolated to points of db1 and printed in a single
text table. For floating-point databases linear interpolation is used, for integer
ones nearest values with smaller timestamps are printed.

