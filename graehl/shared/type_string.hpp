// Copyright 2014 Jonathan Graehl-http://graehl.org/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
/** \file type names for end users. override free function type_string_impl(T const&) in T's namespace to
   define T's
   name

   or, override adl::TypeString<T>::get() if adding a method to the namespace of T isn't an option

   note: type_string returns empty string if nobody specifies a nice name. type_name will return "unnamed
   type" instead.


*/

#ifndef GRAEHL_SHARED__TYPE_STRING_2012531_HPP
#define GRAEHL_SHARED__TYPE_STRING_2012531_HPP
#pragma once


#include <boost/optional.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <graehl/shared/int_types.hpp>
#include <graehl/shared/type_traits.hpp>
#include <graehl/shared/unordered.hpp>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <memory>

namespace graehl {
template <class T>
std::string type_string_impl(T const&) {
  return std::string();
}
}

namespace adl {
template <class T, class Enable = void>
struct TypeString {
  static std::string get() {
    using namespace graehl;
    return type_string_impl(*(T*)0); // NOLINT
  }
};

template <class T>
struct TypeString<T, typename graehl::enable_if<graehl::is_integral<T>::value>::type> {
  static std::string get() { return "integer"; }
};

template <class T>
struct TypeString<std::map<std::string, T>> {
  static std::string get() { return "map to " + TypeString<T>::get(); }
};

template <class K, class T>
struct TypeString<std::map<K, T>> {
  static std::string get() { return "map from " + TypeString<K>::get() + " to " + TypeString<T>::get(); }
};

template <class K, class T>
struct TypeString<graehl::unordered_map<K, T>> {
  static std::string get() { return "map from " + TypeString<K>::get() + " to " + TypeString<T>::get(); }
};

template <class K, class T>
struct TypeString<std::pair<K, T>> {
  static std::string get() { return "pair of (" + TypeString<K>::get() + ", " + TypeString<T>::get() + ")"; }
};

#define GRAEHL_TYPE_STRING_TEMPLATE_1(TT, prefix)                                   \
  template <class T>                                                                \
  struct TypeString<TT<T>> {                                                        \
    static std::string get() { return std::string(prefix) + TypeString<T>::get(); } \
  };

GRAEHL_TYPE_STRING_TEMPLATE_1(std::vector, "sequence of ")
GRAEHL_TYPE_STRING_TEMPLATE_1(std::set, "set of ")
GRAEHL_TYPE_STRING_TEMPLATE_1(graehl::unordered_set, "set of ")
GRAEHL_TYPE_STRING_TEMPLATE_1(boost::optional, "optional ")
GRAEHL_TYPE_STRING_TEMPLATE_1(std::shared_ptr, "")

#define GRAEHL_PRIMITIVE_TYPE_STRING(T, name) \
  template <>                                 \
  struct TypeString<T, void> {                \
    static std::string get() { return name; } \
  };

GRAEHL_PRIMITIVE_TYPE_STRING(std::string, "string");

GRAEHL_PRIMITIVE_TYPE_STRING(bool, "boolean");
GRAEHL_PRIMITIVE_TYPE_STRING(char, "character");
GRAEHL_PRIMITIVE_TYPE_STRING(int8_t, "signed byte");
GRAEHL_PRIMITIVE_TYPE_STRING(uint8_t, "non-negative byte");
GRAEHL_PRIMITIVE_TYPE_STRING(int16_t, "16-bit integer");
GRAEHL_PRIMITIVE_TYPE_STRING(uint16_t, "non-negative 16-bit integer");
#if GRAEHL_HAVE_64BIT_INT64_T
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
#if GRAEHL_HAVE_LONGER_LONG
GRAEHL_PRIMITIVE_TYPE_STRING(long, "large integer");
GRAEHL_PRIMITIVE_TYPE_STRING(unsigned long, "large non-negative integer");
#endif
GRAEHL_PRIMITIVE_TYPE_STRING(float, "real number");
GRAEHL_PRIMITIVE_TYPE_STRING(double, "double-precision real number");
GRAEHL_PRIMITIVE_TYPE_STRING(long double, "long-double-precision real number");
}


namespace graehl {

template <class T>
std::string type_string(T const& t) {
  return adl::TypeString<T>::get();
}

template <class T>
std::string type_string() {
  return adl::TypeString<T>::get();
}

template <class T>
bool no_type_string(T const& t) {
  return adl::TypeString<T>::get().empty();
}

template <class T>
bool no_type_string() {
  return adl::TypeString<T>::get().empty();
}

namespace {
static std::string const unnamed_type = "unnamed type";
}

template <class T>
std::string type_name(T const& t) {
  std::string const& n = type_string(t);
  return n.empty() ? unnamed_type : n;
}
}


#ifdef GRAEHL_TEST
#include <graehl/shared/test.hpp>

namespace graehl {

BOOST_AUTO_TEST_CASE(TEST_type_string) {
  using namespace std;
  typedef std::map<string, string> SS;
  typedef std::map<string, int> SI;
  typedef std::map<int, int> II;
  typedef std::vector<SI> VSI;
  typedef boost::optional<VSI> OptionalVSI;
  typedef shared_ptr<VSI> SharedVSI;
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
