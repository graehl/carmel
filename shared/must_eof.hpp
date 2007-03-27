#ifndef GRAEHL_SHARED__MUST_EOF_HPP
#define GRAEHL_SHARED__MUST_EOF_HPP

#include <stdexcept>
#include <string>

namespace graehl {

template <class I>
inline void must_eof(I &in,char const* msg="Expected end of input, but got: ")
{
    char c;
    if (in >> c)
        throw std::runtime_error(msg+std::string(1,c));
}

}


#endif
