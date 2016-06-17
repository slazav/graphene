#include <tcl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "locklib.h"

#define LOCK_WAIT_DELAY 100 /* in milliseconds */

int lock_init_cmd(ClientData clientData, Tcl_Interp *interp,
		    int objc, Tcl_Obj *CONST objv[]);
int lock_proc(ClientData clientData, Tcl_Interp *interp,
		    int objc, Tcl_Obj *CONST objv[]);

int Locktcl_Init (Tcl_Interp *interp) {
  Tcl_CreateObjCommand(interp, "lock_init", lock_init_cmd,
		       (ClientData)0, (Tcl_CmdDeleteProc*)NULL);
  Tcl_PkgProvide(interp,"Locking","1.1");
  return TCL_OK;
}

static int lockcnt = 0;

#define ct(op) if ((op) != TCL_OK) return TCL_ERROR
#define ce(op) ct(check_error((op),interp))

static int check_error(int res, Tcl_Interp *interp);

int lock_init_cmd (ClientData clientData, Tcl_Interp *interp,
		   int objc, Tcl_Obj *CONST objv[]) {
  int id;
  char namebuf[50], *pname;
  Tcl_CmdInfo cinfo;
  Tcl_Command ctoken;
  
  if (objc < 2 || objc > 3) {
    Tcl_WrongNumArgs(interp, 1, objv, "?name? key");
    return TCL_ERROR;
  }
  /*fprintf(stderr,"key=%s\n",Tcl_GetString(objv[objc-1]));*/
  ce(id = lock_init(Tcl_GetString(objv[objc-1])));
  /*fprintf(stderr,"id=%x\n",id);*/

  if (objc == 2) {
    /* generate unique name */
    do {
      snprintf(namebuf,sizeof(namebuf),"lock%d",lockcnt++);
    } while (Tcl_GetCommandInfo(interp,namebuf,&cinfo));
    pname = namebuf;
  } else {
    pname = Tcl_GetString(objv[1]);
  }

   /* Now create instance procedure */
  ctoken =
    Tcl_CreateObjCommand(interp, pname, lock_proc, (ClientData)id, NULL);
  if (ctoken==NULL) {
    Tcl_SetResult(interp,"instance creation failed",TCL_STATIC);
    return TCL_ERROR;
  }
  Tcl_SetResult(interp,pname,TCL_VOLATILE);
  return TCL_OK;
}

typedef int lock_subproc(int id, Tcl_Interp *interp,
			int objc, Tcl_Obj *CONST objv[]);

static lock_subproc do_lock_get, do_lock_wait, do_lock_release,
  do_lock_blockwait;

static struct {
  char *cmd;
  lock_subproc *proc;
} cmd_table [] = {
  {"get",do_lock_get},
  {"wait",do_lock_wait},
  {"release",do_lock_release},
  {"block",do_lock_blockwait},
  {NULL,NULL}
};

int lock_proc (ClientData clientData, Tcl_Interp *interp,
	       int objc, Tcl_Obj *CONST objv[]) {
  int id = (int) clientData;
  int cmdind;

  if (objc != 2) {
    Tcl_WrongNumArgs(interp, 1, objv, "command");
    return TCL_ERROR;
  }

  if (Tcl_GetIndexFromObjStruct(interp, objv[1], cmd_table,
			  (char*)(&cmd_table[1]) - (char*)cmd_table,
			  "command", TCL_EXACT, &cmdind) != TCL_OK) {
    return TCL_ERROR;
  }
  
  return (cmd_table[cmdind].proc)(id,interp,objc,objv);
}

static int do_lock_get(int id, Tcl_Interp *interp,
		       int objc, Tcl_Obj *CONST objv[]) {
  int res, ans;

  res = lock_get(id);
  if (res < 0) {
    /* is it error or just not avail? */
    if (errno == EAGAIN) ans = 0;
    else {
      check_error(res,interp);
      return TCL_ERROR;
    }
  } else {
    ans = 1;
  }
  Tcl_SetBooleanObj(Tcl_GetObjResult(interp),ans);
  return TCL_OK;
}

static int do_lock_blockwait(int id, Tcl_Interp *interp,
		       int objc, Tcl_Obj *CONST objv[]) {
  int res;

  while (1) {
    res = lock_wait(id);
    if (res < 0) {
      /* is it error or just interrupt? */
      if (errno != EINTR) {
	check_error(res,interp);
	return TCL_ERROR;
      }
      /* give tcl a chance still */
      while (Tcl_DoOneEvent(TCL_ALL_EVENTS|TCL_DONT_WAIT));
    } else {
      Tcl_ResetResult(interp);
      return TCL_OK;
    }
  }
}

static int do_lock_release(int id, Tcl_Interp *interp,
		       int objc, Tcl_Obj *CONST objv[]) {
  ce(lock_release(id));
  return TCL_OK;
}

static void timer_cb(ClientData cd) {
  int *donePtr = (int*) cd;
  *donePtr = 1;
}

static int do_lock_wait(int id, Tcl_Interp *interp,
		       int objc, Tcl_Obj *CONST objv[]) {
  int res, timer_done = 1;
  Tcl_TimerToken timer = NULL;

  while (1) {
    res = lock_get(id);
    if (res == 0) {
      Tcl_DeleteTimerHandler(timer); /* NULL is OK according to man page */
      Tcl_ResetResult(interp);
      return TCL_OK;
    } else {
      if (errno != EAGAIN) {
	/* some bad error */
	Tcl_DeleteTimerHandler(timer);
	check_error(res,interp);
	return TCL_ERROR;
      }
    }
    /* now we will sleep for some time and allow tcl to proceed */
    if (timer_done) {
      /* we need to start new timer */
      timer_done = 0;
      timer = Tcl_CreateTimerHandler(LOCK_WAIT_DELAY,timer_cb,
				     (ClientData) &timer_done);
    }
    /* now allow tcl to proceed */
    Tcl_DoOneEvent(TCL_ALL_EVENTS);
  }
}

static int check_error(int res, Tcl_Interp *interp) {
  if (res < 0) {
    Tcl_ResetResult(interp);
    Tcl_AppendResult(interp,"System error in lock operation: ",
		     strerror(errno),NULL);
    return TCL_ERROR;
  } else {
    return TCL_OK;
  }
}
