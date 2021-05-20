#ifndef GR_TCL_H
#define GR_TCL_H

#include <cstring>
#include <string>
#include <vector>
#include <sstream>
#include <cmath>
#include <memory>

#include "opt/opt.h"
#include "err/err.h"

#include <tcl.h>

/***************************************************/

// TCL interpreter
class GrapheneTCL {

  std::shared_ptr<Tcl_Interp> interp;

  public:

  // Restart the interpreter
  void restart(const std::string & tcl_libdir);

  // Constructor
  GrapheneTCL(const std::string & tcl_libdir) {restart(tcl_libdir);}

  // Run the code, return true if it should be recorded
  bool run(const std::string & code, std::string & t,
           std::vector<std::string> & d, std::string & storage);

};

#endif
