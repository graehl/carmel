#ifndef GRAEHL__SHARED__EPSILON_HPP
#define GRAEHL__SHARED__EPSILON_HPP

#include <cmath>
#include <algorithm>

# ifndef FLOAT_EPSILON
#  define FLOAT_EPSILON 1e-5
# endif

namespace graehl {

#ifndef ONE_PLUS_EPSILON
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
inline bool same_within_abs_epsilon(double a,double b,double epsilon=EPSILON) {
    return std::fabs(a-b) < epsilon;
}

inline bool close_by_first(double a,double b,double epsilon=EPSILON)
{
    return std::fabs(a-b) <= epsilon*std::fabs(a);
}

inline bool very_close(double a,double b,double epsilon=EPSILON)
{
    using std::fabs;
    double diff=fabs(a-b);
    return diff<=epsilon*fabs(a) && diff<=epsilon*fabs(b);
// return close_by_first(a,b,epsilon) && close_by_first(b,a,epsilon);
}

inline bool close_enough(double a,double b,double epsilon=EPSILON)
{
    using std::fabs;
    double diff=fabs(a-b);
    return diff<=epsilon*fabs(a) || diff<=epsilon*fabs(b);
//    return close_by_first(a,b,epsilon) || close_by_first(b,a,epsilon);
}

inline bool close_enough_min_scale(double a,double b,double epsilon=EPSILON,double min_scale=1.) // use max(fabs(a),fabs(b),min_scale), epsilon is relative difference vs that
{
    using std::fabs;
    double diff=fabs(a-b);
    double scale=std::max(min_scale,std::max(fabs(a),fabs(b)));
    return diff<=epsilon*scale;
//    return close_by_first(a,b,epsilon) || close_by_first(b,a,epsilon);
}

}

#endif
