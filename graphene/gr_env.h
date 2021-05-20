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

#include "data.h"
#include "filter.h"

// Formatter callback
typedef void (*GrapheneFmtCB) (const std::string &t,
     const std::vector<std::string> &d, void * cb_data);

// Simple version of the callback. Get pointer to std::ostream
// and print data.
void out_cb_simple(const std::string &t,
                   const std::vector<std::string> &d, void * cb_data);


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

  Filter * filter;
  bool list;

  GrapheneFmtCB fmt_cb;
  void * fmt_cb_data;

  int col; // column number, for the main database

  TimeFMT timefmt;     // output time format
  std::string time0;   // zero time for relative time output (not parsed)

  // constructor -- parse the dataset string, create iostream
  GrapheneEnvFormatter():
          col(-1), timefmt(TFMT_DEF),
          list(false), filter(0),
          fmt_cb(NULL), fmt_cb_data(NULL) {}

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

  // Deleter for the environment
  struct D{
    void operator() (DB_ENV* env) {env->close(env, 0);}
  };

  public:

  // Constructor: open DB environment
  // env_type: "none", "lock", "txn" (default)
  GrapheneEnv(const std::string & dbpath_, const bool readonly, const std::string & env_type);

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

  // get next point after (or equal) t
  void get_next(const std::string & ext_name, const std::string & t,
                const TimeFMT timefmt, GrapheneFmtCB fmt_cb, void * fmt_cb_data) {
    int col = -1, flt = -1;
    auto name = parse_ext_name(ext_name, col, flt);
    auto & db = getdb(name, DB_RDONLY);
    GrapheneEnvFormatter dbo;
    dbo.filter  = db.get_filter_obj(flt);
    dbo.col     = col;
    dbo.timefmt = timefmt;
    dbo.time0   = t;
    dbo.fmt_cb  = fmt_cb;
    dbo.fmt_cb_data  = fmt_cb_data;
    db.get_next(t, dbo);
  }

  // get previous point before t
  void get_prev(const std::string & ext_name, const std::string & t,
                const TimeFMT timefmt, GrapheneFmtCB fmt_cb, void * fmt_cb_data) {
    int col = -1, flt = -1;
    auto name = parse_ext_name(ext_name, col, flt);
    auto & db = getdb(name, DB_RDONLY);
    GrapheneEnvFormatter dbo;
    dbo.filter  = db.get_filter_obj(flt);
    dbo.col     = col;
    dbo.timefmt = timefmt;
    dbo.time0   = t;
    dbo.fmt_cb  = fmt_cb;
    dbo.fmt_cb_data  = fmt_cb_data;
    db.get_prev(t, dbo);
  }

  // get previous or interpolated point
  void get(const std::string & ext_name, const std::string & t,
           const TimeFMT timefmt, GrapheneFmtCB fmt_cb, void * fmt_cb_data) {
    int col = -1, flt = -1;
    auto name = parse_ext_name(ext_name, col, flt);
    auto & db = getdb(name, DB_RDONLY);
    GrapheneEnvFormatter dbo;
    dbo.filter  = db.get_filter_obj(flt);
    dbo.col     = col;
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
    int col = -1, flt = -1;
    auto name = parse_ext_name(ext_name, col, flt);
    auto & db = getdb(name, DB_RDONLY);
    GrapheneEnvFormatter dbo;
    dbo.filter  = db.get_filter_obj(flt);
    dbo.list   = true;
    dbo.col     = col;
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
    int col = -1, flt = -1;
    auto name = parse_ext_name(ext_name, col, flt);
    auto & db = getdb(name, DB_RDONLY);
    GrapheneEnvFormatter dbo;
    dbo.filter  = db.get_filter_obj(flt);
    dbo.list   = true;
    dbo.col     = col;
    dbo.timefmt = timefmt;
    dbo.time0   = t;
    dbo.fmt_cb  = fmt_cb;
    dbo.fmt_cb_data  = fmt_cb_data;
    db.get_count(t,cnt, dbo);
  }

  /****************/

};


#endif
