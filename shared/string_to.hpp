// Copyright 2014 Jonathan Graehl - http://graehl.org/
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

  string_to and to_string, which default to sdl::lexical_cast, but, unlike that, may be overriden for user
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
  sdl::lexical_cast in its latest incarnations (see
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


#include <cstdio>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <graehl/shared/append.hpp>
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

#include <graehl/shared/warning_compiler.h>
#include <graehl/shared/warning_push.h>
CLANG_DIAG_IGNORE_NEWER(unused-local-typedef)
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared.hpp>
#if GRAEHL_USE_BOOST_LEXICAL_CAST
#ifndef BOOST_LEXICAL_CAST_ASSUME_C_LOCALE
#define BOOST_LEXICAL_CAST_ASSUME_C_LOCALE
#endif
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#include <boost/lexical_cast.hpp>
#endif
#include <boost/type_traits/remove_reference.hpp>
#include <graehl/shared/warning_pop.h>
#include <limits>  //numeric_limits
#include <string>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <graehl/shared/word_spacer.hpp>
#include <graehl/shared/nan.hpp>
#include <graehl/shared/have_64_bits.hpp>
#include <graehl/shared/atoi_fast.hpp>
#include <graehl/shared/itoa.hpp>
#include <graehl/shared/is_container.hpp>
#if GRAEHL_USE_FTOA
#include <graehl/shared/ftoa.hpp>
#else
#include <cstdlib>
#endif
#include <graehl/shared/snprintf.hpp>
#include <boost/functional/hash.hpp>
#include <graehl/shared/base64.hpp>

namespace graehl {

// documentation:

inline std::string const& to_string(std::string const& str) {
  return str;
}

template <class Data>
std::string to_string(Data const& out_str_to_data);

template <class Data, class Str>
Data& string_to(const Str& str, Data& out_str_to_data);

template <class Data, class Str>
inline Data string_to(const Str& str);
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

namespace {
const std::string str_true = "true", str_false = "false";

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

}  // ns

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
#if GRAEHL_USE_BOOST_LEXICAL_CAST
  to = sdl::lexical_cast<To>(from);
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
boost::shared_ptr<Val>& string_to_shared_ptr(std::string const& str, boost::shared_ptr<Val>& opt) {
  if (str == "none")
    opt.reset();
  else
    opt = boost::make_shared<Val>(string_to<Val>(str));
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
#if GRAEHL_USE_BOOST_LEXICAL_CAST
    return sdl::lexical_cast<std::string>(d);
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
struct is_pair<std::pair<A, B> > {
  enum { value = 1 };
};

namespace {
std::string string_to_sep_pair = "->";
}

template <class V>
struct to_string_select<V, typename boost::enable_if<is_pair<V> >::type> {
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
struct is_optional<boost::optional<V> > {
  enum { value = 1 };
};

template <class V>
struct is_shared_ptr {
  enum { value = 0 };
};

template <class V>
struct is_shared_ptr<boost::shared_ptr<V> > {
  enum { value = 1 };
};

template <class V>
struct to_string_select<V, typename boost::enable_if<is_optional<V> >::type> {
  static inline std::string to_string(V const& opt) { return opt ? graehl::to_string(*opt) : "none"; }
  template <class Str>
  static inline void string_to(Str const& s, V& v) {
    string_to_optional(s, v);
  }
};

template <class V>
struct to_string_select<V, typename boost::enable_if<is_shared_ptr<V> >::type> {
  static inline std::string to_string(V const& opt) { return opt ? graehl::to_string(*opt) : "none"; }
  template <class Str>
  static inline void string_to(Str const& s, V& v) {
    string_to_shared_ptr(s, v);
  }
};

typedef std::vector<char> string_buffer;

struct string_builder : string_buffer {
  template <class Int>
  string_builder& base64LE_pad(Int x) {
    base64LE_append_pad(*this, x);
    return *this;
  }
  template <class Int>
  string_builder& base64LE(Int x) {
    base64LE_append(*this, x);
    return *this;
  }
  bool operator==(std::string const& str) {
    std::size_t const len = str.size();
    return len == size() && !std::memcmp(begin(), &*str.begin(), len);
  }

  typedef char const* const_iterator;

  typedef char* iterator;

#if _WIN32 && (!defined(_SECURE_SCL) || _SECURE_SCL)
  iterator begin() { return empty() ? 0 : &*string_buffer::begin(); }
  const_iterator begin() const { return empty() ? 0 : &*string_buffer::begin(); }
  iterator end() { return empty() ? 0 : &*string_buffer::begin() + string_buffer::size(); }
  const_iterator end() const { return empty() ? 0 : &*string_buffer::begin() + string_buffer::size(); }
#else
  iterator begin() { return &*string_buffer::begin(); }
  const_iterator begin() const { return &*string_buffer::begin(); }
  iterator end() { return &*string_buffer::end(); }
  const_iterator end() const { return &*string_buffer::end(); }
#endif

  std::pair<char const*, char const*> slice() const {
    return std::pair<char const*, char const*>(begin(), end());
  }

  /**
     for backtracking.
  */
  struct unappend {
    string_buffer& builder;
    std::size_t size;
    unappend(string_buffer& builder) : builder(builder), size(builder.size()) {}
    ~unappend() {
      assert(builder.size() >= size);
      builder.resize(size);
    }
  };

  string_builder& shrink(std::size_t oldsize) {
    assert(oldsize <= size());
    this->resize(oldsize);
    return *this;
  }

  template <class Val>
  string_builder& operator<<(Val const& val) {
    return (*this)(val);
  }

  string_builder() { this->reserve(80); }
  string_builder(unsigned reserveChars) { this->reserve(reserveChars); }
  explicit string_builder(std::string const& str) : string_buffer(str.begin(), str.end()) {}
  string_builder& clear() {
    string_buffer::clear();
    return *this;
  }
  template <class S>
  string_builder& append(S const& s) {
    return (*this)(s);
  }
  template <class S>
  string_builder& append(S const& s1, S const& s2) {
    return (*this)(s1, s2);
  }
  template <class S>
  string_builder& range(S const& s, word_spacer& sp) {
    return range(s.begin(), s.end(), sp);
  }
  template <class S>
  string_builder& range(S s1, S const& s2, word_spacer& sp) {
    for (; s1 != s2; ++s1) {
      sp.append(*this);
      (*this)(*s1);
    }
    return *this;
  }
  template <class S>
  string_builder& range(S const& s, char space = ' ') {
    word_spacer sp(space);
    return range(s.begin(), s.end(), sp);
  }
  template <class S>
  string_builder& range(S s1, S const& s2, char space = ' ') {
    word_spacer sp(space);
    return range(s1, s2, sp);
  }
  /**
     for printing simple numeric values - maxLen must be sufficient or else
     nothing is appended. also note that printf format strings need to match the
     Val type precisely - explicit cast or template arg may make this more
     obvious but compiler should know how to warn/error on mismatch
  */
  template <class Val>
  string_builder& printf(char const* fmt, Val val, unsigned maxLen = 40) {
    std::size_t sz = this->size();
    this->resize(sz + maxLen);
    // For Windows, snprintf is provided by shared/sprintf.hpp (in global namespace)
    unsigned written = (unsigned)snprintf(begin() + sz, maxLen, fmt, val);
    if (written >= maxLen) written = 0;
    this->resize(sz + written);
    return *this;
  }
  enum { kMaxInt32Chars = 12, kMaxInt64Chars = 22 };

  string_builder& operator()(unsigned char x) {
    utos_append((string_buffer&)*this, x);
    return *this;
  }
  string_builder& operator()(unsigned short x) {
    utos_append((string_buffer&)*this, x);
    return *this;
  }
  string_builder& operator()(unsigned x) {
    utos_append((string_buffer&)*this, x);
    return *this;
  }
  string_builder& operator()(std::size_t x) {
    utos_append((string_buffer&)*this, x);
    return *this;
  }

  /**
     enough digits that you get the same value back when parsing the text.
  */
  string_builder& roundtrip(float x) {
    return printf(fmt_double_for_float_roundtrip, (double)x, 15);
    return *this;
  }
  /**
     enough digits that you get the same value back when parsing the text.
  */
  string_builder& roundtrip(double x) {
    return printf(fmt_double_roundtrip, x, 32);
    return *this;
  }
  string_builder& operator()(char c) {
    this->push_back(c);
    return *this;
  }
  string_builder& operator()(string_builder const& o) {
    (*this)(o.begin(), o.end());
    return *this;
  }
  template <class CharIter>
  string_builder& operator()(CharIter i, CharIter end) {
    this->insert(string_buffer::end(), i, end);
    return *this;
  }
  string_builder& operator()(std::pair<char const*, char const*> word) {
    this->insert(string_buffer::end(), word.first, word.second);
    return *this;
  }
  string_builder& operator()(std::string const& s) {
    (*this)(s.begin(), s.end());
    return *this;
  }
  string_builder& operator()(char const* s) {
    for (; *s; ++s) this->push_back(*s);
    return *this;
  }
  string_builder& operator()(char const* s, unsigned len) { return (*this)(s, s + len); }

  template <class T>
  string_builder& operator()(T const& t) {
    (*this)(to_string(t));
    return *this;
  }
  string_builder& operator()(std::streambuf& ibuf) {
    typedef std::istreambuf_iterator<char> I;
    std::copy(I(&ibuf), I(), std::back_inserter(*this));
    return *this;
  }
  string_builder& operator()(std::istream& i) { return (*this)(*i.rdbuf()); }
  std::string& assign(std::string& str) const { return str.assign(this->begin(), this->end()); }
  std::string& to(std::string& str) const { return str.assign(this->begin(), this->end()); }
  boost::shared_ptr<std::string> make_shared_str() const {
    return boost::make_shared<std::string>(this->begin(), this->end());
  }
  std::string str() const { return std::string(this->begin(), this->end()); }
  std::string* new_str() const { return new std::string(this->begin(), this->end()); }
  std::string skipPrefix(std::size_t prefixLen) const {
    return prefixLen > this->size() ? std::string() : std::string(this->begin() + prefixLen, this->end());
  }
  std::string str(std::size_t len) const {
    len = std::min(len, this->size());
    return std::string(this->begin(), this->begin() + len);
  }
  std::string shorten(std::size_t drop_suffix_chars) {
    std::size_t n = this->size();
    if (drop_suffix_chars > n) return std::string();
    return std::string(this->begin(), this->begin() + (n - drop_suffix_chars));
  }

  /**
     append space the second and subsequent times this is called with each
     initial 'bool first = true' (or every time if first == false)
  */
  string_builder& space_except_first(bool& first, char space = ' ') {
    if (!first) operator()(space);
    first = false;
    return *this;
  }

  template <class Sep>
  string_builder& sep_except_first(bool& first, Sep const& sep) {
    if (!first) operator()(sep);
    first = false;
    return *this;
  }

  string_builder& append_space(std::string const& space) {
    if (!this->empty()) operator()(space);
    return *this;
  }
  string_builder& append_space(char space = ' ') {
    if (!this->empty()) operator()(space);
    return *this;
  }

  string_builder& append_2space(char space = ' ') {
    if (!this->empty()) {
      operator()(space);
      operator()(space);
    }
    return *this;
  }

  string_builder& word(std::string const& t, std::string const& space) {
    if (t.empty()) return *this;
    return append_space(space)(t);
  }
  string_builder& word(std::string const& t, char space = ' ') {
    if (t.empty()) return *this;
    return append_space(space)(t);
  }

  string_builder& escape_char(char c) {
    char const quote = '"';
    char const backslash = '\\';
    if (c == quote || c == backslash) this->push_back(backslash);
    return (*this)(c);
  }

  template <class CharIter>
  string_builder& quoted(CharIter i, CharIter const& end) {
    char const quote = '"';
    this->push_back(quote);
    for (; i != end; ++i) escape_char(*i);
    this->push_back(quote);
    return *this;
  }

  template <class Chars>
  string_builder& quoted(Chars const& str) {
    return quoted(str.begin(), str.end());
  }

  template <class S>
  string_builder& range_quoted(S const& s, word_spacer& sp) {
    return range_quoted(s.begin(), s.end(), sp);
  }
  template <class S>
  string_builder& range_quoted(S s1, S const& s2, word_spacer& sp) {
    for (; s1 != s2; ++s1) {
      sp.append(*this);
      quoted(*s1);
    }
    return *this;
  }
  template <class S>
  string_builder& range_quoted(S const& s) {
    word_spacer sp;
    return range_quoted(s.begin(), s.end(), sp);
  }
  template <class S>
  string_builder& range_quoted(S s1, S const& s2) {
    word_spacer sp;
    return range_quoted(s1, s2, sp);
  }
  template <class Out>
  void print(Out& out) const {
    out.write(begin(), string_buffer::size());
  }
  template <class Ch, class Tr>
  friend std::basic_ostream<Ch, Tr>& operator<<(std::basic_ostream<Ch, Tr>& out, string_builder const& self) {
    self.print(out);
    return out;
  }

  /// can't append anything else unless you first pop_back to remove the '\0'
  char* c_str() {
    this->push_back((char)0);
    return begin();
  }

  const_iterator data() const { return begin(); }
  friend inline std::size_t hash_value(string_builder const& self) { return self.hash(); }
  std::size_t hash() const { return boost::hash_range(begin(), end()); }

  template <class Seq, class Sep>
  string_builder& join(Seq const& seq, Sep const& sep) {
    bool first = true;
    for (typename Seq::const_iterator i = seq.begin(), e = seq.end(); i != e; ++i) sep_except_first(first, sep)(*i);
    return *this;
  }
  template <class Seq, class Sep>
  string_builder& join(Seq const& seq) {
    return join(seq, ' ');
  }

  template <class Seq, class After, class Sep>
  string_builder& join_and(Seq const& seq, After const& after, Sep const& sep) {
    for (typename Seq::const_iterator i = seq.begin(), e = seq.end(); i != e; ++i) (*this)(*i)(sep);
    return (*this)(after);
  }
};

template <class Seq, class Sep>
std::string joined_seq(Seq const& seq, Sep const& sep) {
  string_builder b;
  b.join(seq, sep);
  return std::string(b.begin(), b.end());
}

// function object pointing to string_builder or buffer. cheap copy
struct append_string_builder {
  string_builder& b;
  append_string_builder(string_builder& b) : b(b) { b.reserve(100); }
  append_string_builder(append_string_builder const& b) : b(b.b) {}
  append_string_builder& operator()(char c) {
    b(c);
    return *this;
  }
  template <class CharIter>
  append_string_builder const& operator()(CharIter const& i, CharIter const& end) const {
    b(i, end);
    return *this;
  }
  template <class T>
  append_string_builder const& operator()(T const& t) const {
    b(t);
    return *this;
  }
  template <class S>
  append_string_builder const& append(S const& s) const {
    return (*this)(s);
  }
  template <class S>
  append_string_builder const& append(S const& s1, S const& s2) const {
    return (*this)(s1, s2);
  }
  std::string str() const { return std::string(b.begin(), b.end()); }
  std::string str(std::size_t len) const { return b.str(len); }
  std::string shorten(std::size_t drop_suffix_chars) { return b.shorten(drop_suffix_chars); }
};

struct append_string_builder_newline : append_string_builder {
  std::string newline;
  append_string_builder_newline(string_builder& b, std::string const& newline = "\n")
      : append_string_builder(b), newline(newline) {}
  append_string_builder_newline(append_string_builder_newline const& o)
      : append_string_builder(o), newline(o.newline) {}
  template <class S>
  append_string_builder_newline const& operator()(S const& s) const {
    append_string_builder::operator()(s);
    append_string_builder::operator()(newline);
    return *this;
  }
  template <class S>
  append_string_builder_newline const& operator()(S const& s1, S const& s2) const {
    return (*this)(s1, s2);
    append_string_builder::operator()(s1, s2);
    append_string_builder::operator()(newline);
    return *this;
  }
  template <class S>
  append_string_builder_newline const& append(S const& s) const {
    return (*this)(s);
  }
  template <class S>
  append_string_builder_newline const& append(S const& s1, S const& s2) const {
    return (*this)(s1, s2);
  }
};

template <class V>
struct to_string_select<V, typename boost::enable_if<is_nonstring_container<V> >::type> {
  static inline std::string to_string(V const& val) {
    string_builder b;
    b('[');
    bool first = true;
    for (typename V::const_iterator i = val.begin(), e = val.end(); i != e; ++i) {
      if (first)
        first = false;
      else
        b(' ');
      b(*i);
    }
    b(']');
    return b.str();
  }
  template <class Str>
  static inline void string_to(Str const& s, V& v) {
    throw "string_to for sequences not yet supported";
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
