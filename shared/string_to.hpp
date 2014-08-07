#ifndef GRAEHL__SHARED__STRING_TO_HPP
#define GRAEHL__SHARED__STRING_TO_HPP

/*
  string_to and to_string, which default to boost::lexical_cast, but, unlike that, may be overriden for user types other than supporting default/copy ctor and ostream <<, istream >>.

  note: optional<T> is "none" if absent, else regular string rep for T. this is different from the default optional streaming, which gives empty string for none, and " T" (space followed by T rep)

  USAGE:

  X string_to<X>(string);
  string to_string(X);
  X& string_to(string,X &); // note: returns the same ref you passed in, for convenience of use

  default implementation via boost lexical_cast if GRAEHL_USE_BOOST_LEXICAL_CAST, else stringstreams (quite slow, I'm sure)

  fast implementation for string, int, unsigned, float, double, and, if HAVE_LONGER_LONG=1, long

  also: to_string calls itos utos etc

  to override for your type without going through iostreams, define an ADL-locatable string_to_impl and/or to_string_impl

  ----

  may not be any faster than boost::lexical_cast in later incarnations (see http://accu.org/index.php/journals/1375)
  but is slightly simpler. no wide char or locale. if you use "C" locale, boost::lexical_cast takes advantage?

  see http://www.boost.org/doc/libs/1_47_0/libs/conversion/lexical_cast.htm#faq for benchmarks. so this should be faster still

  see also boost.spirit and qi for template-parser-generator stuff that should be about as fast!

  http://alexott.blogspot.com/2010/01/boostspirit2-vs-atoi.html
  and
  http://stackoverflow.com/questions/6404079/alternative-to-boostlexical-cast/6404331#6404331

*/

#include <cstdio>
#include <iostream>
#include <algorithm>
#include <iterator>
#ifndef DEBUG_GRAEHL_STRING_TO
# define DEBUG_GRAEHL_STRING_TO 1
#endif
#include <utility>
#include <graehl/shared/ifdbg.hpp>

#if DEBUG_GRAEHL_STRING_TO
#include <graehl/shared/show.hpp>
DECLARE_DBG_LEVEL(GRSTRINGTO)
# define GRSTRINGTO(x) x
#else
# define GRSTRINGTO(x)
#endif

#ifndef GRAEHL_USE_FTOA
#define GRAEHL_USE_FTOA 0
#endif
#ifndef HAVE_STRTOUL
# define HAVE_STRTOUL 1
#endif

#ifndef GRAEHL_USE_BOOST_LEXICAL_CAST
# define GRAEHL_USE_BOOST_LEXICAL_CAST 0
#endif

#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared.hpp>
#if GRAEHL_USE_BOOST_LEXICAL_CAST
# ifndef BOOST_LEXICAL_CAST_ASSUME_C_LOCALE
#  define BOOST_LEXICAL_CAST_ASSUME_C_LOCALE
# endif
# include <graehl/shared/warning_compiler.h>
GCC_DIAG_OFF(maybe-uninitialized)
# include <boost/lexical_cast.hpp>
GCC_DIAG_ON(maybe-uninitialized)
#endif
#include <boost/type_traits/remove_reference.hpp>
#include <limits> //numeric_limits
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
# include <graehl/shared/ftoa.hpp>
#else
#include <cstdlib>
#endif

#ifndef SPRINTF_JG_2013_05_21_HPP
#define SPRINTF_JG_2013_05_21_HPP
#include <cstdio>
#include <cstddef>
#include <cstdarg>
#include <cassert>
#include <cstdlib>
// for MS, this has some microsoft-only _snprintf etc fns that aren't fully C99 compliant - we'll provide a ::snprintf that is
#include <string>

#ifndef GRAEHL_MUTABLE_STRING_DATA
# if __cplusplus >= 201103L || CPP11
/// see http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2008/n2668.htm
/// - &str[0] required to be a writable array just like std::vector
#  define GRAEHL_MUTABLE_STRING_DATA 1
# else
#  ifdef _MSC_VER
#   if _MSC_VER >= 1500
     // VS 2008 or later
#     define GRAEHL_MUTABLE_STRING_DATA 1
#    else
#     define GRAEHL_MUTABLE_STRING_DATA 0
#   endif
#  else
#   define GRAEHL_MUTABLE_STRING_DATA 1
#  endif
# endif
#endif

#ifdef _MSC_VER
# define GRAEHL_va_free_copy(va)
//C++11/C99 requires va_copy - msvc doesn't have one
# ifndef va_copy
#  define va_copy(dst, src) ((dst) = (src))
# endif
#else
//gcc/clang have real va_copy
# define GRAEHL_va_free_copy(va) va_end(va)
#endif

