/*  A simple program for measure time usage for a few operations.
    It's also an example of the graphene programming interface.
*/

#include <sstream>
#include <iostream>
#include <sys/time.h>

#include "gr_env.h"
#include "err/err.h"

#define DBPATH "."
#define ENV_TYPE "lock"
#define TCL_LIB "/usr/share/graphene/tcllib/"
#define READONLY false
#define DBNAME "time_test"
#define DPOLICY "replace"
#define NVAL 100000
#define NVAL_FLT 100
#define TFMT TFMT_DEF

class TimeCounter{
  struct timeval tv;

  public:

  TimeCounter() {reset();}

  void reset(){ gettimeofday(&tv, NULL); }

  double meas(){
    struct timeval tv1;
    gettimeofday(&tv1, NULL);
    ssize_t s = tv1.tv_sec - tv.tv_sec;
    ssize_t us = tv1.tv_usec - tv.tv_usec;
    return s + us*1e-6;
  }
};


int
main(){

  GrapheneEnv env(DBPATH, READONLY, ENV_TYPE, TCL_LIB);
  TimeCounter tc;

  try {


    tc.reset();
    env.dbcreate(DBNAME, "Test database", DATA_DOUBLE);
    std::cerr << "Create DB: " << tc.meas() << "\n";


    tc.reset();
    std::vector<std::string> dat(1);
    for (int i = 0; i<NVAL; i++){
      std::ostringstream st;
      std::ostringstream sd;
      st << i*0.001;
      sd << i*0.001;
      dat[0] = sd.str();
      env.put(DBNAME, st.str(), dat, DPOLICY);
    }
    std::cerr << "Put " << NVAL << " values: " << tc.meas() << "\n";


    tc.reset();
    for (int i = 0; i<NVAL; i++){
      std::ostringstream st;
      st << i*0.001;
      env.get(DBNAME, st.str(), TFMT, NULL, NULL);
    }
    std::cerr << "Get " << NVAL << " values using get(): " << tc.meas() << "\n";

    tc.reset();
    env.get_range(DBNAME, "0", "inf", "0", TFMT, NULL, NULL);
    std::cerr << "Get " << NVAL << " values using get_range(): " << tc.meas() << "\n";

    env.del_range(DBNAME, "0", "inf");

    // input filter
    env.set_filter(DBNAME, 0, "set data [expr $data**2]");

    tc.reset();
    for (int i = 0; i<NVAL_FLT; i++){
      std::ostringstream st;
      std::ostringstream sd;
      st << i*0.001;
      sd << i*0.001;
      dat[0] = sd.str();
      env.put_flt(DBNAME, st.str(), dat, DPOLICY);
    }
    std::cerr << "Put " << NVAL_FLT << " values through input filter: " << tc.meas() << "\n";

    // output filter
    env.set_filter(DBNAME, 1, "set data [expr $data**2]");

    tc.reset();
    for (int i = 0; i<NVAL; i++){
      std::ostringstream st;
      st << i*0.001;
      env.get(DBNAME ":1", st.str(), TFMT, NULL, NULL);
    }
    std::cerr << "Get " << NVAL << " values using get() w/o filter: " << tc.meas() << "\n";

    tc.reset();
    for (int i = 0; i<NVAL; i++){
      std::ostringstream st;
      st << i*0.001;
      env.get(DBNAME ":f1", st.str(), TFMT, NULL, NULL);
    }
    std::cerr << "Get " << NVAL << " values using get() through filter: " << tc.meas() << "\n";



  } catch (Err & e){
    std::cerr << "Error: " << e.str() << "\n";
  }

  env.dbremove(DBNAME);
}
