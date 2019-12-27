#include <cstdlib>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <cstring> /* memset */
#include "dbgr.h"

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
  if (a->size == sizeof(uint64_t) && b->size == sizeof(uint32_t)){
    v1 = *(uint64_t*)a->data;
    v2 = *(uint32_t*)b->data;
    v2 = v2 << 32; }
  else
  if (a->size == sizeof(uint32_t) && b->size == sizeof(uint64_t)){
    v1 = *(uint32_t*)a->data;
    v2 = *(uint64_t*)b->data;
    v1 = v1 << 32; }
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
// DBgr class

/************************************/
// Constructor -- open a database
//
DBgr::DBgr(DB_ENV *env_,
     const string & path_,
     const string & name_,
     const int flags): env(env_), name(name_){

  info_is_actual = false;

  check_name(name); // check the name

  // get environment flags
  env_flags = 0;
  if (env) {
    int ret = env->get_open_flags(env, &env_flags);
    if (ret != 0)
      throw Err() << name << ".db: " << db_strerror(ret);
  }

  // set flags
  open_flags = flags;
  if (env_flags & DB_INIT_TXN)
   open_flags |= DB_AUTO_COMMIT | DB_READ_UNCOMMITTED;

  string fname = name_ + ".db";
  if (!env) { fname = path_ + "/" + fname; }

  /* Initialize the DB handle */
  DB *dbp1;
  int ret = db_create(&dbp1, env, 0);
  if (ret != 0)
    throw Err() << name << ".db: " << db_strerror(ret);
  dbp = std::shared_ptr<DB>(dbp1, DBgr::D());


  /* set key compare function */
  ret = dbp->set_bt_compare(dbp.get(), cmpfunc);
  if (ret != 0)
    throw Err() << name << ".db: " << db_strerror(ret);

  /* Open the database */
  ret = dbp->open(dbp.get(),     /* Pointer to the database */
                  NULL,          /* Txn pointer */
                  fname.c_str(), /* file */
                  NULL,          /* database */
                  DB_BTREE,      /* Database type (using btree) */
                  open_flags,    /* Open flags */
                  0644);         /* File mode*/
  if (ret != 0){
    throw Err() << name << ".db: " << db_strerror(ret);
  }
}

/************************************/
// Simple transaction wrappers:
// For simple environments env can be NULL, txn can be null.
DB_TXN *
DBgr::txn_begin(int flags){
  DB_TXN *txn = NULL;
  if (env && (env_flags & DB_INIT_TXN)) {
    int ret = env->txn_begin(env, NULL, &txn, flags);
    if (ret != 0) Err() << "Can't create a transaction: " << name << ".db: " << db_strerror(ret);
  }
  return txn;
}

void
DBgr::txn_commit(DB_TXN *txn){
  if (!txn) return;
  int ret = txn->commit(txn, 0);
  if (ret != 0) Err() << "Can't commit a transaction: " << name << ".db: " << db_strerror(ret);
}

void
DBgr::txn_abort(DB_TXN *txn){
  if (!txn) return;
  int ret = txn->abort(txn);
  if (ret != 0) Err() << "Can't abort a transaction: " << name << ".db: " << db_strerror(ret);
}

/************************************/
// Simple wrappers for cursor actions:

/* Get a cursor */
void
DBgr::get_cursor(DB *dbp, DB_TXN *txn, DBC **curs, int flags) {
  *curs=NULL;
  dbp->cursor(dbp, txn, curs, flags);
  if (*curs==NULL)
    throw Err() << name << ".db: can't get a cursor";
}

bool
DBgr::c_get(DBC *curs, DBT *k, DBT *v, int flags) {
  int res = curs->c_get(curs, k, v, flags);
  if (res!=0 && res!=DB_NOTFOUND)
    throw Err() << name << ".db: " << db_strerror(res);
  return res==0;
}



