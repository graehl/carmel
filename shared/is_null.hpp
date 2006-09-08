#ifndef GRAEHL__SHARED__IS_NULL_HPP
#define GRAEHL__SHARED__IS_NULL_HPP

//NOTE: not namespace graehl.
#ifdef _WIN32
#include <float.h>
#include <xmath.h>
#endif


#include <cmath>

template <class C> inline
bool is_null(C const &c) 
{ return !c; }

template <class C> inline
void set_null(C &c)
{ c=C(); }

inline bool is_null(float const& f)
{
#ifndef _WIN32
    return std::isnan(f);//f!=f;
#else
	return _isnan(f) != 0;
#endif
//    return f!=f;
}

inline void set_null(float &f)
{
    f=NAN;//0./0.;
}

inline bool is_null(double const& f)
{
#ifndef _WIN32
    return std::isnan(f);//f!=f;
#else
	return _isnan(f) != 0;
#endif
// return f!=f;
}

inline void set_null(double &f)
{
    f=NAN;//0./0.;
}

struct as_null {};
// tag for constructors

#define MEMBER_IS_SET_NULL MEMBER_SET_NULL MEMBER_IS_NULL

#define MEMBER_SET_NULL     friend bool is_null(self_type const& me) { return me.is_null(); }
#define MEMBER_IS_NULL     friend void is_null(self_type & me) { return me.set_null(); }


#endif
