/* DBpool class: class for storing many opened databases
 */

#ifndef GRAPHENE_DBPOOL_H
#define GRAPHENE_DBPOOL_H

#include <cassert>
#include <cstdlib>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <cstring> /* memset */
#include <db.h>
#include "db.h"
#include <dirent.h>
#include <sys/stat.h>

/***********************************************************/
// Class for keeping a database environment and many opened
// databases.
// Note that DBpool objects can not be copied because of absence
// of proper memory handling.
class DBpool{
  std::string dbpath;
  std::map<std::string, DBgr> pool;
  DB_ENV *env; // database environment
  public:

  // Constructor: open DB environment
  DBpool(const std::string & dbpath_): dbpath(dbpath_) {
    int res = db_env_create(&env, 0);
    if (res != 0)
      throw Err() << "creating DB_ENV: " << dbpath << ": " << db_strerror(res);

    res = env->open(env, dbpath.c_str(),
                   DB_CREATE | DB_INIT_LOCK | DB_INIT_MPOOL, 0644);
    if (res != 0)
      throw Err() << "opening DB_ENV: " << dbpath << ": " << db_strerror(res);
  }
  // Destructor: close the DB environment
  ~DBpool(){
     close();
     env->close(env, 0);
  }

  // create database file
  DBgr dbcreate(const std::string & name) {
    // create database
    if (pool.count(name)) throw Err() << name << ": database exists in the pool\n";
    pool.insert(std::pair<std::string, DBgr>(name,
      DBgr(env, dbpath, name, DB_CREATE | DB_EXCL)));

    // return the database
    return pool.find(name)->second;
  }

  // remove database file
  void dbremove(std::string name){
    name = check_name(name); // check name
    close(name);
    int res = env->dbremove(env, NULL, (name + ".db").c_str(), NULL, 0);
    if (res!=0) throw Err() << name <<  ".db: " << db_strerror(res);
  }

  // rename database file
  void dbrename(std::string name1, std::string name2){
    name1 = check_name(name1); // check name
    name2 = check_name(name2); // check name
    std::string path1 = name1 + ".db";
    std::string path2 = name2 + ".db";

    // check destination to avoid additional error messages:
    struct stat buf;
    int res = stat(path2.c_str(), &buf);
    if (res==0) throw Err() << "renaming " << name1 <<  ".db -> "
                            << name2 << ".db: " << "Destination exists";

    res = env->dbrename(env, NULL, path1.c_str(), NULL, path2.c_str(), 0);
    if (res!=0) throw Err() << "renaming " << name1 <<  ".db -> "
                            << name2 << ".db: " << db_strerror(res);
  }

  // find database in the pool. Open/Reopen if needed
  DBgr & get(const std::string & name, const int fl = 0){

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
  void close(const std::string & name){
    std::map<std::string, DBgr>::iterator i = pool.find(name);
    if (i!=pool.end()) pool.erase(i);
  }
  void close(){ pool.clear(); }

  // sync one database, sync all databases
  void sync(const std::string & name){
    std::map<std::string, DBgr>::iterator i = pool.find(name);
    if (i!=pool.end()) i->second.sync();
  }
  void sync(){
    std::map<std::string, DBgr>::iterator i;
    for (i = pool.begin(); i!=pool.end(); i++) i->second.sync();
  }

};

#endif
