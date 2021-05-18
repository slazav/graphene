#include <cstdlib>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <cstring> /* memset */

#include "data.h"
#include "gr_db.h"
#include "err/err.h"

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
// GrapheneDB class

/************************************/
// Constructor -- open a database
//
GrapheneDB::GrapheneDB(DB_ENV *env_,
     const string & path_,
     const string & name_,
     const int flags):
       env(env_), name(name_),
       ttype(DEF_TIMETYPE), dtype(DEF_DATATYPE), version(DEF_DBVERSION),
       filters(MAX_FILTERS, Filter()) {

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
  dbp = std::shared_ptr<DB>(dbp1, GrapheneDB::D());


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
  if ((flags & DB_CREATE) == 0) read_info();

}

/************************************/
// Simple transaction wrappers:
// For simple environments env can be NULL, txn can be null.
DB_TXN *
GrapheneDB::txn_begin(int flags){
  DB_TXN *txn = NULL;
  if (env && (env_flags & DB_INIT_TXN)) {
    int ret = env->txn_begin(env, NULL, &txn, flags);
    if (ret != 0) Err() << "Can't create a transaction: " << name << ".db: " << db_strerror(ret);
  }
  return txn;
}

void
GrapheneDB::txn_commit(DB_TXN *txn){
  if (!txn) return;
  int ret = txn->commit(txn, 0);
  if (ret != 0) Err() << "Can't commit a transaction: " << name << ".db: " << db_strerror(ret);
}

void
GrapheneDB::txn_abort(DB_TXN *txn){
  if (!txn) return;
  int ret = txn->abort(txn);
  if (ret != 0) Err() << "Can't abort a transaction: " << name << ".db: " << db_strerror(ret);
}

/************************************/
// Simple wrappers for cursor actions:

/* Get a cursor */
void
GrapheneDB::get_cursor(DB *dbp, DB_TXN *txn, DBC **curs, int flags) {
  *curs=NULL;
  dbp->cursor(dbp, txn, curs, flags);
  if (*curs==NULL)
    throw Err() << name << ".db: can't get a cursor";
}

bool
GrapheneDB::c_get(DBC *curs, DBT *k, DBT *v, int flags) {
  int res = curs->c_get(curs, k, v, flags);
  if (res!=0 && res!=DB_NOTFOUND)
    throw Err() << name << ".db: " << db_strerror(res);
  return res==0;
}

/************************************/
// Simple del/put/set operations for database information
void
GrapheneDB::del_key(DB_TXN *txn, uint8_t key){
  DBT k = mk_dbt(&key);
  int ret = dbp->del(dbp.get(), txn, &k, 0);
  if (ret != 0 && ret != DB_NOTFOUND)
    throw Err() << name << ".db: " << db_strerror(ret);
}

void
GrapheneDB::set_key(DB_TXN *txn, uint8_t key, DBT v){
  DBT k = mk_dbt(&key);
  int ret = dbp->put(dbp.get(), txn, &k, &v, 0);
  if (ret != 0)
    throw Err() << name << ".db: " << db_strerror(ret);
}

std::string
GrapheneDB::get_key(DB_TXN *txn, uint8_t key, const std::string & def){
  DBT k = mk_dbt(&key);
  DBT v = mk_dbt();
  int ret = dbp->get(dbp.get(), txn, &k, &v, 0);
  if (ret == 0) return dbt2str(&v);
  if (ret == DB_NOTFOUND) return def;
  throw Err() << name << ".db: " << db_strerror(ret);
}


/************************************/
// Write database information.
// key = (uint8_t)0 (1byte), value = data_fmt (1byte) + description
// key = (uint8_t)1 (1byte), value = version  (1byte)
//
void
GrapheneDB::write_info(){
  int ret;

  // do everything in a single transaction
  DB_TXN *txn = txn_begin();
  try {

    // Write format + description.
    set_key(txn, KEY_DESCR, mk_dbt(string(1, (char)dtype) + descr));

    // Write version.
    set_key(txn, KEY_VERSION, mk_dbt(&version));
  }
  catch (Err e){
    txn_abort(txn);
    throw e;
  }
  sync();
  txn_commit(txn);
}

