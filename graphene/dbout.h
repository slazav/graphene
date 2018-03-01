/* DBout class: data output: columns, filters, tables */

#ifndef GRAPHENE_DBOUT_H
#define GRAPHENE_DBOUT_H

#include <string>
#include <errno.h>
#include <wait.h>
#include "db.h"

/***********************************************************/
// Class for an extended dataset object.
//
// Extended dataset can be just a database name, but it can also
// contain a column, a filter, and some secondary databases
//
// Data are taken from the main database, then data from secondary databases
// are interpolated (using the GET command) and added. Then the multi-column
// result is filtered through the filter.
//
// Example of the extended dataset:
//   dba:2 + dbb:3 + dbc:1 | flt
// This means that data should be found in the 2nd column of dba,
// then all data from dbb and first column of dbc are interpolated
// to the data points, then all data should go through filter program flt.
//
class DBout {
  public:
  std::string name;    // primary database name
  std::ostream & out;
  int col; // column number, for the main database

  // filter name, pid, in/out descriptors
  std::string filter;  // filter program
  pid_t pid;
  int fd1[2], fd2[2];

  // constructor -- parse the dataset string, create iostream
  DBout(const std::string & filterpath, const std::string & str, std::ostream & out):
    col(-1), name(str), out(out){

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
  ~DBout(){
    for (int i=0; i<2; i++){
      if (fd1[i]!=-1) close(fd1[i]);
      if (fd2[i]!=-1) close(fd2[i]);
    }
    int st;
    if (pid>0) waitpid(pid, &st, 0);
  }

  // Process a single point (select a column, make tables, filtering)
  // and call print_point().
  // <list> parameter changes output of TEXT records in a list mode
  // (only first line is shown).
  void proc_point(DBT *k, DBT *v, const DBinfo & info, int list=0) {
    // check for correct key size (do not parse DB info)
    if (k->size!=sizeof(uint64_t) && k->size!=sizeof(uint32_t) ) return;
    // convert DBT to strings
    std::string ks((char *)k->data, (char *)k->data+k->size);
    std::string vs((char *)v->data, (char *)v->data+v->size);
    // print values into a string (always \n in the end!)
    std::ostringstream str;
    str << info.print_time(ks) << " "
        << info.print_data(vs, col) << "\n";
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

  // print_point  -- by default it just prints the line to out,
  // but this function can be overriden.
  virtual void print_point(const std::string & str) { out << str; }
};

#endif
