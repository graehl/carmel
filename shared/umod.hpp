#ifndef GRAEHL_SHARED_UMOD_HPP
#define GRAEHL_SHARED_UMOD_HPP

    /* The binary % operator yields the remainder from the division of the first expression by the second. .... If both operands are nonnegative then the remainder is nonnegative; if not, the sign of the remainder is implementation-defined - seriously, wtf. */
// this always returns somethign in [0,m) unlike x % m which for -x can return something in (-m,m)
// NOTE: (unsigned)x % m where m is not a power of 2 will probably give wrong answers too.  it seems most often that a negative remainder IS returned.

#ifndef HAVE_NEGATIVE_REMAINDER
# define HAVE_NEGATIVE_REMAINDER 1
#endif

#include <boost/cstdint.hpp>

template <class I>
inline I umod(I x,I m) {
#if HAVE_NEGATIVE_REMAINDER
  I r=x%m;
  return r<0?r+m:r;
#else
  return x%m;
#endif
}

#define DEF_UINT_MOD(n) inline uint ## n ## _t umod(uint ## n ## _t x,uint ## n ## _t m) { return x%m; }

DEF_UINT_MOD(8)
DEF_UINT_MOD(16)
DEF_UINT_MOD(32)
DEF_UINT_MOD(64)

template <class I>
inline I unegmod(I x,I m) // return m-umod(x%m), i.e. the mod m inverse of umod(x,m)
{
#if HAVE_NEGATIVE_REMAINDER
  I r=x%m;
  return r<0?-r:m-r;
#else
  return m-(x%m);
#endif
}

#endif
