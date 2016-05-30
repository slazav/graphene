/* DBout class: extended data output: columns, filters, tables */

#ifndef STSDB_DBOUT_H
#define STSDB_DBOUT_H

#include <string>
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
  std::string name;  // primary database name

  // extended names:
  int col; // column number, for the main database

  // constructor -- parse the string
  DBout(const std::string & str){
    col  = -1;
    name = str;

    // extract column
    size_t cp = name.rfind(':');
    if (cp!=std::string::npos){
      char *e;
      col = strtol(name.substr(cp+1,-1).c_str(), &e, 10);
      if (e!=NULL && *e=='\0') name = name.substr(0,cp);
      else col = -1;
    }
    if (col < -1) col = -1;
    name = DBsts::check_name(name);
  }
};

// callback for using with DBsts::get* functions
// usr_data should have DBout* type
void print_value(DBT *k, DBT *v, const DBinfo & info, void *usr_data);

#endif
