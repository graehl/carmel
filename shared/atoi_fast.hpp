#ifndef ATOI_FAST_JG2012615_HPP
#define ATOI_FAST_JG2012615_HPP

#ifndef HAVE_STRTOUL
#ifdef _MSC_VER
# define HAVE_STRTOUL 0
#else
# define HAVE_STRTOUL 1
#endif
#endif

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

#include <graehl/shared/verbose_exception.hpp>

VERBOSE_EXCEPTION_DECLARE(string_to_exception)

namespace graehl {


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

// now for any char iter range
template <class U,class I>
inline U atou_fast(I i,I end) {
  U x=0;
  if (i==end) return x;
  for (;i!=end;++i) {
    const char c=*i;
    if (c<'0' || c>'9') return x;
    x*=10;
    x+=c-'0';
  }
  return x;
}

template <class U,class I>
inline U atou_fast_advance(I &i,I end) {
  U x=0;
  if (i==end) return x;
  for (;i!=end;++i) {
    const char c=*i;
    if (c<'0' || c>'9') return x;
    x*=10;
    x+=c-'0';
  }
  return x;
}

template <class U,class Str,class I>
inline U atou_fast_advance_nooverflow(Str const& s,I &i,I end) {
  const U maxTenth=boost::integer_traits<U>::const_max/10;
  U x=0;
  if (i==end) return x;
  for (;i!=end;++i) {
    const char c=*i;
    if (c<'0' || c>'9') return x;
    if (x>maxTenth)
      THROW_MSG(string_to_exception,"ascii to unsigned overflow on char "<<c<<" in '"<<s<<"': "<<x<<"*10 > "<<boost::integer_traits<U>::const_max);
    x*=10;
    U prev=x;
    x+=c-'0';
    if (x<prev)
      THROW_MSG(string_to_exception,"ascii to unsigned overflow on char "<<c<<" in '"<<s<<"': "<<prev<<" * 10 + "<<c<<" => (overflow) "<<x);
  }
  return x;
}

template <class U,class Str,class I>
inline U atou_fast_complete(Str const& s,I begin,I end) {
  I i=begin;
  U r=atou_fast_advance_nooverflow<U>(s,i,end);
  if (i!=end) THROW_MSG(string_to_exception,"ascii to unsigned incomplete - only used first "<<i-begin<<" characters of '"<<s<<"' => "<<r<<" - trim whitespace too");
  return r;
}

template <class U,class Str>
inline U atou_fast_complete(Str const& s) {
  return atou_fast_complete<U>(s,s.begin(),s.end());
}

template <class U>
inline U atou_fast_complete(char const* s) {
  return atou_fast_complete<U>(s,s,s+std::strlen(s));
}


template <class I>
inline I atou_fast(std::string const& s) { // faster than stdlib atoi. doesn't return how much of string was used.
  return atou_fast<I>(s.begin(),s.end());
}

template <class I,class It>
inline I atoi_fast(It i,It end) {
  I x=0;
  if (i==end) return x;
  bool neg=false;
  if (*i=='-') {
    neg=true;
    ++i;
  }
  for (;i!=end;++i) {
    const char c=*i;
    if (c<'0' || c>'9') return x;
    x*=10;
    x+=c-'0';
  }
  return neg?-x:x;
}

template <class I>
inline I atoi_fast(std::string const& s) { // faster than stdlib atoi. doesn't return how much of string was used.
  return atoi_fast<I>(s.begin(),s.end());
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


inline long strtol_complete(char const* s,int base=0) {
  char *e;
  if (*s) {
    long r=strtol(s,&e,base);
    char c=*e;
    if (!c || std::isspace(c)) //simplifying assumption: we're happy if there's other stuff in the string, so long as the number ends in a space or eos. TODO: loop consuming spaces until end?
      return r;
    THROW_MSG(string_to_exception,"integer from string '"<<s<<"' => "<<r<<" had extra chars: '"<<e<<"'");
  }
  // empty string => 0
  return 0;
}

// returns -INT_MAX or INT_MAX if number is too large/small
inline int strtoi_complete_bounded(char const* s,int base=0) {
  long l=strtol_complete(s,base);
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
inline int strtoi_complete_exact(char const* s,int base=10) {
  long l=strtol_complete(s,base);
  if (l<std::numeric_limits<int>::min() || l>std::numeric_limits<int>::max())
    THROW_MSG(string_to_exception,"Out of range for int " INTRANGE_STR ": '"<<s<<"'");
  return l;
}

//FIXME: preprocessor separation for tokens int<->unsigned int, long<->unsigned long, strtol<->strtoul ? massive code duplication
inline unsigned long strtoul_complete(char const* s,int base=0) {
  unsigned long r;
  if (*s) {
#if HAVE_STRTOUL
    char *e;
    r=strtoul(s,&e,base);
    char c=*e;
    if (!c || std::isspace(c)) //simplifying assumption: we're happy if there's other stuff in the string, so long as the number ends in a space or eos. TODO: loop consuming spaces until end?
      return r;
#else
  int nchars;
// unsigned long r=strtol(s,&e,base); //FIXME: not usually safe
    if (sscanf(s,"%lu%n",&r,&nchars) && s[nchars]=='\0')
      return r;
#endif
  }
  THROW_MSG(string_to_exception,"can't get integer from '"<<s<<"'");
  return 0; // quiet, warning!

}

inline unsigned strtou_complete_bounded(char const* s,int base=10) {
  unsigned long l=strtoul_complete(s,base);
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
inline unsigned strtou_complete_exact(char const* s,int base=10) {
  unsigned long l=strtoul_complete(s,base);
  if (l<std::numeric_limits<unsigned>::min() || l>std::numeric_limits<unsigned>::max())
    THROW_MSG(string_to_exception,"Out of range for unsigned int " UINTRANGE_STR ": '"<<s<<"'");
  return l;
}

}


#endif // ATOI_FAST_JG2012615_HPP
