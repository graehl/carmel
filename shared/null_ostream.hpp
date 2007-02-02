#ifndef GRAEHL__SHARED__NULL_OSTREAM_HPP
#define GRAEHL__SHARED__NULL_OSTREAM_HPP

#include <streambuf>
#include <ostream>
#include <string>

namespace graehl {

template <class C, class T = std::char_traits<C> >
class basic_null_streambuf : public std::basic_streambuf<C,T>
{
    virtual int overflow(typename T::int_type  c) { return T::not_eof(c); }
public:
    basic_null_streambuf() {}
};

typedef basic_null_streambuf<char> null_streambuf;

template <class C, class T = std::char_traits<C> >
class basic_null_ostream: public std::basic_ostream<C,T>
{
    basic_null_streambuf<C,T> nullbuf;
 public:
    basic_null_ostream() :
        std::basic_ios<C,T>(&nullbuf),
        std::basic_ostream<C,T>(&nullbuf)
    { init(&nullbuf); }
};

typedef basic_null_ostream<char> null_ostream;

}//graehl

#endif
