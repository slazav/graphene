#include <tcl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include <float.h>
#include <gpib/ib.h>

int gpib_device_cmd(ClientData clientData, Tcl_Interp *interp,
		    int objc, Tcl_Obj *CONST objv[]);
int gpib_device_proc(ClientData clientData, Tcl_Interp *interp,
		    int objc, Tcl_Obj *CONST objv[]);
int gpib_delete_cmd(ClientData clientData, Tcl_Interp *interp,
		    int objc, Tcl_Obj *CONST objv[]);
void gpib_device_delete_proc(ClientData clientData);

int Gpib_Init (Tcl_Interp *interp) {
  Tcl_CreateObjCommand(interp, "gpib_device", gpib_device_cmd,
		       (ClientData)0, (Tcl_CmdDeleteProc*)NULL);
  Tcl_PkgProvide(interp,"GpibLib","1.3.2");
  return TCL_OK;
}

typedef struct {
  int handle;
  int board;
  int status;
  int errcode;
  int count;
  size_t buflen;
  char* buffer;
  unsigned int magic;
  long errmask;
  int spollmask, spollres;
  double ready_beg, ready_step, ready_max;
  Tcl_Obj *procname;
  Tcl_Command proctoken;
  char *trimleft;
  char *trimright;
} device;
 

#define ct(op) if ((op) != TCL_OK) return TCL_ERROR

static char* error_text(int ecode);

typedef int device_proc(device* dp, Tcl_Interp *interp,
			int objc, Tcl_Obj *CONST objv[]);

static device_proc process_options, get_option;

#define GOOD_MAGIC 0xd0a45781

int gpib_device_cmd (ClientData clientData, Tcl_Interp *interp,
		   int objc, Tcl_Obj *CONST objv[]) {
  char *name=NULL, *s;
  int address=-1, board=-1;
  int i;
  int devnum, status=0, errcode=0;
  device *dp;
  static int devcounter = 0;
  char namebuf[50];

  Tcl_CmdInfo cinfo;

  /* check if name is provided */
  if (objc > 1) {
    s = Tcl_GetString(objv[1]);
    if (s[0] != '-') {
      name = s;
      objc--; objv++;
    }
  }

  /* check whether we are called to delete devices */
  if (name && strcmp(name,"delete") == 0) {
    return gpib_delete_cmd(clientData,interp,objc,objv);
  }

  /* check that we have enough and even number of options */
  if (objc < 3 || objc % 2 == 0) {
    Tcl_WrongNumArgs(interp, 0, objv, "gpib_device ?name? -address addr ?-option value ...?\ngpib_device delete name ?name ...?");
    return TCL_ERROR;
  }

  /* look for -address and -board options */
  for (i=1; i<objc; i+=2) {
    s = Tcl_GetString(objv[i]);
    if (strcmp(s,"-address") == 0) {
      ct(Tcl_GetIntFromObj(interp,objv[i+1],&address));
    } else if (strcmp(s,"-board") == 0) {
      ct(Tcl_GetIntFromObj(interp,objv[i+1],&board));
    }
  }

  if (address < 0 && board < 0) {
    Tcl_SetResult(interp,"Either -address or -board should be provided for gpib_device.", TCL_STATIC);
    return TCL_ERROR;
  }

  if (address < 0) { /* user wants a board descriptor */
    devnum = board;
  } else { /* open device */
    if (board < 0) board = 0; /* default board */
    devnum = ibdev(board, address, 0, T3s, 1, 0); /* device with default
						     settings */ 
    status = ibsta; errcode = iberr;
    /*fprintf(stderr,"devnum = %d, err = %d\n",devnum, status & ERR);*/

    if (status & ERR) {
      Tcl_SetResult(interp,error_text(errcode),TCL_STATIC);
      ibonl(devnum,0);
      return TCL_ERROR;
    }
  }

  /* now allocate device structure */
  dp = (device*) Tcl_Alloc(sizeof(device));
  memset(dp,0,sizeof(device));
  dp->handle = devnum;
  dp->board = board;
  dp->status = status;
  dp->errcode = errcode;
  dp->magic = GOOD_MAGIC;

  dp->ready_beg = 0;
  dp->ready_step = 0.1;
  dp->ready_max = 100;

  /* allocate default buffer */
  dp->buflen = 500;
  dp->buffer = Tcl_Alloc(dp->buflen);

  /* process the rest of the options */
  ct(process_options(dp,interp,objc,objv));

  if (name == NULL) {
    /* generate unique name */
    do {
      snprintf(namebuf,sizeof(namebuf),"gpib_device%d",devcounter++);
    } while (Tcl_GetCommandInfo(interp,namebuf,&cinfo));
    name = namebuf;
  } else {
    /* check that the provided name is unique */
    if (Tcl_GetCommandInfo(interp,name,&cinfo)) {
      Tcl_ResetResult(interp);
      Tcl_AppendResult(interp,"supplied gpib_device name '",
		       name,"' already exists", (char*)NULL);
      return TCL_ERROR;
    }
  }


  /* Now create instance procedure */
  dp->proctoken =
    Tcl_CreateObjCommand(interp, name, gpib_device_proc,
			 (ClientData)dp, gpib_device_delete_proc);
  if (dp->proctoken==NULL) {
    Tcl_SetResult(interp,"instance creation failed",TCL_STATIC);
    /* deallocate storage */
    gpib_device_delete_proc((ClientData)dp);
    return TCL_ERROR;
  }
  dp->procname = Tcl_NewStringObj(name,-1);
  Tcl_IncrRefCount(dp->procname);

  Tcl_SetResult(interp,name,TCL_VOLATILE);
  return TCL_OK;
}

