#include <cstdlib>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <cstring> /* memset */
#include "db.h"

using namespace std;

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


  check_name(name_); // check the name
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
// get data from the database -- get_next
//
void
DBsts::get_next(const uint64_t t1,
                process_data_func proc_func, void *usr_data){
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
  proc_func(&k, &v, info, usr_data);
  curs->close(curs);
}

/************************************/
// get data from the database -- get_prev
//
void
DBsts::get_prev(const uint64_t t2,
                process_data_func proc_func, void *usr_data){
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

  proc_func(&k, &v, info, usr_data);
  curs->close(curs);
}

/************************************/
// get data from the database -- get_interp
//
void
DBsts::get(const uint64_t t,
           process_data_func proc_func, void *usr_data){
  DBinfo info = read_info();

  if (info.val!=DATA_FLOAT && info.val!=DATA_DOUBLE)
    return get_prev(t, proc_func, usr_data);

  /* Get a cursor */
  DBC *curs;
  dbp->cursor(dbp, NULL, &curs, 0);
  if (curs==NULL) throw Err() << name << ".db: can't get a cursor";
  string ks = info.pack_time(t);
  DBT k = mk_dbt(ks);
  DBT v = mk_dbt();

  // find next value
  int res = curs->c_get(curs, &k, &v, DB_SET_RANGE);

  // if there is no next value - give the last value if any
  if (res==DB_NOTFOUND) {
    res = curs->c_get(curs, &k, &v, DB_PREV);
    if (res==0) proc_func(&k, &v, info, usr_data);
    if (res!=0 && res!=DB_NOTFOUND)
      throw Err() << name << ".db: " << db_strerror(res);
    curs->close(curs); return;
  }
  if (res!=0) throw Err() << name << ".db: " << db_strerror(res);

  // if "next" record is asactly at t - return it
  string ks1((char *)k.data, (char *)k.data+k.size);
  string vs1((char *)v.data, (char *)v.data+v.size);
  if (info.unpack_time(ks1) == t) proc_func(&k, &v, info, usr_data);

  // get the previous value and do interpolation
  else {
    // find prev value
    res = curs->c_get(curs, &k, &v, DB_PREV);
    if (res==DB_NOTFOUND) { curs->close(curs); return; }
    if (res!=0) throw Err() << name << ".db: " << db_strerror(res);
    string ks2((char *)k.data, (char *)k.data+k.size);
    string vs2((char *)v.data, (char *)v.data+v.size);
    string ks0 = info.pack_time(t);
    string vs0 = info.interpolate(t, ks1, ks2, vs1, vs2);
    if (vs0!=""){
      DBT k0 = mk_dbt(ks0);
      DBT v0 = mk_dbt(vs0);
      // print_interp converted everything to double and chose correct columns
      proc_func(&k0, &v0, info, usr_data);
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
                 const uint64_t dt,
                 process_data_func proc_func, void *usr_data){

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
      proc_func(&k, &v, info, usr_data);
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
    proc_func(&k, &v, info, usr_data);
    tl=tn; // update last printed value

    // add dt to the key for the next loop:
    string sp = info.pack_time(tl+dt);
    memcpy(k.data,sp.data(),k.size);
  }
  curs->close(curs);
}

/************************************/
// delete data data from the database -- del_range
void
DBsts::del(const uint64_t t1){
  DBinfo info = read_info();
  string ks = info.pack_time(t1);
  DBT k = mk_dbt(ks);
  int res = dbp->del(dbp, NULL, &k, 0);
  if (res!=0) throw Err() << name << ".db: " << db_strerror(res);
}

/************************************/
// delete data data from the database -- del_range
void
DBsts::del_range(const uint64_t t1, const uint64_t t2){
  DBinfo info = read_info();
  /* Get a cursor */
  DBC *curs;
  dbp->cursor(dbp, NULL, &curs, 0);
  if (curs==NULL) throw Err() << name << ".db: can't get a cursor";

  string ks = info.pack_time(t1);
  DBT k = mk_dbt(ks);
  DBT v = mk_dbt();

  int fl = DB_SET_RANGE; // first get t >= t1
  while (1){
    int res = curs->c_get(curs, &k, &v, fl);
    if (res==DB_NOTFOUND) break;
    if (res!=0) throw Err() << name << ".db: " << db_strerror(res);

    // unpack new time value and check the range
    string s((char *)k.data, (char *)k.data+k.size);
    uint64_t tn = info.unpack_time(s);
    if (tn > t2 ) break;

    // delete the point
    res = curs->del(curs, 0);
    if (res!=0) throw Err() << name << ".db: " << db_strerror(res);

    // we want to delete every point, so switch to DB_NEXT and repeat
    fl=DB_NEXT;
  }
  curs->close(curs);
}
