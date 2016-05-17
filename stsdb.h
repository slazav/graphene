#ifndef DBsts_H
#define DBsts_H

#include <cassert>
#include <string>
#include <vector>
#include <sstream>
#include <cstring> /* memset */
#include <db.h>


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

// Enums for the database format
enum TimeFMT { TIME_S, TIME_MS};
enum DataFMT { DATA_TEXT,
         DATA_INT8, DATA_UINT8, DATA_INT16, DATA_UINT16,
         DATA_INT32, DATA_UINT32, DATA_INT64, DATA_UINT64,
         DATA_FLOAT, DATA_DOUBLE};

/// default values for database format
const TimeFMT DEFAULT_TIMEFMT = TIME_S;
const DataFMT DEFAULT_DATAFMT = DATA_DOUBLE;

// last values -- for loops and array dimensions
const TimeFMT LAST_TIMEFMT = TIME_MS;
const DataFMT LAST_DATAFMT = DATA_DOUBLE;

// string names of time formats:
const std::string time_fmt_names[LAST_TIMEFMT+1] = {"S", "MS"};

// string names of data formats
const std::string data_fmt_names[LAST_DATAFMT+1] =
  {"TEXT", "INT8", "UINT8", "INT16", "UINT16",
   "INT32", "UINT32", "INT64", "UINT64", "FLOAT", "DOUBLE"};

// sizes of data elements, bytes
const static size_t data_fmt_sizes[LAST_DATAFMT+1] =
      {1,1,1,2,2,4,4,8,8,4,8}; // bytes


/***********************************************************/

// Class for the database header.
// Key format is int32_t (time in seconds) or uint64_t (time in ms).
class DBhead {
  public:
  TimeFMT key;
  DataFMT val;
  std::string descr;

  DBhead(const TimeFMT k = TIME_S,
      const DataFMT v = DATA_DOUBLE,
      const std::string &d = std::string())
        : key(k),val(v),descr(d) {}

  // return size of the data field, names of data and time formats
  size_t dsize() const { return data_fmt_sizes[val]; }
  std::string dname() const { return data_fmt_names[val]; }
  std::string tname() const { return time_fmt_names[key]; }

  // convert strings into enum members
  static TimeFMT str2timefmt(const std::string & s){
    for (int i = 0; i<=LAST_TIMEFMT; i++)
      if (s == time_fmt_names[i]) return static_cast<TimeFMT>(i);
    throw Err() << "Unknown time format: " << s;
  }
  static DataFMT str2datafmt(const std::string & s){
    for (int i = 0; i<=LAST_DATAFMT; i++)
      if (s == data_fmt_names[i]) return static_cast<DataFMT>(i);
    throw Err() << "Unknown data format: " << s;
  }
  // ...and back
  static std::string timefmt2str(const TimeFMT s){
    return time_fmt_names[s]; }
  static std::string datafmt2str(const DataFMT s){
    return data_fmt_names[s]; }

  bool operator==(const DBhead &o) const{
    return o.key==key && o.val==val && o.descr==descr; }
};

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
    ret.size = str.length()+1;
    return ret;
  }
  template <typename T>
  static DBT mk_dbt(const T * v){
    DBT ret = mk_dbt();
    ret.data = (void *)v;
    ret.size = sizeof(T);
    return ret;
  }
// TODO -- 
//  static DBT time2dbt(const DBhead fmt, const uint64_t t){
//    if (fmt.key == TIME_MS)
//      return DBsts::mk_dbt(t < 0xFFFFFFFF ? t*1000:t);
//    else
//      return DBsts::mk_dbt(t < 0xFFFFFFFF ? (uint32_t)t : (uint32_t)(t/1000));
//  }
//  static uint64_t dbt2time(const DBhead & fmt, const DBT & dbt){
//    if (fmt.key == TIME_MS)
//      return *(uint64_t *)dbt.data;
//    else
//      return *(uint32_t *)dbt.data;
//  }

  /************************************/
  /* database handler and memory management */
    DB *dbp;
    std::string name;
    int open_flags;
    int *refcounter;

    void copy(const DBsts & other){
      dbp        = other.dbp;
      name       = other.name;
      open_flags = other.open_flags;
      refcounter = other.refcounter;
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
  //
  DBsts(const std::string & path_,
       const std::string & name_,
       const int flags){

    refcounter   = new int;
    *refcounter  = 1;

    name = name_;
    open_flags = flags;
    std::string fname = path_ + "/" + name_ + ".db";

    /* Initialize the DB handle */
    int ret = db_create(&dbp, NULL, 0);
    if (ret != 0)
      throw Err() << name << ".db: " << db_strerror(ret);

    /* Open the database */
    ret = dbp->open(dbp,           /* Pointer to the database */
                    NULL,          /* Txn pointer */
                    fname.c_str(), /* File name */
                    NULL,          /* DB name */
                    DB_BTREE,      /* Database type (using btree) */
                    flags,         /* Open flags */
                    0600);         /* File mode. Using defaults */
    if (ret != 0)
      throw Err() << name << ".db: " << db_strerror(ret);
  }

  /************************************/
  // Write database information.
  // key = (uint8_t)0 (1byte),
  // value = time_fmt (1byte) + data_fmt (1byte) + description
  void write_info(const DBhead &head){
    // remove the info entry if it exists
    uint8_t x=0;
    DBT k = mk_dbt(&x);
    int ret = dbp->del(dbp, NULL, &k, 0);
    if (ret != 0 && ret != DB_NOTFOUND)
      throw Err() << name << ".db: " << db_strerror(ret);
    // write new data
    std::string vs = std::string(1, (char)head.key)
                   + std::string(1, (char)head.val)
                   + head.descr;
    DBT v = mk_dbt(vs);
    ret = dbp->put(dbp, NULL, &k, &v, 0);
    if (ret != 0)
      throw Err() << name << ".db: " << db_strerror(ret);
  }

  /************************************/
  // Get database information
  //
  DBhead read_info(){
    uint8_t x=0;
    DBT k = mk_dbt(&x);
    DBT v = mk_dbt();
    int ret = dbp->get(dbp, NULL, &k, &v, 0);
    if (ret != 0)
     throw Err() << name << ".db: " << db_strerror(ret);
    DBhead head;
    uint8_t tfmt = *(uint8_t*)v.data;
    uint8_t dfmt = *((uint8_t*)v.data+1);
    if (tfmt<0 || tfmt > LAST_TIMEFMT)
      throw Err() << name << ".db: broken database, bad time format";
    if (dfmt<0 || dfmt > LAST_DATAFMT)
      throw Err() << name << ".db: broken database, bad data format";
    head.key = static_cast<TimeFMT>(tfmt);
    head.val = static_cast<DataFMT>(dfmt);
    head.descr = std::string((char*)v.data+2, (char*)v.data+v.size-1);
    return head;
  }
};

#endif
