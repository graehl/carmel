#ifndef INT_TYPES_JG2012531_HPP
#define INT_TYPES_JG2012531_HPP

#if defined(__APPLE__) && defined(__GNUC__)
# define INT_DIFFERENT_FROM_INTN 0
# define PTRDIFF_DIFFERENT_FROM_INTN 0
#endif

#ifndef INT_DIFFERENT_FROM_INTN
# define INT_DIFFERENT_FROM_INTN 0
#endif

#ifndef PTRDIFF_DIFFERENT_FROM_INTN
# define PTRDIFF_DIFFERENT_FROM_INTN 0
#endif

#ifndef HAVE_LONGER_LONG
# define HAVE_LONGER_LONG 0
#endif

#ifndef HAVE_LONG_DOUBLE
# define HAVE_LONG_DOUBLE 0
#endif

#ifndef HAVE_64BIT_INT64_T
# define HAVE_64BIT_INT64_T 1
#endif

#include <boost/cstdint.hpp>
#include <limits>

namespace graehl {

template <class T>
struct signed_for_int {
};


#define DEFINE_SIGNED_FOR_3(t,it,ut)                                                                    \
  template <>                                                                                           \
  struct signed_for_int<t> {                                                                            \
    typedef ut unsigned_t;                                                                              \
    typedef it signed_t;                                                                                \
    typedef t original_t;                                                                               \
    enum { toa_bufsize = 3 + std::numeric_limits<t>::digits10, toa_bufsize_minus_1=toa_bufsize-1 };     \
  };

// toa_bufsize will hold enough chars for a c string converting to sign,digits (for both signed and unsigned types), because normally an unsigned would only need 2 extra chars. we reserve 3 explicitly for the case that itoa(buf,UINT_MAX,true) is called, with output +4......

#define DEFINE_SIGNED_FOR(it) DEFINE_SIGNED_FOR_3(it,it,u ## it) DEFINE_SIGNED_FOR_3(u ## it,it,u ## it)
#define DEFINE_SIGNED_FOR_2(sig,unsig) DEFINE_SIGNED_FOR_3(sig,sig,unsig) DEFINE_SIGNED_FOR_3(unsig,sig,unsig)

DEFINE_SIGNED_FOR(int8_t)
DEFINE_SIGNED_FOR(int16_t)
DEFINE_SIGNED_FOR(int32_t)
DEFINE_SIGNED_FOR(int64_t)
#if INT_DIFFERENT_FROM_INTN
DEFINE_SIGNED_FOR_2(int,unsigned)
#if HAVE_LONGER_LONG
DEFINE_SIGNED_FOR_2(long int,long unsigned)
#endif
#endif
#if PTRDIFF_DIFFERENT_FROM_INTN
DEFINE_SIGNED_FOR_2(std::ptrdiff_t,std::size_t)
#endif

#if INT_DIFFERENT_FROM_INTN
#define GRAEHL_FOR_INT_DIFFERENT_FROM_INTN_TYPES(x) x(int) x(unsigned)
#else
#define GRAEHL_FOR_INT_DIFFERENT_FROM_INTN_TYPES(x)
#endif


#if PTRDIFF_DIFFERENT_FROM_INTN
#define GRAEHL_FOR_PTRDIFF_DIFFERENT_FROM_INTN_TYPES(x) x(std::ptrdiff_t) x(std::size_t)
#else
#define GRAEHL_FOR_PTRDIFF_DIFFERENT_FROM_INTN_TYPES(x)
#endif

#if HAVE_LONGER_LONG
#define GRAEHL_FOR_LONGER_LONG_TYPES(x) x(long) x(long unsigned)
#else
#define GRAEHL_FOR_LONGER_LONG_TYPES(x)
#endif

#define GRAEHL_FOR_DISTINCT_INT_TYPES(x) x(int8_t) x(int16_t) x(int32_t) x(int64_t) \
  GRAEHL_FOR_INT_DIFFERENT_FROM_INTN_TYPES(x) \
  GRAEHL_FOR_LONGER_LONG_TYPES(x) \
  GRAEHL_FOR_PTRDIFF_DIFFERENT_FROM_INTN_TYPES(x)


#if HAVE_LONG_DOUBLE
# define GRAEHL_FOR_DISTINCT_FLOAT_TYPES_LONG_DOUBLE(x) x(long double)
#else
# define GRAEHL_FOR_DISTINCT_FLOAT_TYPES_LONG_DOUBLE(x)
#endif

#define GRAEHL_FOR_DISTINCT_FLOAT_TYPES(x) x(float) x(double) GRAEHL_FOR_DISTINCT_FLOAT_TYPES_LONG_DOUBLE(x)
}

#endif // INT_TYPES_JG2012531_HPP
