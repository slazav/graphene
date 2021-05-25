#include <cstring>
#include <string>
#include <algorithm>

#include "err/err.h"
#include "gr_tcl.h"

#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <fstream>
#include <streambuf>
#include "filename/filename.h"


#include <tcl.h>

// format TCL error; We do not want multi-line errors in SPP output
std::string tcl_error(Tcl_Interp *interp){
  auto s = Tcl_GetVar(interp, "errorInfo", TCL_GLOBAL_ONLY);
  if (!s) s = Tcl_GetStringResult(interp);
  std::string ret( s? s:"" );
  std::replace( ret.begin(), ret.end(), '\n', ' ');
  return ret;
}

int
tcl_proc(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]){

  // convert arguments into vector<string>
  std::vector<std::string> args;
  for (int i=0; i<objc; i++){
    auto s = Tcl_GetString(objv[i]);
    if (s) args.emplace_back(s);
  }

  // run the command
  try {
    auto proc = (GrapheneTCLProc *)clientData;
    if (!proc) throw Err() << "GrapheneTCL::add_cmd with null proc";
    auto ret = proc->run(args);
    Tcl_Obj * reto = Tcl_NewStringObj(ret.data(), ret.size());
    Tcl_SetObjResult(interp, reto);
    return TCL_OK;
  }
  catch (const Err & e){
    auto s = e.str();
    Tcl_Obj * reto = Tcl_NewStringObj(s.data(), s.size());
    Tcl_SetObjResult(interp, reto);
    return TCL_ERROR;
  }
}


void
GrapheneTCL::add_cmd(const char *name, GrapheneTCLProc * proc){
interp.get();
  if (!Tcl_CreateObjCommand(interp.get(), name, tcl_proc, proc, NULL))
    throw Err() << "Tcl_CreateObjCommand failed";
}


void
GrapheneTCL::restart(const std::string & tcl_libdir) {

  // create TCL interpreter
  interp = std::shared_ptr<Tcl_Interp> (Tcl_CreateInterp(), Tcl_DeleteInterp);
  if (!interp) throw Err() << "filter: can't run TCL interpreter\n";

  // switch to safe mode
  if (Tcl_MakeSafe(interp.get()) != TCL_OK)
    throw Err() << "filter: Tcl_MakeSafe failed: " << tcl_error(interp.get());

  if (tcl_libdir == "") return;

  // load library
  struct dirent *ent;
  std::string library;
  DIR *dir = opendir(tcl_libdir.c_str());
//  if (dir==NULL) throw Err() << "can't read TCL libraries in "
//    << tcl_libdir << ": " << strerror(errno);
  if (dir==NULL) return;

  library = std::string();

  while ((ent = readdir(dir)) != NULL) {
    if (!file_ext_check(ent->d_name, ".tcl")) continue;
    std::ifstream f(tcl_libdir + "/" + ent->d_name);
    library.append((std::istreambuf_iterator<char>(f)),
                     (std::istreambuf_iterator<char>()));
    library+='\n';
  }
  closedir (dir);

  // use library
  if (library!= "" && Tcl_Eval(interp.get(), library.c_str()) != TCL_OK)
    throw Err() << "filter: can't use TCL library: " << tcl_error(interp.get());
}


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
GrapheneTCL::run(const std::string & code, std::string & t, std::vector<std::string> & d, std::string & storage){

  if (code=="") return true;

  // define global variable time
  if (Tcl_SetVar(interp.get(), "time", t.c_str(), TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL)
    throw Err() << "filter: can't set time variable: " << tcl_error(interp.get());

  // define global variable data
  if (Tcl_SetVar(interp.get(), "data", "", TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL)
    throw Err() << "filter: can't set data variable: " << tcl_error(interp.get());

  // append values to data list
  for (auto const & v:d)
    if (Tcl_SetVar(interp.get(), "data", v.c_str(),
            TCL_GLOBAL_ONLY | TCL_APPEND_VALUE | TCL_LIST_ELEMENT | TCL_LEAVE_ERR_MSG) == NULL)
      throw Err() << "filter: can't set data variable: " << tcl_error(interp.get());

  // define global variable storage
  if (Tcl_SetVar(interp.get(), "storage", storage.c_str(), TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL)
    throw Err() << "filter: can't set storage variable: " << tcl_error(interp.get());


  // run TCL script
  if (Tcl_Eval(interp.get(), code.c_str()) != TCL_OK)
    throw Err() << "filter: can't run TCL script: " << tcl_error(interp.get());

  // get timestamp back
  auto tc = Tcl_GetVar(interp.get(), "time", TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG);
  if (tc==NULL) throw Err() << "filter: can't get time value: " << tcl_error(interp.get());
  t = tc;

  // get data back
  Tcl_Obj* lst = Tcl_GetVar2Ex(interp.get(), "data", NULL, TCL_GLOBAL_ONLY);
  if (lst) {
    Tcl_Obj** elem;
    int n;
    if (Tcl_ListObjGetElements(interp.get(), lst, &n, &elem) != TCL_OK)
      throw Err() << "filter: broken data list: " << tcl_error(interp.get());
    d.clear();
    for (int i = 0; i < n; ++i,++elem){
      int len;
      const char* s = Tcl_GetStringFromObj(*elem, &len);
      if (!s) throw Err() << "filter: can't get string value from storage: " << tcl_error(interp.get());
      d.push_back(std::string(s, s+len));
    }
  }


  // get storage back
  auto storagec = Tcl_GetVar(interp.get(), "storage", TCL_GLOBAL_ONLY);
  storage = storagec? storagec:"";

  // Return value. If value can not be converted assume true
  int ret;
  auto ret_str = Tcl_GetString(Tcl_GetObjResult(interp.get()));
  if (!ret_str || Tcl_GetBoolean(interp.get(), ret_str, &ret) != TCL_OK)
    ret = true;

  return ret;
}