int gpib_delete_cmd (ClientData clientData, Tcl_Interp *interp,
		   int objc, Tcl_Obj *CONST objv[]) {
  int i,k,err=0;
  Tcl_CmdInfo q;
  for (i=1; i<objc; i++) {
    /* for each argument check that command exists and that
       it is indeed a gpib device */
    k = Tcl_GetCommandInfo(interp,Tcl_GetString(objv[i]),&q);
    if (k == 0 || q.objProc != gpib_device_proc) {
      err = 1;
    } else {
      ct(Tcl_DeleteCommandFromToken(interp,((device*)(q.objClientData))->proctoken));
    }
  }
  if (err) {
    Tcl_SetResult(interp,"Non-gpib device(s) in the call to 'gpib_device delete'.",TCL_STATIC);
    return TCL_ERROR;
  }
  return TCL_OK;
}

void gpib_device_delete_proc(ClientData clientData) {
  device *dp = (device*) clientData;
  /*fprintf(stderr,"deleting device %d\n",dp->handle);*/
  ibonl(dp->handle,0);
  if (dp->buffer != NULL) Tcl_Free(dp->buffer);
  Tcl_DecrRefCount(dp->procname);
  if (dp->trimleft != NULL) free(dp->trimleft);
  if (dp->trimright != NULL) free(dp->trimright);
  dp->magic = 0;
  Tcl_Free((char*)dp);
}

static int checkerror(device *dp, Tcl_Interp *interp) {
  dp->errcode = iberr;
  dp->count = ibcntl;
  if (dp->status & ERR && (1<<dp->errcode) & ~dp->errmask) {
    Tcl_SetResult(interp,error_text(dp->errcode),TCL_STATIC);
    return TCL_ERROR;
  }
  return TCL_OK;
}

