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
/** \file

  string_to and to_string, which default to lexical_cast, but, unlike that, may be overriden for user
  types other than supporting default/copy ctor and ostream <<, istream >>.

  note: optional<T> is the string "none" if absent, else regular string rep for
  T. this is different from the default optional streaming, which uses "" for
  none and " some" (space-prefixed printed 'T some;')

  USAGE:

  X string_to<X>(string);
  string to_string(X);
  X& string_to(string, X &); // note: returns the same ref you passed in, for convenience of use

  default implementation via boost lexical_cast if GRAEHL_USE_BOOST_LEXICAL_CAST, else stringstreams (quite
  slow, I'm sure)

  fast implementation for string, int, unsigned, float, double, and, if GRAEHL_HAVE_LONGER_LONG=1, long

  also: to_string calls itos utos etc

  to override for your type without going through iostreams, define an ADL-locatable string_to_impl and/or
  to_string_impl

  ----

  string_to and to_string for ints may not be much faster than
  lexical_cast in its latest incarnations (see
  http://accu.org/index.php/journals/1375) especially if you set the 'C' locale,
  but the code is at least simpler (and more to the point - more easily overriden).

  see also boost.spirit and qi for template-parser-generator stuff that should be about as fast!

  http://alexott.blogspot.com/2010/01/boostspirit2-vs-atoi.html
  and
  http://stackoverflow.com/questions/6404079/alternative-to-boostlexical-cast/6404331#6404331

*/

#ifndef GRAEHL__SHARED__STRING_TO_HPP
#define GRAEHL__SHARED__STRING_TO_HPP
#pragma once
#include <graehl/shared/append.hpp>
#include <graehl/shared/cpp11.hpp>
#include <graehl/shared/type_traits.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <iterator>
#ifndef GRAEHL_DEBUG_STRING_TO
#define GRAEHL_DEBUG_STRING_TO 0
#endif
#include <graehl/shared/ifdbg.hpp>
#if GRAEHL_DEBUG_STRING_TO
#include <graehl/shared/show.hpp>
DECLARE_DBG_LEVEL(GRSTRINGTO)
#define GRSTRINGTO(x) x
#else
#define GRSTRINGTO(x)
#endif
#include <utility>

#ifndef GRAEHL_USE_FTOA
#define GRAEHL_USE_FTOA 0
#endif
#ifndef GRAEHL_HAVE_STRTOUL
#define GRAEHL_HAVE_STRTOUL 1
#endif

#ifndef GRAEHL_USE_BOOST_LEXICAL_CAST
#define GRAEHL_USE_BOOST_LEXICAL_CAST 0
#endif

#include <boost/functional/hash.hpp>
#include <graehl/shared/atoi_fast.hpp>
#include <graehl/shared/base64.hpp>
#include <graehl/shared/have_64_bits.hpp>
#include <graehl/shared/is_container.hpp>
#include <graehl/shared/itoa.hpp>
#include <graehl/shared/nan.hpp>
#include <graehl/shared/shared_ptr.hpp>
#include <graehl/shared/snprintf.hpp>
#include <graehl/shared/word_spacer.hpp>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#if GRAEHL_USE_FTOA
#include <graehl/shared/ftoa.hpp>
#else
#include <cstdlib>
#endif

#include <graehl/shared/warning_compiler.h>
#include <graehl/shared/warning_push.h>

CLANG_DIAG_IGNORE_NEWER(unused-local-typedef)
#include <boost/optional.hpp>
#if GRAEHL_USE_BOOST_LEXICAL_CAST
#ifndef BOOST_LEXICAL_CAST_ASSUME_C_LOCALE
#define BOOST_LEXICAL_CAST_ASSUME_C_LOCALE
#endif
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#include <boost/lexical_cast.hpp>
#endif

#include <graehl/shared/warning_pop.h>

namespace graehl {

// documentation:

inline std::string const& to_string(std::string const& str) {
  return str;
}

template <class Data>
std::string to_string(Data const& out_str_to_data);

template <class Data, class Str>
Data& string_to(Str const& str, Data& out_str_to_data);

template <class Data, class Str>
inline Data string_to(Str const& str);
}

