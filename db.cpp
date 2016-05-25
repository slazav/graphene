#include <cstdlib>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <cstring> /* memset */
#include "db.h"

using namespace std;

// Pack timestamp according with time format.
string
DBinfo::pack_time(const uint64_t t) const{
  string ret(sizeof(uint64_t), '\0');
  *(uint64_t *)ret.data() = t;
  return ret;
}

// same, but with string on input
string
DBinfo::pack_time(const string & ts) const{
  istringstream s(ts);
  uint64_t t;
  s >> t;
  if (s.bad() || s.fail() || !s.eof())
    throw Err() << "Not a timestamp: " << ts;
  return pack_time(t);
}
// Unpack timestamp
uint64_t
DBinfo::unpack_time(const string & s) const{
  if (s.size()!=sizeof(uint64_t))
    throw Err() << "Broken database: wrong timestamp size";
  return *(uint64_t *)s.data();
}

// Pack data according with data format
// string is used as a convenient data storage, which
// can be easily converted into Berkleydb data.
// Output string is not a c-string!
string
DBinfo::pack_data(const vector<string> & strs) const{
  if (strs.size() < 1) throw Err() << "Some data expected";
  string ret;
  if (val == DATA_TEXT){ // text: join all data
    ret = strs[0];
    for (int i=1; i<strs.size(); i++)
      ret+= " " + strs[i];
  }
  else {    // numbers
    ret = string(dsize()*strs.size(), '\0');
    for (int i=0; i<strs.size(); i++){
      istringstream s(strs[i]);
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
string
DBinfo::unpack_data(const string & s, const int col) const{
  if (val == DATA_TEXT){
    // remove linebreaks
    int i;
    string ret=s;
    while ((i=ret.find('\n'))!=string::npos) ret[i] = ' ';
    return ret;
  }
  if (s.size() % dsize() != 0)
    throw Err() << "Broken database: wrong data length";
  // number of columns
  size_t cn = s.size()/dsize();
  // column range we want to show:
  size_t c1=0, c2=cn;
  if (col!=-1) { c1=col; c2=col+1; }

  ostringstream ostr;
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

// Unpack data to a double value (for json output)
// only one column are returned, 0 by default
double
DBinfo::unpack_data_d(const string & s, const int col) const{
  if (val == DATA_TEXT)
    throw Err() << "Can't convert text data to a number";
  if (s.size() % dsize() != 0)
    throw Err() << "Broken database: wrong data length";
  // number of columns
  size_t cn = s.size()/dsize();
  // column  we want to show:
  size_t c = col<0? 0:col;
  if (c>=cn) { return 0.0; }

  switch (val){
    case DATA_INT8:   return (double)((int8_t   *)s.data())[c];
    case DATA_UINT8:  return (double)((uint8_t  *)s.data())[c];
    case DATA_INT16:  return (double)((int16_t  *)s.data())[c];
    case DATA_UINT16: return (double)((uint16_t *)s.data())[c];
    case DATA_INT32:  return (double)((int32_t  *)s.data())[c];
    case DATA_UINT32: return (double)((uint32_t *)s.data())[c];
    case DATA_INT64:  return (double)((int64_t  *)s.data())[c];
    case DATA_UINT64: return (double)((uint64_t *)s.data())[c];
    case DATA_FLOAT:  return (double)((float    *)s.data())[c];
    case DATA_DOUBLE: return (double)((double   *)s.data())[c];
    default: throw Err() << "Unexpected data format";
  }
}

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
  string ks((char *)k->data, (char *)k->data+k->size);
  string vs((char *)v->data, (char *)v->data+v->size);
  // unpack and print values
  cout << info.unpack_time(ks) << " "
            << info.unpack_data(vs, col) << "\n";
}

/***********************************************************/
// Normilize db name. Remove starting /,
// remove . and .. path components.
//
string norm_name(const string & name){
  string ret = "";
  int p1=0, p2=0;
  while ((p2 = name.find('/', p1))!=string::npos){
    string v=name.substr(p1,p2-p1);
    if (v != ".." && v != "." && v != "")
      ret += v + "/";
    p1=p2+1;
  }
  return ret + name.substr(p1, name.length()-p1);
}

/***********************************************************/
// DBsts class

/************************************/
// Constructor -- open a database
//
DBsts::DBsts(const string & path_,
     const string & name_,
     const int flags){

  refcounter   = new int;
  *refcounter  = 1;
  info_is_actual = false;

  name = name_;
  open_flags = flags;
  string fname = path_ + "/" + name_ + ".db";

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
void
DBsts::write_info(const DBinfo &info){
  // remove the info entry if it exists
  uint8_t x=0;
  DBT k = mk_dbt(&x);
  int ret = dbp->del(dbp, NULL, &k, 0);
  if (ret != 0 && ret != DB_NOTFOUND)
    throw Err() << name << ".db: " << db_strerror(ret);
  // write new data
  string vs = string(1, (char)info.val)
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
DBinfo
DBsts::read_info(){
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
  db_info.descr = string((char*)v.data+1, (char*)v.data+v.size);
  info_is_actual = true;
  return db_info;
}

/************************************/
// Put data to the database
// input: timestamp + vector of strings
// The function can be run multiple times without reopening
// the database.
//
void
DBsts::put(const uint64_t t,
           const vector<string> & dat){
  DBinfo info = read_info();
  string ks = info.pack_time(t);
  string vs = info.pack_data(dat);
  DBT k = mk_dbt(ks);
  DBT v = mk_dbt(vs);
  int res = dbp->put(dbp, NULL, &k, &v, 0);
  if (res != 0) throw Err() << name << ".db: " << db_strerror(res);
}

/************************************/
// interpolate data and pack it into DBT string as double array
//
string
DBsts::print_interp(const uint64_t t0,
                    const string & k1, const string & k2,
                    const string & v1, const string & v2,
                    const int col){
  DBinfo info = read_info();
  // check for correct key size (do not parse DB info)
  if (k1.size()!=sizeof(uint64_t)) return "";
  if (k2.size()!=sizeof(uint64_t)) return "";
  // unpack time
  uint64_t t1 = info.unpack_time(k1);
  uint64_t t2 = info.unpack_time(k2);
  // unpack data
  istringstream strv1(info.unpack_data(v1, col));
  istringstream strv2(info.unpack_data(v2, col));
  // calculate first point weight
  uint64_t dt1 = max(t1,t0)-min(t1,t0);
  uint64_t dt2 = max(t2,t0)-min(t2,t0);
  double k = (double)dt2/(dt1+dt2);

  // collect interpolated values
  vector<double> values;
  while (!strv1.eof() && !strv2.eof()){
    double dv1, dv2;
    strv1 >> dv1;
    strv2 >> dv2;
    values.push_back(dv1*k + dv2*(1-k));
  }
  // pack data in a string storage
  return string((char*)values.data(),
     (char*)values.data()+sizeof(double)*values.size());
}

/************************************/
// get data from the database -- get_next
//
void
DBsts::get_next(const uint64_t t1, const int col,
                process_data_func proc_data){
  DBinfo info = read_info();
  /* Get a cursor */
  DBC *curs;
  dbp->cursor(dbp, NULL, &curs, 0);
  if (curs==NULL) throw Err() << name << ".db: can't get a cursor";

  string ks = info.pack_time(t1);
  DBT k = mk_dbt(ks);
  DBT v = mk_dbt();
  int res = curs->c_get(curs, &k, &v, DB_SET_RANGE);
  if (res==DB_NOTFOUND) { curs->close(curs); return; }
  if (res!=0) throw Err() << name << ".db: " << db_strerror(res);
  proc_data(&k, &v, col, info);
  curs->close(curs);
}

/************************************/
// get data from the database -- get_prev
//
void
DBsts::get_prev(const uint64_t t2, const int col,
                process_data_func proc_data){
  DBinfo info = read_info();
  /* Get a cursor */
  DBC *curs;
  dbp->cursor(dbp, NULL, &curs, 0);
  if (curs==NULL) throw Err() << name << ".db: can't get a cursor";
  string ks = info.pack_time(t2);
  DBT k = mk_dbt(ks);
  DBT v = mk_dbt();

  int res = curs->c_get(curs, &k, &v, DB_SET_RANGE);
  if (res!=0 && res!=DB_NOTFOUND)
    throw Err() << name << ".db: " << db_strerror(res);

  // unpack time
  string s((char *)k.data, (char *)k.data+k.size);
  uint64_t tn = info.unpack_time(s);

  // if needed, get previous record:
  if (tn > t2 || res==DB_NOTFOUND){
    res = curs->c_get(curs, &k, &v, DB_PREV);
    if (res==DB_NOTFOUND) { curs->close(curs); return; }
    if (res!=0) throw Err() << name << ".db: " << db_strerror(res);
  }

  proc_data(&k, &v, col, info);
  curs->close(curs);
}

/************************************/
// get data from the database -- get_interp
//
void
DBsts::get_interp(const uint64_t t, const int col,
                  process_data_func proc_data){
  DBinfo info = read_info();
  if (info.val==DATA_TEXT)
    throw Err() << "Can not do interpolation of TEXT data";
  /* Get a cursor */
  DBC *curs;
  dbp->cursor(dbp, NULL, &curs, 0);
  if (curs==NULL) throw Err() << name << ".db: can't get a cursor";
  string ks = info.pack_time(t);
  DBT k = mk_dbt(ks);
  DBT v = mk_dbt();

  int res = curs->c_get(curs, &k, &v, DB_SET_RANGE);
  if (res==DB_NOTFOUND) { curs->close(curs); return; }
  if (res!=0) throw Err() << name << ".db: " << db_strerror(res);

  string ks1((char *)k.data, (char *)k.data+k.size);
  string vs1((char *)v.data, (char *)v.data+v.size);
  if (info.unpack_time(ks1) == t){
    proc_data(&k, &v, col, info);
  }
  else {
    res = curs->c_get(curs, &k, &v, DB_PREV);
    if (res==DB_NOTFOUND) { curs->close(curs); return; }
    if (res!=0) throw Err() << name << ".db: " << db_strerror(res);
    string ks2((char *)k.data, (char *)k.data+k.size);
    string vs2((char *)v.data, (char *)v.data+v.size);
    string ks0 = info.pack_time(t);
    string vs0 = print_interp(t, ks1, ks2, vs1, vs2, col);
    if (vs0!=""){
      DBT k0 = mk_dbt(ks0);
      DBT v0 = mk_dbt(vs0);
      // print_interp converted everything to double and chose correct columns
      proc_data(&k0, &v0, -1, DBinfo(DATA_DOUBLE));
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
void
DBsts::get_range(const uint64_t t1, const uint64_t t2,
                 const uint64_t dt, const int col,
                 process_data_func proc_data){

  DBinfo info = read_info();
  /* Get a cursor */
  DBC *curs;
  dbp->cursor(dbp, NULL, &curs, 0);
  if (curs==NULL) throw Err() << name << ".db: can't get a cursor";

  string ks = info.pack_time(t1);
  DBT k = mk_dbt(ks);
  DBT v = mk_dbt();
  uint64_t tl = -1; // last printed value

  int fl = DB_SET_RANGE; // first get t >= t1
  while (1){
    int res = curs->c_get(curs, &k, &v, fl);
    if (res==DB_NOTFOUND) break;
    if (res!=0) throw Err() << name << ".db: " << db_strerror(res);

    // unpack new time value and check the range
    string s((char *)k.data, (char *)k.data+k.size);
    uint64_t tn = info.unpack_time(s);
    if (tn > t2 ) break;

    // if we want every point, switch to DB_NEXT and repeat
    if (dt<=1){
      proc_data(&k, &v, col, info);
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
      string s((char *)k.data, (char *)k.data+k.size);
      tn = info.unpack_time(s);
      if (tn > t2 ) break;
    }
    proc_data(&k, &v, col, info);
    tl=tn; // update last printed value

    // add dt to the key for the next loop:
    string sp = info.pack_time(tl+dt);
    memcpy(k.data,sp.data(),k.size);
  }
  curs->close(curs);
}