static CONST char *gpib_options[] = {
  "-timeout", "-eot", "-secondary", "-eos",
  "-bufferlen","-address","-board",
  "-trimleft","-trimright","-readymask","-waitready",
  NULL
};
enum gpib_options {
  O_TIMEOUT, O_EOT, O_SECONDARY, O_EOS,
  O_BUFFERLEN, O_ADDRESS, O_BOARD,
  O_TRIMLEFT, O_TRIMRIGHT, O_READYMASK, O_WAITREADY,
};
static double tmo_table[] = {3000, 10e-6, 30e-6, 100e-6, 300e-6,
			     1e-3, 3e-3, 10e-3, 30e-3, 100e-3, 300e-3,
			     1, 3, 10, 30, 100, 300, 1000};

static int process_options(device *dp, Tcl_Interp *interp,
		int objc, Tcl_Obj *CONST objv[]) {
  int i, index;
  Tcl_Obj *po;
  
  if (objc % 2 == 0) {
    Tcl_SetResult(interp,"gpib device option without value",TCL_STATIC);
    return TCL_ERROR;
  }
  
  for (i=1; i<objc; i+=2) {
    if (Tcl_GetIndexFromObj(interp, objv[i], gpib_options, "option", 0,
			    &index) != TCL_OK) {
      return TCL_ERROR;
    }
    po = objv[i+1];
    switch ((enum gpib_options) index) {
    case O_TIMEOUT: {
      /* convert timeout in seconds to lib constant */
      double reqtmo;
      int k = 0;
      ct(Tcl_GetDoubleFromObj(interp,po,&reqtmo));
      for (k = 1; k <= T1000s; k++) {
	if (tmo_table[k] >= reqtmo) break;
      }
      dp->status = ibtmo(dp->handle,k);
      ct(checkerror(dp,interp));
      break;
    }
    case O_EOT: {
      int opt;
      ct(Tcl_GetBooleanFromObj(interp,po,&opt));
      dp->status = ibeot(dp->handle,opt);
      ct(checkerror(dp,interp));
      break;
    }
    case O_SECONDARY: {
      int opt;
      ct(Tcl_GetIntFromObj(interp,po,&opt));
      dp->status = ibsad(dp->handle,opt);
      ct(checkerror(dp,interp));
      break;
    }
    case O_EOS: {
      /* eos parameter is a list. 1st element is a string (first
	 char is used as eos, others are options: read, write, bin */
      int ll, opt=0, slen, j;
      Tcl_Obj *le;
      char *s;
      ct(Tcl_ListObjLength(interp,po,&ll));
      if (ll > 0) {
	ct(Tcl_ListObjIndex(interp,po,0,&le));
	s=Tcl_GetStringFromObj(le,&slen);
	if (slen > 0) opt = s[0] & 0x00ff;
	for (j=1; j<ll; j++) {
	  ct(Tcl_ListObjIndex(interp,po,j,&le));
	  s=Tcl_GetStringFromObj(le,&slen);
	  if (slen==4 && strcmp(s,"read")==0) opt |= REOS;
	  else if (slen==5 && strcmp(s,"write")==0) opt |= XEOS;
	  else if (slen==3 && strcmp(s,"bin")==0) opt |= BIN;
	  else if (slen==4 && strcmp(s,"none")==0) opt &= 0x00ff;
	  else  {
	    Tcl_SetResult(interp,"Bad eos mode. Sould be read, write, bin, none.", TCL_STATIC);
	    return TCL_ERROR;
	  }
	}
      }
      dp->status = ibeos(dp->handle,opt);
      ct(checkerror(dp,interp));
      break;
    }
    case O_BUFFERLEN: {
      int opt;
      ct(Tcl_GetIntFromObj(interp,po,&opt));
      dp->buffer = Tcl_Realloc(dp->buffer,opt);
      dp->buflen = opt;
      break;
    }
    case O_TRIMLEFT: {
      char *s;
      int slen;
      s = Tcl_GetStringFromObj(po,&slen);
      if (dp->trimleft) free(dp->trimleft);
      dp->trimleft = NULL;
      if (slen > 0) dp->trimleft = strdup(s);
      break;
    }
    case O_TRIMRIGHT: {
      char *s;
      int slen;
      s = Tcl_GetStringFromObj(po,&slen);
      if (dp->trimright) free(dp->trimright);
      dp->trimright = NULL;
      if (slen > 0) dp->trimright = strdup(s);
      break;
    }
    case O_READYMASK: {
      int ll;
      Tcl_Obj *le;
      int d;
      ct(Tcl_ListObjLength(interp,po,&ll));
      if (ll == 0) {
	dp->spollmask = dp->spollres = 0;
      } else {
	ct(Tcl_ListObjIndex(interp,po,0,&le));
	ct(Tcl_GetIntFromObj(interp,le,&d));
	dp->spollmask = d;
	if (ll == 1) {
	  dp->spollres = d;
	} else {
	  ct(Tcl_ListObjIndex(interp,po,1,&le));
	  ct(Tcl_GetIntFromObj(interp,le,&d));
	  dp->spollres = d;
	}
      }
      break;
    }
    case O_WAITREADY: {
      int ll;
      Tcl_Obj *le;
      double d;
      ct(Tcl_ListObjLength(interp,po,&ll));
      if (ll < 1 || ll > 3) {
	Tcl_SetResult(interp,
		      "usage: gpib_device ... -waitready {step ?min? ?max?}",
		      TCL_STATIC);
	return TCL_ERROR;
      }
      ct(Tcl_ListObjIndex(interp,po,0,&le));
      ct(Tcl_GetDoubleFromObj(interp,le,&d));
      dp->ready_step = d;
      if (ll > 1) {
	ct(Tcl_ListObjIndex(interp,po,1,&le));
	ct(Tcl_GetDoubleFromObj(interp,le,&d));
	dp->ready_beg = d;
      }
      if (ll > 2) {
	ct(Tcl_ListObjIndex(interp,po,2,&le));
	ct(Tcl_GetDoubleFromObj(interp,le,&d));
	dp->ready_max = d;
      }
      break;
    }
    case O_ADDRESS:
    case O_BOARD:
      /* do nothing */
      break;
    }
  }

  return TCL_OK;
}

