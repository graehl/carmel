#ifndef GRAEHL_SHARED__FTOA_HPP
#define GRAEHL_SHARED__FTOA_HPP

//TODO: handle NAN +INF -INF

/* DECIMAL_FOR_WHOLE ; ftos(123)
   0 ; 123
   1 ; 123.
   2 ; 123.0
     ; ftos(0)
   0 ; 0
   1 ; 0
   2 ; 0 (sorry)
     ; ftos(.01)
   0 ; .01
   1 ; .01
   2 ; 0.01

*/
#ifndef FTOA_ONLY_SPRINTF
#define FTOA_ONLY_SPRINTF 1
#endif

#ifndef DECIMAL_FOR_WHOLE
# define DECIMAL_FOR_WHOLE 0
#endif

// avoid using scientific notation for numbers with small exponents.  may be faster than sprintf.
#ifndef FTOA_AVOID_E
# define FTOA_AVOID_E 1
#endif

#include <limits>
#include <stdint.h>
#include <iostream>
#include <cmath>
#include <assert.h>
#include <graehl/shared/itoa.hpp>
# include <cstdio>

namespace graehl {

template <class Float>
struct ftoa_traits {
};

//eP10,
// sigd decimal places normally printed, roundtripd needed so that round-trip float->string->float is identity

#define DEFINE_FTOA_TRAITS(FLOATT,INTT,P10,sigd,roundtripd,small,large)  \
template <> \
struct ftoa_traits<FLOATT> { \
  typedef INTT int_t; \
  typedef u ## INTT uint_t; \
  typedef FLOATT float_t; \
  enum { digits10=std::numeric_limits<INTT>::digits10, chars_block=P10, sigdig=sigd, roundtripdig=roundtripd, bufsize=roundtripdig+7 };    \
  static const double pow10_block = 1e ## P10; \
  static const float_t small_f = small; \
  static const float_t large_f = large; \
  static inline int sprintf(char *buf,double f) { return std::sprintf(buf,"%." #sigd "g",f); } \
  static inline uint_t fracblock(double frac) { assert(frac>=0 && frac<1); double f=frac*pow10_block;uint_t i=(uint_t)f;assert(i<pow10_block);return i; } \
  static inline float_t mantexp10(float_t f,int &exp) { float_t e=std::log10(f);float_t ec=std::ceil(e); exp=ec; return f/std::pow((float_t)10,ec); } \
  static inline bool use_sci_abs(float_t fa) { return fa<small || fa>large; } \
  static inline bool use_sci(float_t f) { return use_sci_abs(std::fabs(f)); }   \
};
//TODO: decide on computations in double (would hurt long double) or in native float type - any advantage?  more precision is usually better.

//10^22 = 0x1.0f0cf064dd592p73 is the largest exactly representable power of 10 in the binary64 format.  but round down to 18 so int64_t can hold it.
DEFINE_FTOA_TRAITS(double,int64_t,18,15,17,1e-9,1e8)
//i've heard that 1e10 is fine for float.  but we only have 1e9 (9 decimal places) in int32.
DEFINE_FTOA_TRAITS(float,int32_t,9,7,9,1e-5,1e4)


template <class F>
inline void ftoa_error(F f,char const* msg="") {
  using namespace std;
  cerr<<"ftoa error: "<<msg<<" f="<<f<<endl;
  assert(!"ftoa error");
}

// all of the below prepend and return new cursor.  null terminate yourself (like itoa/utoa)

//possibly empty string for ~0 (no sci notation fallback).  left padded.  [ret,p) are the digits.
template <class F>
inline char *prepend_pos_frac_digits(char *p,F frac) {
  assert(frac<1 && frac >0);
  typedef ftoa_traits<F> FT;
  //repeat if very small???  nah, require sci notation to take care of it.
  typename FT::uint_t i=FT::fracblock(frac);
  if (i>0) {
    unsigned n_skipped;
    char *d=utoa_drop_trailing_0(p,i,n_skipped);
    char *b=n_skipped+p-FT::chars_block;
    left_pad(b,d,'0');
    return b;
  } else {
    if (DECIMAL_FOR_WHOLE>1)
      *--p='0';
    return p;
  }


}

template <class F>
inline char *append_pos_frac_digits(char *p,F frac) { // '0' right-padded, nul terminated, return position of nul.  [p,ret) are the digits
  assert(frac<1 && frac >0);
  typedef ftoa_traits<F> FT;
  //repeat if very small???  nah, require sci notation to take care of it.
  typename FT::uint_t i=FT::fracblock(frac);
  if (i>0) {
    char *e=p+FT::chars_block;
    utoa_left_pad(p,e,i,'0');
    *e=0;
    return e;
  } else {
    if (DECIMAL_FOR_WHOLE>1)
      *p++='0';
    *p=0;
    return p;
  }
}

template <class F>
inline char *prepend_pos_frac(char *p,F frac) {
  p=prepend_pos_frac(p,frac);
  *--p='.';
  if (DECIMAL_FOR_WHOLE>1)
    *--p='0';
  return prepend_pos_frac_digits(p,frac);
}

template <class F>
inline char *append_pos_frac(char *p,F frac) {
  if (DECIMAL_FOR_WHOLE>1)
    *p++='0';
  *p++='.';
  return append_pos_frac_digits(p,frac);
}


template <class F>
inline char *prepend_frac(char *p,F frac,bool positive_sign=false) {
  if (frac==0) {
    *--p='0';
  } else if (frac<0) {
    p=prepend_pos_frac_digits(p,-frac);
    *--p='-';
  } else {
    p=prepend_pos_frac_digits(p,frac);
    if (positive_sign)
      *--p='+';
  }
  return p;
}

//TODO: append_frac, append_pos_sci, append_sci.  notice these are all composed according to a pattern (but reversing order of composition in pre vs app).  or can implement with copy from the other directly.


template <class F>
inline char *prepend_sci(char *p,F f,bool positive_sign_mant=false,bool positive_sign_exp=false) {
  typedef ftoa_traits<F> FT;
  int e10;
  F mant=FT::mantexp10(f,e10);
  assert(mant<1.00001);
  if (mant>=1.) {
    e10-=1;
    mant/=10;
  }
  p=itoa(p,e10,positive_sign_exp);
  *--p='e';
  return prepend_frac(p,mant,positive_sign_mant);
}


// modf docs for negative aren't very well defined (would have to test).  so handle sign externally
template <class F>
inline char *prepend_pos_nonsci(char *p,F f) {
  typedef ftoa_traits<F> FT;
  typedef typename FT::uint_t uint_t;
  assert(f<std::numeric_limits<uint_t>::max());
#if 0
  F intpart;
  F frac=std::modf(f,&intpart);
  uint_t u=intpart;
#else
  uint_t u=f;
  F frac=f-u;
#endif
  p=prepend_pos_frac_digits(p,frac);
  *--p='.';
  if (u==0) {
    if (DECIMAL_FOR_WHOLE>1)
      *--p='0';
  } else
    p=utoa(p,u);
}

template <class F>
inline char *prepend_nonsci(char *p,F f,bool positive_sign=false) {
  if (f==0)
    *--p='0';
  else if (f<0) {
    p=prepend_pos_nonsci(p,-f);
    *--p='-';
  } else {
    p=prepend_pos_nonsci(p,f);
    if (positive_sign)
      *--p='+';
  }
  return p;
}

//TODO: implement roundtrip option

// writes left->right so can be used to append. returns position of null terminator (end of string) (will assert bomb and return 0 if somehow float is not IEEE and exponent is out of range). [outbuf,outbuf+ftoa_bufsize) must be writable.
template <class F>
inline char *append_ftoa(char *outbuf,F f,bool roundtrip=false)
{
  typedef ftoa_traits<F> FT;
  char *p=outbuf;
  if (f == 0.0) {
    *p++='0';
    if (DECIMAL_FOR_WHOLE>0) {
      *p++='.';
      if (DECIMAL_FOR_WHOLE>1)
        *p++='0';
    }
    *p=0;
    return p;
  }
  return outbuf+FT::sprintf(outbuf,f);
}

template <class F>
inline char *prepend_ftoa(char *p,F f,bool roundtrip=false)
{
  typedef ftoa_traits<F> FT;
  return (roundtrip || FT::use_sci(f)) ?
    prepend_sci(p,f,roundtrip) :
    prepend_nonsci(p,f);
}

template <class F>
inline std::string ftos_from_append(F f,bool roundtrip=false) {
  typedef ftoa_traits<F> FT;
  char buf[FT::bufsize];
  return std::string(buf,append_ftoa(buf,f,roundtrip));
}

template <class F>
inline std::string ftos(F f,bool roundtrip=false) {
  typedef ftoa_traits<F> FT;
  char buf[FT::bufsize];
  if (FTOA_ONLY_SPRINTF) {
    return std::string(buf,append_ftoa(buf,f,roundtrip));
  } else {
    char *end=buf+FT::bufsize;
    return std::string(prepend_ftoa(end,f,roundtrip),end);
  }
}

namespace {
  const int ftoa_bufsize=30;
  char ftoa_outbuf[ftoa_bufsize];
}

// not even THREADLOCAL - don't use.
inline char *static_ftoa(float f,bool roundtrip=false)
{
  if (FTOA_ONLY_SPRINTF) {
    append_ftoa(ftoa_outbuf,f,roundtrip);
    return ftoa_outbuf;
  } else {
    char *end=ftoa_outbuf+ftoa_bufsize;
    return prepend_ftoa(end,f,roundtrip);
  }
}

}//ns


#ifdef FTOA_SAMPLE
# include <cstdio>
# include <sstream>
# include <iostream>
using namespace std;
using namespace graehl;
int main(int argc,char *argv[]) {
  printf("d U d U d U\n");
  for (int i=1;i<argc;++i) {
    double n;
    sscanf(argv[i],"%lf",&n);
    float f=n;
    cerr<<n<<" ->float= "<<f<<endl;
    cerr <<f<<" ->ftoa= "<<static_ftoa(f)<<endl;
  }
  return 0;
}
#endif

#endif
