#include <iostream>
#include <vector>
#include <string>

#include "err/err.h"
#include "err/assert_err.h"

#include "data.h"

/***************************************************************/
/***************************************************************/

using namespace std;
int main() {
  try{

/***************************************************************/

    // time_v1
    TimeType tt = TIME_V1;
    string s;

    s = graphene_time_parse("0", tt);
    assert_eq(s.size(), 8);
    assert_eq(*((uint64_t *)s.data()), 0);

    s = graphene_time_parse("1000.999", tt);
    assert_eq(s.size(), 8);
    assert_eq(*((uint64_t *)s.data()), 1000999);

    s = graphene_time_parse("1000.9999", tt); // value is truncated to ms
    assert_eq(s.size(), 8);
    assert_eq(*((uint64_t *)s.data()), 1000999);


    s = graphene_time_parse("now", tt);
    assert_eq(s.size(), 8);
    assert_eq(*((uint64_t *)s.data())>1578046981l*1000, true);
    assert_eq(*((uint64_t *)s.data())<4102437600l*1000, true);

    s = graphene_time_parse("now_s", tt);
    assert_eq(s.size(), 8);
    assert_eq(*((uint64_t *)s.data())>1578046981l*1000, true);
    assert_eq(*((uint64_t *)s.data())<4102437600l*1000, true);
    assert_eq(*((uint64_t *)s.data())%1000==0, true);

    s = graphene_time_parse("18446744073709551.615", tt); // largest timestamp
    assert_eq(s.size(), 8);
    assert_eq(*((uint64_t *)s.data()), (uint64_t)-1);

    s = graphene_time_parse("inf", tt); // largest timestamp
    assert_eq(s.size(), 8);
    assert_eq(*((uint64_t *)s.data()), (uint64_t)-1);

    assert_err(graphene_time_parse("18446744073709551.616", tt),
      "Bad timestamp: too large value: 18446744073709551.616");

    assert_err(graphene_time_parse("18446744073709552.000", tt),
      "Bad timestamp: too large value: 18446744073709552.000");

    assert_err(graphene_time_parse("184467440737095520000", tt),
      "Bad timestamp: can't read seconds: 184467440737095520000");

    assert_err(graphene_time_parse("-2", tt),
      "Bad timestamp: positive value expected: -2");

    assert_err(graphene_time_parse("a", tt),
      "Bad timestamp: can't read seconds: a");

    assert_err(graphene_time_parse("1a", tt),
      "Bad timestamp: can't read decimal dot: 1a");

    assert_err(graphene_time_parse("1.a", tt),
      "Bad timestamp: can't read fractional part: 1.a");

    assert_err(graphene_time_parse("", tt),
      "Empty timestamp");

    // time_v2
    uint64_t max = (uint32_t)-1;
    tt = TIME_V2;

    s = graphene_time_parse("0", tt);
    assert_eq(s.size(), 4);
    assert_eq(*((uint32_t *)s.data()), 0);

    s = graphene_time_parse("0.0", tt);
    assert_eq(s.size(), 4);
    assert_eq(*((uint32_t *)s.data()), 0);

    s = graphene_time_parse("1", tt);
    assert_eq(s.size(), 4);
    assert_eq(*((uint32_t *)s.data()), 1);

    s = graphene_time_parse("1.", tt);
    assert_eq(s.size(), 4);
    assert_eq(*((uint32_t *)s.data()), 1);

    s = graphene_time_parse("1.0", tt);
    assert_eq(s.size(), 4);
    assert_eq(*((uint32_t *)s.data()), 1);

    s = graphene_time_parse("1.001", tt);
    assert_eq(s.size(), 8);
    assert_eq(*((uint64_t *)s.data()), (1l<<32) + 1e6);

    s = graphene_time_parse("4294967295.0", tt);
    assert_eq(s.size(), 4);
    assert_eq(*((uint32_t *)s.data()), max);

    s = graphene_time_parse("4294967295.999999999", tt); // largest value
    assert_eq(s.size(), 8);
    assert_eq(*((uint64_t *)s.data()), (max<<32) + 999999999);

    s = graphene_time_parse("4294967295.99999999999", tt); // sub-ns fraction is truncated
    assert_eq(s.size(), 8);
    assert_eq(*((uint64_t *)s.data()), (max<<32) + 999999999);

    s = graphene_time_parse("2020-02-02 01", tt);
    s = graphene_time_parse("2020-02-02", tt);
    assert_eq(s.size(), 4);
    auto val = *((uint32_t *)s.data());
    assert_feq((double)val, 1580598000.0, 24*3600.0);

    s = graphene_time_parse("2020-02-02 01", tt);
    assert_eq(s.size(), 4);
    assert_eq(*((uint32_t *)s.data()), val+3600);

    s = graphene_time_parse("2020-02-02 01:02", tt);
    assert_eq(s.size(), 4);
    assert_eq(*((uint32_t *)s.data()), val+3600+2*60);

    s = graphene_time_parse("2020-02-02 01:02:10", tt);
    assert_eq(s.size(), 4);
    assert_eq(*((uint32_t *)s.data()), val+3600+2*60+10);

    s = graphene_time_parse("2020-02-02 01:02:10.3542", tt);
    assert_eq(s.size(), 8);
    assert_eq(*((uint64_t *)s.data()), (((uint64_t)val+3600+2*60+10)<<32) + int(0.3542*1e9));


    s = graphene_time_parse("inf", tt);
    assert_eq(s.size(), 8);
    assert_eq(*((uint64_t *)s.data()), (max<<32) + 999999999);

    s = graphene_time_parse("now", tt);
    if (s.size() == 8){
      assert_eq(*((uint64_t *)s.data()) > (1578046981l<<32), true);
      assert_eq(*((uint64_t *)s.data()) < (4102437600l<<32), true);
    }
    else{
      assert_eq(s.size(), 4);
      assert_eq(*((uint32_t *)s.data()) > 1578046981l, true);
      assert_eq(*((uint32_t *)s.data()) < 4102437600l, true);
    }

    s = graphene_time_parse("now_s", tt);
    assert_eq(s.size(), 4);
    assert_eq(*((uint32_t *)s.data()) > 1578046981l, true);
    assert_eq(*((uint32_t *)s.data()) < 4102437600l, true);

    // +/-


    s = graphene_time_parse("0+", tt);
    assert_eq(s.size(), 8);
    assert_eq(*((uint64_t *)s.data()), 1);

    s = graphene_time_parse("0-", tt);
    assert_eq(s.size(), 8);
    assert_eq(*((uint64_t *)s.data()), (max<<32) + 999999999);

    s = graphene_time_parse("1+", tt);
    assert_eq(s.size(), 8);
    assert_eq(*((uint64_t *)s.data()), (1l<<32) + 1);

    s = graphene_time_parse("1.+", tt);
    assert_eq(s.size(), 8);
    assert_eq(*((uint64_t *)s.data()), (1l<<32) + 1);

    s = graphene_time_parse("1.000+", tt);
    assert_eq(s.size(), 8);
    assert_eq(*((uint64_t *)s.data()), (1l<<32) + 1);

    s = graphene_time_parse("1-", tt);
    assert_eq(s.size(), 8);
    assert_eq(*((uint64_t *)s.data()), 999999999);

    s = graphene_time_parse("1.999999999+", tt);
    assert_eq(s.size(), 4);
    assert_eq(*((uint32_t *)s.data()), 2);

    s = graphene_time_parse("1.123456789+", tt);
    assert_eq(s.size(), 8);
    assert_eq(*((uint64_t *)s.data()), (1l<<32) + 123456790);

    s = graphene_time_parse("1.123456789-", tt);
    assert_eq(s.size(), 8);
    assert_eq(*((uint64_t *)s.data()), (1l<<32) + 123456788);

    s = graphene_time_parse("4294967295.999999999+", tt);
    assert_eq(s.size(), 4);
    assert_eq(*((uint32_t *)s.data()), 0);

    assert_err(graphene_time_parse("4294967296.0", tt),
      "Bad timestamp: too large value: 4294967296.0");

    assert_err(graphene_time_parse("-2", tt),
      "Bad timestamp: positive value expected: -2");

    assert_err(graphene_time_parse("a", tt),
      "Bad timestamp: can't read seconds: a");

    assert_err(graphene_time_parse("1a", tt),
      "Bad timestamp: can't read decimal dot: 1a");

    assert_err(graphene_time_parse("1.a", tt),
      "Bad timestamp: can't read fractional part: 1.a");

    assert_err(graphene_time_parse("", tt),
      "Empty timestamp");

    /**************************************************************/
    // Time diff
    /**************************************************************/

    tt=TIME_V1;

    assert_feq (graphene_time_diff(
      graphene_time_parse("1", tt),
      graphene_time_parse("0", tt), tt), +1, 1e-10);

    assert_feq (graphene_time_diff(
      graphene_time_parse("0", tt),
      graphene_time_parse("1", tt), tt), -1, 1e-10);

    assert_feq (graphene_time_diff(
      graphene_time_parse("0", tt),
      graphene_time_parse("0", tt), tt), 0, 1e-10);

    assert_feq (graphene_time_diff(
      graphene_time_parse("0.123", tt),
      graphene_time_parse("0.123", tt), tt), 0, 1e-10);

    assert_feq (graphene_time_diff(
      graphene_time_parse("2.02", tt),
      graphene_time_parse("1.03", tt), tt), 0.99, 1e-10);

    assert_feq (graphene_time_diff(
      graphene_time_parse("2.02", tt),
      graphene_time_parse("1.01", tt), tt), 1.01, 1e-10);

    assert_feq (graphene_time_diff(
      graphene_time_parse("1.03", tt),
      graphene_time_parse("2.02", tt), tt), -0.99, 1e-10);

    assert_feq (graphene_time_diff(
      graphene_time_parse("1.01", tt),
      graphene_time_parse("2.02", tt), tt), -1.01, 1e-10);

    assert_feq (graphene_time_diff(
      graphene_time_parse("inf", tt),
      graphene_time_parse("0", tt), tt), 18446744073709551.616, 1);

    assert_feq (graphene_time_diff(
      graphene_time_parse("0", tt),
      graphene_time_parse("inf", tt), tt), -18446744073709551.616, 1e-10);

    assert_feq (graphene_time_diff(
      graphene_time_parse("inf", tt),
      graphene_time_parse("inf", tt), tt), 0, 1e-10);


    // same with V2
    tt = TIME_V2;

    assert_feq (graphene_time_diff(
      graphene_time_parse("1", tt),
      graphene_time_parse("0", tt), tt), +1, 1e-10);

    assert_feq (graphene_time_diff(
      graphene_time_parse("0", tt),
      graphene_time_parse("1", tt), tt), -1, 1e-10);

    assert_feq (graphene_time_diff(
      graphene_time_parse("0", tt),
      graphene_time_parse("0", tt), tt), 0, 1e-10);

    assert_feq (graphene_time_diff(
      graphene_time_parse("0.123", tt),
      graphene_time_parse("0.123", tt), tt), 0, 1e-10);

    assert_feq (graphene_time_diff(
      graphene_time_parse("2.02", tt),
      graphene_time_parse("1.03", tt), tt), 0.99, 1e-10);

    assert_feq (graphene_time_diff(
      graphene_time_parse("2.02", tt),
      graphene_time_parse("1.01", tt), tt), 1.01, 1e-10);

    assert_feq (graphene_time_diff(
      graphene_time_parse("1.03", tt),
      graphene_time_parse("2.02", tt), tt), -0.99, 1e-10);

    assert_feq (graphene_time_diff(
      graphene_time_parse("1.01", tt),
      graphene_time_parse("2.02", tt), tt), -1.01, 1e-10);

    assert_feq (graphene_time_diff(
      graphene_time_parse("inf", tt),
      graphene_time_parse("0", tt), tt), 4294967296.0, 1e-10);

    assert_feq (graphene_time_diff(
      graphene_time_parse("0", tt),
      graphene_time_parse("inf", tt), tt), -4294967296.0, 1e-10);

    assert_feq (graphene_time_diff(
      graphene_time_parse("inf", tt),
      graphene_time_parse("inf", tt), tt), 0, 1e-10);

    /**************************************************************/
    // Time cmp
    /**************************************************************/

    tt = TIME_V1;
    assert_eq (graphene_time_cmp(
      graphene_time_parse("1", tt),
      graphene_time_parse("0", tt), tt), +1);

    assert_eq (graphene_time_cmp(
      graphene_time_parse("0", tt),
      graphene_time_parse("1", tt), tt), -1);

    assert_eq (graphene_time_cmp(
      graphene_time_parse("0", tt),
      graphene_time_parse("0", tt), tt), 0);

    assert_eq (graphene_time_cmp(
      graphene_time_parse("0.123", tt),
      graphene_time_parse("0.123", tt), tt), 0);

    assert_eq (graphene_time_cmp(
      graphene_time_parse("2.02", tt),
      graphene_time_parse("1.03", tt), tt), +1);

    assert_eq (graphene_time_cmp(
      graphene_time_parse("2.02", tt),
      graphene_time_parse("1.01", tt), tt), +1);

    assert_eq (graphene_time_cmp(
      graphene_time_parse("1.03", tt),
      graphene_time_parse("2.02", tt), tt), -1);

    assert_eq (graphene_time_cmp(
      graphene_time_parse("1.01", tt),
      graphene_time_parse("2.02", tt), tt), -1);

    assert_eq (graphene_time_cmp(
      graphene_time_parse("inf", tt),
      graphene_time_parse("0", tt), tt), +1);

    assert_eq (graphene_time_cmp(
      graphene_time_parse("0", tt),
      graphene_time_parse("inf", tt), tt), -1);

    assert_eq (graphene_time_cmp(
      graphene_time_parse("inf", tt),
      graphene_time_parse("inf", tt), tt), 0);


    // same with V2
    tt = TIME_V2;
    assert_eq (graphene_time_cmp(
      graphene_time_parse("1", tt),
      graphene_time_parse("0", tt), tt), +1);

    assert_eq (graphene_time_cmp(
      graphene_time_parse("0", tt),
      graphene_time_parse("1", tt), tt), -1);

    assert_eq (graphene_time_cmp(
      graphene_time_parse("0", tt),
      graphene_time_parse("0", tt), tt), 0);

    assert_eq (graphene_time_cmp(
      graphene_time_parse("0.123", tt),
      graphene_time_parse("0.123", tt), tt), 0);

    assert_eq (graphene_time_cmp(
      graphene_time_parse("2.02", tt),
      graphene_time_parse("1.03", tt), tt), 1);

    assert_eq (graphene_time_cmp(
      graphene_time_parse("2.02", tt),
      graphene_time_parse("1.01", tt), tt), 1);

    assert_eq (graphene_time_cmp(
      graphene_time_parse("1.03", tt),
      graphene_time_parse("2.02", tt), tt), -1);

    assert_eq (graphene_time_cmp(
      graphene_time_parse("1.01", tt),
      graphene_time_parse("2.02", tt), tt), -1);

    assert_eq (graphene_time_cmp(
      graphene_time_parse("inf", tt),
      graphene_time_parse("0", tt), tt), +1);

    assert_eq (graphene_time_cmp(
      graphene_time_parse("0", tt),
      graphene_time_parse("inf", tt), tt), -1);

    assert_eq (graphene_time_cmp(
      graphene_time_parse("inf", tt),
      graphene_time_parse("inf", tt), tt), 0);


    /**************************************************************/
    // Time zero
    /**************************************************************/
    tt = TIME_V1;
    assert_eq(graphene_time_zero(graphene_time_parse("0.0", tt),tt), 1);
    assert_eq(graphene_time_zero(graphene_time_parse("0.001", tt),tt), 0);
    assert_eq(graphene_time_zero(graphene_time_parse("0.0001", tt),tt), 1); // ms precision!
    assert_eq(graphene_time_zero(graphene_time_parse("100", tt),tt), 0);
    assert_eq(graphene_time_zero(graphene_time_parse("inf", tt),tt), 0);

    tt = TIME_V2;
    assert_eq(graphene_time_zero(graphene_time_parse("0.0", tt),tt), 1);
    assert_eq(graphene_time_zero(graphene_time_parse("0.001", tt),tt), 0);
    assert_eq(graphene_time_zero(graphene_time_parse("0.000001", tt),tt), 0);
    assert_eq(graphene_time_zero(graphene_time_parse("0.0000000001", tt),tt), 1); // ns precision!
    assert_eq(graphene_time_zero(graphene_time_parse("100", tt),tt), 0);
    assert_eq(graphene_time_zero(graphene_time_parse("inf", tt),tt), 0);

    /**************************************************************/
    // Time add
    /**************************************************************/

    tt = TIME_V1;

    assert_eq (graphene_time_add(
      graphene_time_parse("1", tt),
      graphene_time_parse("2", tt), tt),
      graphene_time_parse("3", tt));

    assert_eq (graphene_time_add(
      graphene_time_parse("1.999", tt),
      graphene_time_parse("2.999", tt), tt),
      graphene_time_parse("4.998", tt));

    assert_err (graphene_time_add(
      graphene_time_parse("inf", tt),
      graphene_time_parse("0.01", tt), tt),
      "graphene_time_add overfull");

    assert_err (graphene_time_add(
      graphene_time_parse("inf", tt),
      graphene_time_parse("inf", tt), tt),
      "graphene_time_add overfull");

    assert_eq (graphene_time_add(
      graphene_time_parse("0", tt),
      graphene_time_parse("inf", tt), tt),
      graphene_time_parse("inf", tt));

    tt = TIME_V2;

    assert_eq (graphene_time_add(
      graphene_time_parse("1", tt),
      graphene_time_parse("2", tt), tt),
      graphene_time_parse("3", tt));

    assert_eq (graphene_time_add(
      graphene_time_parse("1.999", tt),
      graphene_time_parse("2.999", tt), tt),
      graphene_time_parse("4.998", tt));

    assert_err (graphene_time_add(
      graphene_time_parse("inf", tt),
      graphene_time_parse("0.01", tt), tt),
      "graphene_time_add overfull");

    assert_err (graphene_time_add(
      graphene_time_parse("inf", tt),
      graphene_time_parse("inf", tt), tt),
      "graphene_time_add overfull");

    assert_eq (graphene_time_add(
      graphene_time_parse("0", tt),
      graphene_time_parse("inf", tt), tt),
      graphene_time_parse("inf", tt));

    /**************************************************************/
    // Time print
    /**************************************************************/

    tt = TIME_V1;

    assert_eq(graphene_time_print(graphene_time_parse("0",        tt), tt), "0.000000000");
    assert_eq(graphene_time_print(graphene_time_parse("1000.999", tt), tt), "1000.999000000");
    assert_eq(graphene_time_print(graphene_time_parse("inf",      tt), tt), "18446744073709551.615000000");

    tt = TIME_V2;

    assert_eq(graphene_time_print(graphene_time_parse("0",        tt), tt), "0.000000000");
    assert_eq(graphene_time_print(graphene_time_parse("0+",       tt), tt), "0.000000001");
    assert_eq(graphene_time_print(graphene_time_parse("1000.999", tt), tt), "1000.999000000");
    assert_eq(graphene_time_print(graphene_time_parse("inf",      tt), tt), "4294967295.999999999");
    assert_eq(graphene_time_print(graphene_time_parse("4294967295.999999999+", tt), tt), "0.000000000");

    // TFMT_REL -- relative output
    assert_eq(graphene_time_print(
       graphene_time_parse("0", tt), tt, TFMT_REL, "123.456"),
       "-123.456000000");

    assert_eq(graphene_time_print(
       graphene_time_parse("123.456", tt), tt, TFMT_REL, "12.345"),
       "111.111000000");

    /**************************************************************/
    // SPP text
    /**************************************************************/
    assert_eq(graphene_spp_text("a"), "a");
    assert_eq(graphene_spp_text("a\n"), "a\n");
    assert_eq(graphene_spp_text("#a"), "##a");
    assert_eq(graphene_spp_text("\n\n#a"), "\n\n##a");
    assert_eq(graphene_spp_text("\n\n#a\n"), "\n\n##a\n");
    assert_eq(graphene_spp_text(
      "#0 0.1 100.001\n#abc\n #cde\nff\n"),
      "##0 0.1 100.001\n##abc\n #cde\nff\n");

    /**************************************************************/
    // check_name
    /**************************************************************/
    check_name("abcABCefz0123_,%");

    std::string msg = "symbols '.:+| \\n\\t/' are not allowed in the database name: ";
    assert_err(check_name("a b"), msg + "a b");
    assert_err(check_name("a.b"), msg + "a.b");
    assert_err(check_name("a:b"), msg + "a:b");
    assert_err(check_name("+a"), msg + "+a");
    assert_err(check_name("|a"), msg + "|a");
    assert_err(check_name("a\n"), msg + "a\n");
    assert_err(check_name("a\t"), msg + "a\t");
    assert_err(check_name("//"), msg + "//");

    int c,f;
    assert_eq(parse_ext_name("abc:1", c,f), "abc"); assert_eq(c, 1); assert_eq(f, -1);
    assert_eq(parse_ext_name("abc:0", c,f), "abc"); assert_eq(c, 0); assert_eq(f, -1);
    assert_eq(parse_ext_name("abc:f1", c,f), "abc"); assert_eq(c, -1); assert_eq(f, 1);
    assert_eq(parse_ext_name("abc:f5", c,f), "abc"); assert_eq(c, -1); assert_eq(f, 5);
    assert_eq(parse_ext_name("abc", c,f), "abc");   assert_eq(c, -1); assert_eq(f, -1);

    assert_err(parse_ext_name("abc:f0", c,f), "bad column name: f0");
    assert_err(parse_ext_name("abc:fp", c,f), "bad column name: fp");
    assert_err(parse_ext_name("abc:f-1", c,f), "bad column name: f-1");

    assert_err(parse_ext_name("abc :1", c,f), msg + "abc ");
    assert_err(parse_ext_name("abc:", c,f), msg + "abc:");
    assert_err(parse_ext_name("abc:a", c,f), "bad column name: a");

    assert_err(parse_ext_name("abc:-2", c,f), "bad column name: -2");
    assert_err(parse_ext_name("abc:1.2", c,f), "bad column name: 1.2");

  } catch (Err E){
    std::cerr << E.str() << "\n";
    return 1;
  }
  return 0;
}
