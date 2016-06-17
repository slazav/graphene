#include <tcl.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

int prectimetoday(ClientData clientData, Tcl_Interp *interp,
		    int objc, Tcl_Obj *CONST objv[]);
int prectime(ClientData clientData, Tcl_Interp *interp,
		    int objc, Tcl_Obj *CONST objv[]);

int Prectime_Init (Tcl_Interp *interp) {
  Tcl_CreateObjCommand(interp, "prectimetoday", prectimetoday,
		       (ClientData)0, (Tcl_CmdDeleteProc*)NULL);
  Tcl_CreateObjCommand(interp, "prectime", prectime,
		       (ClientData)0, (Tcl_CmdDeleteProc*)NULL);
  Tcl_PkgProvide(interp,"Prectime","1.1");
  return TCL_OK;
}

int prectimetoday (ClientData clientData, Tcl_Interp *interp,
		   int objc, Tcl_Obj *CONST objv[]) {
  struct timeval tv;
  struct timezone tz;
  struct tm tm2;
  double res;
  gettimeofday(&tv,&tz);
  tm2 = *localtime(&tv.tv_sec);
  res = (double)tv.tv_usec/1e6 + 
    tm2.tm_hour*3600 + tm2.tm_min*60 + tm2.tm_sec;
  Tcl_SetObjResult(interp,Tcl_NewDoubleObj(res));
  return TCL_OK;
}

int prectime (ClientData clientData, Tcl_Interp *interp,
		   int objc, Tcl_Obj *CONST objv[]) {
  struct timeval tv;
  struct timezone tz;
  double res;
  gettimeofday(&tv,&tz);
  res = (double)tv.tv_usec/1e6 + tv.tv_sec;
  Tcl_SetObjResult(interp,Tcl_NewDoubleObj(res));
  return TCL_OK;
}
