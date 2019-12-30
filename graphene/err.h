#ifndef ERR_H
#define ERR_H

#include <string>
#include <sstream>

/***********************************************************/
// Class for exceptions.
// Usage: throw Err() << "something";
//
class Err {
  std::ostringstream s;
  public:
    Err(){}
    Err(const Err & o) { s << o.s.str(); }
    template <typename T>
    Err & operator<<(const T & o){ s << o; return *this; }
    std::string str()   const { return s.str(); }
};

#endif