#include <cstdlib>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>
#include <cstring> /* memset */
#include <db.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>

#include "gr_env.h"
#include "gr_db.h"
#include "err/err.h"


GrapheneEnvFormatter::GrapheneEnvFormatter(GrapheneTCL & tcl_,
          const std::string & ext_name, GrapheneEnv & env_):
          col(-1), flt_num(-1), timefmt(TFMT_DEF), list(false),
          fmt_cb(NULL), fmt_cb_data(NULL), tcl(tcl_), env(env_) {

  // split secondary database names using '+' delimiter
  name = ext_name;
  size_t pos = 0;
  while ((pos = name.rfind('+')) != std::string::npos) {
    secondary.push_back(name.substr(pos+1));
    name.resize(pos);
  }
  std::reverse(secondary.begin(), secondary.end());

  name = parse_ext_name(name, col, flt_num);
  if (flt_num>0) filter = env.getdb(name, DB_RDONLY).get_filter(flt_num);
}


// callback for adding values to a data vector (used for secondary databases)
void
out_cb_addval(const std::string &t, const std::vector<std::string> &d, void * cb_data){
  auto d0 = (std::vector<std::string> *)cb_data;
  d0->insert(d0->end(), d.begin(), d.end());
}

std::string
GrapheneTCLGet::run(const std::vector<std::string> & args) {
  if (args.size()<2 || args.size()>3)
    throw Err() << "graphene_get: wrong number of arguments";
  std::ostringstream out;
  env.get(args[1], args.size()>2? args[2]:"inf", TFMT_DEF, out_cb_simple, &out);
  return out.str();
}


void
GrapheneEnvFormatter::proc_point(const std::string &ks, const std::string &vs,
    const TimeType ttype, const DataType dtype) {

  auto t = graphene_time_print(ks, ttype, timefmt, time0);
  auto d = graphene_data_print(vs, (filter == "" ? col:-1), dtype); // use all columns for filters

  // run filters
  std::string storage; // output filters do not use storage, but we need to provide the variable
  if (!tcl.run(filter, t,d,storage)) return;

  // add data from secondary databases
  for (const auto & s:secondary){
    auto t = graphene_time_print(ks, ttype, TFMT_DEF, "");
    env.get(s, t, TFMT_DEF, out_cb_addval, &d);
  }

  // in list mode keep only first line
  if (list && dtype==DATA_TEXT){
    d.resize(1); // it should be already 1 for TEXT dbs
    auto n = d[0].find('\n');
    if (n!=std::string::npos) d[0].resize(n);
  }

  if (fmt_cb) (fmt_cb)(t, d, fmt_cb_data);
}



// process registration:
// https://docs.oracle.com/cd/E17276_01/html/gsg_xml_txn/cxx/architectrecovery.html#multiprocessrecovery
// snapshot isolation:
// https://docs.oracle.com/cd/E17076_05/html/gsg_txn/C/isolation.html#snapshot_isolation

// Constructor: open DB environment
GrapheneEnv::GrapheneEnv(const std::string & dbpath_, const bool readonly_,
                         const std::string & env_type_, const std::string & tcl_libdir):
    dbpath(dbpath_), env_type(env_type_), readonly(readonly_), tcl(tcl_libdir), tcl_get_cmd(*this) {

  if (env_type == "none"){
    // no invironment
    env=NULL;
    return;
  }

  DB_ENV * e;
  int res = db_env_create(&e, 0);
  if (res != 0)
    throw Err() << "creating DB_ENV: " << dbpath << ": " << db_strerror(res);
  env = std::shared_ptr<DB_ENV>(e, GrapheneEnv::D());

  // set logfile size
  res = env->set_lg_max(env.get(), GRAPHENE_LOGSIZE);
  if (res != 0)
    throw Err() << "set_lg_max failed: " << db_strerror(res);

  // deadlock detection
  res = env->set_lk_detect(env.get(), DB_LOCK_MINWRITE);
  if (res != 0)
    throw Err() << "Error setting lock detect: " << db_strerror(res);

  int flags;
  if (env_type == "txn")
    flags = DB_CREATE |
            DB_INIT_LOCK |
            DB_INIT_MPOOL |
            DB_INIT_LOG |     // logging
            DB_INIT_TXN |     // transactions
            DB_REGISTER |     // register processes to know when recover is possible/needed
            DB_RECOVER  |     // run recover if possible/needed
            DB_MULTIVERSION;  // for snapshot isolation

  else if (env_type == "lock")
    flags = DB_CREATE |
            DB_INIT_LOCK |
            DB_INIT_MPOOL;

  else throw Err() << "unknown env_type";

  // open environment
  res = env->open(env.get(), dbpath.c_str(), flags, 0644);
  if (res != 0)
    throw Err() << "opening DB_ENV: " << dbpath << ": " << db_strerror(res);

  // add commands to TCL interpeter
  tcl.add_cmd("graphene_get", &tcl_get_cmd);

}

// Destructor: close the DB environment
GrapheneEnv::~GrapheneEnv(){
  close();
}


