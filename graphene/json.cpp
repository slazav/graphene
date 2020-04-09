/*  JSON interface to the Graphene time series database.
    See json.h for more information.

  simple-json-datasource documentation:
  https://github.com/grafana/simple-json-datasource

*/

// grafana do not ask maxDataPoints in annotation requests,
// but we do not want to return too many annotations.
#define MAX_ANNOTATIONS 500

#include <string>
#include <sstream>
#include <iomanip>

#include <cstdlib>
#include <ctime>
#include <cstring>
#include <stdint.h>

#include "jsonxx/jsonxx.h"
#include "dbpool.h"
#include "dbgr.h"
#include "dbout.h"

using namespace std;

/* Convert string with time to <seconds>.<ms fraction>, return "" on error.
   Input format: "2016-05-02T11:20:36.356Z"
   Positions:     012345678901234567890123
*/
string convert_time(const string & tstr){
  /* check string format */
  if (tstr.size()!=24) return "";
  for (size_t i=0; i<24; i++){
    switch (i){
      case  4:
      case  7: if (tstr[i]!='-') return "";
               break;
      case 10: if (tstr[i]!='T') return "";
               break;
      case 13:
      case 16: if (tstr[i]!=':') return "";
               break;
      case 19: if (tstr[i]!='.') return "";
               break;
      case 23: if (tstr[i]!='Z') return "";
               break;
      default: if (tstr[i]<'0' || tstr[i]>'9') return "";
    }
  }
  /* parse fields */
  struct tm tt;
  tt.tm_sec  = atoi(tstr.c_str() + 17);        /* seconds */
  tt.tm_min  = atoi(tstr.c_str() + 14);        /* minutes */
  tt.tm_hour = atoi(tstr.c_str() + 11);        /* hours */
  tt.tm_mday = atoi(tstr.c_str() +  8);        /* day of the month */
  tt.tm_mon  = atoi(tstr.c_str() +  5)-1;      /* month */
  tt.tm_year = atoi(tstr.c_str() +  0)-1900;   /* year */
  tt.tm_wday  = 0;   /* day of the week */
  tt.tm_yday  = 0;   /* day in the year */
  tt.tm_isdst = 0;   /* daylight saving time */
  uint64_t ms = atoi(tstr.c_str() + 20); /* milliseconds */

  char *tz;
  tz = getenv("TZ");
  setenv("TZ", "", 1);
  tzset();
  time_t t = mktime(&tt);
  if (tz) setenv("TZ", tz, 1);
  else    unsetenv("TZ");
  tzset();
  if (t<0) return "";

  ostringstream ret;
  ret << t << "." << setw(3) << setfill('0') << ms;
  return ret.str();
}

/* Convert string with time interval to <seconds>.<ms fraction>, return "" on error.
   Input format: <integer number><suffix>, where suffix can be: ms, s, m, h, d
*/
string convert_interval(const string & tstr){
  char *e;
  uint64_t s,ms;
  uint64_t v = strtol(tstr.c_str(), &e, 10);
  if (e==NULL) return "";
  if      (strcmp(e,"ms")==0) { ms=v; s=0;}
  else if (strcmp(e,"s")==0)  { ms=0; s=v;}
  else if (strcmp(e,"m")==0)  { ms=0; s=60*v;}
  else if (strcmp(e,"h")==0)  { ms=0; s=3600*v;}
  else if (strcmp(e,"d")==0)  { ms=0; s=24*3600*v;}
  else return "";
  ostringstream ret;
  ret << s << "." << setw(3) << setfill('0') << ms;
  return ret.str();
}

/***************************************************************************/
// json data formatter
//
class DBoutJSON: public DBout{
  public:
  Json json_buffer;
  bool isnum; // numerical or text data

  DBoutJSON(const bool isnum_):
    DBout(std::cout), json_buffer(Json::array()), isnum(isnum_){
    col=0; // default
  };

  void print_point(const std::string & str) {
    if (isnum){ //read timestamp and one value from the line:
      istringstream istr(str);
      double t; // we need 1ms accuracy
      double v;
      istr >> t >> v;
      Json jpt = Json::array();
      jpt.append(v);
      jpt.append((json_int_t)(1000*t)); // convert to integer milliseconds
      json_buffer.append(jpt);
    }
    else{ // read timestamp, space character and all text from the line:
      istringstream istr(str);
      double t; // we need 1ms accuracy
      string v;
      istr >> t; istr.get(); getline(istr, v);
      Json jpt = Json::object();
      jpt.set("title", v);
      jpt.set("time",  (json_int_t)(1000*t));
      json_buffer.append(jpt);
    }
  };
};