static int get_option(device *dp, Tcl_Interp *interp,
		      int objc, Tcl_Obj *CONST objv[]) {
  int index;
  Tcl_Obj *ro;
  if (Tcl_GetIndexFromObj(interp, objv[1], gpib_options, "option", 0,
			  &index) != TCL_OK) {
    return TCL_ERROR;
  }
  ro = Tcl_GetObjResult(interp);
  switch ((enum gpib_options) index) {
  case O_TIMEOUT: {
    int res;
    dp->status = ibask(dp->handle,IbaTMO,&res);
    ct(checkerror(dp,interp));
    Tcl_SetDoubleObj(ro,tmo_table[res]);
    break;
  }
  case O_EOT: {
    int res;
    dp->status = ibask(dp->handle,IbaEOT,&res);
    ct(checkerror(dp,interp));
    Tcl_SetIntObj(ro,res);
    break;
  }
  case O_SECONDARY: {
    int res;
    dp->status = ibask(dp->handle,IbaSAD,&res);
    ct(checkerror(dp,interp));
    Tcl_SetIntObj(ro,res);
    break;
  }
  case O_BUFFERLEN: {
    Tcl_SetIntObj(ro,dp->buflen);
    break;
  }
  case O_ADDRESS: {
    int res;
    dp->status = ibask(dp->handle,IbaPAD,&res);
    ct(checkerror(dp,interp));
    Tcl_SetIntObj(ro,res);
    break;
  }
  case O_BOARD: {
    int res;
    dp->status = ibask(dp->handle,IbaBNA,&res);
    ct(checkerror(dp,interp));
    Tcl_SetIntObj(ro,res);
    break;
  }
  case O_TRIMLEFT: {
    if (dp->trimleft) {
      Tcl_SetStringObj(ro,dp->trimleft,-1);
    }
    break;
  }
  case O_TRIMRIGHT: {
    if (dp->trimright) {
      Tcl_SetStringObj(ro,dp->trimright,-1);
    }
    break;
  }
  case O_EOS:
  case O_READYMASK:
  case O_WAITREADY:
    Tcl_SetStringObj(ro,"Not implemented",-1);
    return TCL_ERROR;
  }
  return TCL_OK;
}

