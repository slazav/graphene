/* DBpool class: class for storing many opened databases
 */

#ifndef GRAPHENE_DBPOOL_H
#define GRAPHENE_DBPOOL_H

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <cstring> /* memset */
#include <db.h>
#include "gr_db.h"

#include "data.h"
#include "filter.h"

/***********************************************************/
// Class for an extended dataset object.
//
// Extended dataset can be just a database name, but it can also
// contain a column, or a filter:
// <name>:<column>
// <name>:<filter>
//
class DBout {
  public:

  bool spp;    // SPP mode (protect # in the beginning of line)

  TimeType ttype;
  DataType dtype;
  Filter * filter;
  bool list;

  std::ostream & out;  // stream for output
  int col; // column number, for the main database

  TimeFMT timefmt;     // output time format
  std::string time0;   // zero time for relative time output (not parsed)

  // constructor -- parse the dataset string, create iostream
  DBout(std::ostream & out_ = std::cout):
          col(-1), out(out_), spp(false), timefmt(TFMT_DEF),
          ttype(TIME_V2), dtype(DATA_DOUBLE), list(false), filter(0) {}

  // print_point  -- by default it just prints the line to out,
  // but this function can be overriden.
  virtual void print_point(const std::string & str){
    if (spp) out << graphene_spp_text(str);
    else out << str;
  }
};

void proc_point(const std::string &ks, const std::string &vs, void * cb_data);

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
                const bool spp, const TimeFMT timefmt, std::ostream & out) {
    int col = -1, flt = -1;
    auto name = parse_ext_name(ext_name, col, flt);
    auto & db = getdb(name, DB_RDONLY);
    DBout dbo(out);
    dbo.ttype   = db.get_ttype();
    dbo.dtype   = db.get_dtype();
    dbo.filter  = db.get_filter_obj(flt);
    dbo.col     = col;
    dbo.timefmt = timefmt;
    dbo.time0   = t;
    dbo.spp     = spp;
    db.get_next(t, proc_point, &dbo);
  }

  // get previous point before t
  void get_prev(const std::string & ext_name, const std::string & t,
                const bool spp, const TimeFMT timefmt, std::ostream & out) {
    int col = -1, flt = -1;
    auto name = parse_ext_name(ext_name, col, flt);
    auto & db = getdb(name, DB_RDONLY);
    DBout dbo(out);
    dbo.ttype   = db.get_ttype();
    dbo.dtype   = db.get_dtype();
    dbo.filter  = db.get_filter_obj(flt);
    dbo.col     = col;
    dbo.timefmt = timefmt;
    dbo.time0   = t;
    dbo.spp     = spp;
    db.get_prev(t, proc_point, &dbo);
  }

  // get previous or interpolated point
  void get(const std::string & ext_name, const std::string & t,
           const bool spp, const TimeFMT timefmt, std::ostream & out) {
    int col = -1, flt = -1;
    auto name = parse_ext_name(ext_name, col, flt);
    auto & db = getdb(name, DB_RDONLY);
    DBout dbo(out);
    dbo.ttype   = db.get_ttype();
    dbo.dtype   = db.get_dtype();
    dbo.filter  = db.get_filter_obj(flt);
    dbo.col     = col;
    dbo.timefmt = timefmt;
    dbo.time0   = t;
    dbo.spp     = spp;
    db.get(t, proc_point, &dbo);
  }

  // get data range
  void get_range(const std::string & ext_name, const std::string & t1,
                 const std::string & t2, const std::string & dt,
                 const bool spp, const TimeFMT timefmt, std::ostream & out) {
    int col = -1, flt = -1;
    auto name = parse_ext_name(ext_name, col, flt);
    auto & db = getdb(name, DB_RDONLY);
    DBout dbo(out);
    dbo.ttype   = db.get_ttype();
    dbo.dtype   = db.get_dtype();
    dbo.filter  = db.get_filter_obj(flt);
    dbo.list   = true;
    dbo.col     = col;
    dbo.timefmt = timefmt;
    dbo.time0   = t1;
    dbo.spp     = spp;
    db.get_range(t1,t2,dt, proc_point, &dbo);
  }

  // get limited number of points starting at t
  void get_count(const std::string & ext_name,
                 const std::string & t, const std::string & cnt,
                 const bool spp, const TimeFMT timefmt, std::ostream & out) {
    int col = -1, flt = -1;
    auto name = parse_ext_name(ext_name, col, flt);
    auto & db = getdb(name, DB_RDONLY);
    DBout dbo(out);
    dbo.ttype   = db.get_ttype();
    dbo.dtype   = db.get_dtype();
    dbo.filter  = db.get_filter_obj(flt);
    dbo.list   = true;
    dbo.col     = col;
    dbo.timefmt = timefmt;
    dbo.time0   = t;
    dbo.spp     = spp;
    db.get_count(t,cnt, proc_point, &dbo);
  }

  /****************/

};


#endif
