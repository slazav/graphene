#include "dbpool.h"

#include <cstdlib>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <cstring> /* memset */
#include <db.h>
#include "db.h"
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>

// Constructor: open DB environment
DBpool::DBpool(const std::string & dbpath_, const std::string & env_type_): dbpath(dbpath_), env_type(env_type_){

  if (env_type == "none"){
    // no invironment
    env=NULL;
    return;
  }

  int res = db_env_create(&env, 0);
  if (res != 0)
    throw Err() << "creating DB_ENV: " << dbpath << ": " << db_strerror(res);

  env->set_lg_max(env, GRAPHENE_LOGSIZE);

  int flags;
  if (env_type == "txn") flags = DB_CREATE | DB_INIT_LOCK | DB_INIT_MPOOL | DB_INIT_LOG | DB_INIT_TXN;
  else if (env_type == "lock") flags = DB_CREATE | DB_INIT_LOCK | DB_INIT_MPOOL | DB_INIT_LOG;
  else throw Err() << "unknown env_type";

  // deadlock detection
  res = env->set_lk_detect(env, DB_LOCK_MINWRITE);
  if (res != 0)
    throw Err() << "Error setting lock detect: " << db_strerror(res);

  res = env->open(env, dbpath.c_str(), flags, 0644);
  if (res != 0)
    throw Err() << "opening DB_ENV: " << dbpath << ": " << db_strerror(res);
}

// Destructor: close the DB environment
DBpool::~DBpool(){
   close();
   if (env) env->close(env, 0);
}

// remove database file
void
DBpool::dbremove(std::string name){
  check_name(name); // check name
  close(name);
  if (env) {
    int res = env->dbremove(env, NULL, (name + ".db").c_str(), NULL, 0);
    if (res!=0) throw Err() << name <<  ".db: " << db_strerror(res);
  }
  else {
    int res = remove((dbpath + "/" + name + ".db").c_str());
    if (res) throw Err() << name <<  ".db: " << strerror(errno);
  }
}

// rename database file
void
DBpool::dbrename(std::string name1, std::string name2){
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
    res = env->dbrename(env, NULL, path1.c_str(), NULL, path2.c_str(), 0);
    if (res!=0) throw Err() << "renaming " << name1 <<  ".db -> "
                            << name2 << ".db: " << db_strerror(res);
  }
  else {
    res = rename(fpath1.c_str(), fpath2.c_str());
    if (res) throw Err() << "renaming " << name1 <<  ".db -> "
                         << name2 << ".db: " << strerror(errno);
  }
}


// find database in the pool. Open/Reopen if needed
DBgr &
DBpool::get(const std::string & name, const int fl){

  std::map<std::string, DBgr>::iterator i = pool.find(name);

  // if database was opened with wrong flags close it
  if (!(fl & DB_RDONLY) && i!=pool.end() &&
       i->second.open_flags & DB_RDONLY){
    pool.erase(i); i=pool.end();
  }

  // if database is not opened, open it
  if (!pool.count(name)) pool.insert(
    std::pair<std::string, DBgr>(name, DBgr(env, dbpath, name, fl)));

  // return the database
  return pool.find(name)->second;
}


// close one database, close all databases
void
DBpool::close(const std::string & name){
  std::map<std::string, DBgr>::iterator i = pool.find(name);
  if (i!=pool.end()) pool.erase(i);
}

void
DBpool::close(){ pool.clear(); }


// sync one database, sync all databases
void
DBpool::sync(const std::string & name){
  std::map<std::string, DBgr>::iterator i = pool.find(name);
  if (i!=pool.end()) i->second.sync();
}

void
DBpool::sync(){
  std::map<std::string, DBgr>::iterator i;
  for (i = pool.begin(); i!=pool.end(); i++) i->second.sync();
}
