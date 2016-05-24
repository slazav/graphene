#ifndef STSDB_JSON
#define STSDB_JSON
/*  JSON interface to the Simple time series database. */

#include <string>
#include <cstdlib>
#include <stdint.h>

/* Process a JSON request to the database. */
/* Returns allocated buffer with the data, set dsize to its size */
/* Returns NULL on errors, dsize in unspecified then.*/

std::string stsdb_json(const std::string & dbpath,  /* path to databases */
                       const std::string & url,     /* /query, /annotations, etc. */
                       const std::string & data     /* input data */
                      );
#endif
