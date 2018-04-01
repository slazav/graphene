###  Performance

NOTE: this was written for an old Graphene database without BerkleyBD
environment.

Consider a situation: we want to measure some parameter for a year with
10 s period and put 8-byte double values into the database. This means 3.15
million points.

let's create a database DB and put all these points at once:
```
$ graphene -d . create DB
$ for t in $(seq 3153600); do printf "put DB $t $RANDOM\n"; done |
    ./graphene -d . interactive &>/dev/null
```

This takes about 1 minute at my computer. Note that I use interactive
mode to write all points without reopening the database. If I do writing
with command-line interface it takes about 10 hours:
```
$ for t in $(seq 3153600); do ./graphene -d . put DB "$t" "$RANDOM"; done
```

Size of the database is 73.6 Mb, 24.5 bytes/point. Gzip can make in
smaller (17.3 Mb, 5.74 bytes/point), xz even smaller (10.3 Mb, 3.41
bytes/point).

If you use non-integer seconds for timestamps the size will increase by
4 bytes per point.

If you have a few parameters which do not need separate timestamps, you
can put them into a multi-column data points (put DB $t $par1 $par2 ...)
saving some space. Database size grows by 8 bytes per additional column.


You can also configure the database to store 4-byte float values instead
of 8-byte doubles. It will save 4 bytes per data column.

Reading of all the points takes 23s:

```
$ time graphene -d . get_range DB | wc
23.18user 0.10system 0:23.27elapsed 100%CPU (0avgtext+0avgdata 5168maxresident)k
0inputs+0outputs (0major+289minor)pagefaults 0swaps
3153600 6307200 73507308
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

###  Database versions

In v1 time was stored in milliseconds in a 64-bit integer.
Time input and output in the command line interface used integer numbers
of milliseconds:
```
$ graphene put DB 1479463946000 1.0
$ graphene get_range DB
1479463946000 1.0
```

In v2 time is stored as two 32-bit numbers: number of seconds
and number of nanoseconds. On input and output a decimal dot is used:
```
$ graphene put DB 1479463946 1.0
$ graphene put DB 1479463946.123456789 2.0
$ graphene get_range DB
1479463946.000000000 1.0
1479463946.123456789 2.0
```
v1 databases are supported in v2.
