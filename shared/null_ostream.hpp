#ifndef GRAEHL__SHARED__NULL_OSTREAM_HPP
#define GRAEHL__SHARED__NULL_OSTREAM_HPP

#include <streambuf>
#include <ostream>

#ifdef GRAEHL__SINGLE_MAIN
# define GRAEHL__NULL_OSTREAM_MAIN
#endif 

namespace graehl {

class null_streambuf : public std::streambuf
{
    virtual int overflow(int c) { return std::char_traits< char >::not_eof(c); }
public:
    null_streambuf() {}
};

# ifdef GRAEHL__NULL_OSTREAM_MAIN
null_streambuf null_streambuf_instance;
std::ostream null_ostream(&null_streambuf_instance);
# else
extern null_streambuf null_streambuf_instance;
extern std::ostream null_ostream;
# endif

# endif 

}//graehl

#endif