static device_proc device_read, device_write, device_remote, device_cmd_read,
  device_sleep, device_spoll, device_ready, device_waitready,
  device_cmd_wait_read, device_clear, device_go_to_local, device_trigger,
  device_write_list, device_bus_command, device_waitcond;

static struct {
  char *cmd;
  device_proc *proc;
  int nargmin, nargmax;
  char *errmsg;
} cmd_table [] = {
  {"read", device_read, 0, 0, ""},
  {"write", device_write, 1, 1, "string"},
  {"cmd_read", device_cmd_read, 1, 1, "string"},
  {"remote_enable", device_remote, 0, 1, "?state?"},
  {"sleep",device_sleep,1,1,"delay"},
  {"configure",process_options,0,INT_MAX,"-option value ?-option value ...?"},
  {"cget",get_option,1,1,"-option"},
  {"poll",device_spoll,0,1,"?mask?"},
  {"ready",device_ready,0,0,""},
  {"waitready",device_waitready,0,0,""},
  {"cmd_wait_read", device_cmd_wait_read, 1, 1, "string"},
  {"clear",device_clear,0,0,""},
  {"go_to_local",device_go_to_local,0,0,""},
  {"trigger",device_trigger,0,0,""},
  {"write_list",device_write_list,1,2,"list_of_strings ?delay?"},
  {"bus_command", device_bus_command, 1, 1, "string"},
  {"waitcond",device_waitcond,1,1,"list_of_condition_names"},
  {NULL, NULL,0,0,NULL}
};

int gpib_device_proc (ClientData clientData, Tcl_Interp *interp,
		      int objc, Tcl_Obj *CONST objv[]) {
  device *dp = (device*) clientData;
  int cmdind;

  if (objc < 2) {
    Tcl_WrongNumArgs(interp, 1, objv, "command ?arg ...?");
    goto errexit;
  }

  if (Tcl_GetIndexFromObjStruct(interp, objv[1], cmd_table,
			  (char*)(&cmd_table[1]) - (char*)cmd_table,
			  "command", TCL_EXACT, &cmdind) != TCL_OK) {
    goto errexit;
  }

#if 0
  {int i;
  for (i=0;i<objc;i++) fprintf(stderr,"%d: %s\n",i,
			       Tcl_GetString(objv[i]));}
#endif
  if (objc-2 < cmd_table[cmdind].nargmin ||
      objc-2 > cmd_table[cmdind].nargmax) {
    Tcl_WrongNumArgs(interp, 2, objv, cmd_table[cmdind].errmsg);
    goto errexit;
  }
    
  if ( (cmd_table[cmdind].proc)(dp,interp,objc-1,objv+1) != TCL_OK )
    goto errexit;
  return TCL_OK;
 errexit: {
    char buf[50];
    int pad=-1, sad=-1;
    ibask(dp->handle,IbaPAD,&pad);
    ibask(dp->handle,IbaSAD,&sad);
    snprintf(buf,50," <%d:%d:%d>",dp->board,pad,sad);
    Tcl_AppendResult(interp,"\nError for gpib_device ",Tcl_GetString(objv[0]),buf,NULL);
    if (objc >= 2)
      Tcl_AppendResult(interp,", command ",Tcl_GetString(objv[1]),NULL);
    return TCL_ERROR;
  }
}