/************************************/
// Get database information (version, ttype, dtype, filters)
//
void
GrapheneDB::read_info(){

  // do everything in a single transaction (with snapshot isolation)
  DB_TXN *txn = txn_begin(DB_TXN_SNAPSHOT);
  try {
    // Read format + description.
    auto str = get_key(txn, KEY_DESCR);
    if (str.size() < 1)
     throw Err() << name << ".db: broken database, data type is missing";

    dtype = static_cast<DataType>(*((uint8_t*)str.data()));
    graphene_dtype_name(dtype); // throw error if not valid

    descr = string(str.data()+1, str.data()+str.size());

    // Read version
    str = get_key(txn, KEY_VERSION);
    // first version can be without key=1
    version = (str.size()<1)? 1 : *((uint8_t*)str.data());

    switch (version){
      case 1: ttype=TIME_V1; break;
      case 2: ttype=TIME_V2; break;
      default: throw Err() << "unsupported database version: " << (int)version;
    }

    // Read filters
    for (int n=0; n<MAX_FILTERS; n++)
      filters[n].set_code( get_key(txn, KEY_FLT+n) );

  }
  catch (Err e){
    txn_abort(txn);
    throw e;
  }
  txn_commit(txn);
}

/************************************/
void
GrapheneDB::write_filter(const int n, const std::string & code){
  if (n<0 || n>filters.size())
    throw Err() << "filter number out of range: " << n;

  filters[n].set_code(code);
  int ret;
  DB_TXN *txn = txn_begin();
  try {
    // Remove filter 0 storage if it exists:
    if (n==0) del_key(txn, KEY_FLT0DATA);
    set_key(txn, KEY_FLT+n, mk_dbt(filters[n].code));
  }
  catch (Err e){
    txn_abort(txn);
    throw e;
  }
  sync();
  txn_commit(txn);
}

/************************************/
void
GrapheneDB::clear_f0data(){
  int ret;
  DB_TXN *txn = txn_begin(DB_TXN_SNAPSHOT);
  try { del_key(txn, KEY_FLT0DATA); }
  catch (Err e){
    txn_abort(txn);
    throw e;
  }
  sync();
  txn_commit(txn);
}

/************************************/
std::string
GrapheneDB::get_f0data(){

  DB_TXN *txn = txn_begin(DB_TXN_SNAPSHOT);
  std::string storage;
  try { storage = get_key(txn, KEY_FLT0DATA); }
  catch (Err e){
    txn_abort(txn);
    throw e;
  }
  txn_commit(txn);
  return storage;
}

/************************************/
void
GrapheneDB::write_f0data(const std::string & storage){
  DB_TXN *txn = txn_begin(DB_TXN_SNAPSHOT);
  try { set_key(txn, KEY_FLT0DATA, mk_dbt(storage)); }
  catch (Err e){
    txn_abort(txn);
    throw e;
  }
  sync();
  txn_commit(txn);
}


/************************************/

std::string
GrapheneDB::backup_start(){
  std::string ret;
  DB_TXN *txn = txn_begin();
  try {
    // reset temporary timer to inf
    auto t = graphene_time_parse("inf", ttype);
    set_key(txn, KEY_BACKUP_TMP, mk_dbt(t));
    // return main backup timer value:
    t = graphene_time_parse("0",ttype); // default
    t = get_key(txn, KEY_BACKUP_MAIN, t);
    ret = graphene_time_print(t, ttype);
  }
  catch (Err e){
    txn_abort(txn);
    throw e;
  }
  txn_commit(txn);
  return ret;
}

