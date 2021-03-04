#ifndef FILTER_H
#define FILTER_H

#include <cstring>
#include <string>
#include <vector>
#include <sstream>
#include <cmath>

#include "opt/opt.h"
#include "err/err.h"

/***************************************************/

// filter for the graphene database
class Filter {
  public:

  // TCL script.
  std::string code;

  // TCL library.
  static std::string library;

  // Filter data. Should be updated before each call,
  // and saved to the database after it.
  std::string storage;

  /********************************************************/

  std::string get_code() const {return code;}
  std::string get_storage() const {return storage;}
  void set_code(const std::string & c = std::string()) {code=c;}
  void set_storage(const std::string & d = std::string()) {storage=d;}

  // Process input, return true if it should be recorded
  bool run(std::string & t, std::vector<std::string> & d);

  // Read library (all files from tcl_libdir)
  static void load_library(const std::string & tcl_libdir);

};

#endif
