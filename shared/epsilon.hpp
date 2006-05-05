#ifndef GRAEHL__SHARED__EPSILON_HPP
#define GRAEHL__SHARED__EPSILON_HPP

#include <cmath>

#ifndef ONE_PLUS_EPSILON
# ifndef FLOAT_EPSILON
#  define FLOAT_EPSILON .00001
# endif

namespace graehl {

# ifndef EPSILON
static const double EPSILON=FLOAT_EPSILON;
# endif 
static const double ONE_PLUS_EPSILON=1+EPSILON;
#endif

//#define ONE_PLUS_EPSILON (1+EPSILON)


/*
  The simple solution like abs(f1-f2) <= e does not work for very small or very big values. This floating-point comparison algorithm is based on the more confident solution presented by Knuth in [1]. For a given floating point values u and v and a tolerance e:

| u - v | <= e * |u| and | u - v | <= e * |v|
defines a "very close with tolerance e" relationship between u and v
        (1)

| u - v | <= e * |u| or   | u - v | <= e * |v|
defines a "close enough with tolerance e" relationship between u and v
        (2)

Both relationships are commutative but are not transitive. The relationship defined by inequations (1) is stronger that the relationship defined by inequations (2) (i.e. (1) => (2) ). Because of the multiplication in the right side of inequations, that could cause an unwanted underflow condition, the implementation is using modified version of the inequations (1) and (2) where all underflow, overflow conditions could be guarded safely:

| u - v | / |u| <= e and | u - v | / |v| <= e
| u - v | / |u| <= e or   | u - v | / |v| <= e
        (1`)
(2`)
*/


  //intent: if you want to be conservative about an assert of a<b, test a<(slightly smaller b)
  // if you want a<=b to succeed when a is == b but there were rounding errors so that a+epsilon=b, test a<(slightly larger b)
template <class Float>
inline Float slightly_larger(Float target) {
    return target * ONE_PLUS_EPSILON;
}

template <class Float>
inline Float slightly_smaller(Float target) {
    return target * (1. / ONE_PLUS_EPSILON);
}

// note, more meaningful tests exist for values near 0, see Knuth
// (but for logs, near 0 should be absolute-compared)
inline bool same_within_abs_epsilon(double a,double b,double epsilon=graehl::EPSILON) {
    return std::fabs(a-b) < epsilon;
}

}

#endif
