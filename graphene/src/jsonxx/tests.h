// This code based on https://github.com/bvakili-evault/janssonxx

#define ASSERT_OP(lhs, rhs, op, m) {\
    if(!((lhs) op (rhs))) { \
      std::cerr << std::boolalpha; \
      std::cerr << __FILE__ << '[' << __LINE__ << "]: ERROR: " << (m) << std::endl; \
      std::cerr << "\ttest:   " << #lhs << ' ' << #op << ' ' << #rhs << std::endl; \
      std::cerr << "\tlhs: " << (lhs) << std::endl; \
      std::cerr << "\trhs: " << (rhs) << std::endl; \
      return 1; \
    } \
  }
#define ASSERT_EQ(lhs, rhs, m) ASSERT_OP(lhs, rhs, ==, m)
#define ASSERT_NE(lhs, rhs, m) ASSERT_OP(lhs, rhs, !=, m)
#define ASSERT_TRUE(p, m) ASSERT_OP(p, true, ==, m)
#define ASSERT_FALSE(p, m) ASSERT_OP(p, true, !=, m)
#define ASSERT_EX(lhs, rhs, m)\
    try { lhs;\
      std::cerr << (m) << ": no exception" << std::endl;\
      return 1;\
    } catch (Json::Err e) {\
      ASSERT_EQ(e.str(), rhs, m);\
    }