// in unix we already have a C99 compliant ::snprintf
#ifdef _MSC_VER

# ifndef snprintf
#  define snprintf C99snprintf
# endif

/**
   conforms to C99 vsnprintf - you can call w/ buflen less than required, and
   return is number of chars that would have been written (so buflen should be >
   than that by at least 1 for '\0'
*/
inline int C99vsnprintf(char* buf, std::size_t buflen, const char* format
                        , va_list va)
{
  if (buflen) {
    va_list tmpva;
    va_copy(tmpva, va);
    // unfortunately, windows *snprintf_s return -1 if buffer was too small.
    int count = _vsnprintf_s(buf, buflen, _TRUNCATE, format, tmpva);
    GRAEHL_va_free_copy(tmpva);
    if (count >= 0) return count;
  }
  return _vscprintf(format, va); // counts # of chars that would be written
}

/**
   conforms to C99 snprintf - you can call w/ buflen less than required, and
   return is number of chars that would have been written (so buflen should be >
   than that by at least 1 for '\0'
*/
inline int C99snprintf(char* buf, std::size_t buflen, const char* format, ...)
{
  va_list va;
  va_start(va, format);
  int count = C99vsnprintf(buf, buflen, format, va);
  va_end(va);
  return count;
}
// end MSVC C99 wrappers
#else
//unix:
using std::snprintf;
# define C99vsnprintf std::vsnprintf
# define C99snprintf std::snprintf
#endif
#endif
/**

    MSVC lacks C99/posix/C++11 snprintf. this provides it (as snprintf in global
    namespace).

    also,

    SprintfStr str("%d %f %s, 1, 1.f, "cstring")

    is the same as

    std::string str("1 1.0 cstring")

    if you need only a cstr, try

    SprintfCstr<MAX_EXPECTED_CHARS_PLUS_1> buf(...); // works even if your estimate is too low

    buf.c_str(); //only valid as long as buf object exists

    also provides C99 compliant snprintf, C99snprintf (a synonym), and C99vsnprintf (varargs version)
*/

namespace LW { namespace xmt { namespace Util {


/**
   usage: Sprintf<>("%d %f %s, 1, 1.f, "cstring").str() gives string("1 1.0
   cstring"). cstr (or char * implicit) is only valid as long as Sprintf object
   exists. implicitly converts to std::string

   default buflen=52 makes sizeof(Sprintf<>) == 64 (w/ 64-bit ptrs)
   - note that we'll still succeed even if 52 is too small (by heap alloc)
*/
template <unsigned buflen=52>
struct SprintfCstr {
  char buf[buflen];
  unsigned size;
  char *cstr;

  /// with optimization, str() should be more efficient than std::string(*this),
  /// which would have to find strlen(cstr) first
  std::string str() const { return std::string(cstr, size); }

  operator std::string() const { return str(); }

  char *c_str() const { return cstr; }

  typedef char * const_iterator;
  typedef char * iterator;
  typedef char value_type;
  iterator begin() const { return cstr; }
  iterator end() const { return cstr+size; }

  SprintfCstr() : size(), cstr() {}

  void clear() {
    free();
    size = 0;
    cstr = 0;
  }

  void set(char const* format, va_list va) {
    clear();
    init(format, va);
  }

  void set(char const* format, ...) {
    va_list va;
    va_start(va, format);
    set(format, va);
    va_end(va);
  }

  SprintfCstr(char const* format, va_list va) {
    init(format, va);
  }

  SprintfCstr(char const* format, ...)
  {
    va_list va;
    va_start(va, format);
    init(format, va);
    va_end(va);
  }

  ~SprintfCstr() {
    free();
  }

  enum { first_buflen = buflen };
 protected:
  void init(char const* format, va_list va) {
    va_list tmpva;
    va_copy(tmpva, va);
    size = (unsigned)C99vsnprintf(buf, buflen, format, tmpva);
    GRAEHL_va_free_copy(tmpva);
    assert(size != (unsigned)-1);
    if (size >= buflen) {
      unsigned heapsz = size + 1;
      assert(heapsz);
      cstr = (char *)std::malloc(heapsz);
      size = C99vsnprintf(cstr, heapsz, format, va);
    } else
      cstr = buf;
    GRAEHL_va_free_copy(tmpva);
  }

