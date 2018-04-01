### Some BerkleyDB notes

* Documentation can be found here:
 - `https://docs.oracle.com/cd/E17076_05/html/gsg/C/index.html`
 - `https://docs.oracle.com/cd/E17076_05/html/gsg_txn/C/index.html`

* Due to a custom sorting function not all standard BerkleyDB tools can
  be used for Graphene databases. Dumps created by `db_dump` can
  be loaded only by graphene `load` command but not by `db_load` utility;
  `db_verify` can be used only with `-o` option.

* Do not use environments located on remote filesystems!

* Do not use `db_recover` utility when other program is working with a database.
  Normally `db_recover` is not needed, because `graphene` does recovery itself
  if is it possible/needed.

* Do not use `db_hotbackup -u` when other program is working with the destination.

* Working way to do an incremental backup:
- Run `checkpoint -1` on source enviroment.
- Copy (or rsync) all log files to the destination.
  For me this works better then `db_hotbackup -u` (?).
- Run `recover -c` on the destination.