void
GrapheneDB::backup_end(const std::string & t2){
  std::string ret;
  DB_TXN *txn = txn_begin();
  try {
    // read temporary timer
    auto timer = graphene_time_parse("0",ttype); // default
    timer = get_key(txn, KEY_BACKUP_TMP, timer);

    // If t2 is smaller then timer, use t2 instead
    std::string t2s = graphene_time_parse(t2, ttype);
    if (graphene_time_cmp(timer,t2s, ttype)>0) timer = t2s;

    // Commit the temporary timer to the main one
    set_key(txn, KEY_BACKUP_MAIN, mk_dbt(timer));
  }
  catch (Err e){
    txn_abort(txn);
    throw e;
  }
  txn_commit(txn);
}

// reset backup timers to 0
void
GrapheneDB::backup_reset(){
  DB_TXN *txn = txn_begin();
  try {
    auto t = graphene_time_parse("0", ttype);
    set_key(txn, KEY_BACKUP_TMP,  mk_dbt(t));
    set_key(txn, KEY_BACKUP_MAIN, mk_dbt(t));
  }
  catch (Err e){
    txn_abort(txn);
    throw e;
  }
  txn_commit(txn);
}

// get value of the main backup timer
std::string
GrapheneDB::backup_get(){
  DB_TXN *txn = txn_begin();
  try {
    auto timer = graphene_time_parse("0",ttype); // default
    timer = get_key(txn, KEY_BACKUP_MAIN, timer);
    return graphene_time_print(timer, ttype);
  }
  catch (Err e){
    txn_abort(txn);
    throw e;
  }
  txn_commit(txn);
}

// function to be called after each database modification
void
GrapheneDB::backup_upd(DB_TXN *txn, const std::string &t){
  // Read and update both main and temporary timers
  for (int i = 0; i<2; i++) {
    uint8_t key = (i==0)? KEY_BACKUP_TMP : KEY_BACKUP_MAIN;
    auto timer = graphene_time_parse("0",ttype); // default
    timer = get_key(txn, key, timer);
    if (graphene_time_cmp(timer,t, ttype)>0)
      set_key(txn, key, mk_dbt(t));
  }
}

/************************************/
// Put data to the database
// input: timestamp + vector of strings
// The function can be run multiple times without reopening
// the database.
//
void
GrapheneDB::put(const string &t, const vector<string> & dat, const string &dpolicy){
  int ret;
  string ks = graphene_time_parse(t, ttype);
  string vs = graphene_data_parse(dat, dtype);

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
        else if (dpolicy =="sshift")
          ks = graphene_time_add(ks, graphene_time_parse("1", ttype), ttype);
        else if (dpolicy =="nsshift")
          ks = graphene_time_add(ks, graphene_time_parse("0.000000001", ttype), ttype);
        else if (dpolicy =="skip") break;
        else throw Err() << "Unknown dpolicy setting: " << dpolicy;
      }
      else if (res != 0)
        throw Err() << name << ".db: " << db_strerror(res);
    }
    backup_upd(txn, ks);
  }
  catch (Err e){
    txn_abort(txn);
    throw e;
  }
  txn_commit(txn);
}

/************************************/
// Put data to the database using input filter
void
GrapheneDB::put_flt(const string &t, const vector<string> & dat, const string &dpolicy){

  // read filter storage
  std::string storage = get_f0data();

  // run input filter
  auto t1 = graphene_time_print(graphene_time_parse(t, ttype),ttype);
  auto d1(dat);
  if (filters[0].run(t1, d1, storage)) put(t1,d1,dpolicy);

  // write storage
  write_f0data(storage);
}

