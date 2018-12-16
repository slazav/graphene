/* DBpool class: class for storing many opened databases
 */

#ifndef GRAPHENE_DBPOOL_H
#define GRAPHENE_DBPOOL_H

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <cstring> /* memset */
#include <db.h>
#include "dbgr.h"

/***********************************************************/
// Class for keeping a database environment and many opened
// databases.
// Note that DBpool objects can not be copied because of absence
// of proper memory handling.
class DBpool{
  std::string dbpath;
  std::string env_type;
  std::map<std::string, DBgr> pool;
  DB_ENV *env; // database environment
  bool readonly;

  public:

  // Constructor: open DB environment
  // env_type: "none", "lock", "txn" (default)
  DBpool(const std::string & dbpath_, const bool readonly, const std::string & env_type);

  // Destructor: close the DB environment
  ~DBpool();

  // remove database file
  void dbremove(std::string name);

  // rename database file
  void dbrename(std::string name1, std::string name2);

  // find database in the pool. Create/Open/Reopen if needed
  DBgr & get(const std::string & name, const int fl = 0);

  // close one database, close all databases
  void close(const std::string & name);
  void close();

  // sync one database, sync all databases
  void sync(const std::string & name);
  void sync();

};

#endif
