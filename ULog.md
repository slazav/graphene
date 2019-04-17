== Update logs

This text describes an idea which is not working yet.

Consider a situation when you are getting some range of data from a
database. After some time you want to update your data without
downloading everything. It can be a situation of a database copy on
another computer or some viewer which shows data in real time.

Graphene database is designed mostly for collecting some real time data.
You have a data source which adds more and more points with increasing
timestamps. For such data the update procedure is easy: you just read a
time range starting from the last point you have. But it is also possible
that data were modified, deleted or added before this point. For these
data we need more sophysticated procedure.

The database can collect an update log: a separate database for data
modofications. For each write/delete command except ones which add a
point behind the last one it records the timestamp or the range (two
timestamps) of modified data.

Then the update procedure will be the following:

1. reading the data
- get a database local time (t1) - new command is needed
- get all data we want.

2. update
- Get a starting time of the log (t2) - new command.
- If it is larger then t1, reread the whole data range in a normal way.
- Get time of our last point (t3).
- Get update log starting from t2 - new command.
- For each time and range from the log which is smaller then t3 get data
  from the database and update local data.
- Get data range starting from t3 and add to local data.

New commands needed for this operation:
- `time` -- get database local time
- `ulog_start <dbname>` -- get starting time of the update log
- `ulog_get <dbname> <t>` -- get update log starting from <t>
- `ulog_del <dbname> [<t>]` -- delete update log until <t>

The command `time` would be also useful for rough check of
clock syncronization between database computer and measurment
device.