  void free() {
    assert(size != (unsigned)-1);
    if (size >= buflen)
      std::free(cstr);
  }
};


#if GRAEHL_MUTABLE_STRING_DATA
/**
   if you're going to SprintfCstr<>(...).str(), this would be faster as it avoids
   a copy. this implicitly converts to (is a) std::string
*/
template <unsigned buflen=132>
struct Sprintf : std::string {

  /// with optimization, str() should be more efficient than std::string(*this),
  /// which would have to find strlen(cstr) first

  void set(char const* format, va_list va) {
    reinit(format, va);
  }

  void set(char const* format, ...) {
    va_list va;
    va_start(va, format);
    set(format, va);
    va_end(va);
  }

  Sprintf() {}

  Sprintf(char const* format, va_list va)
      : std::string(first_buflen, '\0')
  {
    init(format, va);
  }

  /**
     these overloads are better than a C varargs version because they avoid the
     (possibly non-optimizable va_list copy overhead). calling sprintf twice
     (once to get length, and then possibly again to allocate) requires copying
     varargs in GCC even if no reallocation is needed.
  */
  template <class V1>
  Sprintf(char const* format, V1 const& v1)
      : std::string(first_buflen, '\0')
  {
    unsigned size = (unsigned)C99snprintf(buf(), buflen, format, v1);
    if (size >= first_buflen) {
      unsigned heapsz = size + 1;
      resize(heapsz);
      size = C99snprintf(buf(), heapsz, format, v1);
    }
    resize(size);
  }

  template <class V1, class V2>
  Sprintf(char const* format, V1 const& v1, V2 const& v2)
      : std::string(first_buflen, '\0')
  {
    unsigned size = (unsigned)C99snprintf(buf(), buflen, format, v1, v2);
    if (size >= first_buflen) {
      unsigned heapsz = size + 1;
      resize(heapsz);
      size = C99snprintf(buf(), heapsz, format, v1, v2);
    }
    resize(size);
  }

  template <class V1, class V2, class V3>
  Sprintf(char const* format, V1 const& v1, V2 const& v2, V3 const& v3)
      : std::string(first_buflen, '\0')
  {
    unsigned size = (unsigned)C99snprintf(buf(), buflen, format, v1, v2, v3);
    if (size >= first_buflen) {
      unsigned heapsz = size + 1;
      resize(heapsz);
      size = C99snprintf(buf(), heapsz, format, v1, v2, v3);
    }
    resize(size);
  }

  Sprintf(char const* format, ...)
      : std::string(first_buflen, '\0')
  {
    va_list va;
    va_start(va, format);
    init(format, va);
    va_end(va);
  }
  enum { first_buflen = buflen };
 protected:
  void reinit(char const* format, va_list va) {
    if (size() < first_buflen)
      resize(first_buflen);
    init(format, va);
  }

  char *buf() {
    return &(*this)[0];
  }