/************************************/
// Write database information.
// key = (uint8_t)0 (1byte), value = data_fmt (1byte) + description
// key = (uint8_t)1 (1byte), value = version  (1byte)
//
void
DBgr::write_info(const DBinfo &info){
  int ret;

  // do everything in a single transaction
  DB_TXN *txn = txn_begin();
  try {

    // Write format + description.
    // Remove the entry if it exists:
    uint8_t x=KEY_DESCR;
    DBT k = mk_dbt(&x);
    ret = dbp->del(dbp.get(), txn, &k, 0);
    if (ret != 0 && ret != DB_NOTFOUND)
      throw Err() << name << ".db: " << db_strerror(ret);

    // Write new data:
    string vs = string(1, (char)info.val)
                   + info.descr;
    DBT v = mk_dbt(vs);
    ret = dbp->put(dbp.get(), txn, &k, &v, 0);
    if (ret != 0)
      throw Err() << name << ".db: " << db_strerror(ret);

    // Write version.
    // Remove the entry if it exists.
    x=KEY_VERSION;
    k = mk_dbt(&x);
    ret = dbp->del(dbp.get(), txn, &k, 0);
    if (ret != 0 && ret != DB_NOTFOUND)
      throw Err() << name << ".db: " << db_strerror(ret);

    // write new data
    v = mk_dbt(&info.version);
    ret = dbp->put(dbp.get(), txn, &k, &v, 0);
    if (ret != 0)
      throw Err() << name << ".db: " << db_strerror(ret);
  }
  catch (Err e){
    txn_abort(txn);
    throw e;
  }

  txn_commit(txn);
  db_info = info;
  info_is_actual = true;
}

/************************************/
// Get database information
//
DBinfo
DBgr::read_info(){
  if (info_is_actual) return db_info;

  // do everything in a single transaction (with snapshot isolation)
  DB_TXN *txn = txn_begin(DB_TXN_SNAPSHOT);
  try {
    // Read format + description.
    uint8_t x=KEY_DESCR;
    DBT k = mk_dbt(&x);
    DBT v = mk_dbt();
    int ret = dbp->get(dbp.get(), txn, &k, &v, 0);
    if (ret != 0)
     throw Err() << name << ".db: " << db_strerror(ret);
    uint8_t dfmt = *((uint8_t*)v.data);
    if (dfmt<0 || dfmt > LAST_DATAFMT)
      throw Err() << name << ".db: broken database, bad data format in the header";
    db_info.val = static_cast<DataFMT>(dfmt);
    db_info.descr = string((char*)v.data+1, (char*)v.data+v.size);

    // Read version
    x=KEY_VERSION;
    k = mk_dbt(&x);
    v = mk_dbt();
    ret = dbp->get(dbp.get(), txn, &k, &v, 0);
    if (ret == DB_NOTFOUND)
      db_info.version = 1; // first version can be without key=1
    else if (ret != 0)
      throw Err() << name << ".db: " << db_strerror(ret);
    else
      db_info.version = *((uint8_t*)v.data);
  }
  catch (Err e){
    txn_abort(txn);
    throw e;
  }
  txn_commit(txn);
  info_is_actual = true;
  return db_info;
}

/************************************/
std::string
DBgr::backup_start(){
  std::string timer;
  DBinfo info = read_info();

  DB_TXN *txn = txn_begin();
  DBC *curs = NULL;
  try {
    int ret;
    // Reset temporary backup timer:
    uint8_t x=KEY_BACKUP_TMP;
    DBT k = mk_dbt(&x);
    ret = dbp->del(dbp.get(), txn, &k, 0);
    if (ret != 0 && ret != DB_NOTFOUND)
      throw Err() << name << ".db: " << db_strerror(ret);

    // Return main backup timer value:
    x=KEY_BACKUP_MAIN;
    k = mk_dbt(&x);
    DBT v = mk_dbt();
    ret = dbp->get(dbp.get(), txn, &k, &v, 0);
    if (ret != 0 && ret != DB_NOTFOUND)
     throw Err() << name << ".db: " << db_strerror(ret);

    timer = (ret == DB_NOTFOUND)?
      info.parse_time("inf"):
      string((char *)v.data, (char *)v.data+v.size);
  }
  catch (Err e){
    txn_abort(txn);
    throw e;
  }
  txn_commit(txn);
  return info.print_time(timer);;
}

