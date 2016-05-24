#include <iostream>
#include <iomanip>
#include <malloc.h>
#include <map>

#include "tests.h"
#include "jsonxx.h"

int main() {
  try{

    // create various types of Json objects
    Json e0;                 // not an object
    Json e1("text");         // string
    Json e2(std::string("")); // string
    Json e3(10);             // int
    Json e4(10.);            // double
    Json e5(true);           // true
    Json e6(false);          // false

    // with static functions
    Json e7(Json::null());   // null
    Json e8(Json::object()); // empty object
    Json e9(Json::array());  // empty array
    Json eA(Json::load_string("{\"foo\": true, \"bar\": \"test\"}"));
    Json eB(Json::load_file("test.json"));  // from file

    Json eA1(eA);  // copy constructor
    Json eA2 = eA; // assignment

    // exceptions
    ASSERT_EX(Json::load_file(""), "unable to open : No such file or directory", "no file");
    ASSERT_EX(Json::load_file("test_jsonxx.cpp"), "'[' or '{' expected near '#'", "bad file");
    ASSERT_EX(Json::load_string("{"), "string or '}' expected near end of file", "bad json");

    // check types
    ASSERT_EQ(e0.type(), JSON_NULL,    "e0 wrong type");
    ASSERT_EQ(e1.type(), JSON_STRING,  "e1 wrong type");
    ASSERT_EQ(e2.type(), JSON_STRING,  "e2 wrong type");
    ASSERT_EQ(e3.type(), JSON_INTEGER, "e3 wrong type");
    ASSERT_EQ(e4.type(), JSON_REAL,    "e4 wrong type");
    ASSERT_EQ(e5.type(), JSON_TRUE,    "e5 wrong type");
    ASSERT_EQ(e6.type(), JSON_FALSE,   "e6 wrong type");
    ASSERT_EQ(e7.type(), JSON_NULL,    "e7 wrong type");
    ASSERT_EQ(e8.type(), JSON_OBJECT,  "e8 wrong type");
    ASSERT_EQ(e9.type(), JSON_ARRAY,   "e9 wrong type");
    ASSERT_EQ(eA.type(), JSON_OBJECT,  "eA wrong type");
    ASSERT_EQ(eB.type(), JSON_OBJECT,  "eB wrong type");
    ASSERT_EQ(eA1.type(), JSON_OBJECT, "eA1 wrong type");
    ASSERT_EQ(eA2.type(), JSON_OBJECT, "eA2 wrong type");

    ASSERT_TRUE(e0.is_null(),    "e0 is not null");
    ASSERT_FALSE(e0,             "e0 is defined");
    ASSERT_FALSE(e0.is_object(), "e0 is object");

    ASSERT_TRUE(e1.is_string(),  "e1 is not string");
    ASSERT_FALSE(e1.is_number(), "e1 is number");
    ASSERT_FALSE(e1.is_null(),   "e1 is null");
    ASSERT_TRUE(e1,              "e1 is not defined");

    ASSERT_TRUE(e2.is_string(),  "e2 is not string");

    ASSERT_TRUE(e3.is_integer(), "e3 is not integer");
    ASSERT_TRUE(e3.is_number(),  "e3 is not number");
    ASSERT_FALSE(e3.is_real(),   "e3 is real");

    ASSERT_TRUE(e4.is_real(),    "e3 is not real");
    ASSERT_TRUE(e4.is_number(),  "e3 is not number");
    ASSERT_FALSE(e4.is_integer(),"e3 is integer");

    ASSERT_TRUE(e5.is_true(),    "e5 is not true");
    ASSERT_TRUE(e5.is_bool(),    "e5 is not bool");
    ASSERT_FALSE(e5.is_false(),  "e5 is false");
    ASSERT_FALSE(e5.is_null(),   "e5 is null");
    ASSERT_FALSE(e5.is_object(), "e5 is object");

    ASSERT_TRUE(e6.is_false(),   "e6 is not false");
    ASSERT_TRUE(e6.is_bool(),    "e6 is not bool");
    ASSERT_FALSE(e6.is_true(),   "e6 is true");

    ASSERT_TRUE(e7.is_null(),    "e7 is not null");
    ASSERT_FALSE(e7,             "e7 is defined");
    ASSERT_FALSE(e7.is_object(), "e7 is object");

    ASSERT_TRUE(e8.is_object(),  "e8 is not object");
    ASSERT_TRUE(e9.is_array(),   "e9 is not array");

    // size (non-zero for non-empty arrays and objects)

    ASSERT_EQ(e8.size(), 0,      "e8 has wrong size");
    ASSERT_EQ(e9.size(), 0,      "e9 has wrong size");
    ASSERT_EQ(eA.size(), 2,      "eA has wrong size");
    ASSERT_EQ(eB.size(), 1,      "eB has wrong size");
    ASSERT_EQ(eA1.size(), 2,     "eA1 has wrong size");
    ASSERT_EQ(eA2.size(), 2,     "eA2 has wrong size");

    // extract data from json

    //    Json e0;
    //    Json e1("text");
    //    Json e2(std::string(""));
    //    Json e3(10);
    //    Json e4(10.);
    //    Json e5(true);
    //    Json e6(false);

    ASSERT_TRUE(e3.as_bool(), "e3 as bool");
    ASSERT_TRUE(e5.as_bool(), "e5 as bool");
    ASSERT_FALSE(e6.as_bool(), "e6 as bool");

    ASSERT_EQ(e3.as_integer(), 10, "e3 as integer");
    ASSERT_EQ(e4.as_integer(), 10, "e4 as integer");
    ASSERT_EQ(e5.as_integer(),  1, "e5 as integer");
    ASSERT_EQ(e6.as_integer(),  0, "e6 as integer");

    ASSERT_EQ(e3.as_real(), 10., "e3 as real");
    ASSERT_EQ(e4.as_real(), 10., "e4 as real");

    ASSERT_EQ(e0.as_string(), "",     "e0 as string");
    ASSERT_EQ(e1.as_string(), "text", "e1 as string");
    ASSERT_EQ(e2.as_string(), "",     "e2 as string");
    ASSERT_EQ(e3.as_string(), "10",   "e3 as string");
    ASSERT_EQ(e4.as_string(), "10",   "e4 as string");
    ASSERT_EQ(e5.as_string(), "true", "e5 as string");
    ASSERT_EQ(e6.as_string(), "false","e6 as string");

    // work with object

    e8.set("k0", e0);
    e8.set("k1", e1);
    e8.set("k2", e2);
    e8.set("k3", e3);
    e8.set("k4", e4);
    e8.set(std::string("k5"), e5);
    e8.set("k6", e6);
    e8.set("k7", e7);

    ASSERT_TRUE(e8.exists("k0"), "exists(k0)");
    ASSERT_TRUE(e8.exists("k1"), "exists(k1)");
    ASSERT_TRUE(e8.exists("k2"), "exists(k2)");
    ASSERT_TRUE(e8.exists("k3"), "exists(k3)");
    ASSERT_TRUE(e8.exists("k4"), "exists(k4)");
    ASSERT_TRUE(e8.exists("k5"), "exists(k5)");
    ASSERT_TRUE(e8.exists("k6"), "exists(k6)");
    ASSERT_TRUE(e8.exists("k7"), "exists(k7)");
    ASSERT_FALSE(e8.exists(""),   "exists");
    ASSERT_FALSE(e8.exists("k8"), "exists");

    ASSERT_EQ(e8.size(), 8,      "e8 has wrong size");
    ASSERT_EQ(e8["k0"].as_string(), "",     "e8[k0].as_string");
    ASSERT_EQ(e8["k1"].as_string(), "text", "e8[k1].as_string");
    ASSERT_EQ(e8[std::string("k2")].as_string(), "",     "e8[k2].as_string");
    ASSERT_EQ(e8["k3"].as_string(), "10",   "e8[k2].as_string");
    ASSERT_TRUE(e8["k5"].is_true(),  "e8[k5]");
    ASSERT_TRUE(e8["k6"].is_false(), "e8[k6]");

    ASSERT_TRUE(e8.get("k5").is_true(),  "e8[k5]"); // get instead of []
    ASSERT_TRUE(e8.get("k6").is_false(), "e8[k6]");

    e8.del("k0"); // del
    e8.del(std::string("k1"));
    ASSERT_TRUE(e8["k1"].is_null(), "e8[k1] is null")
    ASSERT_EQ(e8.size(), 6,      "e8 has wrong size");

    e8.update(eA); // update
    ASSERT_EQ(e8.size(), 8,      "e8 has wrong size");
    ASSERT_EQ(e8["bar"].as_string(), "test", "updated");
    eA.set("bar", Json("test1"));
    eA.set("barx", Json("testx"));
    e8.update_existing(eA);
    ASSERT_TRUE(e8["barx"].is_null(), "upd existing");
    ASSERT_EQ(e8["bar"].as_string(), "test1", "upd existing");
    eA.set("bar", Json("test2"));
    e8.update_missing(eA);
    ASSERT_EQ(e8["barx"].as_string(), "testx", "upd missing");
    ASSERT_EQ(e8["bar"].as_string(),  "test1", "upd missing");

    std::map<std::string, std::string> M;  // iterators
    for (Json::iterator i = e8.begin(); i!=e8.end(); i++)
      M[i.key()] = i.val().as_string();

    ASSERT_EQ(M["k2"], "",      "iterators");
    ASSERT_EQ(M["k3"], "10",    "iterators");
    ASSERT_EQ(M["k4"], "10",    "iterators");
    ASSERT_EQ(M["k5"], "true",  "iterators");
    ASSERT_EQ(M["k6"], "false", "iterators");
    ASSERT_EQ(M["k7"], "",      "iterators");
    ASSERT_EQ(M["bar"],  "test1",      "iterators");
    ASSERT_EQ(M["barx"], "testx",      "iterators");
    ASSERT_EQ(M.size(), e8.size(),     "iterators");

    for (Json::iterator i = e1.begin(); i!=e1.end(); i++)
      ASSERT_TRUE(false, "iterators");

    e8.clear();
    ASSERT_EQ(e8.size(), 0,      "e8 has wrong size");


    e8.set("m1", true);
    e8.set("m2", 1);
    e8.set("m3", "12");
    e8.set("m4", std::string("12"));
    ASSERT_TRUE(e8["m1"].is_true(),    "e8[m1]");
    ASSERT_TRUE(e8["m2"].is_integer(), "e8[m2]");
    ASSERT_TRUE(e8["m3"].is_string(),  "e8[m3]");
    ASSERT_TRUE(e8["m4"].is_string(),  "e8[m4]");

    // work with array

    //    Json e0;
    //    Json e1("text");
    //    Json e2(std::string(""));
    //    Json e3(10);
    //    Json e4(10.);
    //    Json e5(true);
    //    Json e6(false);

    e9.append(e0); // ""
    e9.append(e0); // ""
    e9.append(e0); // ""

    e9.set(1,e1); // "text"
    e9.set(2,e2); // ""

    e9.append(e3); // 10
    e9.append(e4); // 10
    e9.insert(2, e5); // true
    e9.append(e6); // false

    ASSERT_EQ(e9.size(), 7, "e9 has wrong size");
    ASSERT_EQ(e9[(size_t)0].as_string(), "",     "array");
    ASSERT_EQ(e9[1].as_string(), "text", "array");
    ASSERT_EQ(e9[2].as_bool(),   true,   "array");
    ASSERT_EQ(e9[3].as_string(), "", "array");
    ASSERT_EQ(e9[4].as_string(), "10", "array");
    ASSERT_EQ(e9.get(5).as_string(), "10",    "array");
    ASSERT_EQ(e9.get(6).as_string(), "false", "array");

    e9.del(2);
    ASSERT_EQ(e9.size(), 6, "e9 has wrong size");
    ASSERT_EQ(e9.get(5).as_string(), "false", "array");

    e9.extend(e9);
    ASSERT_EQ(e9.size(), 12, "e9 has wrong size");
    ASSERT_EQ(e9.get(11).as_string(), "false", "array");

    e9.clear();
    ASSERT_EQ(e9.size(), 0, "e9 has wrong size");

    // dump to string
    const char * exp1 = "{\"foo\": true, \"bar\": \"test2\", \"barx\": \"testx\"}";
    ASSERT_EQ(eA.save_string(JSON_PRESERVE_ORDER), exp1, "save_string");

  } catch(Json::Err e){
    std::cerr << "error: " << e.str() << "\n";
  }
  return 0;
}
