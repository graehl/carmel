// broken attempt to supply default oeprator << if A::default_print
#ifndef DEFAULT_PRINT_ON_HPP
#define DEFAULT_PRINT_ON_HPP
#include <iostream>

#include <vector>

namespace graehl {


template <class V,class charT,class Traits>
std::basic_ostream<charT,Traits> & operator << (std::basic_ostream<charT,Traits> &o,const std::vector<V> &v) 
{
    o << '[';
    bool first=true;
    for (typename std::vector<V>::const_iterator i=v.begin(),e=v.end();i!=e;++i) {
        if (first)
            first=false;
        else
            o << ',';
        o << *i;
    }
    o << ']';
    return o;
}


//# include "print.hpp"

/*
template <class C,class V=void>
struct default_print;

template <class C,class V>
struct default_print {
};

template <class C> 
struct default_print<C,typename C::default_print> {
  typedef void type;
};
*/

//USAGE: typedef Self default_print
//FIXME: doesn't work
template <class A,class charT, class Traits>
inline std::basic_ostream<charT,Traits>&
operator << (std::basic_ostream<charT,Traits>& s, const typename A::default_print &arg)
{
 if (!s.good()) return s;
    std::ios_base::iostate err = std::ios_base::goodbit;
    typename std::basic_ostream<charT, Traits>::sentry sentry(s);
    if (sentry)
        err = arg.print(s);
    if (err)
        s.setstate(err);
    return s;
}

}
#endif
