#include <string>
#include <iostream>
#include "db.h"
#include "dbout.h"

using namespace std;

// callback for using with DBsts::get* functions
void print_value(DBT *k, DBT *v,
                 const DBinfo & info, void *usr_data){
  DBout *dbo = (DBout *)usr_data;
  // check for correct key size (do not parse DB info)
  if (k->size!=sizeof(uint64_t)) return;
  // convert DBT to strings
  string ks((char *)k->data, (char *)k->data+k->size);
  string vs((char *)v->data, (char *)v->data+v->size);
  // unpack and print values
  cout << info.unpack_time(ks) << " "
            << info.unpack_data(vs, dbo->col) << "\n";
}
