#ifndef GRAEHL_SHARED__POWER_OF_10_HPP
#define GRAEHL_SHARED__POWER_OF_10_HPP


//FIXME: according to docs, push/pop should work.  but in 4.3 it has no effect.
#include <graehl/shared/warning_push.hpp>

#pragma GCC diagnostic ignored "-Woverflow"
//anon namespace - so no need to define in just one linkage

/* usage: powers_of_10<Numeric>::max_exact - no rounding error, can be represented
   powers_of_10<Numeric>::max_any - largest index that can be given to below table (22).  for int types, higher powers are modulo 2^n as usual (overflow)
   powers_of_10<Numeric>::pow10[i] - 10^i for nonnegative i.  for floating point types, use pow10 fn if you want more

   defined for Numeric float,double,(u)int{8,16,32,64}_t
 */
#include <boost/cstdint.hpp>

// these two lists could be generated via a combinator loop thingy: FOLDC22C(F,C,Z) =F(C,... F(C,F(C,Z))...)

#define TIMES0TIMES(x) 1
#define TIMES1TIMES(x) x
#define TIMES2TIMES(x) TIMES1TIMES(x)*x
#define TIMES3TIMES(x) TIMES2TIMES(x)*x
#define TIMES4TIMES(x) TIMES3TIMES(x)*x
#define TIMES5TIMES(x) TIMES4TIMES(x)*x
#define TIMES6TIMES(x) TIMES5TIMES(x)*x
#define TIMES7TIMES(x) TIMES6TIMES(x)*x
#define TIMES8TIMES(x) TIMES7TIMES(x)*x
#define TIMES9TIMES(x) TIMES8TIMES(x)*x
#define TIMES10TIMES(x) TIMES9TIMES(x)*x
#define TIMES11TIMES(x) TIMES10TIMES(x)*x
#define TIMES12TIMES(x) TIMES11TIMES(x)*x
#define TIMES13TIMES(x) TIMES12TIMES(x)*x
#define TIMES14TIMES(x) TIMES13TIMES(x)*x
#define TIMES15TIMES(x) TIMES14TIMES(x)*x
#define TIMES16TIMES(x) TIMES15TIMES(x)*x
#define TIMES17TIMES(x) TIMES16TIMES(x)*x
#define TIMES18TIMES(x) TIMES17TIMES(x)*x
#define TIMES19TIMES(x) TIMES18TIMES(x)*x
#define TIMES20TIMES(x) TIMES19TIMES(x)*x
#define TIMES21TIMES(x) TIMES20TIMES(x)*x
#define TIMES22TIMES(x) TIMES21TIMES(x)*x

#define LIST_POWERS_0_BIG(x) \
  TIMES0TIMES(x) , \
  TIMES1TIMES(x) , \
  TIMES2TIMES(x) , \
  TIMES3TIMES(x) , \
  TIMES4TIMES(x) , \
  TIMES5TIMES(x) , \
  TIMES6TIMES(x) , \
  TIMES7TIMES(x) , \
  TIMES8TIMES(x) , \
  TIMES9TIMES(x) , \
  TIMES10TIMES(x) , \
  TIMES11TIMES(x) , \
  TIMES12TIMES(x) , \
  TIMES13TIMES(x) , \
  TIMES14TIMES(x) , \
  TIMES15TIMES(x) , \
  TIMES16TIMES(x) , \
  TIMES17TIMES(x) , \
  TIMES18TIMES(x) , \
  TIMES19TIMES(x) , \
  TIMES20TIMES(x) , \
  TIMES21TIMES(x) , \
  TIMES22TIMES(x)

#define LIST_POWERS_BIGEXP 22

// these powers
#define DECLARE_POWERS_OF_10(F,M) \
template <> \
struct power_of_10<F> { \
  static const int max_exact=M; \
  static const int n_exact=max_exact+1; \
  static const int max_any=LIST_POWERS_BIGEXP; \
  static const int n_any=max_any+1; \
  static const F ten=10; \
  static const F pow10[n_any]; \
};

#define DEFINE_POWERS_OF_10(F,M) \
  const F power_of_10<F>::pow10[power_of_10<F>::n_any]={ LIST_POWERS_0_BIG(power_of_10<F>::ten) };

#define MAKE_POWERS_OF_10(F,M) \
  DECLARE_POWERS_OF_10(F,M) \
  DEFINE_POWERS_OF_10(F,M)

namespace {

template <class Float> struct power_of_10  {};

MAKE_POWERS_OF_10(int8_t,2)
MAKE_POWERS_OF_10(uint8_t,2)
MAKE_POWERS_OF_10(int16_t,4)
MAKE_POWERS_OF_10(uint16_t,4)
MAKE_POWERS_OF_10(int32_t,9)
MAKE_POWERS_OF_10(uint32_t,9)
MAKE_POWERS_OF_10(int64_t,18)
MAKE_POWERS_OF_10(uint64_t,18)
MAKE_POWERS_OF_10(float,11) //FIXME: just a guess - haven't tested.
MAKE_POWERS_OF_10(double,22) // 10^22 = 0x1.0f0cf064dd592p73 is the largest exactly representable power of 10 in the IEEE binary64 format

}//ns


#include <graehl/shared/warning_pop.hpp>

#endif
