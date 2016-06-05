#include "jsonxx.h"
#include <string>
#include <cstring>
#include <stdint.h> // SIZE_MAX

// validate a json using special mask
void
Json::validate(const Json & mask) const{


  switch(type()){

    // json object: mask should contain an object
    // with all possible key names. For each element
    // validate is run recursively.
    case JSON_OBJECT: {
      if (!mask.is_object())
        throw Json::Err() << "unexpected object";
      for (Json::iterator i=begin(); i!=end(); i++){
        if (!mask.exists(i.key()))
          throw Json::Err() << "unexpected key: " << i.key();
        i.val().validate(mask[i.key()]);
      }
      break;
    }

    // json array: mask should contain an array with
    // one element. validate is run for each
    // element of the json and the mask element
    case JSON_ARRAY: {
      if (!mask.is_array())
        throw Json::Err() << "unexpected array";
      if (mask.size()!=1)
        throw Json::Err() << "mask array should have one element";
      for (size_t i=0; i<size(); i++)
        get(i).validate(mask.get((size_t)0));
      break;
    }

    // json string: mask element should be an object
    // with fields: type==string, minlen, maxlen, charset, name,
    case JSON_STRING: {
      if (!mask.is_object() || mask["type"].as_string()!="string")
        throw Json::Err() << "unexpected string";

      std::string name = mask["name"].as_string();
      if (name!="") name += ": ";
      std::string str = as_string();
      size_t len = str.size();

      if (mask.exists("minlen") && len < (size_t)mask["minlen"].as_integer())
        throw Json::Err() << name << "string is too short";
      if (mask.exists("maxlen") && len > (size_t)mask["maxlen"].as_integer())
        throw Json::Err() << name << "string is too long";

      std::string ch = mask["charset"].as_string();

      if (ch == "id"){
        const char charset_id[] = "abcdefghijklmnopqrstuvwxyz"
                                  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                  "0123456789_";
        if (strspn(str.c_str(), charset_id)!=len)
          throw Err() << name << "only letters, numbers and _ are allowed";
      }

      if (ch == "htm"){
        const char icharset_htm[] = "<>";
        if (strcspn(str.c_str(), icharset_htm)!=len)
          throw Err() << name << "< and > are not allowed";
      }
      break;
    }

    // a number
    case JSON_INTEGER:
    case JSON_REAL: {
      if (!mask.is_object() || mask["type"].as_string()!="number")
        throw Json::Err() << "unexpected number";

      std::string name = mask["name"].as_string();
      if (name!="") name += ": ";
      if (mask.exists("min") && as_real() < mask["min"].as_real())
        throw Json::Err() << name << "number is too small";
      if (mask.exists("max") && as_real() > mask["max"].as_real())
        throw Json::Err() << name << "number is too big";
      break;
    }

    // a boolean value
    case JSON_TRUE:
    case JSON_FALSE: {
      if (!mask.is_object() || mask["type"].as_string()!="boolean")
        throw Json::Err() << "unexpected boolean";
      break;
    }

    // null value
    case JSON_NULL: {
      if (!mask.is_object() || mask["type"].as_string()!="null")
        throw Json::Err() << "unexpected null";
      break;
    }

    default:
      throw Json::Err() << "unknown json type";
  }
}

