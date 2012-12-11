#ifndef GRAEHL_SHARED__NAN_HPP
#define GRAEHL_SHARED__NAN_HPP

//TODO: switch to C99 isnan isfinite isinf etc. (faster)


#if defined(_MSC_VER)
# define WIN32_NAN
# define GRAEHL_ISNAN(x) ( _isnan(x) != 0 )
#else
# if defined __FAST_MATH__
// gcc -ffast-math breaks std::isnan
#  undef isnan
#  define GRAEHL_ISNAN isnan
#  define isnan isnan
// above is to prevent anyone else's (e.g. stdint.h) #ifndef isnan from changing it on us
#  include <stdint.h>
static inline int isnan(float f)
{
  union { float f; uint32_t x; } u = { f };
  return (u.x << 1) > 0xff000000u;
}
// TODO: fix for double also
# else
#  if defined(_STLPORT_VERSION)
#   include <math.h>
#   define GRAEHL_ISNAN(x) isnan(x) // in stlport, only c99 version of isnan is available
#  else
#   include <cmath>
#   define GRAEHL_ISNAN(x) std::isnan(x) // gcc native stdlib includes isnan as an exported template function
#  endif
# endif
#endif

#include <limits>

#ifdef WIN32_NAN
# include <float.h>
//# include <xmath.h>
namespace {
const unsigned int graehl_nan[2] = {0xffffffff, 0x7fffffff};
}
# undef NAN
# define NAN (*(const double *) graehl_nan)
#endif

#ifndef NAN
# define NAN (0.0/0.0)
#endif

namespace graehl {

template <bool> struct nan_static_assert;
template <> struct nan_static_assert<true> { };

// is_iec559 i.e. only IEEE 754 float has x != x <=> x is nan
template<typename T>
inline bool is_nan(T x) {
  return GRAEHL_ISNAN(x);
//    return std::numeric_limits<T>::has_quiet_NaN && (x != x);
//    static_cast<void>(sizeof(nan_static_assert<std::numeric_limits<T>::has_quiet_NaN>));
}

template <typename T>
inline bool is_inf(T x) {
//    static_cast<void>(sizeof(nan_static_assert<std::numeric_limits<T>::has_infinity>));
  return x == std::numeric_limits<T>::infinity() || x == -std::numeric_limits<T>::infinity();
}

template <typename T>
inline bool is_pos_inf(T x) {
//    static_cast<void>(sizeof(nan_static_assert<std::numeric_limits<T>::has_infinity>));
  return x == std::numeric_limits<T>::infinity();
}

template <typename T>
inline bool is_neg_inf(T x) {
//    static_cast<void>(sizeof(nan_static_assert<std::numeric_limits<T>::has_infinity>));
  return x == -std::numeric_limits<T>::infinity();
}

//c99 isfinite macro shoudl be much faster
template <typename T>
inline bool is_finite(T x) {
  return !is_nan(x) && !is_inf(x);
}


}//ns


#endif
