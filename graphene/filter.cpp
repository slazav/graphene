#include <cstring>
#include <string>

#include "opt/opt.h"
#include "err/err.h"
#include "ifilter.h"

//#include <lua.hpp>
#include <tcl.h>

/***************************************************/

// Process and optionally modify input, return true if it
// should be recorded.
//
// * `time` - global variable with timestamp, seconds with
//   nanosecond precision as a string. Do not convert to
//   number if you want to keep precision.
//
// * `data` - global variable with data to be filtered
//
// * `storage` - global variable which which will be kept
//   between filter runs
bool
iFilter::run(std::string & t, std::vector<std::string> & d){

  if (code=="") return true;

  // create TCL interpreter
  Tcl_Interp *interp = Tcl_CreateInterp();
  if (interp == NULL) throw Err() << "ifilter: can't run TCL interpreter\n";

  // switch to safe mode
  if (Tcl_MakeSafe(interp) != TCL_OK){
    Tcl_Obj *infoObj = Tcl_GetVar2Ex(interp, "errorInfo", NULL, TCL_GLOBAL_ONLY);
    throw Err() << "ifilter: Tcl_MakeSafe failed: " << Tcl_GetString(infoObj);
  }

  // define global variable time
  if (Tcl_SetVar(interp, "time", t.c_str(), TCL_GLOBAL_ONLY) == NULL)
    throw Err() << "ifilter: can't set time variable: " << t;

  // define global variable data
  for (auto const & v:d)
    if (Tcl_SetVar(interp, "data", v.c_str(),
            TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT) == NULL)
      throw Err() << "ifilter: can't set data variable: " << t;

  // define global variable storage
  if (storage != "") {
    if (Tcl_SetVar(interp, "storage", storage.c_str(), TCL_GLOBAL_ONLY) == NULL)
      throw Err() << "ifilter: can't set storage variable: " << storage;
  }

  // run TCL script
  if (Tcl_Eval(interp, code.c_str()) != TCL_OK)
    throw Err() << "ifilter: can't run TCL script: " << Tcl_GetVar(interp, "errorInfo", TCL_GLOBAL_ONLY);

  // get timestamp back
  auto tc = Tcl_GetVar(interp, "time", TCL_GLOBAL_ONLY);
  if (tc==NULL) throw Err() << "ifilter: can't get time value";
  t = tc;

  // get data back
  Tcl_Obj* lst = Tcl_GetVar2Ex(interp, "data", NULL, TCL_GLOBAL_ONLY);
  if (lst) {
    Tcl_Obj** elem;
    int n;
    if (Tcl_ListObjGetElements(interp, lst, &n, &elem) != TCL_OK)
      throw Err() << "ifilter: broken data list: " << Tcl_GetVar(interp, "errorInfo", TCL_GLOBAL_ONLY);
    d.clear();
    for (int i = 0; i < n; ++i,++elem){
      int len;
      const char* s = Tcl_GetStringFromObj(*elem, &len);
      if (!s) throw Err() << "ifilter: can't get string value from storage";
      d.push_back(std::string(s, s+len));
    }
  }

  // get storage back
  auto storagec = Tcl_GetVar(interp, "storage", TCL_GLOBAL_ONLY);
  storage = storagec? storagec:"";

  // Return value. If value can not be converted assume true
  int ret;
  auto ret_str = Tcl_GetString(Tcl_GetObjResult(interp));
  if (!ret_str || Tcl_GetBoolean(interp, ret_str, &ret) != TCL_OK)
    ret = true;

  // delete TCL interpreter
  Tcl_DeleteInterp(interp);
  return ret;
}