static char* do_trim(device* dp, int* newlen) {
  char *p=dp->buffer, *q;
  int len=dp->count;
  if (dp->trimleft) for (; len>0 && strchr(dp->trimleft,*p); p++, len--); 
  if (dp->trimright) 
    for (q=dp->buffer+dp->count-1; q>=p && strchr(dp->trimright,*q);
	 *q--='\0', len--);
  if (newlen) *newlen=len;
  return p;
}

static int device_read(device *dp, Tcl_Interp *interp,
		int objc, Tcl_Obj *CONST objv[]) {
  char *s;
  int len;
  dp->status = ibrd(dp->handle,dp->buffer,dp->buflen);
  ct(checkerror(dp,interp));
  dp->buffer[dp->count] = '\0';
  s = do_trim(dp,&len);
  Tcl_SetObjResult(interp,Tcl_NewStringObj(s,len));
  return TCL_OK;
}
  
static int device_write(device *dp, Tcl_Interp *interp,
		int objc, Tcl_Obj *CONST objv[]) {
  char *s;
  int len;
  s = Tcl_GetStringFromObj(objv[1],&len);
  if (s == NULL || len == 0) return TCL_OK;
  /*fprintf(stderr,"%d:|%s|\n",dp->handle,s);*/
  dp->status = ibwrt(dp->handle,s,len);
  ct(checkerror(dp,interp));
  return TCL_OK;
}

static CONST char *condition_names[] = {
  "DCAS", "DTAS", "LACS", "TACS",
  "ATN","CIC","REM","LOK",
  "RQS","SRQI","END","TIMO","ERR",
  NULL
};


static int device_waitcond(device *dp, Tcl_Interp *interp,
		int objc, Tcl_Obj *CONST objv[]) {
  int i, n, mask = 0, id;
  Tcl_Obj *el;
  ct(Tcl_ListObjLength(interp,objv[1],&n));
  for (i=0; i<n; i++) {
    ct(Tcl_ListObjIndex(interp,objv[1],i,&el));
    if (Tcl_GetIndexFromObj(interp, el, condition_names, "condition name",
			    TCL_EXACT, &id) != TCL_OK) {
      return TCL_ERROR;
    }
    if (id > 7) id += 3;
    mask |= (1 << id);
  }
  /*fprintf(stderr,"%d:|%d|\n",dp->handle,mask);*/
  dp->status = ibwait(dp->handle,mask);
  ct(checkerror(dp,interp));
  return TCL_OK;  
}

static int device_bus_command(device *dp, Tcl_Interp *interp,
		int objc, Tcl_Obj *CONST objv[]) {
  char *s;
  int len;
  s = Tcl_GetStringFromObj(objv[1],&len);
  if (s == NULL || len == 0) return TCL_OK;
  /*fprintf(stderr,"%d:|%s|\n",dp->board,s);*/
  dp->status = ibcmd(dp->board,s,len);
  ct(checkerror(dp,interp));
  return TCL_OK;
}

static int device_cmd_read(device *dp, Tcl_Interp *interp,
		int objc, Tcl_Obj *CONST objv[]) {
  /* write command */
  char *s;
  int len;
  s = Tcl_GetStringFromObj(objv[1],&len);
  if (s == NULL || len == 0) return TCL_OK; 
  /*fprintf(stderr,"%d:|%s|\n",dp->handle,s);*/
  dp->status = ibwrt(dp->handle,s,len);
  ct(checkerror(dp,interp));
  /* read response */
  dp->status = ibrd(dp->handle,dp->buffer,dp->buflen);
  ct(checkerror(dp,interp));
  dp->buffer[dp->count] = '\0';
  s = do_trim(dp,&len);
  Tcl_SetObjResult(interp,Tcl_NewStringObj(s,len));
  return TCL_OK;
}

static int device_remote(device *dp, Tcl_Interp *interp,
		int objc, Tcl_Obj *CONST objv[]) {
  int flag = 1;
  if (objc > 1) {
    ct(Tcl_GetBooleanFromObj(interp,objv[1],&flag));
  }
  dp->status = ibsre(dp->board,flag);
  ct(checkerror(dp,interp));
  return TCL_OK;
}

