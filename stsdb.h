#ifndef DBsts_H
#define DBsts_H

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


  // Pack timestamp according with time format.
  // std::string is used as a convenient data storage, which
  // can be easily converted into Berkleydb data.
  // It is not a c-string!
  // NOTE: if we store time in seconds, we can't use all the (64bit ms range)/1000
  std::string pack_time(const uint64_t t) const{
    if (key == TIME_MS){
      std::string ret(sizeof(uint64_t), '\0');
      *(uint64_t *)ret.data() = t < 0xFFFFFFFF ? t*1000:t;
      return ret;
    }
    if (key == TIME_S){
      std::string ret(sizeof(uint32_t), '\0');
      if      (t<0xFFFFFFFFll)      *(uint32_t *)ret.data() = t;
      else if (t<0xFFFFFFFFll*1000) *(uint32_t *)ret.data() = t/1000;
      else                          *(uint32_t *)ret.data() = 0xFFFFFFFF;
      return ret;
    }
    throw Err() << "Unexpected time format";
  }
  // same, but with string on input
  std::string pack_time(const std::string & ts) const{
    std::istringstream s(ts);
    uint64_t t;
    s >> t;
    if (s.bad() || s.fail() || !s.eof())
      throw Err() << "Not a timestamp: " << ts;
    return pack_time(t);
  }
  // Unpack timestamp
  uint64_t unpack_time(const std::string & s) const{
    if (key == TIME_MS){
      if (s.size()!=sizeof(uint64_t))
        throw Err() << "Broken database: wrong timestamp size";
      return *(uint64_t *)s.data();
    }
    if (key == TIME_S){
      if (s.size()!=sizeof(uint32_t))
        throw Err() << "Broken database: wrong timestamp size";
      return *(uint32_t *)s.data();
    }
    throw Err() << "Unexpected time format";
  }

  // Pack data according with data format
  // std::string is used as a convenient data storage, which
  // can be easily converted into Berkleydb data.
  // Output string is not a c-string!
  std::string pack_data(const std::vector<std::string> & strs) const{
    if (strs.size() < 1) throw Err() << "Some data expected";
    std::string ret;
    if (val == DATA_TEXT){ // text: join all data
      ret = strs[0];
      for (int i=1; i<strs.size(); i++)
        ret+= " " + strs[i];
    }
    else {    // numbers
      ret = std::string(dsize()*strs.size(), '\0');
      for (int i=0; i<strs.size(); i++){
        std::istringstream s(strs[i]);
        switch (val){
          case DATA_INT8:   s >> ((int8_t   *)ret.data())[i]; break;
          case DATA_UINT8:  s >> ((uint8_t  *)ret.data())[i]; break;
          case DATA_INT16:  s >> ((int16_t  *)ret.data())[i]; break;
          case DATA_UINT16: s >> ((uint16_t *)ret.data())[i]; break;
          case DATA_INT32:  s >> ((int32_t  *)ret.data())[i]; break;
          case DATA_UINT32: s >> ((uint32_t *)ret.data())[i]; break;
          case DATA_INT64:  s >> ((int64_t  *)ret.data())[i]; break;
          case DATA_UINT64: s >> ((uint64_t *)ret.data())[i]; break;
          case DATA_FLOAT:  s >> ((float    *)ret.data())[i]; break;
          case DATA_DOUBLE: s >> ((double   *)ret.data())[i]; break;
          default: throw Err() << "Unexpected data format";
        }
        if (s.bad() || s.fail() || !s.eof())
          throw Err() << "Can't put value into "
                      << datafmt2str(val) << " database: " << strs[i];
      }
    }
    return ret;
  }

  // Unpack data
  std::string unpack_data(const std::string & s, const int col=-1) const{
    if (val == DATA_TEXT) return s;
    if (s.size() % dsize() != 0)
      throw Err() << "Broken database: wrong data length";
    // number of columns
    size_t cn = s.size()/dsize();
    // column range we want to show:
    size_t c1=0, c2=cn;
    if (col!=-1) { c1=col; c2=col+1; }

    std::ostringstream ostr;
    for (size_t i=c1; i<c2; i++){
      if (i>c1) ostr << ' ';
      if (i>=cn) { return "NaN"; }
      switch (val){
        case DATA_INT8:   ostr << ((int8_t   *)s.data())[i]; break;
        case DATA_UINT8:  ostr << ((uint8_t  *)s.data())[i]; break;
        case DATA_INT16:  ostr << ((int16_t  *)s.data())[i]; break;
        case DATA_UINT16: ostr << ((uint16_t *)s.data())[i]; break;
        case DATA_INT32:  ostr << ((int32_t  *)s.data())[i]; break;
        case DATA_UINT32: ostr << ((uint32_t *)s.data())[i]; break;
        case DATA_INT64:  ostr << ((int64_t  *)s.data())[i]; break;
        case DATA_UINT64: ostr << ((uint64_t *)s.data())[i]; break;
        case DATA_FLOAT:  ostr << ((float    *)s.data())[i]; break;
        case DATA_DOUBLE: ostr << ((double   *)s.data())[i]; break;
        default: throw Err() << "Unexpected data format";
      }
    }
    return ostr.str();
  }

};