  void init(char const* format, va_list va) {
    va_list tmpva;
    va_copy(tmpva, va);
    unsigned size = (unsigned)C99vsnprintf(buf(), buflen, format, tmpva);
    GRAEHL_va_free_copy(tmpva);
    assert(size != (unsigned)-1);
    if (size >= first_buflen) {
      unsigned heapsz = size + 1;
      assert(heapsz);
      resize(heapsz);
      size = C99vsnprintf(buf(), heapsz, format, va);
    }
    resize(size);
  }
};

#else // GRAEHL_MUTABLE_STRING_DATA
# define Sprintf SprintfCstr
#endif

typedef Sprintf<> SprintfStr;

namespace graehl {

//documentation:

inline std::string const& to_string(std::string const& str) { return str; }

template <class Data>
std::string to_string(Data const& out_str_to_data);

template <class Data,class Str>
Data& string_to(const Str &str,Data &out_str_to_data);

template <class Data,class Str> inline
Data string_to(const Str &str);
}


namespace graehl {


template <class I,class To>
bool try_stream_into(I & i,To &to,bool complete=true)
{
  i >> to;
  if (i.fail()) return false;
  if (complete) {
    char c;
    return !(i >> c);
  }
  return true;
}

template <class Str,class To>
bool try_string_to(Str const& str,To &to,bool complete=true)
{
  std::istringstream i(str);
  return try_stream_into(i,to,complete);
}

inline std::string to_string_impl(unsigned x) {
  return utos(x);
}

inline std::string to_string_impl(int x) {
  return itos(x);
}

#if HAVE_LONGER_LONG
inline void string_to_impl(std::string const& s,int &x) {
  x=strtoi_complete_exact(s.c_str());
}
inline void string_to_impl(char const* s,int &x) {
  x=strtoi_complete_exact(s);
}
#endif

inline void string_to_impl(std::string const& s,long &x) {
  x=strtol_complete(s.c_str());
}
inline void string_to_impl(char const* s,long &x) {
  x=strtol_complete(s);
}

#ifndef XMT_32 // size_t == unsigned int, avoid signature collision
inline void string_to_impl(std::string const& s,unsigned &x) {
  x=atou_fast_complete<unsigned>(s.c_str());
}

inline void string_to_impl(char const* s,unsigned &x) {
  x=atou_fast_complete<unsigned>(s);
}
#endif

inline void string_to_impl(std::string const& s,std::size_t &x) {
  x=atou_fast_complete<std::size_t>(s.c_str());
}
inline void string_to_impl(char const* s,std::size_t &x) {
  x=atou_fast_complete<std::size_t>(s);
}


namespace {
const std::string str_true="true", str_false="false";

inline char uc(char lc) {
  return lc + ('A' - 'a');
}

inline bool islc(char c, char lc) {
  return c == lc || c == uc(lc);
}

/**
   \return bool if string [p,p+len) is a yaml boolean or c++ printed boolean,
   else throw string_to_exception

   although this code is a bit obnoxious, it should be significantly faster than
   checking membership in a map<string> or serially comparing against a bunch of
   strings.
*/
inline bool parse_bool(char const* p, unsigned len) {
  switch(len) {
    case 1:
      if (*p == 'y' || *p == 'Y' || *p == '1') return true; // y or 1
      if (*p == 'n' || *p == 'N' || *p == '0') return false; // n or 0
      break;
    case 2:
      if (islc(*p, 'o')) {
        if (islc(p[1], 'n')) return true; // on
      } else if (islc(*p, 'n') && islc(p[1], 'o')) return false; // no
      break;
    case 3:
      if (islc(*p, 'y')) {
        if (islc(p[1], 'e') && islc(p[2], 's')) return true; // yes
      } else if (islc(*p, 'o') && islc(p[1], 'f') && islc(p[2], 'f')) return false; // off
      break;
    case 4:
      if (islc(*p, 't') && islc(p[1], 'r') && islc(p[2], 'u') && islc(p[3], 'e')) return true; //true
      break;
    case 5:
      if (islc(*p, 'f') && islc(p[1], 'a') && islc(p[2], 'l') && islc(p[3], 's') && islc(p[4], 'e')) return false; //false
      break;
  }
  VTHROW_A_MSG(string_to_exception,"'"<<std::string(p, p+len)<<"': not boolean - must be {1|y|Y|yes|Yes|YES|n|N|no|No|NO|true|True|TRUE} or {0|false|False|FALSE|on|On|ON|off|Off|OFF}.");
  return false;
}

}//ns

/**
   \return bool if string [p,p+len) is a yaml boolean or c++ printed boolean,
   else throw string_to_exception
*/
inline void string_to_impl(std::string const& s, bool &x) {
  x = parse_bool((char const*)s.data(), (unsigned)s.size());
}

inline void string_to_impl(std::string const& s,char &x) {
  if (s.size()!=1)
    VTHROW_A_MSG(string_to_exception,"'"<<s<<"': converting string to character.");
  x=s[0];
}

inline std::string to_string_impl(bool x)
{
  return x ? str_true : str_false;
}

inline std::string to_string_impl(char x)
{
  return std::string(1,x);
}

inline std::string to_string_impl(char const* s)
{
  return std::string(s);
}

#if HAVE_LONGER_LONG
inline void string_to_impl(std::string const& s,unsigned long &x) {
  x=strtoul_complete(s.c_str());
}
inline void string_to_impl(char const* s,unsigned long &x) {
  x=strtoul_complete(s);
}
#endif
//FIXME: end code duplication


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
  return std::string(buf,buf+std::sprintf(buf, fmt_double_for_float_roundtrip, (double)x));
}

inline std::string to_string_impl(float x) {
#if GRAEHL_USE_FTOA
  return ftos(x);
#else
  char buf[bytes_double_for_float_default];
  return std::string(buf,buf+std::sprintf(buf, fmt_double_for_float_default, (double)x));
#endif
}

inline std::string to_string_roundtrip(double x) {
  char buf[bytes_double_roundtrip];
  return std::string(buf,buf+std::sprintf(buf, fmt_double_roundtrip ,x));
}

inline std::string to_string_impl(double x) {
#if GRAEHL_USE_FTOA
  return ftos(x);
#else
  char buf[bytes_double_default];
  return std::string(buf,buf+std::sprintf(buf, fmt_double_default, x));
#endif
}

inline void string_to_impl(char const* s,double &x) {
  if (s[0]=='n'&&s[1]=='o'&&s[2]=='n'&&s[3]=='e'&&s[4]==0)
    x=(double)NAN;
  else
    x=std::atof(s);
}
inline void string_to_impl(char const* s,float &x) {
  if (s[0]=='n'&&s[1]=='o'&&s[2]=='n'&&s[3]=='e'&&s[4]==0)
    x=(float)NAN;
  else
    x=(float)std::atof(s);
}
inline void string_to_impl(std::string const& s,double &x) {
  string_to_impl(s.c_str(),x);
}
inline void string_to_impl(std::string const& s,float &x) {
  string_to_impl(s.c_str(),x);
}


template <class Str>
bool try_string_to(Str const& str,Str &to,bool complete=true)
{
  str=to;
  return true;
}

inline std::string const& to_string_impl(std::string const& d)
{
  return d;
}

// ADL participates in closest-match

template <class To>
void string_to_impl(char const* str,To &to) {
  return string_to_impl(std::string(str), to);
}

template <class To>
void string_to_impl(char *str,To &to) {
  return string_to_impl(std::string(str), to);
}


template <class From,class To>
void string_to_impl(From const& from,To &to)
{
#if GRAEHL_USE_BOOST_LEXICAL_CAST
  to=boost::lexical_cast<To>(from);
#else
  if (!try_string_to(from,to))
    throw std::runtime_error(std::string("Couldn't convert (string_to): ")+from);
#endif
}

template <class To,class From>
To string_to_impl(From const& from)
{
  To r;
  string_to_impl(from,r);
  return r;
}

template <class Val>
std::string optional_to_string(boost::optional<Val> const& opt)
{
  return opt ?  to_string(*opt) : "none";
}

template <class Val>
boost::optional<Val>& string_to_optional(std::string const& str,boost::optional<Val>& opt)
{
  if (str=="none" || str.empty())
    opt.reset();
  else
    opt=string_to<Val>(str);
  return opt;
}

template <class Val>
boost::shared_ptr<Val>& string_to_shared_ptr(std::string const& str,boost::shared_ptr<Val>& opt)
{
  if (str=="none" || str.empty())
    opt.reset();
  else
    opt = boost::make_shared<Val>(string_to<Val>(str));
  return opt;
}

#if 0
template <class Val>
std::string to_string(boost::optional<Val> const& opt)
{
  SHOWIF1(GRSTRINGTO,0,"(free fn) optional to STRING CALLED",opt);
  return opt ?  to_string_impl(*opt) : "none";
}

template <class Val>
boost::optional<Val>& string_to(std::string const& str,boost::optional<Val>& opt)
{
  string_to_optional(str,opt);
  return opt;
}
#endif

#if 0

namespace detail {

template <class To,class From>
struct self_string_to_opt {
  typedef To result_type;
  static inline To string_to(From const& str)
  {
    return string_to_impl<To>(str);
  }
  static inline void string_to(From const& from,To &to)
  {
    string_to_impl(from,to);
  }
};

template <class Self>
struct self_string_to_opt<Self,Self>
{
  typedef Self const& result_type;
  static inline Self const& string_to(Self const& x) { return x; }
  static inline Self & string_to(Self const& x,Self &to) { return to=x; }
};

}
#endif

template <class Str>
void string_to_impl(Str const &s,Str &d)
{
  d=s;
}

template <class D,class Enable=void>
struct to_string_select
{
  static inline std::string to_string(D const &d)
  {
#if GRAEHL_USE_BOOST_LEXICAL_CAST
    return boost::lexical_cast<std::string>(d);
#else
    std::ostringstream o;
    o << d;
    return o.str();
#endif
  }
  template <class Str>
  static inline D& string_to(Str const &s,D &v)
  {
    string_to_impl(s,v);
    return v;
  }
};


template <class From>
std::string to_string_impl(From const &d)
{
  return to_string_select<From>::to_string(d);
}

template <class From>
std::string
to_string(From const &d)
{
  return to_string_impl(d);
}

#if 0
template <class To,class From>
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


template <class To,class From>
To & string_to(From const& from,To &to)
{
  to_string_select<To>::string_to(from,to);
  return to;
}

template <class To,class From>
To string_to(From const& from)
{
  To to;
  to_string_select<To>::string_to(from,to);
  return to;
}

template <class V>
struct is_pair
{
  enum { value=0 };
};

template <class A,class B>
struct is_pair<std::pair<A,B> >
{
  enum { value=1 };
};

namespace {
std::string string_to_sep_pair="->";
}

template <class V>
struct to_string_select<V,typename boost::enable_if<is_pair<V> >::type> {
  static inline std::string to_string(V const &p)
  {
    return graehl::to_string(p.first)+string_to_sep_pair+graehl::to_string(p.second);
  }
  template <class Str>
  static inline void string_to(Str const &s,V &v)
  {
    using namespace std;
    string::size_type p=s.find(string_to_sep_pair);
    if (p==string::npos)
      VTHROW_A_MSG(string_to_exception,"'"<<s<<"': pair missing "<<string_to_sep_pair<<" separator");
    string::const_iterator b=s.begin();
    graehl::string_to(std::string(b,b+p),v.first);
    graehl::string_to(std::string(b+p+string_to_sep_pair.size(),s.end()),v.second);
  }
};

template <class V>
struct is_optional
{
  enum { value=0 };
};

template <class V>
struct is_optional<boost::optional<V> >
{
  enum { value=1 };
};


template <class V>
struct is_shared_ptr
{
  enum { value=0 };
};

template <class V>
struct is_shared_ptr<boost::shared_ptr<V> >
{
  enum { value=1 };
};


template <class V>
struct to_string_select<V,typename boost::enable_if<is_optional<V> >::type> {
  static inline std::string to_string(V const &opt)
  {
    return opt ? graehl::to_string(*opt) : "none";
  }
  template <class Str>
  static inline void string_to(Str const &s,V &v)
  {
    string_to_optional(s,v);
  }
};

template <class V>
struct to_string_select<V,typename boost::enable_if<is_shared_ptr<V> >::type> {
  static inline std::string to_string(V const &opt)
  {
    return opt ? graehl::to_string(*opt) : "none";
  }
  template <class Str>
  static inline void string_to(Str const &s,V &v)
  {
    string_to_shared_ptr(s,v);
  }
};


typedef std::vector<char> string_buffer;

struct string_builder : string_buffer
{
  bool operator == (std::string const& str) {
    std::size_t const len = str.size();
    return len == size() && !std::memcmp(begin(), &*str.begin(), len);
  }


