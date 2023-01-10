/* DBpool class: class for storing many opened databases
 */

#ifndef GR_ENV_H
#define GR_ENV_H

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <cstring> /* memset */
#include <db.h>
#include "gr_db.h"
#include "gr_tcl.h"

#include "data.h"

// Formatter callback
typedef void (*GrapheneFmtCB) (const std::string &t,
     const std::vector<std::string> &d, void * cb_data);

// Simple version of the callback. Get pointer to std::ostream
// and print data.
void out_cb_simple(const std::string &t,
                   const std::vector<std::string> &d, void * cb_data);


class GrapheneEnv;

// graphene_get, graphene_get_prev, graphene_get_next tcl commands
class GrapheneTCLGet: public GrapheneTCLProc {
  GrapheneEnv & env;
  public:
  GrapheneTCLGet(GrapheneEnv & env_): env(env_) {}
  std::string run(const std::vector<std::string> & args) override;
};

class GrapheneTCLGetP: public GrapheneTCLProc {
  GrapheneEnv & env;
  public:
  GrapheneTCLGetP(GrapheneEnv & env_): env(env_) {}
  std::string run(const std::vector<std::string> & args) override;
};

class GrapheneTCLGetN: public GrapheneTCLProc {
  GrapheneEnv & env;
  public:
  GrapheneTCLGetN(GrapheneEnv & env_): env(env_) {}
  std::string run(const std::vector<std::string> & args) override;
};

/***********************************************************/
// Class for an extended dataset object.
//
// Extended dataset can be just a database name, but it can also
// contain a column, or a filter:
// <name>:<column>
// <name>:<filter>
//
class GrapheneEnvFormatter: public GrapheneFormatter {
  public:

  bool list;

  std::string filter;

  GrapheneFmtCB fmt_cb;
  void * fmt_cb_data;

  std::vector<std::string> secondary;

  GrapheneTCL & tcl; // tcl interpreter
  GrapheneEnv & env;

  int col; // column number, for the main database
  int flt_num;
  std::string name;

  TimeFMT timefmt;     // output time format
  std::string time0;   // zero time for relative time output (not parsed)

  // constructor -- parse the dataset string, create iostream
  GrapheneEnvFormatter(GrapheneTCL & tcl_, const std::string & ext_name, GrapheneEnv & env_);

  // This method is called from GrapheneGB::get_* for each data point
  // It gets unpacked values from the database, do formatting,
  // column selection and filtering and call print_point method.
  void proc_point(const std::string &k, const std::string &v,
     const TimeType ttype, const DataType dtype) override;
};


/***********************************************************/
// Class for keeping a database environment and many opened
// databases.
class GrapheneEnv{
  std::string dbpath;
  std::string env_type;
  std::map<std::string, GrapheneDB> pool;
  std::shared_ptr<DB_ENV> env; // database environment
  bool readonly;

  GrapheneTCL tcl;
  GrapheneTCLGet  tcl_get_cmd;
  GrapheneTCLGetP tcl_getp_cmd;
  GrapheneTCLGetN tcl_getn_cmd;

  // Deleter for the environment
  struct D{
    void operator() (DB_ENV* env) {env->close(env, 0);}
  };

  public:

  // Constructor: open DB environment
  // env_type: "none", "lock", "txn" (default)
  GrapheneEnv(const std::string & dbpath_, const bool readonly,
              const std::string & env_type, const std::string & tcl_libdir);

  ~GrapheneEnv();

  // find database in the pool. Create/Open/Reopen if needed
  GrapheneDB & getdb(const std::string & name, const int fl = 0);

  /****************/

  // return list of all databases
  std::vector<std::string> dblist();

  // create new database
  void dbcreate(const std::string & name, const std::string & descr,
              const DataType type);

  // remove database file
  void dbremove(const std::string & name);

  // rename database file
  void dbrename(const std::string & name1, const std::string & name2);

  // close one database, close all databases
  void close(const std::string & name);
  void close();

  // sync one database, sync all databases
  void sync(const std::string & name);
  void sync();