static void do_sleep_cb(ClientData cd) {
  int *donePtr = (int*) cd;
  *donePtr = 1;
}

static int do_sleep(device *dp, Tcl_Interp *interp, double delay) {
  /* modelled after Tcl_VwaitObjCmd */
  int msdelay, done=0, foundEvent=1;
  Tcl_TimerToken timer;
  msdelay = ceil((delay-DBL_EPSILON)*1000);
  if (msdelay <= 0) return TCL_OK;
  timer = Tcl_CreateTimerHandler(msdelay,do_sleep_cb,(ClientData) &done);
  while (!done && foundEvent) {
    foundEvent = Tcl_DoOneEvent(TCL_ALL_EVENTS);
    if (dp->magic != GOOD_MAGIC) {
      Tcl_DeleteTimerHandler(timer);
      Tcl_SetResult(interp,"gpib device is deleted during waiting!",
		    TCL_STATIC);
      return TCL_ERROR;
    }
  }
  Tcl_ResetResult(interp);
  if (!foundEvent) {
    Tcl_SetResult(interp,"would wait forever",TCL_STATIC);
    return TCL_ERROR;
  }
  return TCL_OK;
}

static int device_sleep(device *dp, Tcl_Interp *interp,
		int objc, Tcl_Obj *CONST objv[]) {
  double delay;
  ct(Tcl_GetDoubleFromObj(interp,objv[1],&delay));
  return do_sleep(dp,interp,delay);
}

static int device_spoll(device *dp, Tcl_Interp *interp,
		int objc, Tcl_Obj *CONST objv[]) {
  char res;
  int mask = 0xff;
  if (objc > 1) {
    ct(Tcl_GetIntFromObj(interp,objv[1],&mask));
  }
  dp->status = ibrsp(dp->handle,&res);
  ct(checkerror(dp,interp));
  Tcl_SetIntObj(Tcl_GetObjResult(interp),res & mask);
  return TCL_OK;
}

static int spoll_ready(device *dp, Tcl_Interp *interp, int *res) {
  char spres;
  dp->status = ibrsp(dp->handle,&spres);
  ct(checkerror(dp,interp));
  *res = (spres & dp->spollmask) == dp->spollres;
  return TCL_OK;
}

static int device_ready(device *dp, Tcl_Interp *interp,
		int objc, Tcl_Obj *CONST objv[]) {
  int res;
  ct(spoll_ready(dp,interp,&res));
  Tcl_SetIntObj(Tcl_GetObjResult(interp),res);
  return TCL_OK;
}

static int spoll_waitready(device *dp, Tcl_Interp *interp) {
  double total = 0;
  int ready;
  if (dp->ready_beg > 0) {
    ct(do_sleep(dp,interp,dp->ready_beg));
    total = dp->ready_beg;
  }
  ct(spoll_ready(dp,interp,&ready));
  while (! ready && (total+=dp->ready_step) <= dp->ready_max) {
    ct(do_sleep(dp,interp,dp->ready_step));
    ct(spoll_ready(dp,interp,&ready));
  }
  if (ready) return TCL_OK;
  Tcl_SetResult(interp,"GPIB: Maximum waiting time exceeded",TCL_STATIC);
  return TCL_ERROR;
}

static int device_waitready(device *dp, Tcl_Interp *interp,
		int objc, Tcl_Obj *CONST objv[]) {
  ct(spoll_waitready(dp,interp));
  Tcl_ResetResult(interp);
  return TCL_OK;
}

