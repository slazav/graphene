#include <iostream>
#include <iomanip>
#include <malloc.h>
#include <map>

#include "tests.h"
#include "jsonxx.h"

int main() {
  try{

    // strings
    const char strmask[] = "{\"name\": \"teststr\","
                           " \"type\": \"string\","
                           " \"minlen\":3, \"maxlen\":6,"
                           " \"charset\": \"id\"}";
    Json("abc").validate(strmask);
    ASSERT_EX(Json("ab").validate(strmask), "teststr: string is too short", "");
    ASSERT_EX(Json("abcdefg").validate(strmask), "teststr: string is too long", "");
    ASSERT_EX(Json("abc&ef").validate(strmask), "teststr: only letters, numbers and _ are allowed", "");
    ASSERT_EX(Json("abc").validate("{\"type\": \"number\"}"),
      "unexpected string", "");
    ASSERT_EX(Json("abc").validate("[]"),
      "unexpected string", "");


    // numbers
    const char nummask[] = "{\"name\": \"testnum\","
                           " \"type\": \"number\","
                           " \"min\":3, \"max\":6}";

    Json(6).validate(nummask);
    Json(5.9).validate(nummask);
    ASSERT_EX(Json(6.1).validate(nummask), "testnum: number is too big", "");
    ASSERT_EX(Json(1).validate(nummask),  "testnum: number is too small", "");
    ASSERT_EX(Json(-10).validate(nummask), "testnum: number is too small", "");
    ASSERT_EX(Json(1e99).validate(nummask), "testnum: number is too big", "");
    ASSERT_EX(Json(10).validate(strmask), "unexpected number", "");

    // boolean and null - not too useful
    Json(true).validate("{\"type\": \"boolean\"}");
    Json(false).validate("{\"type\": \"boolean\"}");
    Json().validate("{\"type\": \"null\"}");

    // objects and arrays
    Json M1 = Json::load_string(
      "{\"name\": {\"type\":\"string\", \"charset\":\"id\","
      "            \"minlen\": 3, \"maxlen\": 6 },"
       " \"value\": {\"type\": \"number\", \"min\": 3, \"max\": 6},"
       " \"arr\": [{\"type\": \"number\", \"min\": 3, \"max\": 6}],"
       " \"sarr\": [{\"type\":\"string\", \"charset\": \"id\"}],"
       " \"obj\": {\"nn\": {\"type\":\"string\"}}}"
    );

    Json::load_string(
      "{\"name\": \"myname\","
      " \"value\": 5,"
      " \"arr\": [3, 4, 5],"
      " \"sarr\": [\"a\",\"b\"],"
      " \"obj\": {\"nn\": \"123\"}}"
    ).validate(M1);

    ASSERT_EX(Json::load_string("{\"uname\": \"myname\"}").validate(M1),
      "unexpected key: uname", "");

    ASSERT_EX(Json::load_string("{\"name\": [\"aaa\"]}").validate(M1),
      "unexpected array", "");

  }
  catch(Json::Err e){
    std::cerr << "error: " << e.str() << "\n";
  }
  return 0;
}
