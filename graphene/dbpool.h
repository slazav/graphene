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

/***********************************************************/
// class for storing many opened databases
class DBpool{
  std::string dbpath;
  std::map<std::string, DBgr> pool;
  public:

  // Fake constructor. See how Pars constructor works.
  DBpool() {}

  DBpool(const std::string & dbpath_): dbpath(dbpath_) {}

  DBgr & get(const std::string & name, const int fl = 0){

    std::map<std::string, DBgr>::iterator i = pool.find(name);

    // if database was opened with wrong flags close it
    if (!(fl & DB_RDONLY) && i!=pool.end() &&
         i->second.open_flags & DB_RDONLY){
      pool.erase(i); i=pool.end();
    }

    // if database is not opened, open it
    if (!pool.count(name)) pool.insert(
      std::pair<std::string, DBgr>(name, DBgr(dbpath, name, fl)));

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
