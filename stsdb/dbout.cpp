#include <string>
#include <iostream>
#include "db.h"
#include "dbout.h"

using namespace std;

// callback for using with DBsts::get* functions
void print_value(DBT *k, DBT *v, const int col,
                 const DBinfo & info){
  // check for correct key size (do not parse DB info)
  if (k->size!=sizeof(uint64_t)) return;
  // convert DBT to strings
  string ks((char *)k->data, (char *)k->data+k->size);
  string vs((char *)v->data, (char *)v->data+v->size);
  // unpack and print values
  cout << info.unpack_time(ks) << " "
            << info.unpack_data(vs, col) << "\n";
}
