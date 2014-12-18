/** \file type names for end users. override free function type_string_impl(T) in T's namespace to define T's
   name

   or, override type_string_traits<T>::get() if adding a method to the namespace of T isn't an option

   note: type_string returns empty string if nobody specifies a nice name. type_name will return "unnamed
   type" instead.


*/

#ifndef TYPE_STRING_2012531_HPP
#define TYPE_STRING_2012531_HPP
#pragma once


#include <vector>
#include <map>
#include <string>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <graehl/shared/int_types.hpp>
#include <boost/type_traits/is_integral.hpp>

namespace graehl {

namespace {
std::string unnamed_type = "unnamed type";
}


template <class T>
std::string type_string_impl(T const&) {
  return std::string();
}

namespace adl {
template <class T>
std::string call_type_string_impl(T const& t) {
  return type_string_impl(t);
}
}

template <class T, bool = boost::is_integral<T>::value>
struct default_type_string {
  // is_integral = false. you can override the type_string free function, or, in case you want to use some
  // template specialization, override type_string_traits
  static std::string get() { return adl::call_type_string_impl(*(T const*)0); }
};

template <class Int>
struct default_type_string<Int, true>  // note: this is the is_integral = true specialization
    {
  static std::string get() { return "integer"; }
};

template <class T>
struct type_string_traits {
  static std::string get() { return default_type_string<T>::get(); }
};

template <class T>
std::string type_string(T const&) {
  return type_string_traits<T>::get();
}

template <class T>
bool no_type_string(T const& t) {
  return type_string(t).empty();
}

template <class T>
std::string type_name(T const& t) {
  std::string n = type_string(t);
  return n.empty() ? unnamed_type : n;
}

#define GRAEHL_PRIMITIVE_TYPE_STRING(T, name) \
  template <>                                 \
  struct type_string_traits<T> {              \
    static std::string get() { return name; } \
  };

GRAEHL_PRIMITIVE_TYPE_STRING(std::string, "string");

GRAEHL_PRIMITIVE_TYPE_STRING(bool, "boolean");
GRAEHL_PRIMITIVE_TYPE_STRING(char, "character");
GRAEHL_PRIMITIVE_TYPE_STRING(int8_t, "signed byte");
GRAEHL_PRIMITIVE_TYPE_STRING(uint8_t, "non-negative byte");
GRAEHL_PRIMITIVE_TYPE_STRING(int16_t, "16-bit integer");
GRAEHL_PRIMITIVE_TYPE_STRING(uint16_t, "non-negative 16-bit integer");
#if HAVE_64BIT_INT64_T
GRAEHL_PRIMITIVE_TYPE_STRING(int64_t, "64-bit integer");
GRAEHL_PRIMITIVE_TYPE_STRING(uint64_t, "non-negative 64-bit integer");
#endif
#if PTRDIFF_DIFFERENT_FROM_INTN
GRAEHL_PRIMITIVE_TYPE_STRING(std::size_t, "64-bit size");
GRAEHL_PRIMITIVE_TYPE_STRING(std::ptrdiff_t, "64-bit offset");
#endif
GRAEHL_PRIMITIVE_TYPE_STRING(int, "integer");
GRAEHL_PRIMITIVE_TYPE_STRING(unsigned, "non-negative integer");
#if INT_DIFFERENT_FROM_INTN
GRAEHL_PRIMITIVE_TYPE_STRING(int32_t, "32-bit integer");
GRAEHL_PRIMITIVE_TYPE_STRING(uint32_t, "non-negative 32-bit integer");
#endif
#if HAVE_LONGER_LONG
GRAEHL_PRIMITIVE_TYPE_STRING(long, "large integer");
GRAEHL_PRIMITIVE_TYPE_STRING(unsigned long, "large non-negative integer");
#endif
GRAEHL_PRIMITIVE_TYPE_STRING(float, "real number");
GRAEHL_PRIMITIVE_TYPE_STRING(double, "double-precision real number");
GRAEHL_PRIMITIVE_TYPE_STRING(long double, "long-double-precision real number");


template <class T>
std::string type_string() {
  return graehl::type_string(*(T const*)0);
}

template <class T>
struct type_string_traits<std::map<std::string, T> > {
  static std::string get() { return "map to " + type_string(*(T const*)0); }
};

template <class K, class T>
struct type_string_traits<std::map<K, T> > {
  static std::string get() {
    return "map from " + type_string(*(K const*)0) + " to " + type_string(*(T const*)0);
  }
};

template <class K, class T>
struct type_string_traits<std::pair<K, T> > {
  static std::string get() {
    return "pair of (" + type_string(*(K const*)0) + ", " + type_string(*(T const*)0) + ")";
  }
};

#define GRAEHL_TYPE_STRING_TEMPLATE_1(T, prefix)                                          \
  template <class T1>                                                                     \
  struct type_string_traits<T<T1> > {                                                     \
    static std::string get() { return std::string(prefix) + type_string(*(T1 const*)0); } \
  };

GRAEHL_TYPE_STRING_TEMPLATE_1(std::vector, "sequence of ")
GRAEHL_TYPE_STRING_TEMPLATE_1(boost::optional, "optional ")
GRAEHL_TYPE_STRING_TEMPLATE_1(boost::shared_ptr, "")
}

#ifdef GRAEHL_TEST
#include <graehl/shared/test.hpp>
namespace graehl {

BOOST_AUTO_TEST_CASE(TEST_type_string) {
  using namespace std;
  typedef map<string, string> SS;
  typedef map<string, int> SI;
  typedef map<int, int> II;
  typedef vector<SI> VSI;
  typedef boost::optional<VSI> OptionalVSI;
  typedef boost::shared_ptr<VSI> SharedVSI;
  typedef std::pair<II, SI> P;
  int i;
  BOOST_CHECK_EQUAL(type_string<uint8_t>(), "non-negative byte");
  BOOST_CHECK_EQUAL(type_string(i), "integer");
  BOOST_CHECK_EQUAL(type_string<SS>(), "map to string");
  BOOST_CHECK_EQUAL(type_string<SI>(), "map to integer");
  BOOST_CHECK_EQUAL(type_string<II>(), "map from integer to integer");
  BOOST_CHECK_EQUAL(type_string<VSI>(), "sequence of map to integer");
  BOOST_CHECK_EQUAL(type_string<SharedVSI>(), "sequence of map to integer");
  BOOST_CHECK_EQUAL(type_string<OptionalVSI>(), "optional sequence of map to integer");
  pair<II, SI> p;
  BOOST_CHECK_EQUAL(type_string(p), "pair of (map from integer to integer, map to integer)");
}


}

#endif

#endif
