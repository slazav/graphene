## Graphene -- a simple time-series database with nanosecond precision for scientific applications

Source code: https://github.com/slazav/graphene

E-mail: Vladislav Zavjalov <slazav@altlinux.org>

### Features

- based on BerkleyDB
- store integer, floating point or text values with nanosecond-precision timestamps
- fast access to data, interpolation, downsampling, time ranges
- multi-column numerical values
- command line and socket interfaces for reading/writing data
- http simple json interface for Grafana viewer
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
format, database version, description. Records with 16-bit keys are
reserved for arbitrary user data. These records are not affected by
regular get/put commands.


### Command line interface

The program `graphene` is used to access data from command line.

Usage: `graphene [options] <command> <parameters>`

#### Options:

- `-d <path> --` database directory (default `.`)
- `-D <word> --` what to do with duplicated timestamps:
                 replace, skip, error, sshift, nsshift (default: replace)
- `-E <word> --` environment type: none, lock, txn (default: txn)
- `-h        --` write help message and exit
- `-i        --` interactive mode, read commands from stdin
- `-s <name> --` socket mode: use unix socket <name> for communications
- `-r        --` output relative times (seconds from requested time) instead of absolute timestamps
- `-R        --` read-only mode

#### Interactive mode:

Use -i option to enter the interactive mode. Then commands are read from
stdin, answers are written to stdout. This allows making many requests
without reopening databases. Opening and closing of databases are long,
it can be useful to open the connection once and do many operations.

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

- `list_dbs`  -- print environment database files for archiving (same as db_archive -s)

- `list_logs`  -- print environment log files (same as db_archive -l)

#### Commands for reading and writing data:

- `put <name> <time> <value1> ... <valueN>` -- Write a data point.

- `get_next <extended name> [<time1>]` -- Get first point with t>=time1.

- `get_prev <extended name> [<time2>]` -- Get last point with t<=time2.

- `get <extended name> [<time2>]` -- For integer and text databases
  get is equivalent to get_prev. For double databases it does linear
  interpolation between points, or return the last point if time is
  larger then that of the latest point.

- `get_range <extended name> [<time1>] [<time2>] [<dt>]` -- Get
  points in the time range. If parameter dt>0 then data are filtered,
  only points with distance >dt between them are shown. This works fast
  for any ratio of dt and interpoint distance. For text data only first
  lines are shown.

You can use words "now", "now_s" and "inf" as a timestamp. You can also
add "+" or "-" symbol to numerical value to add 1 ns. This is convenient
if you know a timestamp of some value and want to read next or  previous
one. Default value for time1 is 0, for time2 is "inf".

The "extended name" used in get_* commands have the following format:
`<name>[:<column>][|<filter>]`

`<column>` is a column number (0,1,..), if it exists, then only this
column is shown. If a certain column is requested but data array is not
long enough, a "NaN" value is returned. Columns are ignored for text data.

`<filter>` is a name of filter program, if it exists, the program is run and
data is filtered through it. The program should be located in the
database directory, program name can not contain '.:|+ \t\n/' symbols.
`TODO: remove this feature?`

#### Commands for deleting data:

- `del <name> <time>` -- Delete a data point.

- `del_range  <name> [<time1>] [<time2>]` -- Delete all points in the range.

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

- `get_time` -- print current time (unix seconds with ms precision).

#### Examples:

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
 -v <level> -- be verbose
                0 - write nothing.
                1 - write some information on start
                2 - write info about connections
                3 - write input data
                4 - write output data
 -l <file>  -- log file, use '-' for stdout
               (default /var/log/graphene.log in daemon mode, '-' in)"
 -f         -- do fork and run as a daemon;
 -h         -- write help message and exit;
```

###  Matlab/octave interface

Nothing is ready yet. You can use something like this to get data using the
graphene program:

  [r, out] = system('graphene get_range my_dataset 1464260400 now 60000');
  [t val1 val2] = strread(out, '%f %f %f');


### `graphene_tab` script

A common problem is to make a single table using values from a few databases
(to plot one parameter as a function of another). This is not supported yet
by graphene program itself, but there is a simple script for doing this, `graphene_tab`.

Usage: `graphene_tab -D <db_prog> -t <t1> -u <t2> db1 db2 db3 ...`
   db_prog -- graphene command: 'graphene -i' (default), 'device -d db' etc.
   t1, t2  -- time range
   db2, db2, db3 -- databases

Values from db2, db3, etc. are interpolated to points of db1 and printed in a single
text table. For floating-point databases linear interpolation is used, for integer
ones nearest values with smaller timestamps are printed.

### Known problems

- Line breaks in text databases. You can put line breaks in text database using
the command line interface. In interactive mode it is not possible, because
line breaks always mean starting of a new command.