// find database in the pool. Open/Reopen if needed
GrapheneDB &
GrapheneEnv::getdb(const std::string & name, const int fl){

  if (readonly && !(fl & DB_RDONLY)) throw Err() << "can't write to database in readonly mode";
  std::map<std::string, GrapheneDB>::iterator i = pool.find(name);

  // if database was opened with wrong flags close it
  if (!(fl & DB_RDONLY) && i!=pool.end() && i->second.is_readonly()){
    pool.erase(i); i=pool.end();
  }

  // if database is not opened, open it
  if (!pool.count(name)) pool.insert(
    std::pair<std::string, GrapheneDB>(name, GrapheneDB(env.get(), dbpath, name, fl)));

  // return the database
  return pool.find(name)->second;
}

/****************/

// make database list
std::vector<std::string>
GrapheneEnv::dblist(){
  std::vector<std::string> ret;
  DIR *dir = opendir(dbpath.c_str());
  if (!dir) throw Err() << "can't open database directory: " << strerror(errno);
  struct dirent *ent;
  std::vector<std::string> names;
  while ((ent = readdir (dir)) != NULL) {
    std::string name(ent->d_name);
    size_t p = name.find(".db");
    if (name.size()>3 && p == name.size()-3)
      ret.push_back(name.substr(0,p));
  }
  closedir(dir);
  std::sort(ret.begin(), ret.end());
  return ret;
}

// create new database
void
GrapheneEnv::dbcreate(const std::string & name, const std::string & descr,
                    const DataType dtype){
  GrapheneDB & db = getdb(name, DB_CREATE | DB_EXCL);
  db.set_dtype(dtype);
  db.set_descr(descr);
}


// remove database file
void
GrapheneEnv::dbremove(const std::string & name){
  if (readonly) throw Err() << "can't remove database in readonly mode";
  check_name(name); // check name
  close(name);
  if (env) {
    int res = env->dbremove(env.get(), NULL, (name + ".db").c_str(), NULL, 0);
    if (res!=0) throw Err() << name <<  ".db: " << db_strerror(res);
  }
  else {
    int res = remove((dbpath + "/" + name + ".db").c_str());
    if (res) throw Err() << name <<  ".db: " << strerror(errno);
  }
}

// rename database file
void
GrapheneEnv::dbrename(const std::string & name1, const std::string & name2){
  if (readonly) throw Err() << "can't rename database in readonly mode";
  check_name(name1); // check name
  check_name(name2); // check name
  std::string path1 = name1 + ".db";
  std::string path2 = name2 + ".db";
  std::string fpath1 = dbpath + "/" + path1;
  std::string fpath2 = dbpath + "/" + path2;

  // check destination to avoid additional error messages:
  struct stat buf;
  int res = stat(fpath2.c_str(), &buf);
  if (res==0) throw Err() << "renaming " << name1 <<  ".db -> "
                          << name2 << ".db: " << "Destination exists";

  close(name1);
  if (env) {
    res = env->dbrename(env.get(), NULL, path1.c_str(), NULL, path2.c_str(), 0);
    if (res!=0) throw Err() << "renaming " << name1 <<  ".db -> "
                            << name2 << ".db: " << db_strerror(res);
  }
  else {
    res = rename(fpath1.c_str(), fpath2.c_str());
    if (res) throw Err() << "renaming " << name1 <<  ".db -> "
                         << name2 << ".db: " << strerror(errno);
  }
}

// close one database, close all databases
void
GrapheneEnv::close(const std::string & name){
  auto i = pool.find(name);
  if (i!=pool.end()) pool.erase(i);
}

void
GrapheneEnv::close(){ pool.clear(); }


// sync one database, sync all databases
void
GrapheneEnv::sync(const std::string & name){
  auto i = pool.find(name);
  if (i!=pool.end()) i->second.sync();
}

void
GrapheneEnv::sync(){
  for (auto &db:pool) db.second.sync();
}

void
GrapheneEnv::list_dbs(){
  // see code in https://web.stanford.edu/class/cs276a/projects/docs/berkeleydb/ref/transapp/archival.html
  int ret;
  char **begin, **list;
  if (!env) throw Err() << "Command can not be run without DB environment";

  /* Get the list of database files. */
  if ((ret = env->log_archive(env.get(), &list, DB_ARCH_DATA)) != 0)
    throw Err() << db_strerror(ret);

  if (list != NULL) {
    for (begin = list; *list != NULL; ++list)  printf("%s\n", *list);
    free (begin);
  }
}

void
GrapheneEnv::list_logs(){
  // see code in https://web.stanford.edu/class/cs276a/projects/docs/berkeleydb/ref/transapp/archival.html
  int ret;
  char **begin, **list;
  if (!env) throw Err() << "Command can not be run without DB environment";

  /* Get the list of log files. */
  if ((ret = env->log_archive(env.get(), &list, DB_ARCH_LOG)) != 0)
    throw Err() << db_strerror(ret);

  if (list != NULL) {
    for (begin = list; *list != NULL; ++list) printf("%s\n", *list);
    free (begin);
  }
}




void
out_cb_simple(const std::string &t,  const std::vector<std::string> &d, void * cb_data){
  auto out = (std::ostream *)cb_data;
  *out << t;
  for (auto const & v:d) *out << " " << v;
  *out << "\n";
}

