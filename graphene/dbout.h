/* DBout class: data output: columns, filters, tables */
#ifndef GRAPHENE_DBOUT_H
#define GRAPHENE_DBOUT_H

#include <string>
#include <iostream>

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

  // time format setting
  enum timefmt_t {TIME_DEF, TIME_REL_S};

  struct Pars {
    bool list;           // list mode -- print 1 line per point
    bool interactive;    // interactive mode (protect # in the beginning of line)
    timefmt_t timefmt;   // time format
    std::string time0;   // zero time for relative time output or ""

    Pars(): interactive(false), list(false), timefmt(TIME_DEF) {};

    void set_timefmt(const std::string & s){
      if (strcasecmp(s.c_str(),"default") == 0) {timefmt=TIME_DEF; return;}
      if (strcasecmp(s.c_str(),"rel_s") == 0) {timefmt=TIME_REL_S; return;}
    }

    std::string print_timefmt() const {
      switch (timefmt){
        case TIME_DEF:   return "default";
        case TIME_REL_S: return "rel_s";
        default: throw Err() << "Unknown time format: " << timefmt;
      }
    }
  };


  std::string name;    // primary database name
  std::ostream & out;  // stream for output
  int col; // column number, for the main database
  Pars pars;

  // filter name, pid, in/out descriptors
  std::string filter;  // filter program
  pid_t pid;
  int fd1[2], fd2[2];

  // constructor -- parse the dataset string, create iostream
  DBout(const std::string & filterpath,
        const std::string & str,
        std::ostream & out = std::cout);
  ~DBout();

  // Process a single point (select a column, make tables, filtering)
  // and call print_point().
  // <list> parameter changes output of TEXT records in a list mode
  // (only first line is shown).
  void proc_point(DBT *k, DBT *v, const DBinfo & info);

  // print_point  -- by default it just prints the line to out,
  // but this function can be overriden.
  virtual void print_point(const std::string & str);

};

#endif
