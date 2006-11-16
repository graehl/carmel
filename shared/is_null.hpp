#ifndef GRAEHL__SHARED__IS_NULL_HPP
#define GRAEHL__SHARED__IS_NULL_HPP

//NOTE: not namespace graehl.

#include <cmath>

#if defined(_WIN32) && !defined(__CYGWIN__)
# define WIN32_NAN
#endif

#ifdef WIN32_NAN
#include <float.h>
#include <xmath.h>

#ifdef NAN
#undef NAN
#endif NAN

static const unsigned int nan[2] = {0xffffffff, 0x7fffffff}; 

#define NAN (*(const double *) nan)

#endif

#ifndef NAN
 #define NAN (0.0/0.0)
#endif

//#define FLOAT_NULL HUGE_VALF
//#define DOUBLE_NULL HUGE_VAL
#define FLOAT_NULL NAN
#define DOUBLE_NULL NAN



template <class C> inline
bool is_null(C const &c) 
{ return !c; }

template <class C> inline
void set_null(C &c)
{ c=C(); }

inline bool is_null(float const& f)
{
#ifdef WIN32_NAN
    return _isnan(f) != 0;
#else
    return std::isnan(f);//f!=f;
#endif
//    return f!=f;
}

inline void set_null(float &f)
{
    f=FLOAT_NULL;//0./0.;
}

inline bool is_null(double const& f)
{
#ifdef WIN32_NAN
    return _isnan(f) != 0;
#else
    return std::isnan(f);//f!=f;
#endif
// return f!=f;
}

inline void set_null(double &f)
{
    f=DOUBLE_NULL;//0./0.;
}

struct as_null {};
// tag for constructors

#define MEMBER_IS_SET_NULL MEMBER_SET_NULL MEMBER_IS_NULL

#define MEMBER_SET_NULL     friend bool is_null(self_type const& me) { return me.is_null(); }
#define MEMBER_IS_NULL     friend void is_null(self_type & me) { return me.set_null(); }


#endif
