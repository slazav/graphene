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
#include "gr_db.h"

/***********************************************************/
// Class for keeping a database environment and many opened
// databases.
class GrapheneEnv{
  std::string dbpath;
  std::string env_type;
  std::map<std::string, GrapheneDB> pool;
  std::shared_ptr<DB_ENV> env; // database environment
  bool readonly;

  // Deleter for the environment
  struct D{
    void operator() (DB_ENV* env) {env->close(env, 0);}
  };

  public:

  // Constructor: open DB environment
  // env_type: "none", "lock", "txn" (default)
  GrapheneEnv(const std::string & dbpath_, const bool readonly, const std::string & env_type);

  ~GrapheneEnv();

  // find database in the pool. Create/Open/Reopen if needed
  GrapheneDB & getdb(const std::string & name, const int fl = 0);

  /****************/

  // return list of all databases
  std::vector<std::string> dblist();

  // create new database
  void dbcreate(const std::string & name, const std::string & descr,
              const DataType type);

  // remove database file
  void dbremove(const std::string & name);

  // rename database file
  void dbrename(const std::string & name1, const std::string & name2);

  /****************/

  // change database description
  void set_descr(const std::string & name, const std::string & descr);

  // get database description
  std::string get_descr(const std::string & name) {
    return getdb(name, DB_RDONLY).descr; }

  // get database type
  DataType get_type(const std::string & name) {
    return getdb(name, DB_RDONLY).dtype; }

  /****************/

  // backup start: notify that we are going to start backup.
  // - reset temporary backup timer
  // - return value of the main backup timer
  std::string backup_start(const std::string & name) {
    return getdb(name).backup_start(); }

  // backup end: notify that backup is successfully done
  // - commit temporary backup timer into main one
  // args: backup_end <name> [<timestamp>]
  void backup_end(const std::string & name, const std::string & t) {
    getdb(name).backup_end(t); }

  // reset backup timer
  void backup_reset(const std::string & name) {
    getdb(name).backup_reset(); }

  // get value of the backup timer
  std::string backup_get(const std::string & name) {
    return getdb(name).backup_get(); }

  /****************/

  // write data to the database
  void put(const std::string & name, const std::string & t,
           const std::vector<std::string> & d, const std::string & dpolicy){
    getdb(name).put(t, d, dpolicy);
  }

  // write data to the database through the input filter
  void put_flt(const std::string & name, const std::string & t,
           const std::vector<std::string> & d, const std::string & dpolicy){
    getdb(name).put_flt(t, d, dpolicy);
  }

  /****************/

  // close one database, close all databases
  void close(const std::string & name);
  void close();

  // sync one database, sync all databases
  void sync(const std::string & name);
  void sync();


  // print environment database files for archiving (same as db_archive -s)
  void list_dbs();

  // print environment log files (same as db_archive -l)
  void list_logs();

};

#endif
