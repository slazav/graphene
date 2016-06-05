#ifndef JSONXX_H
#define JSONXX_H

#include <string>
#include <iostream>
#include <sstream>
#include <cassert>
#include <jansson.h>

#define DEBUG_MEM false

// Simple C++ wrapper for jansson library.
//   https://github.com/akheron/jansson
//   http://jansson.readthedocs.org/en/latest/
//
//   V.Zavjalov, 29.3.2016
//
// See another wrapper here: https://github.com/bvakili-evault/janssonxx

class Json{

  /************************************/
  // Error class for exceptions
  public:
    class Err {
      std::ostringstream s;
      public:
        Err(){}
        Err(const Err & o) { s << o.s.str(); }
        template <typename T>
        Err & operator<<(const T & o){ s << o; return *this; }
        std::string json() const {
          return std::string("{\"error_type\": \"jsonxx\", ") +
                              "\"error_message\":\"" + s.str() + "\"}"; }
        std::string text() const { return "jsonxx error: " + s.str(); }
        std::string str()  const { return s.str(); }
    };

  /************************************/
  // The json data. It contains its own memory management
  // with reference counter. I try to use it here.
  private:
    json_t *json;

  /* assign and destroy functions */
  private:
    void assign(const Json & other){
      json = other.json;
      if (DEBUG_MEM && json)
        std::cerr << "A:" << (void *)json << " " << json->refcount << "\n";
      if (json) json_incref(json);
    }
    void destroy(void){
      if (DEBUG_MEM && json)
        std::cerr << "D:" << (void *)json << " " << json->refcount << "\n";
      if (json) json_decref(json);
    }

  /************************************/
  /* Copy constructor, destructor, assignment */
  public:

    Json(const Json & other){ assign(other); }

    Json & operator=(const Json & other){
      if (this != &other){ destroy(); assign(other); }
      return *this;
    }

    ~Json(){ destroy(); }

  /************************************/
  // constructors
  // use incref=true if you extract reference to the existing object

  private:
    void create(json_t * json_=NULL, const bool incref=false){
      json = (json_!=NULL) ? json_ : json_null();
      if (incref) json_incref(json);
      if (DEBUG_MEM && json)
        std::cerr << "C:" << (void *)json << " " << json->refcount << "\n";
    }
    Json(json_t * json_, const bool incref=false) { create(json_, incref); }

  public:
    Json() { create(); }
    Json(const char * v)        { create(json_string(v)); }
    Json(const std::string & v) { create(json_string(v.c_str())); }
    Json(const bool v)          { create(v?json_true():json_false()); }
    Json(const json_int_t v)    { create(json_integer(v)); }
    Json(const int v)           { create(json_integer(v)); }
    Json(const double v)        { create(json_real(v)); }

  /************************************/
  // static function for creating various types of json

  // create an empty object
  static Json object(){ return Json(json_object());}

  // create an empty array
  static Json array(){ return Json(json_array());}

  // explicitely create null object (you can use also Json())
  static Json null(){ return Json(json_null());}

  // parse string (can contain \0!)
  static Json load_string(const std::string & s, const size_t flags=0){
    json_error_t e;
    Json ret(json_loadb(s.data(), s.length(), flags, &e));
    if (!ret) throw Err() << e.text;
    return ret;
  }

  // parse file
  static Json load_file(const std::string & f, const size_t flags=0){
    json_error_t e;
    Json ret(json_load_file(f.c_str(), flags, &e));
    if (!ret) throw Err() << e.text;
    return ret;
  }

  /************************************/
  // check types

  int type() const { return json_typeof(json); }
  bool is_object()  const { return type() == JSON_OBJECT; }
  bool is_array()   const { return type() == JSON_ARRAY; }
  bool is_string()  const { return type() == JSON_STRING; }
  bool is_integer() const { return type() == JSON_INTEGER; }
  bool is_real()    const { return type() == JSON_REAL; }
  bool is_true()    const { return type() == JSON_TRUE; }
  bool is_false()   const { return type() == JSON_FALSE; }
  bool is_null()    const { return type() == JSON_NULL; }
  bool is_bool()    const { return type() == JSON_TRUE ||
                                   type() == JSON_FALSE; }
  bool is_number()  const { return type() == JSON_INTEGER ||
                                   type() == JSON_REAL; }
  operator bool() {return !is_null();}


  /************************************/
  // extract data from json

  // cast to boolean
  bool as_bool() const{
    if (is_bool())    return is_true();
    if (is_integer()) return json_integer_value(json)!=0;
    throw Err() << "can't cast to boolean";
  }

  // cast to int
  json_int_t as_integer() const {
    if (is_integer()) return json_integer_value(json);
    if (is_real())    return (json_int_t)json_real_value(json);
    if (is_bool())    return is_true()? 1:0;
    throw Err() << "can't cast to integer";
  }

