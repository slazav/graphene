#include <tcl.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

int daemonize(ClientData clientData, Tcl_Interp *interp,
	      int objc, Tcl_Obj *CONST objv[]) {
  Tcl_ResetResult(interp);
  if (daemon(0,0) < 0) {
    Tcl_AppendResult(interp,"Error when backgrounding: ",
		     strerror(errno),(char*)NULL);
    return TCL_ERROR;
  }
  return TCL_OK;
}

int kill_myself(ClientData clientData, Tcl_Interp *interp,
	      int objc, Tcl_Obj *CONST objv[]) {
  Tcl_ResetResult(interp);
  if (kill(getpid(),SIGTERM) < 0) {
    Tcl_AppendResult(interp,"Error when killing myself: ",
		     strerror(errno),(char*)NULL);
    return TCL_ERROR;
  }
  return TCL_OK;
}

int Daemon_Init (Tcl_Interp *interp) {
  Tcl_CreateObjCommand(interp, "daemonize", daemonize,
		       (ClientData)0, (Tcl_CmdDeleteProc*)NULL);
  Tcl_CreateObjCommand(interp, "kill_myself", kill_myself,
		       (ClientData)0, (Tcl_CmdDeleteProc*)NULL);
  return TCL_OK;
}
