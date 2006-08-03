#ifndef GRAEHL__SHARED__EXACT_CAST_HPP
#define GRAEHL__SHARED__EXACT_CAST_HPP

#include <stdexcept>

namespace graehl {

struct inexact_cast : public std::runtime_error 
{
    inexact_cast() : std::runtime_error("inexact_cast - casting to a different type lost information") {}
};

template <class To,class From>
To exact_static_assign(To &to,From const& from) 
{
    to=static_cast<To>(from);
    if (static_cast<From>(to)!=from)
        throw inexact_cast();
    return to;
}

template <class To,class From>
To exact_static_cast(From const& from) 
{
    To to;
    exact_static_assign(to,from);
    return to;
}



}//graehl

#endif
