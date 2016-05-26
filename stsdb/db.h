/* work with BerkleyDB for the Simple time series database. */

#ifndef STSDB_DB_H
#define STSDB_DB_H

#include <cassert>
#include <cstdlib>
#include <string>
#include <vector>
#include <sstream>
#include <cstring> /* memset */
#include <db.h>

// bercleydb:
//  http://docs.oracle.com/cd/E17076_02/html/gsg/C/index.html
//  https://web.stanford.edu/class/cs276a/projects/docs/berkeleydb/reftoc.html

/***********************************************************/
// Class for exceptions.
// Usage: throw Err() << "something";
//
class Err {
  std::ostringstream s;
  public:
    Err(){}
    Err(const Err & o) { s << o.s.str(); }
    template <typename T>
    Err & operator<<(const T & o){ s << o; return *this; }
    std::string str()   const { return s.str(); }
};

/***********************************************************/

// Enum for the data format
enum DataFMT { DATA_TEXT,
         DATA_INT8, DATA_UINT8, DATA_INT16, DATA_UINT16,
         DATA_INT32, DATA_UINT32, DATA_INT64, DATA_UINT64,
         DATA_FLOAT, DATA_DOUBLE};

/// default value
const DataFMT DEFAULT_DATAFMT = DATA_DOUBLE;

// last values -- for loops and array dimensions
const DataFMT LAST_DATAFMT = DATA_DOUBLE;

// string names of data formats
const std::string data_fmt_names[LAST_DATAFMT+1] =
  {"TEXT", "INT8", "UINT8", "INT16", "UINT16",
   "INT32", "UINT32", "INT64", "UINT64", "FLOAT", "DOUBLE"};

// sizes of data elements, bytes
const static size_t data_fmt_sizes[LAST_DATAFMT+1] =
      {1,1,1,2,2,4,4,8,8,4,8}; // bytes


/***********************************************************/

// Class for the database information.
class DBinfo {
  public:
  DataFMT val;
  std::string descr;

  DBinfo(const DataFMT v = DATA_DOUBLE,
         const std::string &d = std::string())
            : val(v),descr(d) {}

  // return size and name of the data format
  size_t dsize() const { return data_fmt_sizes[val]; }
  std::string dname() const { return data_fmt_names[val]; }

  // convert string into enum member
  static DataFMT str2datafmt(const std::string & s){
    for (int i = 0; i<=LAST_DATAFMT; i++)
      if (s == data_fmt_names[i]) return static_cast<DataFMT>(i);
    throw Err() << "Unknown data format: " << s;
  }
  // ...and back
  static std::string datafmt2str(const DataFMT s){
    return data_fmt_names[s]; }

  bool operator==(const DBinfo &o) const{
    return o.val==val && o.descr==descr; }

  // Pack timestamp according with time format.
  // std::string is used as a convenient data storage, which
  // can be easily converted into Berkleydb data.
  // It is not a c-string!
  std::string pack_time(const uint64_t t) const;

  // same, but with string on input
  std::string pack_time(const std::string & ts) const;

  // Unpack timestamp
  uint64_t unpack_time(const std::string & s) const;

  // Pack data according with data format
  // std::string is used as a convenient data storage, which
  // can be easily converted into Berkleydb data.
  // Output string is not a c-string!
  std::string pack_data(const std::vector<std::string> & strs) const;

  // Unpack data
  std::string unpack_data(const std::string & s, const int col=-1) const;

  // Unpack data to a double value (for json output)
  // only one column are returned, 0 by default
  double unpack_data_d(const std::string & s, const int col=-1) const;

};

/***********************************************************/
// Key compare function for the database
// I compare all types of integers (64 bits are used for timestamps,
// 8 bits are used for database information)
// Shorter ints always smaller!
//int cmpfunc(DB *dbp, const DBT *a, const DBT *b);

/***********************************************************/
// type for data processing function.
typedef void(process_data_func)(DBT*,DBT*,const int,const DBinfo&);

//implementation for stdout printing
void print_value(DBT *k, DBT *v, const int col, const DBinfo & info);

/***********************************************************/
// Normilize db name. Remove starting /,
// remove . and .. path components.
std::string norm_name(const std::string & name);

/***********************************************************/
/***********************************************************/

/* class for wrapping BerkleyDB */
class DBsts{
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

    void copy(const DBsts & other){
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

    DBsts(const DBsts & other){ copy(other); }
    DBsts & operator=(const DBsts & other){
      if (this != &other){ destroy(); copy(other); }
      return *this;
    }
    ~DBsts(){ destroy(); }

  /************************************/
  // Constructor -- open a database
  DBsts(const std::string & path_,
       const std::string & name_,
       const int flags);

  // Write database information.
  // key = (uint8_t)0 (1byte),
  // value = data_fmt (1byte) + description
  void write_info(const DBinfo &info);

  // Get database information
  DBinfo read_info();

  // Put data to the database
  // input: timestamp + vector of strings
  // The function can be run multiple times without reopening
  // the database.
  void put(const uint64_t t, const std::vector<std::string> & dat);

  // interpolate data and pack it into DBT string as double array
  std::string print_interp(const uint64_t t0,
                           const std::string & k1, const std::string & k2,
                           const std::string & v1, const std::string & v2,
                           const int col);

  // get data from the database -- get_next
  void get_next(const uint64_t t1, const int col,
                process_data_func proc_data);

  // get data from the database -- get_prev
  void get_prev(const uint64_t t2, const int col,
                process_data_func proc_data);

  // get data from the database -- get_interp
  void get_interp(const uint64_t t, const int col,
                  process_data_func proc_data);

  // get data from the database -- get_range
  void get_range(const uint64_t t1, const uint64_t t2,
                 const uint64_t dt, const int col,
                 process_data_func proc_data);


  // delete data data from the database -- del_range
  void del(const uint64_t t1);

  // delete data data from the database -- del_range
  void del_range(const uint64_t t1, const uint64_t t2);

};

#endif
