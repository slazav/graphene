#include "db.h"
#include "dbout.h"

#include <string>
#include <iomanip>
#include <iostream>
#include <errno.h>
#include <wait.h>

DBout::DBout(const std::string & filterpath,
      const std::string & str,
      std::ostream & out,
      const std::string & time0):
  col(-1), name(str), out(out), time0(time0){

  // extract filter
  size_t cp = name.rfind('|');
  if (cp!=std::string::npos){
    filter = name.substr(cp+1,-1);
    name = name.substr(0,cp);
  }

  // extract column
  cp = name.rfind(':');
  if (cp!=std::string::npos){
    char *e;
    col = strtol(name.substr(cp+1,-1).c_str(), &e, 10);
    if (e!=NULL && *e=='\0') name = name.substr(0,cp);
    else col = -1;
  }
  if (col < -1) col = -1;
  name   = check_name(name);
  filter = check_name(filter);

  fd1[0]=fd1[1]=fd2[0]=fd2[1]=-1;
  pid = -1;
  if (filter!=""){
    // two pipes and a fork
    if (pipe(fd1)!=0 || pipe(fd2)!=0) throw Err() << "can't create pipes";
    pid = fork();
    if (pid < 0) throw Err() << "can't do fork";

    // in the child process we set redirect pipe to stdin
    // and run filter program
    if (pid == 0) {
      if (dup2(fd1[0], 0)!=0 || close(fd1[0]) != 0 || close(fd1[1]) != 0)
        throw Err() << "can't redirect stdin";
      if (dup2(fd2[1], 1)!=1 || close(fd2[0]) != 0 || close(fd2[1]) != 0)
        throw Err() << "can't redirect stdout";
      std::string f = filterpath + "/" + filter;
      execl(f.c_str(), f.c_str(), NULL);
      exit(0); // how to terminate process correctly?!
    }
    close(fd1[0]); fd1[0] = -1;
    close(fd2[1]); fd2[1] = -1;
  }
}


DBout::~DBout(){
  for (int i=0; i<2; i++){
    if (fd1[i]!=-1) close(fd1[i]);
    if (fd2[i]!=-1) close(fd2[i]);
  }
  int st;
  if (pid>0) waitpid(pid, &st, 0);
}


void
DBout::proc_point(DBT *k, DBT *v, const DBinfo & info, int list) {
  // check for correct key size (do not parse DB info)
  if (k->size!=sizeof(uint64_t) && k->size!=sizeof(uint32_t) ) return;
  // convert DBT to strings
  std::string ks((char *)k->data, (char *)k->data+k->size);
  std::string vs((char *)v->data, (char *)v->data+v->size);
  // print values into a string (always \n in the end!)
  std::ostringstream str;

  if (time0.size() == 0) str << info.print_time(ks); // absolute timestamp
  else str << std::fixed << std::setprecision(9) << info.time_diff(ks, info.parse_time(time0)); // relative time
  str << " " << info.print_data(vs, col) << "\n"; // data
  std::string s = str.str();

  // keep only first line (s always ends with \n - see above)
 if (list==1 && info.val==DATA_TEXT)
    s.resize(s.find('\n')+1);

  // do filtering
  if (pid>0){
    write(fd1[1], s.data(), s.length());
    char buf[256];
    size_t n;
    std::string out;
    while ((n = read(fd2[0], buf, sizeof(buf)))>0){
      out+=std::string(buf, buf+n);
      if (out.find('\n')!=std::string::npos) break;
    }
    print_point(out);
  }
  else{
    print_point(s);
  }
};


void
DBout::print_point(const std::string & str) { out << str; }