void
DBgr::backup_end(){

  // do everything in a single transaction
  DB_TXN *txn = txn_begin();
  try {
    int ret;

    // Read temporary backup timer
    uint8_t x = KEY_BACKUP_TMP;
    DBT k = mk_dbt(&x);
    DBT v = mk_dbt();
    ret = dbp->get(dbp.get(), txn, &k, &v, 0);
    if (ret != 0 && ret != DB_NOTFOUND)
     throw Err() << name << ".db: " << db_strerror(ret);

    // Commit the temporary timer to the main one
    x=KEY_BACKUP_MAIN;
    k = mk_dbt(&x);
    if (ret == DB_NOTFOUND)
      ret = dbp->del(dbp.get(), txn, &k, 0);
    else
      ret = dbp->put(dbp.get(), txn, &k, &v, 0);

    if (ret != 0 && ret != DB_NOTFOUND)
      throw Err() << name << ".db: " << db_strerror(ret);
  }
  catch (Err e){
    txn_abort(txn);
    throw e;
  }
  txn_commit(txn);
}

void
DBgr::backup_upd(const std::string &t){
  DBinfo info = read_info();

  DB_TXN *txn = txn_begin();
  try {
    // Read and update both main and temporary timers
    for (int i = 0; i<2; i++) {
      int ret=0;
      uint8_t x = (i==0)? KEY_BACKUP_TMP : KEY_BACKUP_MAIN;
      DBT k = mk_dbt(&x);
      DBT v = mk_dbt();
      ret = dbp->get(dbp.get(), txn, &k, &v, 0);
      if (ret != 0 && ret != DB_NOTFOUND)
        throw Err() << name << ".db: " << db_strerror(ret);

      if (ret == DB_NOTFOUND) {
        v = mk_dbt(t);
        ret = dbp->put(dbp.get(), txn, &k, &v, 0);
      }
      else {
        std::string timer = string((char *)v.data, (char *)v.data+v.size);
        if (info.cmp_time(timer,t)>0){
          v = mk_dbt(t);
          ret = dbp->put(dbp.get(), txn, &k, &v, 0);
        }
      }
      if (ret != 0)
        throw Err() << name << ".db: " << db_strerror(ret);
    }

  }
  catch (Err e){
    txn_abort(txn);
    throw e;
  }
  txn_commit(txn);
}

/************************************/
// Put data to the database
// input: timestamp + vector of strings
// The function can be run multiple times without reopening
// the database.
//
void
DBgr::put(const string &t, const vector<string> & dat, const string &dpolicy){
  int ret;
  DBinfo info = read_info();
  string ks = info.parse_time(t);
  string vs = info.parse_data(dat);

  // do everything in a single transaction
  DB_TXN *txn = txn_begin();
  try {

    int flags = (dpolicy =="replace")? 0:DB_NOOVERWRITE;
    int res = -1;
    while (res!=0){
      DBT k = mk_dbt(ks);
      DBT v = mk_dbt(vs);
      res = dbp->put(dbp.get(), txn, &k, &v, flags);
      if (res == DB_KEYEXIST){
        if (dpolicy =="error") throw Err() << name << ".db: " << "Timestamp exists";
        else if (dpolicy =="sshift")  ks = info.add_time(ks, info.parse_time("1"));
        else if (dpolicy =="nsshift") ks = info.add_time(ks, info.parse_time("0.000000001"));
        else if (dpolicy =="skip") break;
        else throw Err() << "Unknown dpolicy setting: " << dpolicy;
      }
      else if (res != 0)
        throw Err() << name << ".db: " << db_strerror(res);
    }
  }
  catch (Err e){
    txn_abort(txn);
    throw e;
  }
  txn_commit(txn);
  backup_upd(ks);
}

