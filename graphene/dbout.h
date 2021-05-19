/* DBout class: data output: columns, filters, tables */
#ifndef GRAPHENE_DBOUT_H
#define GRAPHENE_DBOUT_H

#include <string>
#include <iostream>

#include "data.h"
#include "filter.h"

/***********************************************************/
// Class for an extended dataset object.
//
// Extended dataset can be just a database name, but it can also
// contain a column, or a filter:
// <name>:<column>
// <name>:<filter>
//
class DBout {
  public:

  bool spp;    // SPP mode (protect # in the beginning of line)

  TimeType ttype;
  DataType dtype;
  Filter * filter;
  bool list;

  std::ostream & out;  // stream for output
  int col; // column number, for the main database

  TimeFMT timefmt;     // output time format
  std::string time0;   // zero time for relative time output (not parsed)

  // constructor -- parse the dataset string, create iostream
  DBout(std::ostream & out_ = std::cout):
          col(-1), out(out_), spp(false), timefmt(TFMT_DEF),
          ttype(TIME_V2), dtype(DATA_DOUBLE), list(false), filter(0) {}

  // print_point  -- by default it just prints the line to out,
  // but this function can be overriden.
  virtual void print_point(const std::string & str){
    if (spp) out << graphene_spp_text(str);
    else out << str;
  }

};


#endif
