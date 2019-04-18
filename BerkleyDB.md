### Some BerkleyDB notes

Graphene use BerkleyDB environment for databases.

* By default a full transactional environment is used. To change it use
  `-E` command-line option: you can have no environment at
  all (each dataset is a single db file, only one program can work with
  it at a time), or a simple environment with locking.

* Do not use environments located on remote filesystems!

* Due to a custom sorting function not all standard BerkleyDB tools can
  be used for Graphene databases. Dumps created by `db_dump` can
  be loaded only by graphene `load` command but not by `db_load` utility;
  `db_verify` can be used only with `-o` option.

* Do not use `db_recover` utility when other program is working with a database.
  Normally `db_recover` is not needed. `Graphene` is configured
  with Process Registration and Auto Recovery options and can do recovery itself
  if is it possible/needed.

* You can do hot and incremental backups using `db_hotbackup` tool.

* Read books:
  - `https://docs.oracle.com/cd/E17076_05/html/gsg/C/index.html`
  - `https://docs.oracle.com/cd/E17076_05/html/gsg_txn/C/index.html`
