// This code based on https://github.com/bvakili-evault/janssonxx

#define ASSERT_OP(lhs, rhs, op) {\
    if(!((lhs) op (rhs))) { \
      std::cerr << std::boolalpha; \
      std::cerr << __FILE__ << '[' << __LINE__ << "]: ERROR" << std::endl; \
      std::cerr << "\ttest:   " << #lhs << ' ' << #op << ' ' << #rhs << std::endl; \
      std::cerr << "\tlhs: " << (lhs) << std::endl; \
      std::cerr << "\trhs: " << (rhs) << std::endl; \
      return 1; \
    } \
  }
#define ASSERT_EQ(lhs, rhs) ASSERT_OP(lhs, rhs, ==)
#define ASSERT_NE(lhs, rhs) ASSERT_OP(lhs, rhs, !=)
#define ASSERT_TRUE(p) ASSERT_OP(p, true, ==)
#define ASSERT_FALSE(p) ASSERT_OP(p, true, !=)
#define ASSERT_EX(lhs, rhs)\
    try { lhs;\
      std::cerr << ": no exception" << std::endl;\
      return 1;\
    } catch (Err e) { ASSERT_EQ(e.str(), rhs); }
