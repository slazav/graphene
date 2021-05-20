/* GrapheneDB class: wrapper for BerkleyDB, open/put/get/del functions
 */

#ifndef GR_DB_H
#define GR_DB_H

#include <memory>
#include <cassert>
#include <cstdlib>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <cstring> /* memset */
#include <db.h>

#include "err/err.h"
#include "data.h"

#include <iomanip>

// bercleydb:
//  http://docs.oracle.com/cd/E17076_02/html/gsg/C/index.html
//  https://web.stanford.edu/class/cs276a/projects/docs/berkeleydb/reftoc.html

#define GRAPHENE_LOGSIZE 1<<20

#define KEY_DESCR   0
#define KEY_VERSION 1
#define KEY_BACKUP_MAIN  0x10
#define KEY_BACKUP_TMP   0x11

// Filters occupy MAX_FILTERS keys starting
// from KEY_FLT. Filter 0 data uses KEY_FLT0DATA key
#define MAX_FILTERS 16
#define KEY_FLT  0x20
#define KEY_FLT0DATA  0x19

#define DEF_DBVERSION  2
#define DEF_TIMETYPE   TIME_V2
#define DEF_DATATYPE   DATA_DOUBLE

// Base formatter class for GrapheneDB. All get_* methods call
// GrapheneFormatter::proc_point on each record (without any
// filtering or column selection).
class GrapheneFormatter {
  public:
  virtual void proc_point(const std::string &k, const std::string &v,
     const TimeType ttype, const DataType dtype) = 0;
};

/***********************************************************/
/* class for wrapping BerkleyDB */
class GrapheneDB{
  /************************************/
  // Create DBT objects of various kinds.
  //
  static DBT mk_dbt(){
    DBT ret;
    memset(&ret, 0, sizeof(DBT));
    return ret;
  }
  static DBT mk_dbt(const std::string & str){
    DBT ret = mk_dbt();
    ret.data = (void *) str.data();
    ret.size = str.length();
    return ret;
  }
  template <typename T>
  static DBT mk_dbt(const T * v){
    DBT ret = mk_dbt();
    ret.data = (void *)v;
    ret.size = sizeof(T);
    return ret;
  }
  static std::string dbt2str(DBT *k) {
    return std::string((char *)k->data, (char *)k->data+k->size);}

  // check if database key is a valid timestamp (not a 1- or 2-byte special keys)
  static bool is_tstamp(DBT *k) { return k->size==sizeof(uint64_t) || k->size==sizeof(uint32_t); }

  /************************************/
  /* data */
    std::shared_ptr<DB> dbp;
    DB_ENV * env;
    std::string name;    // database name
    uint32_t open_flags; // database open flags
    uint32_t env_flags;  // environment flags

    uint8_t  version;  // database version
    DataType dtype;    // data type
    TimeType ttype;    // timestamp type
    std::string descr; // database description

  // database deleter
  struct D {
    void operator()(DB* dbp) { dbp->close(dbp, 0); }
  };

  /****************************/
  // Simple transaction wrappers:
    DB_TXN *txn_begin(int flags=0);
    void txn_commit(DB_TXN *txn);
    void txn_abort(DB_TXN *txn);

  /****************************/
  // Simple wrappers for cursor operations:
    void get_cursor(DB *dbp, DB_TXN *txn, DBC **curs, int flags);
    bool c_get(DBC *curs, DBT *k, DBT *v, int flags);

  /****************************/
  // Simple del/put/set operations for database information
    void del_key(DB_TXN *txn, uint8_t key);
    void set_key(DB_TXN *txn, uint8_t key, DBT v);
    std::string get_key(DB_TXN *txn, uint8_t key,
                        const std::string & def = std::string());

  /****************************/
  // Read/Write database information.
  // key = (uint8_t)0 (1byte),  value = data_fmt (1byte) + description
  // key = (uint8_t)1 (1byte),  value = version  (1byte)
    void write_info();
    void read_info();

  public:

  /************************************/
  // Constructor -- open a database
  // Path is a path to the database foolder.
  // Name is a database name, it can not contain some symbols (.|+ \n\t)
  GrapheneDB(DB_ENV *env,
       const std::string & path_,
       const std::string & name_,
       const int flags);

  // change database description
  void set_descr(const std::string & d){ descr = d; write_info(); }

  // Set data type. Do it only after creating a new database.
  void set_dtype(const DataType & t){ dtype = t; write_info(); }

  // get database description
  std::string get_descr() const { return descr; }

  // get data type
  DataType get_dtype() const { return dtype; }

  // get timestamp type
  TimeType get_ttype() const { return ttype; }

  // is the database opened readonly?
  bool is_readonly() const {return open_flags & DB_RDONLY;}

  // clear a filter
  void clear_filter(const int N);

  // read a filter from database
  std::string get_filter(const int N);

  // write filter N. For input filter (N=0) storage is cleared
  void write_filter(const int N, const std::string & code);


  // clear storage of the input filter
  void clear_f0data();

  // read storage of the input filter from database
  std::string get_f0data();

  // write storage of the input filter to database
  void write_f0data(const std::string & storage);

  /****************************/
  // Backup system:

  // backup start: notify that we are going to start backup.
  // - reset temporary backup timer to inf
  // - return value of the main backup timer
  // args: backup_start <name>
  std::string backup_start();

  // backup end: notify that backup is successfully done
  // - commit min(temporary backup timer and timestamp) into main one
  // args: backup_end <name> [<timestamp>]
  void backup_end(const std::string & t2);

  // reset backup timers to 0
  void backup_reset();

  // get value of the main backup timer
  std::string backup_get();

  // Internal function, should be called after each
  // database modification.
  void backup_upd(DB_TXN *txn, const std::string &t);

  /****************************/
  // Put data to the database
  // input: timestamp + vector of strings + dpolicy
  // The function can be run multiple times without reopening
  // the database.
  // dpolicy -- what to do with duplicated timestamps:
  //   (replace, skip, error, sshift, nsshift)
  void put(const std::string &t, const std::vector<std::string> & dat,
           const std::string &dpolicy);

  // All get* functions get some data from the database
  // and call cb for each key-value pair

  // get data from the database -- get_next
  void get_next(const std::string &t1, GrapheneFormatter & out);

  // get data from the database -- get_prev
  void get_prev(const std::string &t2, GrapheneFormatter & out);

  // get data from the database -- get
  void get(const std::string &t, GrapheneFormatter & out);

  // get data from the database -- get_range
  void get_range(const std::string &t1, const std::string &t2,
                 const std::string &dt, GrapheneFormatter & out);

  // get data from the database -- get_count
  void get_count(const std::string &t1,
                 const std::string &count, GrapheneFormatter & out);

  // delete data data from the database -- del_range
  void del(const std::string &t1);

  // delete data data from the database -- del_range
  void del_range(const std::string &t1, const std::string &t2);

  // sync the database
  void sync() {dbp->sync(dbp.get(), 0);}

  // load file in a db_dump format
  // (we can not use db_load because of user-defined comparison function)
  void load(const std::string &file);

  // dump file in a db_dump format
  // (db_dump utility can be used instead)
  void dump(const std::string &file);

};

#endif