static int device_cmd_wait_read(device *dp, Tcl_Interp *interp,
		int objc, Tcl_Obj *CONST objv[]) {
  /* write command */
  char *s;
  int len;
  s = Tcl_GetStringFromObj(objv[1],&len);
  if (s == NULL || len == 0) return TCL_OK; 
  /*fprintf(stderr,"%d:|%s|\n",dp->handle,s);*/
  dp->status = ibwrt(dp->handle,s,len);
  ct(checkerror(dp,interp));
  /* wait ready */
  ct(spoll_waitready(dp,interp));
  /* read response */
  dp->status = ibrd(dp->handle,dp->buffer,dp->buflen);
  ct(checkerror(dp,interp));
  dp->buffer[dp->count] = '\0';
  s = do_trim(dp,&len);
  Tcl_SetObjResult(interp,Tcl_NewStringObj(s,len));
  return TCL_OK;
}

static int device_clear(device *dp, Tcl_Interp *interp,
		int objc, Tcl_Obj *CONST objv[]) {
  dp->status = ibclr(dp->handle);
  ct(checkerror(dp,interp));
  return TCL_OK;
}

static int device_go_to_local(device *dp, Tcl_Interp *interp,
		int objc, Tcl_Obj *CONST objv[]) {
  dp->status = ibloc(dp->handle);
  ct(checkerror(dp,interp));
  return TCL_OK;
}

static int device_trigger(device *dp, Tcl_Interp *interp,
		int objc, Tcl_Obj *CONST objv[]) {
  dp->status = ibtrg(dp->handle);
  ct(checkerror(dp,interp));
  return TCL_OK;
}

static int device_write_list(device *dp, Tcl_Interp *interp,
		int objc, Tcl_Obj *CONST objv[]) {
  double delay = dp->ready_step;
  int i, n, len;
  Tcl_Obj *wstr;
  char *s;
  if (objc > 2) {
    ct(Tcl_GetDoubleFromObj(interp,objv[2],&delay));
  }
  ct(Tcl_ListObjLength(interp,objv[1],&n));
  for (i=0; i<n; i++) {
    if (i > 0 && delay > 0) {
      ct(do_sleep(dp,interp,delay));
    }
    ct(Tcl_ListObjIndex(interp,objv[1],i,&wstr));
    s = Tcl_GetStringFromObj(wstr,&len);
    if (s == NULL || len == 0) continue; 
    /*fprintf(stderr,"%d:|%s|\n",dp->handle,s);*/
    dp->status = ibwrt(dp->handle,s,len);
    ct(checkerror(dp,interp));
  }
  return TCL_OK;
}

static struct errdata_t {
  int code;
  char *name;
  char *desc;
} errdata[] = {
  {EDVR,"EDVR", "GPIB: System error"},
  {ECIC,"ECIC", "GPIB: Board is not a CIC"},
  {ENOL,"ENOL", "GPIB: No listeners"},
  {EADR,"EADR", "GPIB: Board is not addressed correctly"},
  {EARG,"EARG", "GPIB: Invalid argument"},
  {ESAC,"ESAC", "GPIB: Board is not a system controller"},
  {EABO,"EABO", "GPIB: I/O operation aborted (timeout)"},
  {ENEB,"ENEB", "GPIB: Board does not exists"},
  {EDMA,"EDMA", "GPIB: DMA error"},
  {EOIP,"EOIP", "GPIB: Asynchronous I/O in progress"},
  {ECAP,"ECAP", "GPIB: No capability for operation"},
  {EFSO,"EFSO", "GPIB: File system error"},
  {EBUS,"EBUS", "GPIB: Bus error"},
  {ESTB,"ESTB", "GPIB: Serial poll queue overflow"},
  {ESRQ,"ESQR", "GPIB: SRQ stuck in ON position"},
  {ETAB,"ETAB", "GPIB: Table problem"},
  {-1,NULL,NULL}
};

static char* error_text(int ecode) {
  int i;
  for (i=0; errdata[i].name; i++)
    if (errdata[i].code == ecode) return errdata[i].desc;
  return "GPIB: Unknown error";
}
