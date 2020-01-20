#include "dbgr.h"
#include "dbout.h"
#include "err/err.h"

#include <string>
#include <iomanip>
#include <iostream>
#include <errno.h>
#include <wait.h>

DBout::DBout(const std::string & name_, std::ostream & out_):
          col(-1), name(name_), out(out_) {

  // extract column
  size_t cp = name.rfind(':');
  if (cp!=std::string::npos){
    char *e;
    col = strtol(name.substr(cp+1,-1).c_str(), &e, 10);
    if (e!=NULL && *e=='\0') name = name.substr(0,cp);
    else col = -1;
  }
  if (col < -1) col = -1;
  check_name(name);
}

void
DBout::print_point(const std::string & str) {
  if (pars.interactive) out << graphene_spp_text(str);
  else out << str;
}

