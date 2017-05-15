/* DBgr class: wrapper for BerkleyDB, open/put/get/del functions
 */

#ifndef GRAPHENE_DB_H
#define GRAPHENE_DB_H

#include <cassert>
#include <cstdlib>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <cstring> /* memset */
#include <db.h>
#include "dbinfo.h"
#include "dbout.h"

// bercleydb:
//  http://docs.oracle.com/cd/E17076_02/html/gsg/C/index.html
//  https://web.stanford.edu/class/cs276a/projects/docs/berkeleydb/reftoc.html


/***********************************************************/
// type for data processing function.
typedef void(process_data_func)(DBT*,DBT*,const DBinfo&, void *usr_data);


/***********************************************************/
/* class for wrapping BerkleyDB */
class DBgr{
  public:
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

  /************************************/
  /* data and memory management */
    DB *dbp;
    std::string name;    // database name
    int open_flags;      // database open flags
    DBinfo db_info;      // database information
    bool info_is_actual; // is the info the same as in the file?
    int *refcounter;

    void copy(const DBgr & other){
      dbp        = other.dbp;
      name       = other.name;
      open_flags = other.open_flags;
      refcounter = other.refcounter;
      db_info    = other.db_info;
      info_is_actual = other.info_is_actual;
      (*refcounter)++;
      assert(*refcounter >0);
    }
    void destroy(void){
      (*refcounter)--;
      if (*refcounter<=0){
        int ret = dbp->close(dbp, 0); // close db
        if (ret!=0) throw Err() << name << ".db: " << db_strerror(ret);
        delete refcounter;
      }
    }

  /************************************/
  // Copy constructor, destructor, assignment
  public:

    DBgr(const DBgr & other){ copy(other); }
    DBgr & operator=(const DBgr & other){
      if (this != &other){ destroy(); copy(other); }
      return *this;
    }
    ~DBgr(){ destroy(); }

  /************************************/
  // Constructor -- open a database
  // Path is a path to the database foolder.
  // Name is a database name, it can not contain some symbols (.|+ \n\t)
  DBgr(DB_ENV *env,
       const std::string & path_,
       const std::string & name_,
       const int flags);

  // Write database information.
  // key = (uint8_t)0 (1byte),  value = data_fmt (1byte) + description
  // key = (uint8_t)1 (1byte),  value = version  (1byte)
  void write_info(const DBinfo &info);

  // Get database information
  DBinfo read_info();

  // Put data to the database
  // input: timestamp + vector of strings + dpolicy
  // The function can be run multiple times without reopening
  // the database.
  // dpolicy -- what to do with duplicated timestamps:
  //   (replace, skip, error, sshift, nsshift)
  void put(const std::string &t, const std::vector<std::string> & dat,
           const std::string &dpolicy);

  // All get* functions get some data from the database
  // and call dbo.proc_point() for each key-value pair

  // get data from the database -- get_next
  void get_next(const std::string &t1, DBout & dbo);

  // get data from the database -- get_prev
  void get_prev(const std::string &t2, DBout & dbo);

  // get data from the database -- get
  void get(const std::string &t,  DBout & dbo);

  // get data from the database -- get_range
  void get_range(const std::string &t1, const std::string &t2,
                 const std::string &dt, DBout & dbo);


  // delete data data from the database -- del_range
  void del(const std::string &t1);

  // delete data data from the database -- del_range
  void del_range(const std::string &t1, const std::string &t2);

  // sync the database
  void sync() {dbp->sync(dbp, 0);}

};

#endif
