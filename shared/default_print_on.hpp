#ifndef DEFAULT_PRINT_ON_HPP
#define DEFAULT_PRINT_ON_HPP

/// In your clase, define a type: if you defined a
///  print_on(ostream &) const
///   method like genio.h: GENIO_print_on
///  typedef void has_print_on;
/// or if has two arg print_on(ostream &, Writer &w)
///   typedef void has_print_on_writer;
/// if you inherit from a class that has defined one of these, override it with some other type than void: typedef bool has_print_on_writer would disable


#include <iostream>
#include "print_on.hpp"

/*
template <class T,class c,class v>
std::basic_ostream<c,v> &operator <<(std::basic_ostream<c,v> &o,const T &t) 
{
    t.print_on(o);
    return o;
}
*/
                                    
template <class C,class charT, class Traits>
std::basic_ostream<charT,Traits>&
operator << (std::basic_ostream<charT,Traits>& os, const C &arg,typename not_has_print_on<A>::type* dummy = 0)) 
{
 if (!s.good()) return s;
    std::ios_base::iostate err = std::ios_base::goodbit;
    typename std::basic_ostream<charT, Traits>::sentry sentry(s);
    if (sentry)
        err = arg.print_on(s);
    if (err)
        s.setstate(err);
    return s;
}

#endif
