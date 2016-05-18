#ifndef DBsts_H
#define DBsts_H

#include <cassert>
#include <cstdlib>
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
  // DBT for timestamp depends on the format:
  static DBT time2dbt(const DBhead fmt, const uint64_t t){
    DBT ret = mk_dbt();
    if (fmt.key == TIME_MS){
      size_t n = sizeof(uint64_t);
      ret.size  = n;
      ret.data  = malloc(n);
      ret.flags = DB_DBT_APPMALLOC;
      if (ret.data == NULL) throw Err() << "Malloc error";
      *(uint64_t *)ret.data = t < 0xFFFFFFFF ? t*1000:t;
    }
    else {
      size_t n = sizeof(uint32_t);
      ret.size  = n;
      ret.data  = malloc(n);
      ret.flags = DB_DBT_APPMALLOC;
      if (ret.data == NULL) throw Err() << "Malloc error";
      *(uint32_t *)ret.data = t < 0xFFFFFFFF ? t:t/1000;
    }
  }
  static uint64_t dbt2time(const DBhead & fmt, const DBT * dbt){
    if (fmt.key == TIME_MS) return *(uint64_t *)dbt->data;
    else                    return *(uint32_t *)dbt->data;
  }

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

  /************************************/
  // Put data to the database
  // input: timestamp + vector of strings + db header
  // The function can be run multiple times without reopening
  // the database and rereading the header.
  //
  void put(const uint64_t t,
           const std::vector<std::string> & dat,
           const DBhead & head){

    if (dat.size() < 1) throw Err() << "Some data expected";

    DBT k = time2dbt(head, t);
    DBT v = mk_dbt();

    // text: join all data
    if (head.val == DATA_TEXT){
      std::string txt = dat[0];
      for (int i=1; i<dat.size(); i++)
        txt+= " " + dat[i];
      v = mk_dbt(txt);
    }
    // numbers: allocate data array
    else {
      size_t buf_size = head.dsize()*dat.size();
      void * buf = malloc(buf_size);
      if (buf == NULL) throw Err() << "malloc error";
      for (int i=0; i<dat.size(); i++){
        std::istringstream s(dat[i]);
        switch (head.val){
          case DATA_INT8:   s >> ((int8_t   *)buf)[i]; break;
          case DATA_UINT8:  s >> ((uint8_t  *)buf)[i]; break;
          case DATA_INT16:  s >> ((int16_t  *)buf)[i]; break;
          case DATA_UINT16: s >> ((uint16_t *)buf)[i]; break;
          case DATA_INT32:  s >> ((int32_t  *)buf)[i]; break;
          case DATA_UINT32: s >> ((uint32_t *)buf)[i]; break;
          case DATA_INT64:  s >> ((int64_t  *)buf)[i]; break;
          case DATA_UINT64: s >> ((uint64_t *)buf)[i]; break;
          case DATA_FLOAT:  s >> ((float    *)buf)[i]; break;
          case DATA_DOUBLE: s >> ((double   *)buf)[i]; break;
          default: throw Err() << "Unexpected data format";
        }
      }
      v.data  = buf;
      v.size  = buf_size;
      v.flags = DB_DBT_APPMALLOC;
    }
    int res = dbp->put(dbp, NULL, &k, &v, 0);
    if (res != 0) throw Err() << name << ".db: " << db_strerror(res);
  }

  /************************************/
  // print data value
  //
  void print_value(const DBhead & head, DBT *k, DBT *v, int col){
    std::cout << dbt2time(head,k) << " ";

    if (head.val==DATA_TEXT){
      // TODO: write v->size chars!
      std::cout << (char *)v->data << "\n";
      return;
    }
/*
    if (v->size % head.dsize() != 0)
      throw Err() << name << ".db: broken database: wrong data length";
    size_t cn = v->size/head.dsize(); // number of columns

    if (col==-1 || head.val==DATA_TEXT){ // write all the data
        switch (head.val){
          case DATA_INT8:   s >> ((int8_t   *)buf)[i]; break;
          case DATA_UINT8:  s >> ((uint8_t  *)buf)[i]; break;
          case DATA_INT16:  s >> ((int16_t  *)buf)[i]; break;
          case DATA_UINT16: s >> ((uint16_t *)buf)[i]; break;
          case DATA_INT32:  s >> ((int32_t  *)buf)[i]; break;
          case DATA_UINT32: s >> ((uint32_t *)buf)[i]; break;
          case DATA_INT64:  s >> ((int64_t  *)buf)[i]; break;
          case DATA_UINT64: s >> ((uint64_t *)buf)[i]; break;
          case DATA_FLOAT:  s >> ((float    *)buf)[i]; break;
          case DATA_DOUBLE: s >> ((double   *)buf)[i]; break;
          default: throw Err() << "Unexpected data format";
        }

    }
*/
  }

  /************************************/
  // get data from the database
  //
  void get_next(const uint64_t t1,
                const int col,
                const DBhead & head){
    /* Get a cursor */
    DBC *curs;
    dbp->cursor(dbp, NULL, &curs, 0);
    if (curs==NULL) throw Err() << name << ".db: can't get a cursor";

    DBT k = time2dbt(head, t1);
    DBT v = mk_dbt();
    int res = curs->c_get(curs, &k, &v, DB_SET_RANGE);
    curs->close(curs);
    if (res!=0) throw Err() << name << ".db: " << db_strerror(res);
    print_value(head, &k, &v, col);
  }

  void get_prev(const uint64_t t2,
                const int col,
                const DBhead & head){
    // TODO
  }

  void get_interp(const uint64_t t,
                  const int col,
           const DBhead & head){
    // TODO
  }

  void get_range(const uint64_t t1,
                 const uint64_t t2,
                 const uint64_t dt,
                 const int col,
                 const DBhead & head){
    /* Get a cursor */
    DBC *curs;
    dbp->cursor(dbp, NULL, &curs, 0);
    if (curs==NULL) throw Err() << name << ".db: can't get a cursor";

    DBT k = time2dbt(head, t1);
    DBT v = mk_dbt();
    int fl = DB_SET_RANGE; // first get t >= t1
    while (curs->c_get(curs, &k, &v, fl)==0 &&
           t2 >= dbt2time(head,&k)){
      fl = DB_NEXT;
      // TODO
      // use v.data
    }
    curs->close(curs);
  }
};

#endif
