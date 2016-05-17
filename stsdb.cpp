/*  Command-line interface for the Simple time series database.
*/

#include <cstdlib>
#include <stdint.h>
#include <cstring>
#include <cstdio>
#include <cerrno>
#include <dirent.h>
#include <sys/stat.h>

#include <map>
#include <string>
#include <iostream>
#include "stsdb.h"

using namespace std;

/**********************************************************/
/* global parameters */
class Pars{
  public:
  std::string dbpath; /* path to the databases */

  // defaults
  Pars(){
    dbpath = "/var/lib/stsdb/"; 
  }

  // print help message and exit
  void print_help(){
    Pars p; // default parameters
    cout << "stsdb -- command line interface to Simple Time Series Database\n"
            "Usage: stsdb [options] <command> <parameters>\n"
            "Options:\n"
            "  -d <path> -- database directory (default " << p.dbpath << "\n"
            "  -h        -- write this help message and exit\n"
            "Comands:\n"
            "  create <name> <time_fmt> <data_fmt> <description>\n"
            "      -- create a database\n"
            "  delete <name>\n"
            "      -- delete a database\n"
            "  rename <old_name> <new_name>\n"
            "      -- rename a database\n"
            "  set_descr <name> <description>\n"
            "      -- change database description\n"
            "  info <name>\n"
            "      -- print database information, tab-separated time format,\n"
            "         data format and description (if it is not empty)\n"
            "  list\n"
            "      -- list all databases in the data folder\n"
    ;
    throw Err();
  }

  // parse options, modify argc/argv
  void parse_cmdline_options(int *argc, char ***argv){
    /* parse  options */
    while(1){
      switch (getopt(*argc, *argv, "d:h")){
        case -1: *argc-=optind;
                 *argv+=optind;
                  optind=0;
                  return; /* end*/
        case '?':
        case ':': throw Err(); /* error msg is printed by getopt*/
        case 'd': dbpath = optarg; break;
        case 'h': print_help();
      }
    }
  }

};


/**********************************************************/
int
main(int argc, char **argv) {

  try {

    Pars p;  /* program parameters */
    p.parse_cmdline_options(&argc, &argv);

    if (argc < 1) p.print_help();
    string cmd(argv[0]);

    // create new database
    if (cmd == "create"){
      if (argc<2) throw Err() << "database name expected";
      if (argc>5) throw Err() << "too many parameters";
      string name(argv[1]);
      DBhead head(
         argc<3 ? DEFAULT_TIMEFMT : DBhead::str2timefmt(argv[2]),
         argc<4 ? DEFAULT_DATAFMT : DBhead::str2datafmt(argv[3]),
         argc<5 ? "" : argv[4] );
      DBsts db(p.dbpath, name, DB_CREATE | DB_EXCL);
      db.write_info(head);
      return 0;
    }

    // delete a database
    if (cmd == "delete"){
      if (argc<2) throw Err() << "database name expected";
      if (argc>2) throw Err() << "too many parameters";
      string name = string(argv[1]) + ".db";
      int res = remove((p.dbpath + "/" + name).c_str());
      if (res) throw Err() << name << ": " << strerror(errno);
      return 0;
    }

    // rename a database
    if (cmd == "rename"){
      if (argc<3) throw Err() << "database old and new names expected";
      if (argc>3) throw Err() << "too many parameters";
      string name1 = p.dbpath + "/" + argv[1] + ".db";
      string name2 = p.dbpath + "/" + argv[2] + ".db";
      // check if destination exists
      struct stat buf;
      int res = stat(name2.c_str(), &buf);
      if (res==0) throw Err() << "can't rename database: destination exists";
      // do rename
      res = rename(name1.c_str(), name2.c_str());
      if (res) throw Err() << "can't rename database: " << strerror(errno);
      return 0;
    }

    // change database description
    if (cmd == "set_descr"){
      if (argc<3) throw Err() << "database name and new description text expected";
      if (argc>3) throw Err() << "too many parameters";
      DBsts db(p.dbpath, argv[1], 0);
      DBhead head = db.read_info();
      head.descr = argv[2];
      db.write_info(head);
      return 0;
    }

    // print database info
    if (cmd == "info"){
      if (argc<2) throw Err() << "database name expected";
      if (argc>2) throw Err() << "too many parameters";
      DBsts db(p.dbpath, argv[1], DB_RDONLY);
      DBhead head = db.read_info();
      cout << DBhead::timefmt2str(head.key) << '\t'
           << DBhead::datafmt2str(head.val);
      if (head.descr!="") cout << '\t' << head.descr;
      cout << "\n";
      return 0;
    }

    // print database list
    if (cmd == "list"){
      if (argc>1) throw Err() << "too many parameters";
      DIR *dir = opendir(p.dbpath.c_str());
      if (!dir) throw Err() << "can't open database directory: " << strerror(errno);

      struct dirent *ent;
      while ((ent = readdir (dir)) != NULL) {
        string name(ent->d_name);
        size_t p = name.find(".db");
        if (p!=string::npos) cout << name.substr(0,p) << "\n";
      }
      closedir(dir);
      return 0;
    }

    throw Err() << "Unknown command";

  } catch(Err e){
    if (e.str()!="") cout << "Error: " << e.str() << "\n";
    return 1;
  }
}