  typedef char const* const_iterator;

  typedef char * iterator;

#if _WIN32 && (!defined(_SECURE_SCL) || _SECURE_SCL)
  iterator begin() {
    return empty() ? 0 : &*string_buffer::begin();
  }
  const_iterator begin() const {
    return empty() ? 0 : &*string_buffer::begin();
  }
  iterator end() {
    return empty() ? 0 : &*string_buffer::begin() + string_buffer::size();
  }
  const_iterator end() const {
    return empty() ? 0 : &*string_buffer::begin() + string_buffer::size();
  }
#else
  iterator begin() {
    return &*string_buffer::begin();
  }
  const_iterator begin() const {
    return &*string_buffer::begin();
  }
  iterator end() {
    return &*string_buffer::end();
  }
  const_iterator end() const {
    return &*string_buffer::end();
  }
#endif

  std::pair<char const*, char const*> slice() const {
    return std::pair<char const*, char const*>(begin(), end());
  }

  /**
     for backtracking.
  */
  struct unappend {
    string_buffer &builder;
    std::size_t size;
    unappend(string_buffer &builder)
        : builder(builder)
        , size(builder.size())
    {}
    ~unappend() {
      assert(builder.size() >= size);
      builder.resize(size);
    }
  };

  string_builder & shrink(std::size_t oldsize) {
    assert(oldsize <= size());
    this->resize(oldsize);
    return *this;
  }

