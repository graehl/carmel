#ifndef GRAEHL__SHARED__STREAM_UTIL_HPP
#define GRAEHL__SHARED__STREAM_UTIL_HPP

#include <iomanip>

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
