#ifndef GRAEHL__SHARED__STREAM_UTIL_HPP
#define GRAEHL__SHARED__STREAM_UTIL_HPP

#include <iomanip>
#include <iostream>

#define TO_OSTREAM_PRINT                                                                     \
    template <class C,class T> \
    friend std::basic_ostream<C,T> & operator <<(std::basic_ostream<C,T> &o, self_type const& me)       \
    { me.print(o);return o; } \
    typedef self_type has_print;

#define FROM_ISTREAM_READ                                                 \
    template <class C,class T> \
    friend std::basic_istream<C,T>& operator >>(std::basic_istream<C,T> &i,self_type & me)     \
    { me.read(i);return i; }

#define TO_OSTREAM_PRINT_FREE(self_type) \
    template <class C,class T> inline \
    std::basic_ostream<C,T> & operator <<(std::basic_ostream<C,T> &o, self_type const& me)      \
    { me.print(o);return o; } \

#define FROM_ISTREAM_READ_FREE(self_type)                                                    \
    template <class C,class T> inline                                                              \
    std::basic_istream<C,T>& operator >>(std::basic_istream<C,T> &i,self_type & me)     \
    { me.read(i);return i; }

namespace graehl {

template <class Stream>
struct local_stream_flags
{
    typedef Stream stream_type;
    stream_type *pstream;
    std::ios::fmtflags saved_flags;
    local_stream_flags(stream_type &stream) : pstream(&stream), saved_flags(stream.flags()) {}
    ~local_stream_flags() 
    {
        pstream->flags(saved_flags);
    }
};

}//graehl


#endif
