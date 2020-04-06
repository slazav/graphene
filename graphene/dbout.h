/* DBout class: data output: columns, filters, tables */
#ifndef GRAPHENE_DBOUT_H
#define GRAPHENE_DBOUT_H

#include <string>
#include <iostream>

#include "data.h"

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

  bool spp;    // SPP mode (protect # in the beginning of line)

  std::ostream & out;  // stream for output
  int col; // column number, for the main database

  // constructor -- parse the dataset string, create iostream
  DBout(std::ostream & out_ = std::cout):
          col(-1), out(out_), spp(false) {}

  // print_point  -- by default it just prints the line to out,
  // but this function can be overriden.
  virtual void print_point(const std::string & str){
    if (spp) out << graphene_spp_text(str);
    else out << str;
  }

};

// version with string output (for HTTP get interface)
class DBoutS: public DBout {
  std::string mystr;
  public:

  void print_point(const std::string & str){ mystr += str; }
  std::string & get_str() {return mystr;}

};


#endif
