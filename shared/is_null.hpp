#ifndef GRAEHL__SHARED__IS_NULL_HPP
#define GRAEHL__SHARED__IS_NULL_HPP

#include <cmath>

template <class C> inline
bool is_null(C const &c) 
{ return !c; }

template <class C> inline
void set_null(C &c)
{ c=C(); }

inline bool is_null(float const& f)
{
    return std::isnan(f);//f!=f;
//    return f!=f;
}

inline void set_null(float &f)
{
    f=NAN;//0./0.;
}

inline bool is_null(double const& f)
{
    return std::isnan(f);
// return f!=f;
}

inline void set_null(double &f)
{
    f=NAN;//0./0.;
}



#endif
