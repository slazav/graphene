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
  std::string pack_time(const uint64_t t) const{
    std::string ret(sizeof(uint64_t), '\0');
    *(uint64_t *)ret.data() = t;
    return ret;
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
    if (s.size()!=sizeof(uint64_t))
      throw Err() << "Broken database: wrong timestamp size";
    return *(uint64_t *)s.data();
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
    if (val == DATA_TEXT){
      // remove linebreaks
      int i;
      std::string ret=s;
      while ((i=ret.find('\n'))!=std::string::npos) ret[i] = ' ';
      return ret;
    }
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
// type for data processing function and an implementation
// for stdout printing
//
typedef void(process_data_func)(DBT*,DBT*,const int,const DBinfo&);

void print_value(DBT *k, DBT *v, const int col,
                 const DBinfo & info){
  // check for correct key size (do not parse DB info)
  if (k->size!=sizeof(uint64_t)) return;
  // convert DBT to strings
  std::string ks((char *)k->data, (char *)k->data+k->size);
  std::string vs((char *)v->data, (char *)v->data+v->size);
  // unpack and print values
  std::cout << info.unpack_time(ks) << " "
            << info.unpack_data(vs, col) << "\n";
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
  //
  DBsts(const std::string & path_,
       const std::string & name_,
       const int flags){

    refcounter   = new int;
    *refcounter  = 1;
    info_is_actual = false;

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
  // value = data_fmt (1byte) + description
  //
  void write_info(const DBinfo &info){
    // remove the info entry if it exists
    uint8_t x=0;
    DBT k = mk_dbt(&x);
    int ret = dbp->del(dbp, NULL, &k, 0);
    if (ret != 0 && ret != DB_NOTFOUND)
      throw Err() << name << ".db: " << db_strerror(ret);
    // write new data
    std::string vs = std::string(1, (char)info.val)
                   + info.descr;
    DBT v = mk_dbt(vs);
    ret = dbp->put(dbp, NULL, &k, &v, 0);
    if (ret != 0)
      throw Err() << name << ".db: " << db_strerror(ret);
    db_info = info;
    info_is_actual = true;
  }

  /************************************/
  // Get database information
  //
  DBinfo read_info(){
    if (info_is_actual) return db_info;
    uint8_t x=0;
    DBT k = mk_dbt(&x);
    DBT v = mk_dbt();
    int ret = dbp->get(dbp, NULL, &k, &v, 0);
    if (ret != 0)
     throw Err() << name << ".db: " << db_strerror(ret);
    uint8_t dfmt = *((uint8_t*)v.data);
    if (dfmt<0 || dfmt > LAST_DATAFMT)
      throw Err() << name << ".db: broken database, bad data format in the header";
    db_info.val = static_cast<DataFMT>(dfmt);
    db_info.descr = std::string((char*)v.data+1, (char*)v.data+v.size);
    info_is_actual = true;
    return db_info;
  }

  /************************************/
  // Put data to the database
  // input: timestamp + vector of strings
  // The function can be run multiple times without reopening
  // the database.
  //
  void put(const uint64_t t,
           const std::vector<std::string> & dat){
    DBinfo info = read_info();
    std::string ks = info.pack_time(t);
    std::string vs = info.pack_data(dat);
    DBT k = mk_dbt(ks);
    DBT v = mk_dbt(vs);
    int res = dbp->put(dbp, NULL, &k, &v, 0);
    if (res != 0) throw Err() << name << ".db: " << db_strerror(res);
  }

  /************************************/
  // interpolate data and pack it into DBT string as double array
  //
  std::string print_interp(const uint64_t t0,
                           const std::string & k1, const std::string & k2,
                           const std::string & v1, const std::string & v2,
                           const int col){
    DBinfo info = read_info();
    // check for correct key size (do not parse DB info)
    if (k1.size()!=sizeof(uint64_t)) return "";
    if (k2.size()!=sizeof(uint64_t)) return "";
    // unpack time
    uint64_t t1 = info.unpack_time(k1);
    uint64_t t2 = info.unpack_time(k2);
    // unpack data
    std::istringstream strv1(info.unpack_data(v1, col));
    std::istringstream strv2(info.unpack_data(v2, col));
    // calculate first point weight
    uint64_t dt1 = std::max(t1,t0)-std::min(t1,t0);
    uint64_t dt2 = std::max(t2,t0)-std::min(t2,t0);
    double k = (double)dt2/(dt1+dt2);

    // collect interpolated values
    std::vector<double> values;
    while (!strv1.eof() && !strv2.eof()){
      double dv1, dv2;
      strv1 >> dv1;
      strv2 >> dv2;
      values.push_back(dv1*k + dv2*(1-k));
    }
    // pack data in a string storage
    return std::string((char*)values.data(),
       (char*)values.data()+sizeof(double)*values.size());
  }

  /************************************/
  // get data from the database -- get_next
  //
  void get_next(const uint64_t t1, const int col,
                process_data_func proc_data){
    DBinfo info = read_info();
    /* Get a cursor */
    DBC *curs;
    dbp->cursor(dbp, NULL, &curs, 0);
    if (curs==NULL) throw Err() << name << ".db: can't get a cursor";

    std::string ks = info.pack_time(t1);
    DBT k = mk_dbt(ks);
    DBT v = mk_dbt();
    int res = curs->c_get(curs, &k, &v, DB_SET_RANGE);
    if (res==DB_NOTFOUND) { curs->close(curs); return; }
    if (res!=0) throw Err() << name << ".db: " << db_strerror(res);
    print_value(&k, &v, col, info);
    curs->close(curs);
  }

  /************************************/
  // get data from the database -- get_prev
  //
  void get_prev(const uint64_t t2, const int col,
                process_data_func proc_data){
    DBinfo info = read_info();
    /* Get a cursor */
    DBC *curs;
    dbp->cursor(dbp, NULL, &curs, 0);
    if (curs==NULL) throw Err() << name << ".db: can't get a cursor";
    std::string ks = info.pack_time(t2);
    DBT k = mk_dbt(ks);
    DBT v = mk_dbt();

    int res = curs->c_get(curs, &k, &v, DB_SET_RANGE);
    if (res!=0 && res!=DB_NOTFOUND)
      throw Err() << name << ".db: " << db_strerror(res);

    // unpack time
    std::string s((char *)k.data, (char *)k.data+k.size);
    uint64_t tn = info.unpack_time(s);

    // if needed, get previous record:
    if (tn > t2 || res==DB_NOTFOUND){
      res = curs->c_get(curs, &k, &v, DB_PREV);
      if (res==DB_NOTFOUND) { curs->close(curs); return; }
      if (res!=0) throw Err() << name << ".db: " << db_strerror(res);
    }

    print_value(&k, &v, col, info);
    curs->close(curs);
  }

  /************************************/
  // get data from the database -- get_interp
  //
  void get_interp(const uint64_t t, const int col,
                process_data_func proc_data){
    DBinfo info = read_info();
    if (info.val==DATA_TEXT)
      throw Err() << "Can not do interpolation of TEXT data";
    /* Get a cursor */
    DBC *curs;
    dbp->cursor(dbp, NULL, &curs, 0);
    if (curs==NULL) throw Err() << name << ".db: can't get a cursor";
    std::string ks = info.pack_time(t);
    DBT k = mk_dbt(ks);
    DBT v = mk_dbt();

    int res = curs->c_get(curs, &k, &v, DB_SET_RANGE);
    if (res==DB_NOTFOUND) { curs->close(curs); return; }
    if (res!=0) throw Err() << name << ".db: " << db_strerror(res);

    std::string ks1((char *)k.data, (char *)k.data+k.size);
    std::string vs1((char *)v.data, (char *)v.data+v.size);
    if (info.unpack_time(ks1) == t){
      print_value(&k, &v, col, info);
    }
    else {
      res = curs->c_get(curs, &k, &v, DB_PREV);
      if (res==DB_NOTFOUND) { curs->close(curs); return; }
      if (res!=0) throw Err() << name << ".db: " << db_strerror(res);
      std::string ks2((char *)k.data, (char *)k.data+k.size);
      std::string vs2((char *)v.data, (char *)v.data+v.size);
      std::string ks0 = info.pack_time(t);
      std::string vs0 = print_interp(t, ks1, ks2, vs1, vs2, col);
      if (vs0!=""){
        DBT k0 = mk_dbt(ks0);
        DBT v0 = mk_dbt(vs0);
        // print_interp converted everything to double and chose correct columns
        print_value(&k0, &v0, -1, DBinfo(DATA_DOUBLE));
      }
    }
    curs->close(curs);
  }

  /************************************/
  // get data from the database -- get_range
  //
  // There can be two different cases: distance between
  // data points << dt or >> dt.
  // In the first case it is better to use DB_SET_RANGE,
  // in the second one -- DB_NEXT.
  // To work fast in both cases we use both methods
  // (order is not too important, but we prefer the 1nd case):
  // use DB_SET_RANGE with dt shift, if the point didn't
  // change - use DB_NEXT.
  void get_range(const uint64_t t1, const uint64_t t2,
                 const uint64_t dt, const int col,
                 process_data_func proc_data){

    DBinfo info = read_info();
    /* Get a cursor */
    DBC *curs;
    dbp->cursor(dbp, NULL, &curs, 0);
    if (curs==NULL) throw Err() << name << ".db: can't get a cursor";

    std::string ks = info.pack_time(t1);
    DBT k = mk_dbt(ks);
    DBT v = mk_dbt();
    uint64_t tl = -1; // last printed value

    int fl = DB_SET_RANGE; // first get t >= t1
    while (1){
      int res = curs->c_get(curs, &k, &v, fl);
      if (res==DB_NOTFOUND) break;
      if (res!=0) throw Err() << name << ".db: " << db_strerror(res);

      // unpack new time value and check the range
      std::string s((char *)k.data, (char *)k.data+k.size);
      uint64_t tn = info.unpack_time(s);
      if (tn > t2 ) break;

      // if we want every point, switch to DB_NEXT and repeat
      if (dt<=1){
        print_value(&k, &v, col, info);
        fl=DB_NEXT;
        continue;
      }

      // If dt >=1 we continue using fl=DB_SET_RANGE.
      // If new value the same as old
      if (tl==tn){
        // get next value
        res = curs->c_get(curs, &k, &v, DB_NEXT);
        if (res==DB_NOTFOUND) break;
        if (res!=0) throw Err() << name << ".db: " << db_strerror(res);
        // unpack new time value and check the range
        std::string s((char *)k.data, (char *)k.data+k.size);
        tn = info.unpack_time(s);
        if (tn > t2 ) break;
      }

      print_value(&k, &v, col, info);
      tl=tn; // update last printed value

      // add dt to the key for the next loop:
      std::string sp = info.pack_time(tl+dt);
      memcpy(k.data,sp.data(),k.size);
    }
    curs->close(curs);
  }

};

#endif