/************************************/
// get data from the database -- get_next
//
void
DBgr::get_next(const string &t1, DBout & dbo){
  DBinfo info = read_info();
  string t1p = info.parse_time(t1);
  DBT k = mk_dbt(t1p);
  DBT v = mk_dbt();

  // do everything in a single transaction (with snapshot isolation)
  DB_TXN *txn = txn_begin(DB_TXN_SNAPSHOT);
  DBC *curs = NULL;
  try {
    /* Get a cursor */
    get_cursor(dbp.get(), txn, &curs, 0);

    if (c_get(curs, &k, &v, DB_SET_RANGE))
      dbo.proc_point(&k, &v, info);

    curs->close(curs);
  }
  catch (Err e){
    if (curs) curs->close(curs);
    txn_abort(txn);
    throw e;
  }
  txn_commit(txn);


}

/************************************/
// get data from the database -- get_prev
//
void
DBgr::get_prev(const string &t2, DBout & dbo){
  DBinfo info = read_info();

  string t2p = info.parse_time(t2);
  DBT k = mk_dbt(t2p);
  DBT v = mk_dbt();

  // do everything in a single transaction (with snapshot isolation)
  DB_TXN *txn = txn_begin(DB_TXN_SNAPSHOT);
  DBC *curs = NULL;
  try {
    /* Get a cursor */
    get_cursor(dbp.get(), txn, &curs, 0);

    bool found = c_get(curs, &k, &v, DB_SET_RANGE);

    // unpack time
    string tp((char *)k.data, (char *)k.data+k.size);

    // if needed, get previous record:
    if (info.cmp_time(tp,t2p)>0 || !found)
      found=c_get(curs, &k, &v, DB_PREV);

    if (found) dbo.proc_point(&k, &v, info);

    curs->close(curs);
  }
  catch (Err e){
    if (curs) curs->close(curs);
    txn_abort(txn);
    throw e;
  }
  txn_commit(txn);
}

