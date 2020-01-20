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

  struct Pars {
    bool interactive;    // interactive mode (protect # in the beginning of line)
    Pars(): interactive(false) {};
  };

  std::string name;    // primary database name
  std::ostream & out;  // stream for output
  int col; // column number, for the main database
  Pars pars;

  // constructor -- parse the dataset string, create iostream
  DBout(const std::string & str, std::ostream & out = std::cout);

  // Process a single point (select a column) and call print_point().
  void proc_point(const std::string & s);

  // print_point  -- by default it just prints the line to out,
  // but this function can be overriden.
  virtual void print_point(const std::string & name);

};

#endif