  // cast to double
  double as_real() const {
    if (is_real())    return json_real_value(json);
    if (is_integer()) return (double)json_integer_value(json);
    throw Err() << "can't cast to real";
  }

  // cast to string
  std::string as_string() const {
    if (is_string())  return json_string_value(json);
    if (is_null())    return std::string();
    if (is_integer()) {
      std::ostringstream s;
      s << json_integer_value(json);
      return s.str();
    }
    if (is_real()) {
      std::ostringstream s;
      s << json_real_value(json);
      return s.str();
    }
    if (is_true())  return std::string("true");
    if (is_false()) return std::string("false");
    // if (is_object() || is_array()) ...
    throw Err() << "can't cast to string";
    return std::string("");
  }

  // save json as a string (only arrays, objects), with flags
  std::string save_string(const size_t flags=0) const{
    char* tmp = json_dumps(json, flags);
    if (tmp==0) throw Err() << "save_string error";
    std::string s(tmp);
    free(tmp);
    return s;
  }

  /************************************/
  // functions common for objects and arrays

  // size (non-zero for non-empty arrays and objects)
  size_t size() const{
    if (is_object()) return json_object_size(json);
    if (is_array())  return json_array_size(json);
    throw Err() << "size: not an object or array";
  }

  // clear
  void clear(){
    if (is_object()) { json_object_clear(json); return; }
    if (is_array())  { json_array_clear(json); return; }
    throw Err() << "clear: not an object or array";
  }

  /************************************/
  // manipulating objects

  // get field, return json_null if no such field
  Json get(const char *key) const{
    return Json(json_object_get(json, key), 1); }

  Json operator[](const char *key) const {
    return Json(json_object_get(json, key), 1); }

  // does the field exist
  bool exists(const char *key) const{
    return json_object_get(json, key) != NULL; }

  // set field, exception on error
  void set(const char *key, const Json & val){
    if (json_object_set(json, key, val.json))
      throw Err() << "json_object_set error"; }

  // delete key from object, throw error if key not found
  void del(const char *key){
    if (json_object_del(json, key))
      throw Err() << "json_object_del: not found"; }

  // same, but with std::string keys
  Json get(const std::string &key) const { return get(key.c_str()); }
  Json operator[](const std::string &key) const { return get(key.c_str()); }
  bool exists(const std::string &key) const { return exists(key.c_str()); }
  void set(const std::string &key, const Json & val){ set(key.c_str(), val); }
  void del(const std::string &key){ del(key.c_str()); }

  // set functions for various types
  template <typename T>
  void set(const char *key, const T v){ set(key, Json(v)); }
  template <typename T>
  void set(const std::string &key, const T v){ set(key, Json(v)); }

  // update object all/existing/missing fields using
  // another object, throw exception on errors
  void update(const Json & j){
    if (json_object_update(json, j.json))
      throw Err() << "object_update error"; }
  void update_existing(const Json & j){
    if (json_object_update_existing(json, j.json))
      throw Err() << "object_update error"; }
  void update_missing(const Json & j){
    if (json_object_update_missing(json, j.json))
      throw Err() << "object_update error"; }

  // object iterator
  class iterator{
    void *iter;
    json_t *json;
    public:
    iterator(json_t * j, void *i = NULL): iter(i),json(j) {}
    std::string key(){
      return std::string(json_object_iter_key(iter));}
    Json val(){
      return Json(json_object_iter_value(iter), 1);}
    iterator & operator++() {
      iter = json_object_iter_next(json, iter);
      return *this;}
    iterator operator++(int) {
      void *iter0 = iter;
      iter = json_object_iter_next(json, iter);
      return iterator(json, iter0);}
    operator bool() const { return iter!=NULL; }
  };
  iterator begin() const{
    return iterator(json, json_object_iter(json)); }
  iterator end() const  {
    return iterator(json); }

  /************************************/
  // manipulating arrays

  // array get (returns json_null on error)
  Json get(const size_t i) const{
    return Json(json_array_get(json,i), 1); }
  Json operator[](const size_t i) const {
    return Json(json_array_get(json, i), 1); }

  // array set, del, insert, append (throw exception on error)
  void set(const size_t i, const Json & val){
    if (json_array_set(json,i, val.json))
      throw Err() << "array_set error"; }
  void del(const size_t i){
    if (json_array_remove(json, i))
      throw Err() << "array_remove error"; }
  void insert(const size_t i, const Json & val){
    if (json_array_insert(json,i, val.json))
      throw Err() << "array_insert error"; }
  void append(const Json & val){
    if (json_array_append(json, val.json))
      throw Err() << "array_append error"; }

  // append another array (throw exception on error)
  void extend(const Json & j){
    if (json_array_extend(json, j.json))
      throw Err() << "array_extend error"; }

  /************************************/
  // validate a json using special mask
  void validate(const Json & mask) const;

  void validate(const char *str) const{
    validate(Json::load_string(str)); }

};

#endif