  // print environment database files for archiving (same as db_archive -s)
  void list_dbs();

  // print environment log files (same as db_archive -l)
  void list_logs();

  /****************/
  void set_descr(const std::string & name, const std::string & descr) {
     getdb(name).set_descr(descr); }

  std::string get_descr(const std::string & name) {
     return getdb(name, DB_RDONLY).get_descr(); }

  DataType get_dtype(const std::string & name) {
     return getdb(name, DB_RDONLY).get_dtype(); }

  TimeType get_ttype(const std::string & name) {
     return getdb(name, DB_RDONLY).get_ttype(); }

  /****************/

  // backup start: notify that we are going to start backup.
  // - reset temporary backup timer
  // - return value of the main backup timer
  std::string backup_start(const std::string & name) {
    return getdb(name).backup_start(); }

  // backup end: notify that backup is successfully done
  // - commit temporary backup timer into main one
  void backup_end(const std::string & name, const std::string & t2 = "inf") {
    getdb(name).backup_end(t2); }

  // reset backup timer
  void backup_reset(const std::string & name) {
     getdb(name).backup_reset(); }

  // get value of the backup timer
  std::string backup_get(const std::string & name) {
     return getdb(name, DB_RDONLY).backup_get(); }

  // is backup needed?
  bool backup_needed(const std::string & name) {
     return getdb(name, DB_RDONLY).backup_needed(); }

  /****************/

  void put(const std::string & name, const std::string & t,
           const std::vector<std::string> & dat, const std::string &dpolicy);

  void put_flt(const std::string & name, const std::string &t,
               const std::vector<std::string> & dat, const std::string &dpolicy);

  /****************/

  // get next point after (or equal) t
  void get_next(const std::string & ext_name, const std::string & t,
                const TimeFMT timefmt, GrapheneFmtCB fmt_cb, void * fmt_cb_data);

  // get previous point before t
  void get_prev(const std::string & ext_name, const std::string & t,
                const TimeFMT timefmt, GrapheneFmtCB fmt_cb, void * fmt_cb_data);

  // get previous or interpolated point
  void get(const std::string & ext_name, const std::string & t,
           const TimeFMT timefmt, GrapheneFmtCB fmt_cb, void * fmt_cb_data);

  // get data range
  void get_range(const std::string & ext_name, const std::string & t1,
                 const std::string & t2, const std::string & dt,
                 const TimeFMT timefmt, GrapheneFmtCB fmt_cb, void * fmt_cb_data);

  // get wide range (get_prev, get_range, get_next)
  void get_wrange(const std::string & ext_name, const std::string & t1,
                 const std::string & t2, const std::string & dt,
                 const TimeFMT timefmt, GrapheneFmtCB fmt_cb, void * fmt_cb_data);

  // get limited number of points starting at t
  void get_count(const std::string & ext_name,
                 const std::string & t, const std::string & cnt,
                 const TimeFMT timefmt, GrapheneFmtCB fmt_cb, void * fmt_cb_data);

  /****************/

  // delete one data point
  void del(const std::string & name, const std::string & t1){
    getdb(name).del(t1); }

  // delete all points in the data range
  void del_range(const std::string & name, const std::string & t1, const std::string & t2){
    getdb(name).del_range(t1,t2); }

  /****************/

  // create db and load file in db_dump format
  // (we can not use db_load because of user-defined comparison function)
  void load(const std::string & name, const std::string & fname){
    getdb(name, DB_CREATE | DB_EXCL).load(fname); }

  void dump(const std::string & name, const std::string & fname){
    getdb(name, DB_RDONLY).dump(fname); }

  /****************/

  void set_filter(const std::string & name, const int N, const std::string & code){
    getdb(name).write_filter(N, code);}

  std::string get_filter(const std::string & name, const int N){
    return getdb(name, DB_RDONLY).get_filter(N);}

  std::string get_f0data(const std::string & name){
    return getdb(name, DB_RDONLY).get_f0data();}

  void clear_f0data(const std::string & name){
    getdb(name).clear_f0data();}

};


#endif
