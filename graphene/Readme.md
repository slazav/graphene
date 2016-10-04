## Graphene -- a simple time-series database for scientific applications

Source code: https://github.com/slazav/graphene

E-mail: Vladislav Zavjalov <slazav@altlinux.org>

### Features

- based on BerkleyDB
- store integer, floating point or text values with millisecond timestamps
- fast access to data, interpolation, time ranges, downsampling
- multi-column numerical values
- command line interface for reading/writing data
- http simple json interface for Grafana viewer
- user filters for data processing (calibration tables etc.)

### Data storage

Each dataset is a separate BerkleyDB file with `.db` extension, located in
a database directory (default `/var/lib/graphene`). Name of file represents
the name of the dataset. Name may contain path symbols '/', but can not
contain symbols '.:|+ \t\n', You can use name `cryostat/temperature` but
not `../temperature` for your dataset. Currently all subfolders have to
be created manually.

Data are stored as a set of sorted key-value pairs. Key is time in
milliseconds (64-bit unsigned integer), counted from 1970-01-01 UTC.
Duplicated time values are allowed (but can not be correctly shown now).
Value can contain an array if numbers of arbitrary length (data columns)
or some text. The data format can be chosen during the database
creation. Possible variants are: TEXT, INT8, UINT8, INT16, UINT16,
INT32, UINT32, INT64, UINT64, FLOAT, DOUBLE.

Records with 8-bit keys are reserved for database information, for
example the data format and database description. Records with 16-bit
keys are reserved for arbitrary user data. These records are not
affected by regular get/put commands.

### Known problems

- Database names with '/'. It is conveniend to use subfolders for databases.
You can make good structure for your data, you can separate folders with
different permissions for different users. But now manipuating these
names via graphene interface does not work well. Creation of a new database
is possible only if all subfolders exist. List command shows only databases in
the root database folder.
- Linebreaks in text databases. You can put linebreaks in text database using
the command line interface. In interactive mode it is not possible, because
line breaks always mean starting of a new command. On output line breaks are
always converted to spaces.
- Duplicated timestamps. You can put entries with duplicated timestamps to a
database, but now get_* commands can not read them correctly.
- Strict timestamp format. Now you should always use milliseconds for time.
Not seconds, not any human-readable format. Timestamp always occupies 8 bytes.
Maybe it is possible to do more flexible format with auto-conversions,
use only 4 bytes if 1-second precision is needed. But it is not clear how
to do this in a good way without a mess.

### Command line interface

The program `graphene` is used to access data from command line.

Usage: `graphene [options] <command> <parameters>`

Options:
- -d <path> -- database directory (default `/var/lib/graphene/`)
- -h        -- write this help message and exit

Commands for manipulating databases:

- `create <name> [<data_fmt>] [<description>]` -- Create a database file.

- `delete <name>` -- Delete a database file.

- `rename <old_name> <new_name>` -- Rename a database file.

Delete and rename commands just do simple file operations.
A database can be renamed only if destination does not exists.

- `set_descr <name> <description>` -- Change database description.

- `info <name>` -- Print database format and description.

- `list` -- List all databases in the data directory.


Commands for reading and writing data:

- `put <name> <time> <value1> ... <valueN>` -- Write a data point.

- `get_next <extended name> [<time1>]` -- Get next point with t>=time1.

- `get_prev <extended name> [<time2>]` -- Get previous point with t<=time2.

- `get <extended name> [<time2>]` -- For integer and text databases
  get is equivalent to get_prev. For double databases it does linear
  interpolation between points, or return the last point if time is
  larger then that of the newest point.

- `get_range <extended name> [<time1>] [<time2>] [<dt>]` -- Get
  points in the time range. If parameter dt>1 then data are filtered,
  only points with distance >dt between them are shown. This works fast
  for any ratio of dt and interpoint distance.

You can use words "now" and "inf" as a time.
Default value for time1 is 0, for time2 is "inf".

Text values are printed with line breaks converted to spaces.

The "extended name" used in get_* commands have the following format:
`<name>[:<column>][|<filter>]`

`<column>` is a column number (0,1,..), if it exists, then only this
column is shown. If a certain column is requested but data array is not
long enough, a "NaN" value is returned. Columns are ignored for text data.

`<filter>` is a name of filter program, if it exists, the program is run and
data is filtered through it. The program should be located in the
database directory, program name can not contain '.:|+ \t\n' symbols.

Commands for deleting data:

- `del <name> <time>` -- Delete a data point.

- `del_range  <name> [<time1>] [<time2>]` -- Delete all points in the range.

Interactive mode commands:

- `interactive` -- Interactive mode. Commands are read from stdin.
Allows making many requests without reopening databases. Opening and closing of
databases are long operations. It can be useful to open the connection once
and do many operations. Each response ends with "OK" or "Error: <message>"
line.

- `sync` -- In the command line mode it does nothing. In the
interactive mode it closes all previously opened databases. It is useful
if you keep a connection (for example via ssh) for a long time, but want
to commit your changes to the databases regularly.


Examples:

See `examples/*` in the source folder


### HTTP + Simple JSON interface

The simple JSON interface can be used with grafana frontend to access
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

  [r, out] = system('graphene get_range my_dataset 1464260400000 now 60000');
  [t val1 val2] = strread(out, '%f %f %f');


###  Performance

Consider a situation: we want to measure some parameter for a year with
10 s period and put 8-byte double values into the database. This means 3.15
million points.

let's create a database DB and put all these points at once:
```
$ graphene -d . create DB
$ for t in $(seq 3153600); do printf "put DB $t $RANDOM\n"; done |
    ./graphene -d . interactive
```

This takes about 1 minute at my computer. Note that I use interactive
mode to write all points without reopening the database. If I do writing
with command-line interface it takes about 10 hours:
```
$ for t in $(seq 3153600); do ./graphene -d . put DB "$t" "$RANDOM"; done
```

Size of the database is 86 Mb, 28.6 bytes/point. Gzip can make in
smaller (17.5 Mb, 5.81 bytes/point), xz even smaller (8.8 Mb, 2.94
bytes/point).

If you have a few parameters which do not need separate timestamps, you
can put them into a multi-column data points (put DB $t $par1 $par2 ...)
saving some space. Database size grows by 8 bytes per additional column.

You can also configure the database to store 4-byte float values instead
of 8-byte doubles. It will save 4 bytes per data column.

Reading of all the points takes 15s:

```
$ time graphene -d . get_range DB | wc
15.81user 0.07system 0:15.90elapsed 99%CPU (0avgtext+0avgdata 5208maxresident)k
0inputs+0outputs (0major+293minor)pagefaults 0swaps
3153600 6307200 41969235
```

Reading of last 3000 points, first 3000 points, every 1000th point takes
almost nothing:

```
$ time graphene -d . get_range DB 3150601 3153600 | wc
0.01user 0.00system 0:00.01elapsed 94%CPU (0avgtext+0avgdata 5024maxresident)k
0inputs+0outputs (0major+257minor)pagefaults 0swaps
   3000    6000   40965
```

```
$ time graphene -d . get_range DB 0 3000 | wc
0.01user 0.00system 0:00.01elapsed 94%CPU (0avgtext+0avgdata 4896maxresident)k
0inputs+0outputs (0major+249minor)pagefaults 0swaps
   3000    6000   30873
```

```
$ time graphene -d . get_range DB 0 -1 1000 | wc
0.02user 0.00system 0:00.03elapsed 96%CPU (0avgtext+0avgdata 5204maxresident)k
0inputs+0outputs (0major+292minor)pagefaults 0swaps
   3154    6308   42011
```
