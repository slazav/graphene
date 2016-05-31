/* DBout class: extended data output: columns, filters, tables */

#ifndef STSDB_DBOUT_H
#define STSDB_DBOUT_H

#include <string>
#include <errno.h>
#include "db.h"

/***********************************************************/
// Class for an extended dataset object.
//
// Extended dataset can be just a database name, but it can also
// contain a column, a filter, and some secondary databases
//
// Data are taken from the main database, then data from secondary databases
// are interpolated (using the GET command) and added. Then the multi-column
// result is filtered through the filter.
//
// Example of the extended dataset:
//   dba:2 + dbb:3 + dbc:1 | flt
// This means that data should be found in the 2nd column of dba,
// then all data from dbb and first column of dbc are interpolated
// to the data points, then all data should go through filter program flt.
//
class DBout {
  public:
  std::string name;    // primary database name
  std::string filter;  // filter program

  // extended names:
  int col; // column number, for the main database

  // constructor -- parse the dataset string, create iostream
  DBout(const std::string & dbpath, const std::string & str){
    col  = -1;
    name = str;

    // extract filter
    size_t cp = name.rfind('|');
    if (cp!=std::string::npos){
      filter = name.substr(cp+1,-1);
      name = name.substr(0,cp);
    }

    // extract column
    cp = name.rfind(':');
    if (cp!=std::string::npos){
      char *e;
      col = strtol(name.substr(cp+1,-1).c_str(), &e, 10);
      if (e!=NULL && *e=='\0') name = name.substr(0,cp);
      else col = -1;
    }
    if (col < -1) col = -1;
    name   = DBsts::check_name(name);
    filter = DBsts::check_name(filter);

    if (filter!=""){
      // fork
      int fd[2];
      if (pipe(fd) != 0)
        throw Err() << "can't create a pipe";
      pid_t pid = fork();
      if (pid < 0)
        throw Err() << "can't do fork";

      // in the child process we set redirect pipe to stdin
      // and run filter program
      if (pid == 0) {
        if (dup2(fd[0], 0) != 0)
          throw Err() << "can't set up standard input: " << strerror(errno);
        if (close(fd[0]) != 0 || close(fd[1]) != 0)
          throw Err() << "can't set up standard input";

        // This process communicates only via stdout.
        // Here we ignore all errors.
        std::string f = dbpath + "/" + filter;
        execl(f.c_str(), f.c_str(), NULL);
        throw Err();
      }
      //in parent we redirect stdout to the pipe
      if (dup2(fd[1], 1) != 1)
        throw Err() << "can't set up standard output: " << strerror(errno);
      if (close(fd[0]) != 0 || close(fd[1]) != 0)
        throw Err() << "can't set up standard output";
    }
  }
};

// callback for using with DBsts::get* functions
// usr_data should have DBout* type
void print_value(DBT *k, DBT *v, const DBinfo & info, void *usr_data);

#endif