/***********************************************************/
// Key compare function for the database
// I compare all types of integers (64 and 32 bits are used for timestamps,
// 8 bits are used for database information)
// Shorter ints always smaller!
// (THINKABOUT: do we want to mix 64 and 32 bit integers in one DB?)
int cmpfunc(DB *dbp, const DBT *a, const DBT *b){
  uint64_t v1,v2;
  if (a->size == sizeof(uint64_t) && b->size == sizeof(uint64_t)){
    v1 = *(uint64_t*)a->data;
    v2 = *(uint64_t*)b->data; }
  else
  if (a->size == sizeof(uint32_t) && b->size == sizeof(uint32_t)){
    v1 = *(uint32_t*)a->data;
    v2 = *(uint32_t*)b->data; }
  else
  if (a->size == sizeof(uint16_t) && b->size == sizeof(uint16_t)){
    v1 = *(uint16_t*)a->data;
    v2 = *(uint16_t*)b->data; }
  else
  if (a->size == sizeof(uint8_t) && b->size == sizeof(uint8_t)){
    v1 = *(uint8_t*)a->data;
    v2 = *(uint8_t*)b->data; }
  else {
    v1 = a->size;
    v2 = b->size;
  }
  if (v1>v2) return 1;
  if (v2>v1) return -1;
  return 0;
}

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

    /* set key compare function */
    ret = dbp->set_bt_compare(dbp, cmpfunc);
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
    head.descr = std::string((char*)v.data+2, (char*)v.data+v.size);
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
    std::string ks = head.pack_time(t);
    std::string vs = head.pack_data(dat);
    DBT k = mk_dbt(ks);
    DBT v = mk_dbt(vs);
    int res = dbp->put(dbp, NULL, &k, &v, 0);
    if (res != 0) throw Err() << name << ".db: " << db_strerror(res);
  }

  /************************************/
  // print data value
  //
  void print_value(const DBhead & head, DBT *k, DBT *v, int col){
    if (k->size!=sizeof(uint32_t) && k->size!=sizeof(uint64_t)) return;
    std::string ks((char *)k->data, (char *)k->data+k->size);
    std::string vs((char *)v->data, (char *)v->data+v->size);
    std::cout << head.unpack_time(ks) << " "
              << head.unpack_data(vs) << "\n";
  }

  /************************************/
  // get data from the database -- get_next
  //
  void get_next(uint64_t t1,
                const int col,
                const DBhead & head){
    /* Get a cursor */
    DBC *curs;
    dbp->cursor(dbp, NULL, &curs, 0);
    if (curs==NULL) throw Err() << name << ".db: can't get a cursor";

    std::string ks = head.pack_time(t1);
    DBT k = mk_dbt(ks);
    DBT v = mk_dbt();
    int res = curs->c_get(curs, &k, &v, DB_SET_RANGE);
    if (res==DB_NOTFOUND) return;
    if (res!=0) throw Err() << name << ".db: " << db_strerror(res);
    print_value(head, &k, &v, col);
    curs->close(curs);
  }

  /************************************/
  // get data from the database -- get_prev
  //
  void get_prev(const uint64_t t2,
                const int col,
                const DBhead & head){
    /* Get a cursor */
    DBC *curs;
    dbp->cursor(dbp, NULL, &curs, 0);
    if (curs==NULL) throw Err() << name << ".db: can't get a cursor";
    std::string ks = head.pack_time(t2);
    DBT k = mk_dbt(ks);
    DBT v = mk_dbt();

    int res = curs->c_get(curs, &k, &v, DB_SET_RANGE);
    if (res!=0 && res!=DB_NOTFOUND)
      throw Err() << name << ".db: " << db_strerror(res);
    if (k.size<sizeof(int32_t)) return;

    res = curs->c_get(curs, &k, &v, DB_PREV);
    if (res==DB_NOTFOUND) return;
    if (res!=0) throw Err() << name << ".db: " << db_strerror(res);
    if (k.size<sizeof(int32_t)) return;

    print_value(head, &k, &v, col);
    curs->close(curs);
  }

  /************************************/
  // get data from the database -- get_interp
  //
  void get_interp(const uint64_t t,
                  const int col,
           const DBhead & head){
    // TODO
  }

  /************************************/
  // get data from the database -- get_range
  //
  void get_range(const uint64_t t1,
                 const uint64_t t2,
                 const uint64_t dt,
                 const int col,
                 const DBhead & head){
    /* Get a cursor */
    DBC *curs;
    dbp->cursor(dbp, NULL, &curs, 0);
    if (curs==NULL) throw Err() << name << ".db: can't get a cursor";

    std::string ks = head.pack_time(t1);
    DBT k = mk_dbt(ks);
    DBT v = mk_dbt();
    int fl = DB_SET_RANGE; // first get t >= t1
    while (curs->c_get(curs, &k, &v, fl)==0){
      std::string s((char *)v.data, (char *)v.data+v.size);
      if (t2 >= head.unpack_time(s)) break;

      fl = DB_NEXT;
      // TODO
      // use v.data
    }
    curs->close(curs);
  }
};

#endif