/************************************/
// get data from the database -- get_next
//
void
GrapheneDB::get_next(const string &t1, DBout & dbo){
  string t1p = graphene_time_parse(t1, ttype);
  DBT k = mk_dbt(t1p);
  DBT v = mk_dbt();

  // do everything in a single transaction (with snapshot isolation)
  DB_TXN *txn = txn_begin(DB_TXN_SNAPSHOT);
  DBC *curs = NULL;
  try {
    /* Get a cursor */
    get_cursor(dbp.get(), txn, &curs, 0);

    if (c_get(curs, &k, &v, DB_SET_RANGE) && is_tstamp(&k))
      proc_point(dbt2str(&k), dbt2str(&v), false, dbo);

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
GrapheneDB::get_prev(const string &t2, DBout & dbo){

  string t2p = graphene_time_parse(t2, ttype);
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
    string tp = dbt2str(&k);

    // if needed, get previous record:
    if (graphene_time_cmp(tp,t2p, ttype)>0 || !found)
      found=c_get(curs, &k, &v, DB_PREV);

    if (found && is_tstamp(&k))
      proc_point(dbt2str(&k), dbt2str(&v), false, dbo);

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
GrapheneDB::get(const string &t, DBout & dbo){

  /* for non-float databases use get_prev */
  if (dtype!=DATA_FLOAT && dtype!=DATA_DOUBLE)
    return get_prev(t, dbo);

  string tp = graphene_time_parse(t, ttype);
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
    if (!is_tstamp(&k)) goto finish;

    // if there is no next value - give the last value if any
    if (!found) {
      if (c_get(curs, &k, &v, DB_PREV) && is_tstamp(&k))
        proc_point(dbt2str(&k), dbt2str(&v), false, dbo);
      goto finish;
    }

    // if "next" record is exactly at t - return it
    t1p = dbt2str(&k);
    v1p = dbt2str(&v);
    if (graphene_time_cmp(t1p,tp, ttype) == 0){
      proc_point(t1p, v1p, false, dbo);
      goto finish;
    }

    // get the previous value and do interpolation
    // find prev value
    found = c_get(curs, &k, &v, DB_PREV);
    if (!found || !is_tstamp(&k)) goto finish; // not found or not a timestamp

    t2p = dbt2str(&k);
    v2p = dbt2str(&v);
    vp = graphene_interpolate(tp, t1p, t2p, v1p, v2p, ttype, dtype);
    if (vp!="") proc_point(tp, vp, false, dbo);

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
GrapheneDB::get_range(const string &t1, const string &t2,
                const string &dt, DBout & dbo){

  string t1p = graphene_time_parse(t1, ttype);
  string t2p = graphene_time_parse(t2, ttype);
  string dtp = graphene_time_parse(dt, ttype);
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

      string pre = dbt2str(&k);
      if (!c_get(curs, &k, &v, fl)) break;

      if (!is_tstamp(&k)) {
        fl=DB_NEXT;
        continue;
      }

      // unpack new time value and check the range
      string tnp = dbt2str(&k);
      if (graphene_time_cmp(tnp,t2p,ttype)>0) break;

      // I have a broken database where DB_SET_RANGE/DB_NEXT can
      // get non-increasing values. Let's check this to prevent the
      // program from infinite loops..
      if (graphene_time_cmp(tnp,pre,ttype)<0)
        throw Err() << "Broken database (DB_SET_RANGE/DB_NEXT get smaller timestamp)";

      // if we want every point, switch to DB_NEXT and repeat
      if (graphene_time_zero(dtp, ttype)){
        proc_point(dbt2str(&k), dbt2str(&v), true, dbo);
        fl=DB_NEXT;
        continue;
      }

      // If dt >0 we continue using fl=DB_SET_RANGE.
      // If new value the same as old
      if (tlp.size()>0 && graphene_time_cmp(tlp,tnp,ttype)==0){
        // get next value
        if (!c_get(curs, &k, &v, DB_NEXT)) break;
        // unpack new time value and check the range
        tnp = dbt2str(&k);
        if (graphene_time_cmp(tnp,t2p,ttype) > 0 ) break;
      }
      proc_point(dbt2str(&k), dbt2str(&v), true, dbo);
      tlp=tnp; // update last printed value

      // add dt to the key for the next loop:
      string sp = graphene_time_add(tlp, dtp, ttype);
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
// get data from the database -- get_count
//
void
GrapheneDB::get_count(const string &t1,
                const string &count, DBout & dbo){

  string t1p = graphene_time_parse(t1, ttype);
  istringstream s(count);
  uint64_t N = 0;
  s >> N;
  if (s.bad() || s.fail() || !s.eof())
    throw Err() << "Can't parse data count: " << count;

  DBT k = mk_dbt(t1p);
  DBT v = mk_dbt();

  // do everything in a single transaction (with snapshot isolation)
  DB_TXN *txn = txn_begin(DB_TXN_SNAPSHOT);
  DBC *curs = NULL;
  try {

    // Get a cursor
    get_cursor(dbp.get(), txn, &curs, 0);

    int fl = DB_SET_RANGE; // first get t >= t1
    for (int i=0; i<N; ++i) {
      string pre = dbt2str(&k);

      if (!c_get(curs, &k, &v, fl)) break;

      // unpack new time value
      string tnp = dbt2str(&k);

      // I have a broken database where DB_SET_RANGE/DB_NEXT can
      // get non-increasing values. Let's check this to prevent the
      // program from infinite loops..
      if (graphene_time_cmp(tnp,pre,ttype)<0)
        throw Err() << "Broken database (DB_SET_RANGE/DB_NEXT get smaller timestamp)";

      // we want every point, switch to DB_NEXT and repeat
      proc_point(dbt2str(&k), dbt2str(&v), true, dbo);
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

}



/************************************/
// delete data data from the database -- del
void
GrapheneDB::del(const string &t1){
  int ret;
  string t1p = graphene_time_parse(t1, ttype);
  DBT k = mk_dbt(t1p);

  DB_TXN *txn = txn_begin();
  try{
    ret = dbp->del(dbp.get(), txn, &k, 0);
    if (ret == DB_NOTFOUND)
      throw Err() << name << ".db: No such record: " << t1;
    if (ret != 0)
      throw Err() << name << ".db: " << db_strerror(ret);
    backup_upd(txn, t1p);
  }
  catch (Err e){
    txn_abort(txn);
    throw e;
  }
  txn_commit(txn);
}

/************************************/
// delete data data from the database -- del_range
void
GrapheneDB::del_range(const string &t1, const string &t2){
  int ret;
  std::string first_del; // for lastmod timestamp

  string t1p = graphene_time_parse(t1, ttype);
  string t2p = graphene_time_parse(t2, ttype);
  DBT k = mk_dbt(t1p);
  DBT v = mk_dbt();

  DB_TXN *txn = txn_begin();
  DBC *curs = NULL;
  try {

    /* Get a cursor */
    get_cursor(dbp.get(), txn, &curs, 0);

    int fl = DB_SET_RANGE; // first get t >= t1
    while (1){

      string pre = dbt2str(&k);

      if (!c_get(curs, &k, &v, fl)) break;

      // get packed time value and check the range
      string tp = dbt2str(&k);
      if (graphene_time_cmp(tp,t2p,ttype)>0) break;

      // I have a broken database where DB_SET_RANGE/DB_NEXT can
      // get non-increasing values. Let's check this to prevent the
      // program from infinite loops..
      if (graphene_time_cmp(tp,pre,ttype)<0)
        throw Err() << "Broken database (DB_SET_RANGE/DB_NEXT get smaller timestamp)";

      // delete the point
      int res = curs->del(curs, 0);
      if (res!=0)
        throw Err() << name << ".db: " << db_strerror(res);
      if (first_del=="") first_del = tp;

      // we want to delete every point, so switch to DB_NEXT and repeat
      fl=DB_NEXT;
    }

    curs->close(curs);
    if (first_del!="") backup_upd(txn, first_del);
  }
  catch (Err e){
    if (curs) curs->close(curs);
    txn_abort(txn);
    throw e;
  }
  txn_commit(txn);
}

/************************************/
// functions for GrapheneDB::load method
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
GrapheneDB::load(const std::string &file){
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
GrapheneDB::dump(const std::string &file){
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


