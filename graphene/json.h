#ifndef GRAPHENE_JSON
#define GRAPHENE_JSON
/*  JSON interface to the Graphene time series database. */

#include <string>
#include <cstdlib>
#include <stdint.h>
#include "jsonxx/jsonxx.h"

#include "gr_env.h"

/* Process a JSON request to the database. */
/* Returns allocated buffer with the data, set dsize to its size */
/* Returns NULL on errors, dsize in unspecified then.*/

std::string graphene_json(GrapheneEnv * env,
                          const std::string & url,      /* /query, /annotations, etc. */
                          const std::string & data      /* input data */
                         );
#endif