/************************************/
// get data from the database -- get_interp
//
void
DBgr::get(const string &t, DBout & dbo){
  DBinfo info = read_info();

  /* for non-float databases use get_prev */
  if (info.val!=DATA_FLOAT && info.val!=DATA_DOUBLE)
    return get_prev(t, dbo);

  string tp = info.parse_time(t);
  DBT k = mk_dbt(tp);
  DBT v = mk_dbt();
  string t1p, v1p, t2p, v2p, vp;

  // do everything in a single transaction (with snapshot isolation)
  DB_TXN *txn = txn_begin(DB_TXN_SNAPSHOT);
  DBC *curs = NULL;
  try {

    /* Get a cursor */
    get_cursor(dbp.get(), txn, &curs, 0);

    // find next value
    bool found = c_get(curs, &k, &v, DB_SET_RANGE);

    // if there is no next value - give the last value if any
    if (!found) {
      if (c_get(curs, &k, &v, DB_PREV))
        dbo.proc_point(&k, &v, info);
      goto finish;
    }

    // if "next" record is exactly at t - return it
    t1p = string((char *)k.data, (char *)k.data+k.size);
    v1p = string((char *)v.data, (char *)v.data+v.size);
    if (info.cmp_time(t1p,tp) == 0){
      dbo.proc_point(&k, &v, info);
      goto finish;
    }

    // get the previous value and do interpolation
    // find prev value
    found = c_get(curs, &k, &v, DB_PREV);
    if (!found) goto finish;

    t2p = string((char *)k.data, (char *)k.data+k.size);
    v2p = string((char *)v.data, (char *)v.data+v.size);
    vp = info.interpolate(tp, t1p, t2p, v1p, v2p);
    if (vp!=""){
      DBT k0 = mk_dbt(tp);
      DBT v0 = mk_dbt(vp);
      dbo.proc_point(&k0, &v0, info);
    }

    finish:
    curs->close(curs);
  }
  catch (Err e){
    if (curs) curs->close(curs);
    txn_abort(txn);
    throw e;
  }
  txn_commit(txn);
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
DBgr::get_range(const string &t1, const string &t2,
                const string &dt, DBout & dbo){

  DBinfo info = read_info();

  string t1p = info.parse_time(t1);
  string t2p = info.parse_time(t2);
  string dtp = info.parse_time(dt);
  DBT k = mk_dbt(t1p);
  DBT v = mk_dbt();
  string tlp; // last printed value

  // do everything in a single transaction (with snapshot isolation)
  DB_TXN *txn = txn_begin(DB_TXN_SNAPSHOT);
  DBC *curs = NULL;
  try {

    // Get a cursor
    get_cursor(dbp.get(), txn, &curs, 0);

    int fl = DB_SET_RANGE; // first get t >= t1
    while (1){

      string pre((char *)k.data, (char *)k.data+k.size);

      if (!c_get(curs, &k, &v, fl)) break;

      // unpack new time value and check the range
      string tnp((char *)k.data, (char *)k.data+k.size);
      if (info.cmp_time(tnp,t2p)>0) break;

      // I have a broken database where DB_SET_RANGE/DB_NEXT can
      // get non-increasing values. Let's check this to prevent the
      // program from infinite loops..
      if (info.cmp_time(tnp,pre)<0)
        throw Err() << "Broken database (DB_SET_RANGE/DB_NEXT get smaller timestamp)";

      // if we want every point, switch to DB_NEXT and repeat
      if (info.is_zero_time(dtp)){
        dbo.proc_point(&k, &v, info, 1);
        fl=DB_NEXT;
        continue;
      }

      // If dt >0 we continue using fl=DB_SET_RANGE.
      // If new value the same as old
      if (tlp.size()>0 && info.cmp_time(tlp,tnp)==0){
        // get next value
        if (!c_get(curs, &k, &v, DB_NEXT)) break;
        // unpack new time value and check the range
        tnp = string((char *)k.data, (char *)k.data+k.size);
        if (info.cmp_time(tnp,t2p) > 0 ) break;
      }
      dbo.proc_point(&k, &v, info, 1);
      tlp=tnp; // update last printed value

      // add dt to the key for the next loop:
      string sp = info.add_time(tlp, dtp);
      memcpy(k.data,sp.data(),k.size);
      k = mk_dbt(sp);
    }
    curs->close(curs);
  }
  catch (Err e){
    if (curs) curs->close(curs);
    txn_abort(txn);
    throw e;
  }
  txn_commit(txn);

}

/************************************/
// delete data data from the database -- del
void
DBgr::del(const string &t1){
  int ret;
  DBinfo info = read_info();
  string t1p = info.parse_time(t1);
  DBT k = mk_dbt(t1p);

  DB_TXN *txn = txn_begin();
  // delete data
  ret = dbp->del(dbp.get(), txn, &k, 0);
  if (ret!=0){
    txn_abort(txn);
    if (ret == DB_NOTFOUND)
      throw Err() << name << ".db: No such record: " << t1;
    else
      throw Err() << name << ".db: " << db_strerror(ret);
  }
  txn_commit(txn);
  backup_upd(t1p);
}

/************************************/
// delete data data from the database -- del_range
void
DBgr::del_range(const string &t1, const string &t2){
  int ret;
  DBinfo info = read_info();
  std::string first_del; // for lastmod timestamp

  string t1p = info.parse_time(t1);
  string t2p = info.parse_time(t2);
  DBT k = mk_dbt(t1p);
  DBT v = mk_dbt();

  DB_TXN *txn = txn_begin();
  DBC *curs = NULL;
  try {

    /* Get a cursor */
    get_cursor(dbp.get(), txn, &curs, 0);

    int fl = DB_SET_RANGE; // first get t >= t1
    while (1){

      if (!c_get(curs, &k, &v, fl)) break;

      // get packed time value and check the range
      string tp((char *)k.data, (char *)k.data+k.size);
      if (info.cmp_time(tp,t2p)>0) break;

      // delete the point
      int res = curs->del(curs, 0);
      if (res!=0)
        throw Err() << name << ".db: " << db_strerror(res);
      if (first_del=="") first_del = tp;

      // we want to delete every point, so switch to DB_NEXT and repeat
      fl=DB_NEXT;
    }

    curs->close(curs);
  }
  catch (Err e){
    if (curs) curs->close(curs);
    txn_abort(txn);
    throw e;
  }
  txn_commit(txn);
  if (first_del!="") backup_upd(first_del);
}

/************************************/
// functions for DBgr::load method
uint8_t DIG(const char c){
  if (c>='0' && c<='9') return c-'0';
  if (c=='a' || c=='A') return 10;
  if (c=='b' || c=='B') return 11;
  if (c=='c' || c=='C') return 12;
  if (c=='d' || c=='D') return 13;
  if (c=='e' || c=='E') return 14;
  if (c=='f' || c=='F') return 15;
  throw Err() << "bad data formatting";
}
// convert hex string to binary data (for load command)
std::string
strconv(const std::string &s){
  std::string ret;
  size_t len = s.length();
  for (int i=0;i<len; i++){
    if (s[i]==' ') continue;
    if (i+1>=len) throw Err() << "bad data formatting";
    ret.push_back((DIG(s[i])<<4) + DIG(s[++i]));
  }
  return ret;
}

/************************************/
// load file in a db_dump format
// (we can not use db_load because of user-defined comparison function)
// Note:
// - db_dump header is ignored now. We always assume
//   format=bytevalue
//   type=btree
// - load command is used without BerkleyDB environment!
//   (see how it is called in graphene.cpp)
void
DBgr::load(const std::string &file){
  ifstream ff(file.c_str());
  // skip header
  while (1){
    string s;
    getline(ff, s);
    if (ff.eof()) throw Err() << "Unexpected EOF while reading header";
    if (s == "HEADER=END") break;
  }

  // read data
  while (1){
    string ks,vs;
    getline(ff, ks);
    getline(ff, vs);
    if (ks == "DATA=END") break;
    if (vs == "DATA=END") throw Err() << "Unexpected end of data";
    if (ff.eof()) throw Err() << "Unexpected EOF while reading data";
    if (vs=="" || ks=="") throw Err() << "Error reading data";
    std::string kp = strconv(ks);
    std::string vp = strconv(vs);
    // write new data
    DBT k = mk_dbt(kp);
    DBT v = mk_dbt(vp);
    int ret = dbp->put(dbp.get(), NULL, &k, &v, 0);
    if (ret != 0)
      throw Err() << name << ".db: " << db_strerror(ret);
  }
}

/************************************/
// dump file in a db_dump format
// should be same as db_dump utility
// Note:
// - dump command is used without BerkleyDB environment
//   in readonly mode (see how it is called in graphene.cpp)
void
DBgr::dump(const std::string &file){
  ofstream ff(file.c_str());

  // write header
  ff << "VERSION=3\n"
     << "format=bytevalue\n"
     << "type=btree\n"
     << "db_pagesize=4096\n"
     << "HEADER=END\n";

  // write data
  DBT k = mk_dbt("\0"); // start from 1-byte 0
  DBT v = mk_dbt();
  DBC *curs = NULL;
  try {

    // Get a cursor
    get_cursor(dbp.get(), NULL, &curs, 0);

    int fl = DB_SET_RANGE;
    while (1){
      if (!c_get(curs, &k, &v, fl)) break;
      fl=DB_NEXT;

      ff << ' ';
      // print key and value as hex code
      for (string::size_type i = 0; i < k.size; ++i)
        ff << std::hex << std::setfill('0') << std::setw(2)
           << (int)((uint8_t*)k.data)[i];
      ff << "\n";

      ff << ' ';
      for (string::size_type i = 0; i < v.size; ++i)
        ff << std::hex << std::setfill('0') << std::setw(2)
           << (int)((uint8_t*)v.data)[i];
      ff << "\n";
    }
    curs->close(curs);
    ff << "DATA=END\n";
  }
  catch (Err e){
    if (curs) curs->close(curs);
    throw e;
  }
}
