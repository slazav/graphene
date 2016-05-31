/*  JSON interface to the Simple time series database.
    See json.h for more information.

  simple-json-datasource documentation:
  https://github.com/grafana/simple-json-datasource

*/

#include <string>

#include <cstdlib>
#include <ctime>
#include <cstring>
#include <stdint.h>
#include <errno.h>
#include "jsonxx/jsonxx.h"
#include "db.h"
#include "dbout.h"

using namespace std;

/* Convert string with time to milliseconds, return 0 on error.
   Input format: "2016-05-02T11:20:36.356Z"
                  012345678901234567890123
*/
uint64_t convert_time(const string & tstr){
  /* check string format */
  if (tstr.size()!=24) return 0;
  for (size_t i=0; i<24; i++){
    switch (i){
      case  4:
      case  7: if (tstr[i]!='-') return 0;
               break;
      case 10: if (tstr[i]!='T') return 0;
               break;
      case 13:
      case 16: if (tstr[i]!=':') return 0;
               break;
      case 19: if (tstr[i]!='.') return 0;
               break;
      case 23: if (tstr[i]!='Z') return 0;
               break;
      default: if (tstr[i]<'0' || tstr[i]>'9') return 0;
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
  uint64_t ret = atoi(tstr.c_str() + 20); /* milliseconds */

  char *tz;
  tz = getenv("TZ");
  setenv("TZ", "", 1);
  tzset();
  time_t t = mktime(&tt);
  if (tz) setenv("TZ", tz, 1);
  else    unsetenv("TZ");
  tzset();
  if (t<0) return 0;
  ret += 1000*(uint64_t)t;
  return ret;
}

/* Convert string with time interval to milliseconds, return 0 on error.
   Input format: <integer number><suffix>, where suffix can be: ms, s, m, h, d
*/
uint64_t convert_interval(const string & tstr){
  char *e;
  uint64_t ret;
  ret = strtol(tstr.c_str(), &e, 10);
  if (e==NULL) return 0;
  if (strcmp(e,"ms")==0) return ret;
  if (strcmp(e,"s")==0)  return 1000*ret;
  if (strcmp(e,"m")==0)  return 60*1000*ret;
  if (strcmp(e,"h")==0)  return 3600*1000*ret;
  if (strcmp(e,"d")==0)  return 24*3600*1000*ret;
  return 0;
}

/***************************************************************************/
// process /query
Json json_query(const string & dbpath, const Json & ji){

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
  uint64_t t1 = convert_time( ji["range"]["from"].as_string() );
  uint64_t t2 = convert_time( ji["range"]["to"].as_string() );
  if (t1==0 || t2==0) throw Json::Err() << "Bad range setting";

  /* check format */
  if (ji["format"].as_string() != "json") 
    throw Json::Err() << "Unknown format";

  /* parse interval */
  uint64_t dt = convert_interval( ji["interval"].as_string() );
  if (dt==0) throw Json::Err() << "Bad interval";

  /* parse maxDataPoints */
  uint64_t maxpt = ji["maxDataPoints"].as_integer();
  if (maxpt==0) throw Json::Err() << "Bad maxDataPoints";

  /* parse targets and run command */
  Json out = Json::array();
  for (int i=0; i<ji["targets"].size(); i++){

    // extract db name and column number
    DBout dbo(dbpath, ji["targets"][i]["target"].as_string());

    // Get data from the database
    DBsts db(dbpath, dbo.name, DB_RDONLY);

    // check DB format
    if (db.read_info().val == DATA_TEXT)
    throw Json::Err() << "Can not do query from TEXT database. Use annotations";

    // fork
    int fd[2];
    if (pipe(fd) != 0)
      throw Json::Err() << "can't create a pipe";

    pid_t pid = fork();
    if (pid < 0)
      throw Json::Err() << "can't do fork";

    // in the child process we set redirect stdout to the pipe
    // and start printing values using db.get_range()
    if (pid == 0) {
      if (dup2(fd[1], 1) != 1)
        throw Json::Err() << "can't set up standard output: " << strerror(errno);
      if (close(fd[1]) != 0 || close(fd[0]) != 0)
        throw Json::Err() << "can't set up standard output";


      // This process communicates only via stdout.
      // Here we ignore all errors.
      try{ db.get_range(t1,t2,dt, print_value, &dbo); }
      catch (Err e){ };
      throw Err();
    }
    close(fd[1]);

    // read line-by-line from file descriptor
    // and fill json
    Json ja = Json::array();
    char buf[128];
    std::string s;
    int n = 0;
    while(n = read(fd[0], buf, sizeof(buf))){
      s+=string(buf, buf+n);
      int pos;
      while ((pos = s.find('\n'))!=string::npos){
        string line = s.substr(0, pos);
        s = s.substr(pos+1,-1);

        // read timestamp and one value from this line:
        istringstream istr(line);
        uint64_t t;
        double   v;
        istr >> t >> v;
        Json jpt = Json::array();
        jpt.append(v);
        jpt.append((json_int_t)t);
        ja.append(jpt);

      }
    }

    Json jt = Json::object();
    jt.set("target", ji["targets"][i]["target"]);
    jt.set("datapoints", ja);
    out.append(jt);
  }

  return out;
}

/***************************************************************************/
// process /annotations
Json json_annotations(const string & dbpath, const Json & ji){

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
  uint64_t t1 = convert_time( ji["range"]["from"].as_string() );
  uint64_t t2 = convert_time( ji["range"]["to"].as_string() );
  if (t1==0 || t2==0) throw Json::Err() << "Bad range setting";


  // extract db name
  DBout dbo(dbpath, ji["annotation"]["name"].as_string());

  // Get data from the database
  DBsts db(dbpath, dbo.name, DB_RDONLY);

  // check DB format
  if (db.read_info().val != DATA_TEXT)
    throw Json::Err() << "Annotations can be found only in TEXT databases";


  // fork
  int fd[2];
  if (pipe(fd) != 0)
    throw Json::Err() << "can't create a pipe";

  pid_t pid = fork();
  if (pid < 0)
    throw Json::Err() << "can't do fork";

  // in the child process we set redirect stdout to the pipe
  // and start printing values using db.get_range()
  if (pid == 0) {
    if (dup2(fd[1], 1) != 1)
      throw Json::Err() << "can't set up standard output: " << strerror(errno);
    if (close(fd[1]) != 0 || close(fd[0]) != 0)
      throw Json::Err() << "can't set up standard output";

    // This process communicates only via stdout.
    // Here we ignore all errors.
    try{ db.get_range(t1,t2, 0, print_value, &dbo); }
    catch (Err e){ };
    throw Err();
  }
  close(fd[1]);

  // read line-by-line from file descriptor
  // and fill json
  Json ja = Json::array();
  char buf[128];
  std::string s;
  int n = 0;
  while(n = read(fd[0], buf, sizeof(buf))){
    s+=string(buf, buf+n);
    int pos;
    while ((pos = s.find('\n'))!=string::npos){
      string line = s.substr(0, pos);
      s = s.substr(pos+1,-1);

      // read timestamp, space character and all text from the line:
      istringstream istr(line);
      uint64_t t; string v;
      istr >> t; istr.get(); getline(istr, v);
      Json jpt = Json::object();
      jpt.set("title", v);
      jpt.set("time",  (json_int_t)t);
      jpt.set("annotation", ji["annotation"]);
      ja.append(jpt);
    }
  }

  return ja;
}

/***************************************************************************/
// process /search
Json json_search(const string & dbpath, const Json & ji){
  Json out = Json::array();
  return out;
}

/***************************************************************************/
/* Process a JSON request to the database. */
/* Returns error message on errors.*/
string stsdb_json(const string & dbpath,  /* path to databases */
                  const string & url,     /* /query, /annotations, etc. */
                  const string & data){    /* input data */

  try {
    /* parse input JSON */
    Json ji = Json::load_string(data);
    Json jo;
    int out_fl = JSON_PRESERVE_ORDER;

    if (url == "/query")
      return json_query(dbpath, ji).save_string(out_fl);

    if (url == "/search")
      return json_search(dbpath, ji).save_string(out_fl);

    if (url == "/annotations")
      return json_annotations(dbpath, ji).save_string(out_fl);

    throw Json::Err() << "Unknown query";
  }
  catch (Json::Err e){
    return e.json();
  }
}
