#ifndef FTOA_IEEE_HPP
#define FTOA_IEEE_HPP
/* essentially junk - needs fixing.  just use ftoa.hpp */

#ifndef DECIMAL_FOR_WHOLE
# define DECIMAL_FOR_WHOLE 0
#endif

// will use fast binary IEEE binary implementation, otherwise std::sprintf
#ifndef IEEE_FLOAT
# define IEEE_FLOAT 1
#endif

#include <assert.h>
#include <cstdio>
#include <graehl/shared/itoa.hpp>

inline void ftoa_ieee_error(float f,char const* msg="") {
  std::printf("ftoa_ieee error: %s f=%g\n",msg,f);
  assert(!"ftoa_ieee error");
}

namespace {
  const int ftoa_ieee_small_bufsize=22;//15;
  char ftoa_ieee_small_outbuf[ftoa_bufsize];
}

typedef union {
  int32_t L;
  float	F;
} LF_t;

//FIXME: this sucks.  no scientific notation style output.  also, may not work at all (fractional digits)?
inline char *append_ftoa_ieee_small(float f,char *outbuf)
{
  int32_t mantissa, int_part, frac_part;
  int32_t exp2; //TODO: does this work with regular int?
  LF_t x;

  x.F = f;

  exp2 = (0xFF & (x.L >> 23)) - 127;
  mantissa = (x.L & 0xFFFFFF) | 0x800000;
  frac_part = 0;
  int_part = 0;

  if (exp2 >= 31 || exp2 < -23) {
    ftoa_ieee_error(exp2,"exp2 out of range");
    return 0;
  } else if (exp2 >= 23)
    int_part = mantissa << (exp2 - 23);
  else if (exp2 >= 0) {
    int_part = mantissa >> (23 - exp2);
    frac_part = (mantissa << (exp2 + 1)) & 0xFFFFFF;
  }
  else /* if (exp2 < 0) */
    frac_part = (mantissa & 0xFFFFFF) >> -(exp2 + 1);

  if (x.L < 0)
    *p++ = '-';

  if (int_part == 0)
    *p++ = '0';
  else
    p=append_utoa(p,int_part);
  if (DECIMAL_FOR_WHOLE || frac_part)
    *p++ = '.';
  if (frac_part == 0)
    if (DECIMAL_FOR_WHOLE>1)
      *p++ = '0';
  else {
    char max = ftoa_bufsize - (p - outbuf) - 1;
    if (max > 7)
      max = 7;
    for (char m = 0; m < max; m++) {
/* frac_part *= 10;	*/
      frac_part = (frac_part << 3) + (frac_part << 1);
      *p++ = (frac_part >> 24) + '0';
      frac_part &= 0xFFFFFF;
    }
/* delete ending zeros */
    for (--p; p[0] == '0' && p[-1] != '.'; --p)
      ;
    ++p;
  }
  *p = 0;
  return p;
}


inline std::string ftos_small(float f) {
  char buf[ftoa_ieee_small_bufsize];
  return std::string(buf,append_ftoa_ieee_small(f,buf));
}

// not even THREADLOCAL - don't use.
inline char *static_ftoa_small(float f)
{
  append_ftoa_ieee_small(f,ftoa_ieee_small_outbuf);
  return ftoa_ieee_small_outbuf;
}

#endif
