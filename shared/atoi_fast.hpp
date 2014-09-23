#ifndef GRAEHL__ATOI_FAST_JG2012615_HPP
#define GRAEHL__ATOI_FAST_JG2012615_HPP

#ifndef HAVE_STRTOUL
#ifdef _MSC_VER
# define HAVE_STRTOUL 0
#else
# define HAVE_STRTOUL 1
#endif
#endif

#include <graehl/shared/warning_compiler.h>
#include <graehl/shared/verbose_exception.hpp>
#include <boost/integer_traits.hpp>
#include <limits>
#include <cstdlib>
#include <cctype>
// for faster numeric to/from string. TODO: separate into optional header
#if HAVE_STRTOUL
#include <limits.h> //strtoul
#endif
#undef min // damn you, windows
#undef max
#undef DELETE

#include <algorithm>
#include <iterator>

namespace graehl {

VERBOSE_EXCEPTION_DECLARE(string_to_exception)

//NOTE: stdlib atoi consumes dead whitespace; these don't
template <class U>
inline U atou_fast(const char *p) { // faster than stdlib atoi. doesn't return how much of string was used.
  U x=0;
  while (*p >= '0' && *p <= '9') {
    x*=10;
    x+=*p-'0';
    ++p;
  }
  return x;
}

template <class I>
inline I atoi_fast(const char *p) { // faster than stdlib atoi. doesn't return how much of string was used.
  I x=0;
  bool neg=false;
  if (*p == '-') {
    neg = true;
    ++p;
  }
  while (*p >= '0' && *p <= '9') {
    x*=10;
    x+=*p-'0';
    ++p;
  }
  return neg?-x:x;
}

template <class CharIt>
struct PutChars {
  CharIt begin, end;
  PutChars(CharIt begin, CharIt end)
      : begin(begin)
      , end(end)
  {}
  friend inline std::ostream& operator<<(std::ostream &out, PutChars const& self) {
    self.print(out);
    return out;
  }
  void print(std::ostream &out) const {
    for (CharIt i = begin; i != end; ++i)
      out.put(*i);
  }
};

template <class CharIt>
PutChars<CharIt> putChars(CharIt begin, CharIt end) {
  return PutChars<CharIt>(begin, end);
}


template <class U>
inline U hextou(char const* begin, char const* end, bool complete = true) {
  char const* p = begin;
  U x = 0;
  for(;;) {
    if (*p >= '0' && *p <= '9') {
      x *= 16;
      x += *p - '0';
    } else if (*p >= 'a' && *p <= 'f') {
      x *= 16;
      x += 10 + *p - 'a';
    } else if (*p >= 'A' && *p <= 'F') {
      x *= 16;
      x += 10 + *p - 'A';
    } else {
      if (complete)
        THROW_MSG(string_to_exception,
                  "hexidecimal string '" << putChars(begin, end) << "' had extra characters - value so far is " << x);
      return x;
    }
    if (++p == end)
      return x;
  }
}

template <class U>
inline U octaltou(char const* begin, char const* end, bool complete = true) {
  char const* p = begin;
  U x = 0;
  for(;;) {
    if (*p >= '0' && *p <= '7') {
      x *= 8;
      x += *p - '0';
    } else {
      if (complete)
        THROW_MSG(string_to_exception,
                  "hexidecimal string '" << putChars(begin, end) << "' had extra characters - value so far is " << x);
      return x;
    }
    if (++p == end)
      return x;
  }
}

template <class U, class I>
inline U atou_fast_advance(I &i, I end) {
  typedef typename std::iterator_traits<I>::value_type Char;
  U x=0;
  if (i==end) return x;
  for (;i!=end;++i) {
    const Char c=*i;
    if (c<'0' || c>'9') return x;
    x*=10;
    x+=c-'0';
  }
  return x;
}

template <class U, class I>
inline U atou_fast(I i, I end) {
  return atou_fast_advance<U>(i, end);
}

template <class U, class I>
inline U atou_fast_advance_nooverflow(I &i, I end) {
  typedef typename std::iterator_traits<I>::value_type Char;
  I begin = i;
  const U maxTenth=boost::integer_traits<U>::const_max/10;
  U x=0;
  if (i==end) return x;
  for (;i!=end;++i) {
    const Char c=*i;
    if (c<'0' || c>'9') return x;
    if (x>maxTenth)
      THROW_MSG(string_to_exception,
                "ascii to unsigned overflow on char " << c<<" in '"<<
                putChars(begin, end) << "': " << x<<"*10 > " << boost::integer_traits<U>::const_max);
    x*=10;
    U prev=x;
    x+=c-'0';
    if (x<prev)
      THROW_MSG(string_to_exception,
                "ascii to unsigned overflow on char " << c<<" in '"<<
                putChars(begin, end) << "': " << prev << " * 10 + " << c<<" => (overflow) " << x);
  }
  return x;
}

template <class U, class Str, class I>
inline U atou_fast_advance_nooverflow(Str const& s, I &i, I end) {
  return atou_fast_advance_nooverflow<U>(i, end);
}

template <class U, class It>
inline U atou_fast_complete(It begin, It end) {
  It i = begin;
  U r = atou_fast_advance<U>(i, end);
  if (i != end) THROW_MSG(string_to_exception,
                          "ascii to unsigned incomplete - only used first " << i-begin<<
                          " characters of '" << putChars(begin, end) << "' => " << r<<" - trim whitespace too");
  return r;
}


template <class U, class Str, class I>
inline U atou_fast_complete(Str const& s, I begin, I end) {
  return atou_fast_complete<U>(begin, end);
}

template <class U, class Str>
inline U atou_fast_complete(Str const& s) {
  return atou_fast_complete<U>(s.begin(), s.end());
}

template <class U>
inline U atou_fast_complete(char const* s) {
  return atou_fast_complete<U>(s, s+std::strlen(s)); //TODO: could skip strlen call w/ a few more lines of code
}


template <class I>
inline I atou_fast(std::string const& s) { // faster than stdlib atoi. doesn't return how much of string was used.
  return atou_fast<I>(s.begin(), s.end());
}

template <class I, class It>
inline I atoi_fast_advance(It &i, It end) {
  typedef typename std::iterator_traits<It>::value_type Char;
  I x=0;
  if (i==end) return x;
  bool neg=false;
  if (*i=='-') {
    neg=true;
    ++i;
  }
  for (;i!=end;++i) {
    const Char c=*i;
    if (c<'0' || c>'9') return x;
    x*=10;
    x+=c-'0';
  }
  return neg?-x:x;
}

template <class I, class It>
inline I atoi_fast(It i, It end) {
  return atoi_fast_advance<I>(i, end);
}


template <class I, class It>
inline I atoi_fast_complete(It begin, It end) {
  It i = begin;
  I r = atoi_fast_advance<I>(i, end);
  if (i != end) THROW_MSG(string_to_exception,
                          "ascii to int incomplete - only used first " << i-begin<<
                          " characters of '" << putChars(begin, end) << "' => " << r<<" - trim whitespace too");
  return r;
}

template <class I>
inline I atoi_fast(std::string const& s) { // faster than stdlib atoi. doesn't return how much of string was used.
  return atoi_fast<I>(s.begin(), s.end());
}

inline int atoi_nows(std::string const& s) {
  return atoi_fast<int>(s);
}

inline int atoi_nows(char const* s) {
  return atoi_fast<int>(s);
}

inline unsigned atou_nows(std::string const& s) {
  return atou_fast<unsigned>(s);
}

inline unsigned atou_nows(char const* s) {
  return atou_fast<unsigned>(s);
}


inline long strtol_complete(char const* s, int base=0) {
  char *e;
  if (*s) {
    long r=strtol(s, &e, base);
    char c=*e;
    if (!c || std::isspace(c)) //simplifying assumption: we're happy if there's other stuff in the string, so long as the number ends in a space or eos. TODO: loop consuming spaces until end?
      return r;
    THROW_MSG(string_to_exception, "integer from string '" << s<<"' => " << r<<" had extra chars: '" << e<<"'");
  }
  // empty string => 0
  return 0;
}

// returns -INT_MAX or INT_MAX if number is too large/small
inline int strtoi_complete_bounded(char const* s, int base=0) {
  long l=strtol_complete(s, base);
  if (l<std::numeric_limits<int>::min())
    return std::numeric_limits<int>::min();
  if (l>std::numeric_limits<int>::max())
    return std::numeric_limits<int>::max();
  return l;
}
#define RANGE_STR(x) #x
#ifdef INT_MIN
# define INTRANGE_STR "[" RANGE_STR(INT_MIN) "," RANGE_STR(INT_MAX) "]"
#else
# define INTRANGE_STR "[-2137483648,2147483647]"
#endif

// throw if out of int range
inline int strtoi_complete_exact(char const* s, int base=10) {
  long l=strtol_complete(s, base);
  if (l<std::numeric_limits<int>::min() || l>std::numeric_limits<int>::max())
    THROW_MSG(string_to_exception, "Out of range for int " INTRANGE_STR ": '" << s<<"'");
  return l;
}

//FIXME: preprocessor separation for tokens int<->unsigned, long<->unsigned long, strtol<->strtoul ? massive code duplication
inline unsigned long strtoul_complete(char const* s, int base=0) {
  unsigned long r;
  if (*s) {
#if HAVE_STRTOUL
    char *e;
    r=strtoul(s, &e, base);
    char c=*e;
    if (!c || std::isspace(c)) //simplifying assumption: we're happy if there's other stuff in the string, so long as the number ends in a space or eos. TODO: loop consuming spaces until end?
      return r;
#else
  int nchars;
// unsigned long r=strtol(s, &e, base); //FIXME: not usually safe
    if (sscanf(s, "%lu%n", &r, &nchars) && s[nchars]=='\0')
      return r;
#endif
  }
  THROW_MSG(string_to_exception, "can't get integer from '" << s<<"'");
  return 0; // quiet, warning!

}

inline unsigned strtou_complete_bounded(char const* s, int base=10) {
  unsigned long l=strtoul_complete(s, base);
#include <graehl/shared/warning_compiler.h>
  CLANG_DIAG_OFF(tautological-compare)
  if (l<std::numeric_limits<unsigned>::min())
    return std::numeric_limits<unsigned>::min();
  if (l>std::numeric_limits<unsigned>::max())
    return std::numeric_limits<unsigned>::max();
  return l;
}

#ifdef UINT_MIN
# define UINTRANGE_STR "[" RANGE_STR(UINT_MIN) "-" RANGE_STR(UINT_MAX) "]"
#else
# define UINTRANGE_STR "[0-4,294,967,295]"
#endif

// throw if out of int range
inline unsigned strtou_complete_exact(char const* s, int base=10) {
  unsigned long l=strtoul_complete(s, base);
  if (l<std::numeric_limits<unsigned>::min() || l>std::numeric_limits<unsigned>::max())
    THROW_MSG(string_to_exception, "Out of range for unsigned " UINTRANGE_STR ": '" << s<<"'");
  CLANG_DIAG_ON(tautological-compare)
  return l;
}


struct StrCursor {
  char const* p;
  char const* end;
  bool operator !() const {
    return p == end;
  }
  operator bool() const {
    return p != end;
  }
  template <class Str>
  StrCursor(Str const& str)
      : p(&*str.begin())
      , end(&*str.end())
  {}
  StrCursor(char const* cstr)
      : p(cstr)
      , end(cstr + std::strlen(cstr))
  {}
  StrCursor(char const* begin, char const* end)
      : p(begin)
      , end(end)
  {}
};

struct CstrCursor {
  char const* p;
  bool operator !() const {
    return !*p;
  }
  operator bool() const {
    return *p;
  }
  CstrCursor(char const* cstr)
      : p(cstr)
  {}
};


inline bool space_or_tab(char c) {
  return c == ' ' || c == '\t';
}

inline bool digit_char(char c) {
  return c >= '0' && c <= '9';
}

/**
   \return a Float (e.g. float or double) from a string - assumes valid regular
   (not nan, inf, -inf, etc) nonnegative real numbers like 1.23E-20 (with no + sign)

   std::strtod has the deficiency that the string is assumed to be
   null-terminated. if we have a Field (a char * pair) rather than a std::string
   we need this

   the next-fastest alternative is the heavier-to-compile Boost spirit
*/
template <class Float, class StrCursor>
Float scan_real_no_sign(StrCursor &c) {
  // before decimal
  Float value = (Float)0;
  while(digit_char(*c.p)) {
    value = value * (Float)10 + (*c.p - '0');
    ++c.p; if (!c) return value;
  }

  // optional decimal, after decimal
  if (*c.p == '.') {
    ++c.p;
    if (!c) return value;
    Float value_of_digit = (Float)0.1;
    while (digit_char(*c.p)) {
      value += (*c.p - '0') * value_of_digit;
      ++c.p; if (!c) return value;
      value_of_digit *= (Float)0.1;
    }
  }

  // optional exponent
  if (c && ((*c.p == 'e') || (*c.p == 'E'))) {
    ++c.p; if (!c) return value;

    // optional exponent sign
    bool const negative_exponent = (*c.p == '-');
    if (negative_exponent) {
      ++c.p; if (!c) return value;
    } else if (*c.p == '+') {
      ++c.p; if (!c) return value;
    }

    unsigned exponent = 0;
    while(digit_char(*c.p)) {
      exponent = exponent * 10 + (*c.p - '0');
      ++c.p; if (!c) break;
    }

    if (exponent > std::numeric_limits<Float>::max_exponent)
      return negative_exponent ? (Float)0 : std::numeric_limits<Float>::infinity();

    // more numerically accurate than std::pow10 and about as fast (since |exponent| is never large)
    if (negative_exponent) {
      while (exponent >= 8) { value *= (Float)1e-8; exponent -= 8; }
      while (exponent > 0) { value *= (Float)1e-1; exponent -= 1; }
    } else {
      while (exponent >= 8) { value *= (Float)1e8; exponent -= 8; }
      while (exponent > 0) { value *= (Float)1e1; exponent -= 1; }
    }
  }

  return value;
}

template <class StrCursor>
bool cursor_at_inf(StrCursor &i) {
  if (i) {
    char const* p = i.p;
    if (*i.p == 'i' || *i.p == 'I')
      if (++i.p) {
        if (*i.p == 'n' || *i.p == 'N')
          if (++i.p)
            if (*i.p == 'f' || *i.p == 'F') {
              ++i.p;
              return true;
            }
        i.p = p;
      }
  }
  return false;
}

/**
   as scan_real_no_sign but handling - or + sign, optionally leading space, and
   optionally allowing INF or inf (but not NaN) input
*/
template <class Float, class StrCursor>
Float scan_real(StrCursor &c, bool skip_leading_space = false, bool parse_inf = true) {
  if (!c) return (Float)0;
  if (skip_leading_space)
    while (space_or_tab(*c.p)) {
      ++c.p; if (!c) return (Float)0;
    }
  // optional sign
  bool negative = false;
  if (*c.p == '-') {
    ++c.p; if (!c) return (Float)0;
    negative = true;
  } else if (*c.p == '+') {
    ++c.p; if (!c) return (Float)0;
  }
  if (parse_inf && cursor_at_inf(c))
    return negative ? -std::numeric_limits<Float>::infinity() : std::numeric_limits<Float>::infinity();
  else {
    Float const value = scan_real_no_sign<Float>(c);
    return negative ? -value : value;
  }
}

template <class Float>
Float parse_real(char const* p, char const* end, bool require_complete = true) {
  StrCursor str(p, end);
  if (require_complete) {
    Float r = scan_real<Float>(str);
    if (str)
      THROW_MSG(string_to_exception,
                "conversion to real number from '" << putChars(p, end) << "' leaves unused characters " << putChars(str.p, end));
    return r;
  } else
    return scan_real<Float>(str);
}

inline float parse_float(char const* p, char const* end, bool require_complete = true) {
  return parse_real<float>(p, end, require_complete);
}

inline double parse_double(char const* p, char const* end, bool require_complete = true) {
  return parse_real<double>(p, end, require_complete);
}


template <class Float>
Float parse_real(char const* cstr, bool require_complete = true) {
  CstrCursor str(cstr);
  if (require_complete) {
    Float r = scan_real<Float>(str);
    if (str)
      THROW_MSG(string_to_exception,
                "conversion to real number from '" << cstr << "' leaves unused characters " << str.p);
    return r;
  } else
    return scan_real<Float>(str);
}

inline float parse_float(char const* cstr, bool require_complete = true) {
  return parse_real<float>(cstr, require_complete);
}

inline double parse_double(char const* cstr, bool require_complete = true) {
  return parse_real<double>(cstr, require_complete);
}

#ifdef GRAEHL_TEST
BOOST_AUTO_TEST_CASE(test_scan_real)
{
  BOOST_CHECK_EQUAL(parse_float("1.25"), 1.25f);
  BOOST_CHECK_THROW(parse_float("1.25a"), string_to_exception);
  BOOST_CHECK_THROW(parse_float("1.25 "), string_to_exception);
  BOOST_CHECK_EQUAL(parse_float("1.25 ", false), 1.25f);
  BOOST_CHECK_EQUAL(parse_float("1.25e10"), 1.25e10f);
  BOOST_CHECK_EQUAL(parse_float("1.25e+10"), 1.25e10f);
  BOOST_CHECK_EQUAL(parse_float("1.25e-10"), 1.25e-10f);
  BOOST_CHECK_EQUAL(parse_float("1e10"), 1e10f);
  BOOST_CHECK_EQUAL(parse_float("1e+10"), 1e10f);
  BOOST_CHECK_EQUAL(parse_float("1e-10"), 1e-10f);
  BOOST_CHECK_EQUAL(parse_float("123456"), 123456.f);
  BOOST_CHECK_EQUAL(parse_float("0.001953125"), 0.001953125f);
}
#endif


}

#endif
