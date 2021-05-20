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

class GrapheneTCLGet: public GrapheneTCLProc {
  GrapheneEnv & env;
  public:
  GrapheneTCLGet(GrapheneEnv & env_): env(env_) {}
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
  GrapheneTCLGet tcl_get_cmd;

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
  void put(const std::string & name, const std::string & t,
           const std::vector<std::string> & dat, const std::string &dpolicy){
    auto & db = getdb(name);
    db.put(t, dat, dpolicy);
  }

  void put_flt(const std::string & name, const std::string &t,
               const std::vector<std::string> & dat, const std::string &dpolicy){
    auto & db = getdb(name);

    auto ttype = db.get_ttype();
    std::string storage = db.get_f0data();
    // run input filter
    auto t1 = graphene_time_print(graphene_time_parse(t, ttype),ttype);
    auto d1(dat);
    if (tcl.run(db.get_filter(0), t1, d1, storage)) db.put(t1,d1,dpolicy);

    // write storage
    db.write_f0data(storage);
  }

  /****************/

  // get next point after (or equal) t
  void get_next(const std::string & ext_name, const std::string & t,
                const TimeFMT timefmt, GrapheneFmtCB fmt_cb, void * fmt_cb_data) {
    GrapheneEnvFormatter dbo(tcl, ext_name, *this);
    auto & db = getdb(dbo.name, DB_RDONLY);
    dbo.timefmt = timefmt;
    dbo.time0   = t;
    dbo.fmt_cb  = fmt_cb;
    dbo.fmt_cb_data  = fmt_cb_data;
    db.get_next(t, dbo);
  }

  // get previous point before t
  void get_prev(const std::string & ext_name, const std::string & t,
                const TimeFMT timefmt, GrapheneFmtCB fmt_cb, void * fmt_cb_data) {
    GrapheneEnvFormatter dbo(tcl, ext_name, *this);
    auto & db = getdb(dbo.name, DB_RDONLY);
    dbo.timefmt = timefmt;
    dbo.time0   = t;
    dbo.fmt_cb  = fmt_cb;
    dbo.fmt_cb_data  = fmt_cb_data;
    db.get_prev(t, dbo);
  }

  // get previous or interpolated point
  void get(const std::string & ext_name, const std::string & t,
           const TimeFMT timefmt, GrapheneFmtCB fmt_cb, void * fmt_cb_data) {
    GrapheneEnvFormatter dbo(tcl, ext_name, *this);
    auto & db = getdb(dbo.name, DB_RDONLY);
    dbo.timefmt = timefmt;
    dbo.time0   = t;
    dbo.fmt_cb  = fmt_cb;
    dbo.fmt_cb_data  = fmt_cb_data;
    db.get(t, dbo);
  }

  // get data range
  void get_range(const std::string & ext_name, const std::string & t1,
                 const std::string & t2, const std::string & dt,
                 const TimeFMT timefmt, GrapheneFmtCB fmt_cb, void * fmt_cb_data) {
    GrapheneEnvFormatter dbo(tcl, ext_name, *this);
    auto & db = getdb(dbo.name, DB_RDONLY);
    dbo.list = true;
    dbo.timefmt = timefmt;
    dbo.time0   = t1;
    dbo.fmt_cb  = fmt_cb;
    dbo.fmt_cb_data  = fmt_cb_data;
    db.get_range(t1,t2,dt, dbo);
  }

  // get limited number of points starting at t
  void get_count(const std::string & ext_name,
                 const std::string & t, const std::string & cnt,
                 const TimeFMT timefmt, GrapheneFmtCB fmt_cb, void * fmt_cb_data) {
    GrapheneEnvFormatter dbo(tcl, ext_name, *this);
    auto & db = getdb(dbo.name, DB_RDONLY);
    dbo.list = true;
    dbo.timefmt = timefmt;
    dbo.time0   = t;
    dbo.fmt_cb  = fmt_cb;
    dbo.fmt_cb_data  = fmt_cb_data;
    db.get_count(t,cnt, dbo);
  }

  /****************/

};


#endif
