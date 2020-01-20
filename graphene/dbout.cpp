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

  name = parse_ext_name(name, col);
}

void
DBout::print_point(const std::string & str) {
  if (pars.interactive) out << graphene_spp_text(str);
  else out << str;
}