/***************************************************************************/
// process /query
Json json_query(DBpool * pool, const Json & ji){

  /*
  /query input:
  {"panelId":3,
    "range":{"from":"2016-05-02T11:20:36.356Z","to":"2016-05-02T11:24:04.932Z"},
    "rangeRaw":{"from":"2016-05-02T11:20:36.356Z","to":"2016-05-02T11:24:04.932Z"},
    "interval":"100ms",
    "targets":[{"refId":"A","target":"t1"},{"refId":"B","target":"t2"},
    "format":"json",
    "maxDataPoints":1387
  }
  /query output:
  [ { "target":"t1", "datapoints":[ [622,1450754160000], [365,1450754220000]] },
    { "target":"t2", "datapoints":[ [861,1450754160000], [767,1450754220000]] } ]
  */
  /* parse time range */
  string t1 = convert_time( ji["range"]["from"].as_string() );
  string t2 = convert_time( ji["range"]["to"].as_string() );
  if (t1=="" || t2=="") throw Err() << "Bad range setting";

  /* check format */
  //if (ji["format"].as_string() != "json") 
  //  throw Err() << "Unknown format";

  /* parse interval */
  string dt = convert_interval( ji["interval"].as_string() );
  if (dt=="") throw Err() << "Bad interval";

  /* parse maxDataPoints */
  uint64_t maxpt = ji["maxDataPoints"].as_integer();
  if (maxpt==0) throw Err() << "Bad maxDataPoints";

  /* parse targets and run command */
  Json out = Json::array();
  for (int i=0; i<ji["targets"].size(); i++){

    // Get a database
    int col,flt;
    std::string name = parse_ext_name(ji["targets"][i]["target"].as_string(), col, flt);
    DBgr db = pool->get(name, DB_RDONLY);

    // output formatter
    DBoutJSON dbo(true);
    dbo.col = col<0 ? 0:col;
    dbo.flt = flt;

    // check DB format
    if (db.dtype == DATA_TEXT)
      throw Err() << "Can not do query from TEXT database. Use annotations";

    // Get data from the database
    db.get_range(t1,t2,dt, dbo);

    Json jt = Json::object();
    jt.set("target", ji["targets"][i]["target"]);
    jt.set("datapoints", dbo.json_buffer);
    out.append(jt);
  }

  return out;
}

/***************************************************************************/
// process /annotations
Json json_annotations(DBpool * pool, const Json & ji){

  /*
  /annotations input:
  {"range":{"from": "2016-04-15T13:44:39.070Z","to":"2016-04-15T14:44:39.070Z"},
   "rangeRaw":{"from":"now-1h","to":"now"},
   "annotation":{"name":"deploy","datasource":"Simple JSON Datasource",
                 "iconColor":"rgba(255, 96, 96, 1)","enable":true,"query":"#deploy"}
  }
  /annotations output:
  [{annotation: annotation, // The original annotation sent from Grafana.
    time:  time, // Time since UNIX Epoch in milliseconds. (required)
    title: title, // The title for the annotation tooltip. (required)
    tags:  tags, // Tags for the annotation. (optional)
    text:  text // Text for the annotation. (optional)
    }]
  */
  /* parse time range */
  string t1 = convert_time( ji["range"]["from"].as_string() );
  string t2 = convert_time( ji["range"]["to"].as_string() );
  if (t1=="" || t2=="") throw Err() << "Bad range setting";


  // Get a database
  int col,flt;
  std::string name = parse_ext_name(ji["annotation"]["name"].as_string(), col,flt);
  DBgr db = pool->get(name, DB_RDONLY);

  // output formatter
  DBoutJSON dbo(false);
  dbo.col = col<0 ? 0:col;
  dbo.flt = flt;

  // check DB format
  if (db.dtype != DATA_TEXT)
    throw Err() << "Annotations can be found only in TEXT databases";

  ostringstream ss; ss << fixed << (atof(t2.c_str())-atof(t1.c_str()))/MAX_ANNOTATIONS;

  db.get_range(t1,t2, ss.str(), dbo);
  for (size_t i=0; i<dbo.json_buffer.size(); i++){
    dbo.json_buffer[i].set("annotation", ji["annotation"]);
  }
  return dbo.json_buffer;
}

/***************************************************************************/
// process /search
Json json_search(DBpool * pool, const Json & ji){
  Json out = Json::array();
  auto names = pool->dblist();
  for (auto const & n:names)
    out.append(Json(n));
  return out;
}

/***************************************************************************/
/* Process a JSON request to the database. */
string graphene_json(DBpool * pool,
                     const string & url,     /* /query, /annotations, etc. */
                     const string & data){    /* input data */

  /* parse input JSON */
  Json ji = Json::load_string(data);
  Json jo;
  int out_fl = JSON_PRESERVE_ORDER;

  if (url == "/query")
    return json_query(pool, ji).save_string(out_fl);

  if (url == "/search")
    return json_search(pool, ji).save_string(out_fl);

  if (url == "/annotations")
    return json_annotations(pool, ji).save_string(out_fl);

  throw Err() << "Unknown query";
}