namespace graehl {

template <class I, class To>
bool try_stream_into(I& i, To& to, bool complete = true) {
  i >> to;
  if (i.fail()) return false;
  if (complete) {
    char c;
    return !(i >> c);
  }
  return true;
}

template <class Str, class To>
bool try_string_to(Str const& str, To& to, bool complete = true) {
  std::istringstream i(str);
  return try_stream_into(i, to, complete);
}

inline std::string to_string_impl(unsigned x) {
  return utos(x);
}

inline std::string to_string_impl(int x) {
  return itos(x);
}

#if GRAEHL_HAVE_LONGER_LONG
inline void string_to_impl(std::string const& s, int& x) {
  x = strtoi_complete_exact(s.c_str());
}
inline void string_to_impl(char const* s, int& x) {
  x = strtoi_complete_exact(s);
}
#endif

inline void string_to_impl(std::string const& s, long& x) {
  x = strtol_complete(s.c_str());
}
inline void string_to_impl(char const* s, long& x) {
  x = strtol_complete(s);
}

#ifndef SDL_32  // size_t == unsigned, avoid signature collision
inline void string_to_impl(std::string const& s, unsigned& x) {
  x = atou_fast_complete<unsigned>(s.c_str());
}

inline void string_to_impl(char const* s, unsigned& x) {
  x = atou_fast_complete<unsigned>(s);
}
#endif

inline void string_to_impl(std::string const& s, std::size_t& x) {
  x = atou_fast_complete<std::size_t>(s.c_str());
}
inline void string_to_impl(char const* s, std::size_t& x) {
  x = atou_fast_complete<std::size_t>(s);
}

static std::string const str_true = "true";
static std::string const str_false = "false";

inline char uc(char lc) {
  return lc + ('A' - 'a');
}

inline bool islc(char c, char lc) {
  return c == lc || c == uc(lc);
}

/**
   \return bool if string [p, p+len) is a yaml boolean or c++ printed boolean,
   else throw string_to_exception

   although this code is a bit obnoxious, it should be significantly faster than
   checking membership in a map<string> or serially comparing against a bunch of
   strings.
*/
inline bool parse_bool(char const* p, unsigned len) {
  switch (len) {
    case 1:
      if (*p == 'y' || *p == 'Y' || *p == '1') return true;  // y or 1
      if (*p == 'n' || *p == 'N' || *p == '0') return false;  // n or 0
      break;
    case 2:
      if (islc(*p, 'o')) {
        if (islc(p[1], 'n')) return true;  // on
      } else if (islc(*p, 'n') && islc(p[1], 'o'))
        return false;  // no
      break;
    case 3:
      if (islc(*p, 'y')) {
        if (islc(p[1], 'e') && islc(p[2], 's')) return true;  // yes
      } else if (islc(*p, 'o') && islc(p[1], 'f') && islc(p[2], 'f'))
        return false;  // off
      break;
    case 4:
      if (islc(*p, 't') && islc(p[1], 'r') && islc(p[2], 'u') && islc(p[3], 'e')) return true;  // true
      break;
    case 5:
      if (islc(*p, 'f') && islc(p[1], 'a') && islc(p[2], 'l') && islc(p[3], 's') && islc(p[4], 'e'))
        return false;  // false
      break;
  }
  VTHROW_A_MSG(string_to_exception, "'" << std::string(p, p + len)
                                        << "': not boolean - must be "
                                           "{1|y|Y|yes|Yes|YES|n|N|no|No|NO|true|True|TRUE} or "
                                           "{0|false|False|FALSE|on|On|ON|off|Off|OFF}.");
  return false;
}

/**
   \return bool if string [p, p+len) is a yaml boolean or c++ printed boolean,
   else throw string_to_exception
*/
inline void string_to_impl(std::string const& s, bool& x) {
  x = parse_bool((char const*)s.data(), (unsigned)s.size());
}

inline void string_to_impl(std::string const& s, char& x) {
  if (s.size() != 1) VTHROW_A_MSG(string_to_exception, "'" << s << "': converting string to character.");
  x = s[0];
}

inline std::string to_string_impl(bool x) {
  return x ? str_true : str_false;
}

inline std::string to_string_impl(char x) {
  return std::string(1, x);
}

inline std::string to_string_impl(char const* s) {
  return std::string(s);
}

#if GRAEHL_HAVE_LONGER_LONG
inline void string_to_impl(std::string const& s, unsigned long& x) {
  x = strtoul_complete(s.c_str());
}
inline void string_to_impl(char const* s, unsigned long& x) {
  x = strtoul_complete(s);
}
#endif
// FIXME: end code duplication

typedef char const* const PrintfFormat;
typedef unsigned const PrintfBytes;

PrintfFormat fmt_double_for_float_roundtrip = "%.9g";
PrintfFormat fmt_double_for_float_default = "%.7g";
PrintfFormat fmt_double_roundtrip = "%.17g";
PrintfFormat fmt_double_default = "%.15g";
PrintfFormat fmt_double_precision2
    = "%.*g";  // http://www.cplusplus.com/reference/cstdio/printf/ suggests this is std

/**
   enough space to printf 0-terminated -1.238945783e+0301 or whatever the max is
*/
PrintfBytes bytes_double_for_float_roundtrip = 24;
PrintfBytes bytes_double_for_float_default = 20;
PrintfBytes bytes_double_roundtrip = 36;
PrintfBytes bytes_double_default = 32;

/* 9 decimal places needed to avoid rounding error in float->string->float. 17 for double->string->double
   in terms of usable decimal places, there are 6 for float and 15 for double
*/
inline std::string to_string_roundtrip(float x) {
  char buf[bytes_double_for_float_roundtrip];
  return std::string(buf, buf + std::sprintf(buf, fmt_double_for_float_roundtrip, (double)x));
}

inline std::string to_string_impl(float x) {
#if GRAEHL_USE_FTOA
  return ftos(x);
#else
  char buf[bytes_double_for_float_default];
  return std::string(buf, buf + std::sprintf(buf, fmt_double_for_float_default, (double)x));
#endif
}

inline std::string to_string_roundtrip(double x) {
  char buf[bytes_double_roundtrip];
  return std::string(buf, buf + std::sprintf(buf, fmt_double_roundtrip, x));
}

inline std::string to_string_impl(double x) {
#if GRAEHL_USE_FTOA
  return ftos(x);
#else
  char buf[bytes_double_default];
  return std::string(buf, buf + std::sprintf(buf, fmt_double_default, x));
#endif
}

inline void string_to_impl(char const* s, double& x) {
  if (s[0] == 'n' && s[1] == 'o' && s[2] == 'n' && s[3] == 'e' && s[4] == 0)
    x = (double)NAN;
  else
    x = std::atof(s);
}
inline void string_to_impl(char const* s, float& x) {
  if (s[0] == 'n' && s[1] == 'o' && s[2] == 'n' && s[3] == 'e' && s[4] == 0)
    x = (float)NAN;
  else
    x = (float)std::atof(s);
}
inline void string_to_impl(std::string const& s, double& x) {
  string_to_impl(s.c_str(), x);
}
inline void string_to_impl(std::string const& s, float& x) {
  string_to_impl(s.c_str(), x);
}

template <class Str>
bool try_string_to(Str const& str, Str& to, bool complete = true) {
  str = to;
  return true;
}

inline std::string const& to_string_impl(std::string const& d) {
  return d;
}

// ADL participates in closest-match

template <class To>
void string_to_impl(char const* str, To& to) {
  return string_to_impl(std::string(str), to);
}

template <class To>
void string_to_impl(char* str, To& to) {
  return string_to_impl(std::string(str), to);
}

template <class From, class To>
void string_to_impl(From const& from, To& to) {
  using namespace boost;
#if GRAEHL_USE_BOOST_LEXICAL_CAST
  to = lexical_cast<To>(from);
#else
  if (!try_string_to(from, to))
    throw std::runtime_error(std::string("Couldn't convert (string_to): ") + from);
#endif
}

template <class To, class From>
To string_to_impl(From const& from) {
  To r;
  string_to_impl(from, r);
  return r;
}

template <class Val>
std::string optional_to_string(boost::optional<Val> const& opt) {
  return opt ? to_string(*opt) : "none";
}

template <class Val>
boost::optional<Val>& string_to_optional(std::string const& str, boost::optional<Val>& opt) {
  if (str == "none" || str.empty())
    opt.reset();
  else
    opt = string_to<Val>(str);
  return opt;
}

template <class Val>
shared_ptr<Val>& string_to_shared_ptr(std::string const& str, shared_ptr<Val>& opt) {
  if (str == "none")
    opt.reset();
  else
    opt = make_shared<Val>(string_to<Val>(str));
  return opt;
}

#if 0
template <class Val>
std::string to_string(boost::optional<Val> const& opt)
{
  SHOWIF1(GRSTRINGTO,0, "(free fn) optional to STRING CALLED", opt);
  return opt ?  to_string_impl(*opt) : "none";
}

template <class Val>
boost::optional<Val>& string_to(std::string const& str, boost::optional<Val>& opt)
{
  string_to_optional(str, opt);
  return opt;
}
#endif

#if 0

namespace detail {

template <class To, class From>
struct self_string_to_opt {
  typedef To result_type;
  static inline To string_to(From const& str)
  {
    return string_to_impl<To>(str);
  }
  static inline void string_to(From const& from, To &to)
  {
    string_to_impl(from, to);
  }
};

template <class Self>
struct self_string_to_opt<Self, Self>
{
  typedef Self const& result_type;
  static inline Self const& string_to(Self const& x) { return x; }
  static inline Self & string_to(Self const& x, Self &to) { return to=x; }
};

}
#endif

template <class Str>
void string_to_impl(Str const& s, Str& d) {
  d = s;
}

template <class D, class Enable = void>
struct to_string_select {
  static inline std::string to_string(D const& d) {
    using namespace boost;
#if GRAEHL_USE_BOOST_LEXICAL_CAST
    return lexical_cast<std::string>(d);
#else
    std::ostringstream o;
    o << d;
    return o.str();
#endif
  }
  template <class Str>
  static inline D& string_to(Str const& s, D& v) {
    string_to_impl(s, v);
    return v;
  }
};

template <class From>
std::string to_string_impl(From const& d) {
  return to_string_select<From>::to_string(d);
}

template <class From>
std::string to_string(From const& d) {
  return to_string_impl(d);
}

#if 0
template <class To, class From>
typename detail::self_string_to_opt<
  typename boost::remove_reference<To>::type
 ,typename boost::remove_reference<From>::type
>::result_type
string_to(From const& from)
{
  return
    detail::self_string_to_opt<typename boost::remove_reference<To>::type
                              ,typename boost::remove_reference<From> >
    ::string_to(from);
}
#endif

template <class To, class From>
To& string_to(From const& from, To& to) {
  to_string_select<To>::string_to(from, to);
  return to;
}

template <class To, class From>
To string_to(From const& from) {
  To to;
  to_string_select<To>::string_to(from, to);
  return to;
}

template <class V>
struct is_pair {
  enum { value = 0 };
};

template <class A, class B>
struct is_pair<std::pair<A, B>> {
  enum { value = 1 };
};

template <class V>
struct is_vector {
  enum { value = 0 };
};

template <class A, class B>
struct is_vector<std::vector<A, B>> {
  enum { value = 1 };
};

static std::string const string_to_sep_pair = "->";

template <class V>
struct to_string_select<V, typename enable_if<is_pair<V>::value>::type> {
  static inline std::string to_string(V const& p) {
    return graehl::to_string(p.first) + string_to_sep_pair + graehl::to_string(p.second);
  }
  template <class Str>
  static inline void string_to(Str const& s, V& v) {
    using namespace std;
    string::size_type p = s.find(string_to_sep_pair);
    if (p == string::npos)
      VTHROW_A_MSG(string_to_exception, "'" << s << "': pair missing " << string_to_sep_pair << " separator");
    string::const_iterator b = s.begin();
    graehl::string_to(std::string(b, b + p), v.first);
    graehl::string_to(std::string(b + p + string_to_sep_pair.size(), s.end()), v.second);
  }
};


template <class V>
struct is_optional {
  enum { value = 0 };
};

template <class V>
struct is_optional<boost::optional<V>> {
  enum { value = 1 };
};

template <class V>
struct is_shared_ptr {
  enum { value = 0 };
};

#if GRAEHL_CPP11
template <class V>
struct is_shared_ptr<std::shared_ptr<V>> {
  enum { value = 1 };
};
#endif

template <class V>
struct is_shared_ptr<boost::shared_ptr<V>> {
  enum { value = 1 };
};

template <class V>
struct to_string_select<V, typename enable_if<is_optional<V>::value>::type> {
  static inline std::string to_string(V const& opt) { return opt ? graehl::to_string(*opt) : "none"; }
  template <class Str>
  static inline void string_to(Str const& s, V& v) {
    string_to_optional(s, v);
  }
};

template <class V>
struct to_string_select<V, typename enable_if<is_shared_ptr<V>::value>::type> {
  static inline std::string to_string(V const& opt) { return opt ? graehl::to_string(*opt) : "none"; }
  template <class Str>
  static inline void string_to(Str const& s, V& v) {
    string_to_shared_ptr(s, v);
  }
};


static char const nonstring_container_sep = ',';

template <class V>
struct to_string_select<V, typename enable_if<is_nonstring_container<V>::value>::type> {
  static inline std::string to_string(V const& p, char sep = nonstring_container_sep) {
    std::string r;
    typename V::const_iterator i = p.begin(), e = p.end();
    if (i != e) {
      r.append(graehl::to_string(*i));
      while (++i != e) {
        r.push_back(sep);
        r.append(graehl::to_string(*i));
      }
    }
    return r;
  }
  template <class Str>
  static inline void string_to(Str const& str, V& v) {
    typename Str::size_type start = 0, end;
    while ((end = str.find(nonstring_container_sep, start)) != Str::npos)
      append(Str(str, start, end - start), v);
    append(Str(str, start), v);
  }

 private:
  template <class Str>
  static inline void append(Str const& str, V& v) {
#if GRAEHL_CPP17
    graehl::string_to(str, v.emplace_back());
#else
#if GRAEHL_CPP11
    v.emplace_back();
#else
    v.push_back(V());
#endif
    graehl::string_to(str, v.back());
#endif
  }
};

#ifdef GRAEHL_TEST
BOOST_AUTO_TEST_CASE(test_string_to) {
  BOOST_CHECK_EQUAL("1.5", to_string(1.5f));
  std::string mil = to_string(1500000.f);
  BOOST_CHECK(mil == "1500000" || mil == "1.5e+06" || mil == "1.5e+006");
  BOOST_CHECK_EQUAL("123456", to_string(123456.f));
  BOOST_CHECK_EQUAL("0.001953125", to_string(0.001953125f));
}
#endif


}

#endif