  template <class Val>
  string_builder & operator<<(Val const& val) {
    return (*this)(val);
  }

  string_builder() { this->reserve(80); }
  string_builder(unsigned reserveChars) { this->reserve(reserveChars); }
  explicit string_builder(std::string const& str)
      : string_buffer(str.begin(),str.end()) {}
  string_builder & clear() {
    string_buffer::clear(); return *this; }
  template <class S>
  string_builder & append(S const& s)
  {
    return (*this)(s);
  }
  template <class S>
  string_builder & append(S const& s1,S const& s2)
  {
    return (*this)(s1,s2);
  }
  template <class S>
  string_builder & range(S const& s, word_spacer &sp)
  {
    return range(s.begin(), s.end(), sp);
  }
  template <class S>
  string_builder & range(S s1, S const& s2, word_spacer &sp)
  {
    for (; s1 != s2; ++s1) {
      sp.append(*this);
      (*this)(*s1);
    }
    return *this;
  }
  template <class S>
  string_builder & range(S const& s, char space = ' ')
  {
    word_spacer sp(space);
    return range(s.begin(), s.end(), sp);
  }
  template <class S>
  string_builder & range(S s1, S const& s2, char space = ' ')
  {
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
  string_builder & printf(char const* fmt, Val val, unsigned maxLen=40)
  {
    std::size_t sz = this->size();
    this->resize(sz + maxLen);
    // For Windows, snprintf is provided by Sprintf.hpp (in global namespace)
    unsigned written = (unsigned)snprintf(begin() + sz, maxLen, fmt, val);
    if (written >= maxLen) written = 0;
    this->resize(sz + written);
    return *this;
  }
  /**
     enough digits that you get the same value back when parsing the text.
  */
  string_builder & roundtrip(float x)
  {
    return printf(fmt_double_for_float_roundtrip, (double)x, 15);
    return *this; }
  /**
     enough digits that you get the same value back when parsing the text.
  */
  string_builder & roundtrip(double x)
  {
    return printf(fmt_double_roundtrip, x, 32);
    return *this; }
  string_builder & operator()(char c)
  {
    this->push_back(c);
    return *this; }
  string_builder & operator()(string_builder const& o)
  {
    (*this)(o.begin(), o.end());
    return *this; }
  template <class CharIter>
  string_builder & operator()(CharIter i,CharIter end)
  {
    this->insert(string_buffer::end(),i,end);
    return *this; }
  string_builder & operator()(std::pair<char const*, char const*> word)
  {
    this->insert(string_buffer::end(), word.first, word.second);
    return *this; }
  string_builder & operator()(std::string const& s)
  {
    (*this)(s.begin(),s.end());
    return *this; }
  string_builder & operator()(char const* s)
  {
    for (;*s;++s)
      this->push_back(*s);
    return *this; }
  string_builder & operator()(char const* s, unsigned len)
  { return (*this)(s, s+len); }

  template <class T>
  string_builder & operator()(T const& t)
  {
    (*this)(to_string(t));
    return *this;
  }
  string_builder & operator()(std::streambuf &ibuf)
  {
    typedef std::istreambuf_iterator<char> I;
    std::copy(I(&ibuf),I(),std::back_inserter(*this));
    return *this;
  }
  string_builder & operator()(std::istream &i)
  {
    return (*this)(*i.rdbuf());
  }
  std::string &assign(std::string &str) const
  {
    return str.assign(this->begin(),this->end());
  }
  std::string &to(std::string &str) const
  {
    return str.assign(this->begin(),this->end());
  }
  std::string str() const
  {
    return std::string(this->begin(),this->end());
  }
  std::string *new_str() const
  {
    return new std::string(this->begin(),this->end());
  }
  std::string strSkipPrefix(std::size_t prefixLen) const
  {
    return prefixLen > this->size() ? std::string() : std::string(this->begin() + prefixLen, this->end());
  }
  std::string str(std::size_t len) const
  {
    len=std::min(len,this->size());
    return std::string(this->begin(),this->begin()+len);
  }
  std::string shorten(std::size_t drop_suffix_chars)
  {
    std::size_t n=this->size();
    if (drop_suffix_chars>n)
      return std::string();
    return std::string(this->begin(),this->begin()+(n-drop_suffix_chars));
  }

  /**
     append space the second and subsequent times this is called with each
     initial 'bool first = true' (or every time if first == false)
  */
  string_builder & space_except_first(bool &first, char space=' ') {
    if (!first)
      operator()(space);
    first = false;
    return *this;
  }

  string_builder & append_space(std::string const& space)
  {
    if (!this->empty())
      operator()(space);
    return *this; }
  string_builder & append_space(char space=' ')
  {
    if (!this->empty())
      operator()(space);
    return *this; }

  string_builder & append_2space(char space=' ')
  {
    if (!this->empty()) {
      operator()(space);
      operator()(space);
    }
    return *this; }

  string_builder & word(std::string const& t,std::string const& space)
  {
    if (t.empty()) return *this;
    return append_space(space)(t);
  }
  string_builder & word(std::string const& t,char space=' ')
  {
    if (t.empty()) return *this;
    return append_space(space)(t);
  }


  string_builder &escape_char(char c) {
    char const quote = '"';
    char const backslash = '\\';
    if (c == quote || c== backslash)
      this->push_back(backslash);
    return (*this)(c);
  }

  template <class CharIter>
  string_builder &quoted(CharIter i, CharIter const& end) {
    char const quote = '"';
    this->push_back(quote);
    for (; i!=end; ++i)
      escape_char(*i);
    this->push_back(quote);
    return *this;
  }

  template <class Chars>
  string_builder &quoted(Chars const& str) {
    return quoted(str.begin(), str.end());
  }

  template <class S>
  string_builder & range_quoted(S const& s, word_spacer &sp)
  {
    return range_quoted(s.begin(), s.end(), sp);
  }
  template <class S>
  string_builder & range_quoted(S s1, S const& s2, word_spacer &sp)
  {
    for (; s1 != s2; ++s1) {
      sp.append(*this);
      quoted(*s1);
    }
    return *this;
  }
  template <class S>
  string_builder & range_quoted(S const& s)
  {
    word_spacer sp;
    return range_quoted(s.begin(), s.end(), sp);
  }
  template <class S>
  string_builder & range_quoted(S s1, S const& s2)
  {
    word_spacer sp;
    return range_quoted(s1, s2, sp);
  }
  template <class Out>
  void print(Out &out) const {
    out.write(begin(), string_buffer::size());
  }
  template <class Ch,class Tr>
  friend std::basic_ostream<Ch,Tr>& operator<<(std::basic_ostream<Ch,Tr> &out, string_builder const& self)
  { self.print(out); return out; }

  /// can't append anything else unless you first pop_back to remove the '\0'
  char * c_str() {
    this->push_back((char)0);
    return begin();
  }

  const_iterator data() const {
    return begin();
  }
};

// function object pointing to string_builder or buffer. cheap copy
struct append_string_builder
{
  string_builder &b;
  append_string_builder(string_builder &b) : b(b) { b.reserve(100); }
  append_string_builder(append_string_builder const& b) : b(b.b) {}
  append_string_builder & operator()(char c)
  {
    b(c); return *this;
  }
  template <class CharIter>
  append_string_builder const& operator()(CharIter const& i,CharIter const& end) const
  {
    b(i,end); return *this;
  }
  template <class T>
  append_string_builder const& operator()(T const& t) const
  {
    b(t); return *this;
  }
  template <class S>
  append_string_builder const& append(S const& s) const
  {
    return (*this)(s);
  }
  template <class S>
  append_string_builder const& append(S const& s1,S const& s2) const
  {
    return (*this)(s1,s2);
  }
  std::string str() const
  {
    return std::string(b.begin(),b.end());
  }
  std::string str(std::size_t len) const
  {
    return b.str(len);
  }
  std::string shorten(std::size_t drop_suffix_chars)
  {
    return b.shorten(drop_suffix_chars);
  }
};

struct append_string_builder_newline : append_string_builder
{
  std::string newline;
  append_string_builder_newline(string_builder &b,std::string const& newline="\n")
    : append_string_builder(b),newline(newline) {}
  append_string_builder_newline(append_string_builder_newline const& o)
    : append_string_builder(o),newline(o.newline) {}
  template <class S>
  append_string_builder_newline const& operator()(S const& s) const
  {
    append_string_builder::operator()(s);
    append_string_builder::operator()(newline);
    return *this;
  }
  template <class S>
  append_string_builder_newline const& operator()(S const& s1,S const& s2) const
  {
    return (*this)(s1,s2);
    append_string_builder::operator()(s1,s2);
    append_string_builder::operator()(newline);
    return *this;
  }
  template <class S>
  append_string_builder_newline const& append(S const& s) const
  {
    return (*this)(s);
  }
  template <class S>
  append_string_builder_newline const& append(S const& s1,S const& s2) const
  {
    return (*this)(s1,s2);
  }
};

template <class V>
struct to_string_select<V,typename boost::enable_if<is_nonstring_container<V> >::type> {
  static inline std::string to_string(V const &val)
  {
    string_builder b;
    b('[');
    bool first=true;
    for (typename V::const_iterator i=val.begin(),e=val.end();i!=e;++i) {
      if (first)
        first=false;
      else
        b(' ');
      b(*i);
    }
    b(']');
    return b.str();
  }
  template <class Str>
  static inline void string_to(Str const &s,V &v)
  {
    throw "string_to for sequences not yet supported";
  }
};

#ifdef GRAEHL_TEST
BOOST_AUTO_TEST_CASE(test_string_to)
{
  BOOST_CHECK_EQUAL("1.5", to_string(1.5f));
  std::string mil = to_string(1500000.f);
  BOOST_CHECK(mil == "1500000" || mil == "1.5e+06" || mil == "1.5e+006");
  BOOST_CHECK_EQUAL("123456", to_string(123456.f));
  BOOST_CHECK_EQUAL("0.001953125", to_string(0.001953125f));
}
#endif

}


#endif
