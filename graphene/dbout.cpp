#include "dbgr.h"
#include "dbout.h"
#include "err/err.h"

#include <string>
#include <iomanip>
#include <iostream>
#include <errno.h>
#include <wait.h>

DBout::DBout(std::ostream & out_):
          col(-1), out(out_), spp(false) {}

void
DBout::print_point(const std::string & str) {
  if (spp) out << graphene_spp_text(str);
  else out << str;
}

